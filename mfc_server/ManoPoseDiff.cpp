#include <vector>
#include <array>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <numeric>
#include <opencv2/core.hpp>

#include "ManoPoseDiff.h"


namespace shapediff {

    // --------- 내부 유틸 ---------
    static inline double _norm3(const cv::Point3f& v) {
        return std::sqrt((double)v.x * v.x + (double)v.y * v.y + (double)v.z * v.z);
    }
    static inline double _dot3(const cv::Point3f& a, const cv::Point3f& b) {
        return (double)a.x * b.x + (double)a.y * b.y + (double)a.z * b.z;
    }
    static inline double _angle_deg(const cv::Point3f& a, const cv::Point3f& b, double eps = 1e-8) {
        double na = _norm3(a), nb = _norm3(b);
        if (na < eps || nb < eps) return 0.0;
        double c = _dot3(a, b) / (na * nb);
        c = std::max(-1.0, std::min(1.0, c));
        return std::acos(c) * 180.0 / CV_PI;
    }
    static inline cv::Point3f _vec(const std::vector<cv::Point3f>& P, int i, int j) {
        return { P[j].x - P[i].x, P[j].y - P[i].y, P[j].z - P[i].z };
    }

    // --------- 공개 유틸 구현 ---------
    double ManoPoseL2(const std::vector<float>& V1, const std::vector<float>& V2) {
        if (V1.size() != V2.size() || V1.empty()) return DBL_MAX;
        double s = 0.0;
        for (size_t i = 0; i < V1.size(); ++i) {
            const double d = (double)V1[i] - (double)V2[i];
            s += d * d;
        }
        return std::sqrt(s)/ V1.size();
    }

    FingerDiff PerFingerAngleDiff3D(const std::vector<cv::Point3f>& A,
        const std::vector<cv::Point3f>& B)
    {
        FingerDiff D;
        if (A.size() < 21 || B.size() < 21) return D;

        auto avg4 = [&](int a0, int a1, int a2, int a3, int a4)->double {
            cv::Point3f a01 = _vec(A, a0, a1), b01 = _vec(B, a0, a1);
            cv::Point3f a12 = _vec(A, a1, a2), b12 = _vec(B, a1, a2);
            cv::Point3f a23 = _vec(A, a2, a3), b23 = _vec(B, a2, a3);
            cv::Point3f a34 = _vec(A, a3, a4), b34 = _vec(B, a3, a4);
            const double d01 = _angle_deg(a01, b01);
            const double d12 = _angle_deg(a12, b12);
            const double d23 = _angle_deg(a23, b23);
            const double d34 = _angle_deg(a34, b34);
            return (d01 + d12 + d23 + d34) / 4.0; // 평균(필요하면 RMS 가능)
            };

        D.T = avg4(0, 1, 2, 3, 4);
        D.I = avg4(0, 5, 6, 7, 8);
        D.M = avg4(0, 9, 10, 11, 12);
        D.R = avg4(0, 13, 14, 15, 16);
        D.P = avg4(0, 17, 18, 19, 20);
        return D;
    }

    // softmax 가중: 가장 큰 손가락에 가중 집중 (tau↓, alpha↑ -> 더 집중)
    static inline std::array<double, 5> _softmaxWeights(const FingerDiff& d, const Params& prm) {
        const double v[5] = { d.T,d.I,d.M,d.R,d.P };
        const double vmax = *std::max_element(v, v + 5);
        std::array<double, 5> w{};
        double denom = 0.0;
        for (int k = 0; k < 5; ++k) {
            const double z = (v[k] - vmax) / std::max(1e-8, prm.tau);
            const double e = prm.user_w[k] * std::exp(prm.alpha * z);
            w[k] = e; denom += e;
        }
        for (int k = 0; k < 5; ++k) w[k] /= std::max(1e-12, denom);
        return w; // 합=1
    }

    static inline double _fingerWeightedScore(const FingerDiff& d, const std::array<double, 5>& w) {
        const double v[5] = { d.T,d.I,d.M,d.R,d.P };
        double s = 0.0;
        for (int k = 0; k < 5; ++k) s += w[k] * v[k] * v[k];
        return std::sqrt(s)/5;
    }

    // --------- 최종 스코어 ---------
    double FinalScore3D(const std::vector<float>& poseA,
        const std::vector<float>& poseB,
        const std::vector<cv::Point3f>& jointsA,
        const std::vector<cv::Point3f>& jointsB,
        const Params& prm)
    {
        if (poseA.size() != poseB.size()) return DBL_MAX;

        const double d_pca = ManoPoseL2(poseA, poseB);
        if (!std::isfinite(d_pca)) return DBL_MAX;

        const FingerDiff d = PerFingerAngleDiff3D(jointsA, jointsB);
        const auto w = _softmaxWeights(d, prm);
        const double d_f = _fingerWeightedScore(d, w);
        if (!std::isfinite(d_f)) return DBL_MAX;

        // PCA 부스터: 가장 큰 손가락 차이가 크면 PCA도 같이 키움
        const double vals[5] = { d.T,d.I,d.M,d.R,d.P };
        const double vmax = *std::max_element(vals, vals + 5);
        const double mean = (d.T + d.I + d.M + d.R + d.P) / 5.0;
        const double booster = 1.0 + prm.pca_boost_k * (vmax / std::max(prm.eps, mean));

        const double lambda = std::max(0.0, std::min(1.0, prm.lambda));
        return lambda * (d_pca * booster) + (1.0 - lambda) * d_f;
    }

    double FinalScore2D(const std::vector<float>& poseA,
        const std::vector<float>& poseB,
        const std::vector<cv::Point2f>& A2,
        const std::vector<cv::Point2f>& B2,
        const Params& prm)
    {
        if (A2.size() < 21 || B2.size() < 21) return DBL_MAX;
        std::vector<cv::Point3f> A3; A3.reserve(A2.size());
        std::vector<cv::Point3f> B3; B3.reserve(B2.size());
        for (auto& p : A2) A3.emplace_back(p.x, p.y, 0.0f);
        for (auto& p : B2) B3.emplace_back(p.x, p.y, 0.0f);
        return FinalScore3D(poseA, poseB, A3, B3, prm);
    }

} // namespace shapediff
