#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <tuple>

namespace Utils {
    double Distance(cv::Point2f p1, cv::Point2f p2);
    std::vector<std::string> SplitString(const std::string& str, char delimiter);
    std::tuple<int, int, double, int> GetVideoInfo(cv::VideoCapture& cap);
    cv::Rect ParseROI(const std::string& roi_str);
}
