#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "Utils.hpp"

// Constants
constexpr int RESET_MOTION_STATUS = -1;
constexpr int READY_MOTION_STATUS = 0;
constexpr int SPEAK_MOTION_STATUS = 1;
constexpr int RAPID_MOTION_STATUS = 2;

constexpr double OPTICALFLOW_THRESH = 0.45;
constexpr int OPTICAL_FLOW_HOLD_FRAME = 3;
constexpr int READY_LOCATION = 700;
constexpr double RAPID_DISTANCE = 0.1;

class Logic {
public:
    static double GetMotionValueWithOpticalFlow(cv::Mat& prev_gray, cv::Mat& curr_gray);
    
    static int GetRoughHandStatusFromMP(const std::vector<cv::Point3f>& mp_pose);
    
    static int GetMotionStatusFromMP(int current_motion_status, 
                                     const std::vector<cv::Point3f>& cur_mp, 
                                     const std::vector<cv::Point3f>& prev_mp);

    static std::vector<cv::Point2f> GetRefHL2DPointsMP(const std::vector<cv::Point3f>& joints);

private:
    static const std::vector<cv::Point2f> warpCorners;
};
