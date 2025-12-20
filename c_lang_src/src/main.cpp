#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <pybind11/embed.h>

#include "Utils.hpp"
#include "Logic.hpp"
#include "PoseEstimator.hpp"
#include "Network.hpp"
#include "HandTurnDetector.hpp"

namespace py = pybind11;

struct Config {
    std::string video_path;
    std::string roi_str;
    std::string server = "localhost:50051";
    bool no_gui = false;
    bool debug_file = false;
    cv::Rect roi;
    int start_frame = -1;
    int end_frame = -1;
};

// Simple arg parser
Config ParseArgs(int argc, char* argv[]) {
    Config config;
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <video_path> <roi> [--server <addr>] [--no-gui] [--debugFile] [--frame <start> <end>]" << std::endl;
        exit(1);
    }
    config.video_path = argv[1];
    config.roi_str = argv[2];
    config.roi = Utils::ParseROI(config.roi_str);

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server" && i + 1 < argc) {
            config.server = argv[++i];
        } else if (arg == "--no-gui") {
            config.no_gui = true;
        } else if (arg == "--debugFile") {
            config.debug_file = true;
        } else if (arg == "--frame" && i + 2 < argc) {
            config.start_frame = std::stoi(argv[++i]);
            config.end_frame = std::stoi(argv[++i]);
        }
    }
    return config;
}

int main(int argc, char* argv[]) {
    // Initialize Python Interpreter ONCE
    py::scoped_interpreter guard{};

    Config config = ParseArgs(argc, argv);

    // Initialize Components
    cv::VideoCapture cap(config.video_path);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open video: " << config.video_path << std::endl;
        return 1;
    }

    // PoseEstimator initialization (loads python module)
    // Ensure mp_detect.py is in working directory or python path
    PoseEstimator pose_estimator;
    
    // gRPC
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(config.server, grpc::InsecureChannelCredentials());
    GrpcClient grpc_client(channel);
    
    std::string session_id = "session_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    if (!grpc_client.StartStream(session_id)) {
        std::cerr << "Failed to start gRPC stream" << std::endl;
        return 1;
    }

    // Logic State
    HandTurnDetector right_hand_detector;
    HandTurnDetector left_hand_detector;
    
    int current_motion_status = READY_MOTION_STATUS;
    std::vector<cv::Point3f> prev_skeleton;
    cv::Mat prev_gray;
    
    bool speak_start_flag = false;
    int frozen_flag = 0;
    
    std::ofstream debug_csv;
    if (config.debug_file) {
        // Simple logic to create filename: video_path_start_end.csv (simplified)
        size_t last_slash = config.video_path.find_last_of("/\\");
        std::string filename = (last_slash == std::string::npos) ? config.video_path : config.video_path.substr(last_slash + 1);
        filename += "_debug.csv";
        
        debug_csv.open(filename);
        debug_csv << "frame_index,avg_motion,Rdev,Ldev" << std::endl;
        std::cout << "Saving debug to " << filename << std::endl;
    }

    auto [w, h, fps, total_frames] = Utils::GetVideoInfo(cap);
    std::cout << "Processing video: " << w << "x" << h << " @ " << fps << "fps (" << total_frames << " frames)" << std::endl;

    int frame_count = 0;
    cv::Mat frame, roi_frame;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        frame_count++;

        // Range Check
        if (config.start_frame > 0 && frame_count < config.start_frame) {
            continue;
        }
        if (config.end_frame > 0 && frame_count > config.end_frame) {
            std::cout << "Reached end frame: " << config.end_frame << ". Stopping." << std::endl;
            break;
        }

        if (frame_count % 30 == 0) std::cout << "Processing frame " << frame_count << std::endl;

        // ROI crop
        if (!config.roi.empty()) {
            // Safe cropping
            cv::Rect safe_roi = config.roi & cv::Rect(0, 0, frame.cols, frame.rows);
            if (safe_roi.empty()) roi_frame = frame;
            else roi_frame = frame(safe_roi);
        } else {
            roi_frame = frame;
        }

        // Resize if width >= 320
        cv::Mat processed_frame;
        if (roi_frame.cols >= 320) {
            double scale = 320.0 / roi_frame.cols;
            cv::resize(roi_frame, processed_frame, cv::Size(), scale, scale);
        } else {
            processed_frame = roi_frame.clone();
        }

        // Optical Flow Preparation
        cv::Mat curr_gray;
        cv::cvtColor(processed_frame, curr_gray, cv::COLOR_BGR2GRAY);
        double avg_motion = 0.0;

        // Pose Estimation
        auto current_skeleton = pose_estimator.ProcessFrame(processed_frame);

        // Logic Update
        current_motion_status = Logic::GetMotionStatusFromMP(current_motion_status, current_skeleton, prev_skeleton);
        
        int hand_status = Logic::GetRoughHandStatusFromMP(current_skeleton);
        bool is_turn_detected = false;

        if (hand_status != 0) {
            if (!prev_gray.empty()) {
                avg_motion = Logic::GetMotionValueWithOpticalFlow(prev_gray, curr_gray);
            }
            prev_gray = curr_gray.clone();

            if (current_skeleton.size() > 16) {
                double dt = 1.0 / 30.0; 
                if (right_hand_detector.update(cv::Point2f(current_skeleton[16].x, current_skeleton[16].y), dt)) {
                    is_turn_detected = true;
                }
                if (left_hand_detector.update(cv::Point2f(current_skeleton[15].x, current_skeleton[15].y), dt)) {
                    is_turn_detected = true;
                }
            }
        }

        if (is_turn_detected) {
            speak_start_flag = true;
        }

        bool should_send = false;
        if (speak_start_flag && avg_motion < OPTICALFLOW_THRESH && frozen_flag == 0 && hand_status != 0 && !is_turn_detected) {
            should_send = true;
            frozen_flag = OPTICAL_FLOW_HOLD_FRAME;
            std::cout << "KeyFrame detected at frame " << frame_count << " (AvgMotion: " << avg_motion << ")" << std::endl;
        }

        if (frozen_flag > 0) frozen_flag--;

        // Debug CSV
        if (config.debug_file && current_skeleton.size() > 16 && prev_skeleton.size() > 16) {
             double Rdev = Utils::Distance(cv::Point2f(prev_skeleton[16].x - current_skeleton[16].x, prev_skeleton[16].y - current_skeleton[16].y), cv::Point2f(0,0));
             double Ldev = Utils::Distance(cv::Point2f(prev_skeleton[15].x - current_skeleton[15].x, prev_skeleton[15].y - current_skeleton[15].y), cv::Point2f(0,0));
             debug_csv << frame_count << "," << avg_motion << "," << Rdev << "," << Ldev << std::endl;
        }

        // Send
        if (should_send) {
            grpc_client.WriteFrame(frame_count, 0, processed_frame, current_skeleton);
        }

        // Visualization
        if (!config.no_gui) {
            cv::Mat vis = processed_frame.clone();
            if (should_send) {
                cv::rectangle(vis, cv::Point(0,0), cv::Point(vis.cols-1, vis.rows-1), cv::Scalar(0,0,255), 4);
                cv::putText(vis, "KEYFRAME", cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0,0,255), 2);
            }
            cv::imshow("KSL Client C++", vis);
            if (cv::waitKey(1) == 27) break; // ESC
        }

        prev_skeleton = current_skeleton;
    }

    std::string response = grpc_client.FinishStream();
    std::cout << "Server Response: " << response << std::endl;

    if (config.debug_file) debug_csv.close();
    return 0;
}
