#include "PoseEstimator.hpp"
#include "MatTypeCaster.hpp"
#include <iostream>

PoseEstimator::PoseEstimator() {
    try {
        module_ = py::module_::import("mp_detect");
        func_mp_pose_ = module_.attr("mediapipe_pose_func");
        func_mp_hand_ = module_.attr("mediapipe_hand_func");
    } catch (py::error_already_set& e) {
        std::cerr << "Python error in PoseEstimator init: " << e.what() << std::endl;
        throw;
    }
}

PoseEstimator::~PoseEstimator() {}

std::vector<cv::Point3f> PoseEstimator::ProcessFrame(const cv::Mat& img) {
    std::vector<cv::Point3f> landmarks;
    if (img.empty()) return landmarks;

    try {
        // Pose
        py::object result_pose = func_mp_pose_(img);
        auto mp_result = result_pose.cast<std::vector<std::vector<double>>>();

        if (mp_result.size() >= 3 && !mp_result[0].empty()) {
            for (size_t k = 0; k < mp_result[0].size(); k++) {
                landmarks.emplace_back((float)mp_result[0][k], (float)mp_result[1][k], (float)mp_result[2][k]);
            }
        }
        
        // Hand
        py::object result_hand = func_mp_hand_(img);
        auto hand_result = result_hand.cast<std::vector<std::vector<double>>>();
        
        if (hand_result.size() >= 3 && !hand_result[0].empty()) {
             for (size_t k = 0; k < hand_result[0].size(); k++) {
                landmarks.emplace_back((float)hand_result[0][k], (float)hand_result[1][k], (float)hand_result[2][k]);
            }
        }

    } catch (py::error_already_set& e) {
        std::cerr << "Python error in ProcessFrame: " << e.what() << std::endl;
    }

    return landmarks;
}
