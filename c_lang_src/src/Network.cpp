#include "Network.hpp"
#include <iostream>

GrpcClient::GrpcClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(SequenceService::NewStub(channel)) {}

bool GrpcClient::StartStream(const std::string& session_id) {
    context_ = std::make_unique<grpc::ClientContext>();
    response_ = SubmitResultResponse();
    writer_ = stub_->SendFrames(context_.get(), &response_);
    session_id_ = session_id;
    return true;
}

bool GrpcClient::WriteFrame(int index, int flag, const cv::Mat& img, const std::vector<cv::Point3f>& skeleton) {
    if (!writer_) return false;

    Frame frame;
    frame.set_session_id(session_id_);
    frame.set_index(index);
    frame.set_flag(flag);
    frame.set_width(img.cols);
    frame.set_height(img.rows);
    
    // Check type mapping
    // OpenCV types: CV_8U=0, CV_8S=1, CV_16U=2, CV_16S=3, CV_32S=4, CV_32F=5, CV_64F=6
    // CV_MAKETYPE(depth, cn) = ((depth) & 7) + ((cn) - 1) << 3
    frame.set_type(img.type()); 

    // Copy data
    if (img.isContinuous()) {
        frame.set_data(img.data, img.total() * img.elemSize());
    } else {
        cv::Mat continuous = img.clone();
        frame.set_data(continuous.data, continuous.total() * continuous.elemSize());
    }

    for (const auto& p : skeleton) {
        auto* pt = frame.add_pose_points();
        pt->set_x(p.x);
        pt->set_y(p.y);
        pt->set_z(p.z);
    }

    return writer_->Write(frame);
}

std::string GrpcClient::FinishStream() {
    if (!writer_) return "No active stream";
    writer_->WritesDone();
    grpc::Status status = writer_->Finish();
    
    if (status.ok()) {
        return response_.message();
    } else {
        return "RPC failed: " + status.error_message();
    }
}
