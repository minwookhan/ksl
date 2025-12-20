#include "Utils.hpp"
#include <sstream>

namespace Utils {

    double Distance(cv::Point2f p1, cv::Point2f p2) {
        return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
    }

    std::vector<std::string> SplitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::tuple<int, int, double, int> GetVideoInfo(cv::VideoCapture& cap) {
        int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        double fps = cap.get(cv::CAP_PROP_FPS);
        int total_frames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        return std::make_tuple(width, height, fps, total_frames);
    }

    cv::Rect ParseROI(const std::string& roi_str) {
        // Handle spaces
        std::string clean_roi = roi_str;
        clean_roi.erase(remove(clean_roi.begin(), clean_roi.end(), ' '), clean_roi.end());
        
        auto tokens = SplitString(clean_roi, ',');
        if (tokens.size() == 4) {
            int x = std::stoi(tokens[0]);
            int y = std::stoi(tokens[1]);
            int w = std::stoi(tokens[2]);
            int h = std::stoi(tokens[3]);
            return cv::Rect(x, y, w, h);
        }
        return cv::Rect();
    }
}
