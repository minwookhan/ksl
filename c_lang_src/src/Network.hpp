#pragma once
#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include "ksl_sentence_recognition.grpc.pb.h"

using namespace vision::raw::v1;

class GrpcClient {
public:
    GrpcClient(std::shared_ptr<grpc::Channel> channel);
    
    bool StartStream(const std::string& session_id);
    bool WriteFrame(int index, int flag, const cv::Mat& img, const std::vector<cv::Point3f>& skeleton);
    std::string FinishStream();

private:
    std::unique_ptr<SequenceService::Stub> stub_;
    std::unique_ptr<grpc::ClientContext> context_;
    std::unique_ptr<grpc::ClientWriter<Frame>> writer_;
    SubmitResultResponse response_;
    std::string session_id_;
};
