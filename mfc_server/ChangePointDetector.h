#pragma once

#include <vector>
#include <deque>
#include <opencv2/core.hpp>

// 실시간 "정지(Stable) 구간" + "로컬 미니마(국소 최소)" 검출기
class ChangePointDetector {
public:
    struct Event {
        bool  segment_started = false;   // 이번 프레임에 정지구간 진입 확정
        bool  segment_ended = false;   // 이번 프레임에 정지구간 이탈 확정
        bool  is_stable_now = false;   // 현재 상태
        int   stable_start = -1;      // 마지막 정지구간 시작 프레임
        int   stable_end = -1;      // 마지막 정지구간 종료 프레임
        int   representative = -1;      // 구간 대표 프레임(중앙)
        float motion_raw = 0.f;     // 관절 이동량의 median
        float motion_smooth = 0.f;     // EMA 모션
        float thr_low = 0.f;     // 진입 임계치
        float thr_high = 0.f;     // 이탈 임계치

        // 로컬 미니마(국소 최소) 이벤트
        bool  local_min = false;   // 이번 프레임 기준 국소 최소 검출
        int   local_min_index = -1;      // 해당 인덱스 (보통 frame_idx_-1)
        float local_min_value = 0.f;     // 그때의 ema 값
    };

    struct Params {
        // 기본값은 Mediapipe 정규화 좌표(0~1)에 맞춤
        int    fps = 30;
        int    win_median = 15;
        float  ema_alpha = 0.3f;
        float  floor_low = 0.0010f;
        float  floor_high = 0.0030f;
        float  rel_low_scale = 0.6f;
        float  rel_high_scale = 0.9f;
        int    dwell_in_frames = 8;
        int    dwell_out_frames = 3;
        int    min_joints = 3;        // 모션 계산 최소 관절 수(선택 관절 기준)
        bool   use_z = false;    // 깊이 신뢰도 낮으면 false 권장

        // 모션 계산용 관절 인덱스(비우면 전체 관절 사용)
        std::vector<int> joint_indices;

        // 로컬 미니마 검출 파라미터
        float valley_prominence = 0.00010f; // 이웃 대비 최소 하강폭
        int   valley_min_dist = 4;        // 미니마 간 최소 간격(프레임)
        float valley_below_scale = 1.05f;    // thr_low * scale 이하에서만 인정
        int   valley_roi_start = -1;       // ROI 시작(미사용: -1)
        int   valley_roi_end = -1;       // ROI 끝(미사용: -1)
    };

    explicit ChangePointDetector(const Params& p = Params());

    void reset();
    Event update(const std::vector<cv::Point3f>& mp_skeleton);
    Event finalize();

    void setJointIndices(const std::vector<int>& joints) { joint_indices_ = joints; }

private:
    Params params_;
    int frame_idx_ = -1;

    std::vector<cv::Point3f> prev_;
    bool has_prev_ = false;

    std::deque<float> motion_hist_;   // raw(median) 히스토리
    float ema_motion_ = 0.f;
    bool  ema_init_ = false;

    enum class State { MOVING, STABLE };
    State state_ = State::MOVING;

    int dwell_counter_in_ = 0;
    int dwell_counter_out_ = 0;

    int candidate_start_ = -1;
    int last_stable_start_ = -1;
    int last_stable_end_ = -1;

    std::vector<int> joint_indices_;

    // 로컬 미니마용 상태(3점 패턴 검사)
    bool  have_ema1_ = false, have_ema2_ = false;
    float ema_prev1_ = 0.f, ema_prev2_ = 0.f;
    int   last_valley_idx_ = -1000000;

    // 내부 유틸
    static float l2(const cv::Point3f& a, const cv::Point3f& b, bool useZ);
    static float median(std::deque<float> v);

    void  computeThresholds(float& thr_low, float& thr_high) const;
    Event step(float motion_raw);
};
