#include "pch.h"                  // PCH 사용 시 유지(최상단). 미사용이면 삭제.
#include "ChangePointDetector.h"

#include <algorithm>
#include <cmath>
#include <limits>

ChangePointDetector::ChangePointDetector(const Params& p)
    : params_(p), joint_indices_(p.joint_indices) {
}

void ChangePointDetector::reset() {
    frame_idx_ = -1;
    prev_.clear(); has_prev_ = false;

    motion_hist_.clear();
    ema_motion_ = 0.f; ema_init_ = false;

    state_ = State::MOVING;
    dwell_counter_in_ = dwell_counter_out_ = 0;
    candidate_start_ = -1;
    last_stable_start_ = last_stable_end_ = -1;

    have_ema1_ = have_ema2_ = false;
    last_valley_idx_ = -1000000;
}

float ChangePointDetector::l2(const cv::Point3f& a, const cv::Point3f& b, bool useZ) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    if (useZ) {
        const float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    return std::sqrt(dx * dx + dy * dy);
}

float ChangePointDetector::median(std::deque<float> v) {
    if (v.empty()) return 0.f;
    std::nth_element(v.begin(), v.begin() + static_cast<long>(v.size() / 2), v.end());
    return v[v.size() / 2];
}

void ChangePointDetector::computeThresholds(float& thr_low, float& thr_high) const {
    float med = 0.f;
    if (!motion_hist_.empty()) {
        std::deque<float> tmp = motion_hist_;
        med = median(tmp);
    }
    thr_low = std::max(params_.floor_low, params_.rel_low_scale * med);
    thr_high = std::max(params_.floor_high, params_.rel_high_scale * med);
    if (thr_high < thr_low) thr_high = thr_low;
}

ChangePointDetector::Event ChangePointDetector::step(float motion_raw) {
    Event ev{};
    ev.motion_raw = motion_raw;

    // EMA
    if (!ema_init_) { ema_motion_ = motion_raw; ema_init_ = true; }
    else {
        ema_motion_ = params_.ema_alpha * motion_raw
            + (1.f - params_.ema_alpha) * ema_motion_;
    }
    ev.motion_smooth = ema_motion_;

    // 롤링 윈도우(오래된 것 제거)
    motion_hist_.push_back(motion_raw);
    while ((int)motion_hist_.size() > params_.win_median)
        motion_hist_.pop_front();

    // 임계치
    float thr_low = 0.f, thr_high = 0.f;
    computeThresholds(thr_low, thr_high);
    ev.thr_low = thr_low;
    ev.thr_high = thr_high;

    const bool below = (ema_motion_ < thr_low);
    const bool above = (ema_motion_ > thr_high);

    if (state_ == State::MOVING) {
        if (below) {
            if (dwell_counter_in_ == 0) candidate_start_ = frame_idx_;
            if (++dwell_counter_in_ >= params_.dwell_in_frames) {
                state_ = State::STABLE;
                ev.segment_started = true;
                last_stable_start_ = candidate_start_;
                ev.stable_start = last_stable_start_;
                dwell_counter_out_ = 0;
            }
        }
        else {
            dwell_counter_in_ = 0;
            candidate_start_ = -1;
        }
    }
    else { // STABLE
        ev.is_stable_now = true;
        if (above) {
            if (++dwell_counter_out_ >= params_.dwell_out_frames) {
                state_ = State::MOVING;
                ev.segment_ended = true;
                last_stable_end_ = frame_idx_ - (dwell_counter_out_ - 1);
                ev.stable_end = last_stable_end_;
                if (last_stable_start_ >= 0 && last_stable_end_ >= last_stable_start_) {
                    ev.representative = (last_stable_start_ + last_stable_end_) / 2;
                }
                dwell_counter_in_ = 0;
            }
        }
        else {
            dwell_counter_out_ = 0;
        }
    }

    // -------- 로컬 미니마(국소 최소) 검출 --------
    {
        // ROI 체크
        bool in_roi = true;
        if (params_.valley_roi_start >= 0 && params_.valley_roi_end >= 0) {
            in_roi = (frame_idx_ >= params_.valley_roi_start &&
                frame_idx_ <= params_.valley_roi_end);
        }

        if (in_roi && have_ema1_ && have_ema2_) {
            // y2 (t-2), y1 (t-1, 후보), y0 (t)
            float y2 = ema_prev2_, y1 = ema_prev1_, y0 = ema_motion_;

            // 3점 패턴: y2 > y1 < y0  → y1이 국소 최소
            bool is_valley_shape = (y2 > y1 && y1 <= y0);

            // prominence(양쪽에서 얼마나 내려갔는지)의 최소값
            float left_drop = y2 - y1;
            float right_drop = y0 - y1;
            float prom = std::min(left_drop, right_drop);

            // thr_low 근처에서만 미니마 인정(잡음 방지)
            float gate = ev.thr_low * params_.valley_below_scale;

            if (is_valley_shape &&
                std::isfinite(prom) &&
                prom >= std::max(params_.valley_prominence, 0.2f * ev.thr_low) &&
                y1 <= gate &&
                (frame_idx_ - 1 - last_valley_idx_) >= params_.valley_min_dist)
            {
                ev.local_min = true;
                ev.local_min_index = frame_idx_ - 1; // y1의 프레임
                ev.local_min_value = y1;
                last_valley_idx_ = ev.local_min_index;
            }
        }

        // 다음 프레임을 위한 3점 버퍼 갱신
        have_ema2_ = have_ema1_;
        ema_prev2_ = ema_prev1_;
        have_ema1_ = true;
        ema_prev1_ = ema_motion_;
    }
    // -------------------------------------------

    ev.is_stable_now = (state_ == State::STABLE);
    return ev;
}

