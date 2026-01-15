#pragma once
#include <vector>
#include <array>
#include <opencv2/core.hpp>

// 손 모양 차이 계산 모듈 (PCA + 손가락 가중)
// - std_vals = 1.0 가정 (whitened MANO)
// - 특정 손가락 차이가 클 때 softmax로 가중 집중

namespace shapediff {

    // 파라미터
    struct Params {
        double lambda = 0.4;          // Final = λ*PCA + (1-λ)*Finger
        double tau = 0.25;         // softmax 온도(작을수록 한 손가락으로 가중 쏠림)
        double alpha = 1.0;          // softmax 기울기(클수록 강조)
        double eps = 1e-8;         // 수치 안정
        std::array<double, 5> user_w{ 1,1,1,1,1 }; // 손가락 고정 가중(thumb,index,middle,ring,pinky)
        double pca_boost_k = 0.5;     // PCA 부스터 세기(0이면 끔)
    };

    // 3D 조인트 기반 최종 스코어
    double FinalScore3D(const std::vector<float>& poseA,   // MANO PCA 계수 (whitened, 차원 동일)
        const std::vector<float>& poseB,
        const std::vector<cv::Point3f>& jointsA, // 21점
        const std::vector<cv::Point3f>& jointsB, // 21점
        const Params& prm = {});

    // 2D 조인트만 있을 때(자동으로 z=0으로 만들어 3D 계산 재사용)
    double FinalScore2D(const std::vector<float>& poseA,
        const std::vector<float>& poseB,
        const std::vector<cv::Point2f>& jointsA2D, // 21점
        const std::vector<cv::Point2f>& jointsB2D, // 21점
        const Params& prm = {});

    // (선택) 개별 구성 요소만 필요할 때
    double ManoPoseL2(const std::vector<float>& V1, const std::vector<float>& V2); // std_vals=1 가정

    struct FingerDiff { double T = 0, I = 0, M = 0, R = 0, P = 0; }; // 각도(°)
    FingerDiff PerFingerAngleDiff3D(const std::vector<cv::Point3f>& A,
        const std::vector<cv::Point3f>& B);

} // namespace shapediff
