#include "Logic.hpp"
#include <iostream>

const std::vector<cv::Point2f> Logic::warpCorners = {
    cv::Point2f(720, 500),
    cv::Point2f(1200, 500),
    cv::Point2f(960, 400)
};

double Logic::GetMotionValueWithOpticalFlow(cv::Mat& prevGray, cv::Mat& gray) {
    if (prevGray.empty() || gray.empty()) return 0.0;

    std::vector<cv::Point2f> prevPts;
    cv::goodFeaturesToTrack(prevGray, prevPts, 300, 0.01, 7);

    if (prevPts.empty()) return 0.0;

    std::vector<cv::Point2f> nextPts;
    std::vector<unsigned char> status;
    std::vector<float> err;

    cv::calcOpticalFlowPyrLK(prevGray, gray, prevPts, nextPts, status, err);

    double sumLen = 0.0;
    int cnt = 0;
    for (size_t i = 0; i < nextPts.size(); ++i) {
        if (!status[i]) continue;
        double dx = nextPts[i].x - prevPts[i].x;
        double dy = nextPts[i].y - prevPts[i].y;
        double len = std::sqrt(dx * dx + dy * dy);
        sumLen += len;
        cnt++;
    }

    double avgMotion = (cnt > 0) ? (sumLen / cnt) : 0.0;
    // std::cout << "avg motion (Optical Flow): " << avgMotion << std::endl;
    return avgMotion;
}

std::vector<cv::Point2f> Logic::GetRefHL2DPointsMP(const std::vector<cv::Point3f>& joints) {
    std::vector<cv::Point2f> ref_hl_points;

    if (joints.size() <= 16) return ref_hl_points; // Need at least index 16

    // 2D Hand Location - Scale normalized to 1920x1080 as per original logic
    std::vector<cv::Point2f> pixels;
    for (const auto& joint : joints) {
        pixels.emplace_back(joint.x * 1920.0f, joint.y * 1080.0f);
    }

    if (pixels.size() <= 12) return ref_hl_points;

    // Geometric Transformation
    std::vector<cv::Point2f> corners = { pixels[12], pixels[11], pixels[0] }; // RightShoulder, LeftShoulder, Nose
    cv::Mat trans = cv::getAffineTransform(corners, warpCorners);

    std::vector<cv::Point2f> hl_point = { pixels[15], pixels[16] }; // Left wrist, Right wrist
    std::vector<cv::Point2f> dst;

    cv::transform(hl_point, dst, trans);

    for (auto& pt : dst) {
        pt.x = std::clamp(pt.x, 0.0f, 1920.0f);
        pt.y = std::clamp(pt.y, 0.0f, 1080.0f);
    }

    ref_hl_points.push_back(dst[0]);
    ref_hl_points.push_back(dst[1]);

    return ref_hl_points;
}

int Logic::GetRoughHandStatusFromMP(const std::vector<cv::Point3f>& mp_pose) {
    if (mp_pose.empty()) return -1;

    auto hl = GetRefHL2DPointsMP(mp_pose);
    if (hl.size() < 2) return -1;

    int status = 0;
    if (hl[1].y <= READY_LOCATION && hl[0].y <= READY_LOCATION) status = 3;      // Both up
    else if (hl[1].y <= READY_LOCATION && hl[0].y > READY_LOCATION) status = 1; // Right up
    else if (hl[1].y > READY_LOCATION && hl[0].y <= READY_LOCATION) status = 2; // Left up
    else status = 0; // Ready (Down)

    return status;
}

int Logic::GetMotionStatusFromMP(int cur_motion_status, 
                                 const std::vector<cv::Point3f>& cur_mp, 
                                 const std::vector<cv::Point3f>& prev_mp) {
    
    if (cur_motion_status == RESET_MOTION_STATUS) return RESET_MOTION_STATUS;

    int motion_status = cur_motion_status;

    if (prev_mp.empty() || cur_mp.empty() || prev_mp.size() <= 16 || cur_mp.size() <= 16) {
        return READY_MOTION_STATUS;
    }

    double Rdev = Utils::Distance(cv::Point2f(prev_mp[16].x - cur_mp[16].x, prev_mp[16].y - cur_mp[16].y), cv::Point2f(0,0));
    double Ldev = Utils::Distance(cv::Point2f(prev_mp[15].x - cur_mp[15].x, prev_mp[15].y - cur_mp[15].y), cv::Point2f(0,0));

    // Debug output similar to original if needed
    // std::cout << "Rdev Ldev: " << Rdev << " " << Ldev << std::endl;

    if (Rdev > RAPID_DISTANCE || Ldev > RAPID_DISTANCE) return RAPID_MOTION_STATUS;

    if (cur_motion_status == READY_MOTION_STATUS) {
        int hand_status = GetRoughHandStatusFromMP(cur_mp);
        if (hand_status > 0) motion_status = SPEAK_MOTION_STATUS;
    } else if (cur_motion_status == SPEAK_MOTION_STATUS) {
        // Simple transition back to READY if stable (simplified from original logic)
        motion_status = READY_MOTION_STATUS; 
    }

    return motion_status;
}
