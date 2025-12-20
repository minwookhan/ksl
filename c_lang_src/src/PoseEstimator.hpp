#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class PoseEstimator {
public:
    PoseEstimator();
    ~PoseEstimator();

    // Returns 3D landmarks (x, y, z)
    std::vector<cv::Point3f> ProcessFrame(const cv::Mat& img);

private:
    py::object module_;
    py::object func_mp_pose_;
    py::object func_mp_hand_;
};