ChangePointDetector::Event
ChangePointDetector::update(const std::vector<cv::Point3f>& mp_skeleton) {
    frame_idx_++;

    // 입력 비었으면 상태만 유지
    if (mp_skeleton.empty()) {
        Event ev{};
        ev.motion_raw = std::numeric_limits<float>::quiet_NaN();
        ev.motion_smooth = ema_motion_;
        computeThresholds(ev.thr_low, ev.thr_high);
        ev.is_stable_now = (state_ == State::STABLE);
        return ev;
    }

    float motion_raw = 0.f;

    // 첫 프레임 처리
    if (!has_prev_) {
        prev_ = mp_skeleton;
        has_prev_ = true;
        motion_raw = 0.f;
        return step(motion_raw);
    }

    // 선택 관절 구성
    const size_t n = std::min(prev_.size(), mp_skeleton.size());
    std::vector<int> sel;
    sel.reserve(joint_indices_.empty() ? n : joint_indices_.size());

    if (joint_indices_.empty()) {
        for (size_t i = 0; i < n; ++i) sel.push_back((int)i);
    }
    else {
        for (int idx : joint_indices_) {
            if (idx >= 0 && (size_t)idx < n) sel.push_back(idx);
        }
    }

    // 거리 계산
    std::vector<float> dists; dists.reserve(sel.size());
    for (int idx : sel) {
        float d = l2(mp_skeleton[(size_t)idx], prev_[(size_t)idx], params_.use_z);
        if (std::isfinite(d)) dists.push_back(d);
    }

    // 다음 기준 갱신(항상)
    prev_ = mp_skeleton;

    // 관절이 적을 때도 동작하도록 min을 클램프
    const int effective_min = std::max(1, std::min(params_.min_joints, (int)sel.size()));

    if ((int)dists.size() < effective_min) {
        Event ev{};
        ev.motion_raw = std::numeric_limits<float>::quiet_NaN();
        ev.motion_smooth = ema_motion_;
        computeThresholds(ev.thr_low, ev.thr_high);
        ev.is_stable_now = (state_ == State::STABLE);
        return ev;
    }

    // median -> motion_raw
    std::nth_element(dists.begin(), dists.begin() + (long)(dists.size() / 2), dists.end());
    motion_raw = dists[dists.size() / 2];

    return step(motion_raw);
}

ChangePointDetector::Event ChangePointDetector::finalize() {
    Event ev{};
    if (state_ == State::STABLE && last_stable_start_ >= 0) {
        ev.segment_ended = true;
        last_stable_end_ = frame_idx_;
        ev.stable_end = last_stable_end_;
        ev.stable_start = last_stable_start_;
        ev.representative = (last_stable_start_ + last_stable_end_) / 2;
        state_ = State::MOVING;
    }
    return ev;
}
