// HandShapeDiff.hpp
#pragma once
/*
  Header-only hand-shape difference metrics with fixed mu(45) and sigma(45).
  입력: std::vector<float> (길이 45)
  출력: DzL2, Dcos, Drob, Combined
*/

#include <vector>
#include <array>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace HandShapeDiff {

    constexpr int kDim = 45;

    // ===== Fixed mean (mu) and std (sigma) =====
    inline constexpr std::array<double, kDim> kMu = {
        -0.0107877,-0.17917,0.214436,0.000151044,-0.0289676,-0.288624,0.0940558,-0.11011,-0.0604065,
        -0.0508494,-0.0323973,0.378308,0.0126006,0.0267069,-0.223166,-0.070689,-0.0383971,-0.0617277,
        -0.0345397,0.179927,0.0307853,-0.0314454,-0.0405835,-0.0680428,-0.0160187,-0.00416727,0.142881,
        -0.0102941,0.00250916,0.23912,0.00700981,0.0396974,-0.104858,-0.0564946,-0.0594426,0.0622733,
        -0.0611285,0.0045575,-0.150548,-5.86734e-05,-0.0755417,0.196749,-0.0277526,-0.143554,0.0777195
    };

    inline constexpr std::array<double, kDim> kSigma = {
        0.055931,0.13582,0.399117,0.0420313,0.063684,0.388116,0.204485,0.0767902,0.246719,
        0.119485,0.106296,0.574167,0.0399533,0.0524782,0.392292,0.100434,0.0311385,0.258208,
        0.324902,0.167344,0.484995,0.198642,0.103681,0.382504,0.240011,0.110611,0.275251,
        0.145976,0.116291,0.564044,0.160473,0.0829934,0.424087,0.101042,0.0379307,0.279588,
        0.184361,0.175819,0.148311,0.136563,0.171149,0.147344,0.233296,0.318637,0.294859
    };

    // ===== Output struct =====
    struct Scores {
        double DzL2 = 0.0;    // Z-L2 거리
        double Dcos = 0.0;    // 1 - cosine( whitened v1, whitened v2 )
        double Drob = 0.0;    // Robust Huber-Mahalanobis
        double Combined = 0.0;
    };

    // ===== Internal helpers =====
    inline double dot45(const double* a, const double* b) {
        double s = 0.0;
        for (int i = 0; i < kDim; ++i) s += a[i] * b[i];
        return s;
    }
    inline double norm45(const double* a) {
        return std::sqrt(dot45(a, a));
    }
    inline double huber(double x, double delta) {
        const double ax = std::fabs(x);
        if (ax <= delta) return 0.5 * x * x;
        return delta * (ax - 0.5 * delta);
    }

    // ===== Core compute =====
    inline Scores Compute(const std::vector<float>& vf1,
        const std::vector<float>& vf2,
        double huber_delta = 1.5,
        double alpha = 1.0, double beta = 0.5, double gamma = 0.5)
    {
        if (vf1.size() != kDim || vf2.size() != kDim)
            throw std::invalid_argument("Input vectors must have length 45.");

        // double 버퍼로 변환
        double v1[kDim], v2[kDim];
        for (int i = 0; i < kDim; ++i) { v1[i] = static_cast<double>(vf1[i]); v2[i] = static_cast<double>(vf2[i]); }

        // z = (v1 - v2) / sigma
        double z[kDim], w1[kDim], w2[kDim];
        for (int i = 0; i < kDim; ++i) {
            const double s = kSigma[i];
            if (!(s > 0.0)) throw std::invalid_argument("kSigma contains non-positive value.");
            z[i] = (v1[i] - v2[i]) / s;
            w1[i] = (v1[i] - kMu[i]) / s;
            w2[i] = (v2[i] - kMu[i]) / s;
        }

        Scores S;

        // 1) DzL2
        S.DzL2 = norm45(z);

        // 2) Dcos
        const double n1 = norm45(w1);
        const double n2 = norm45(w2);
        double cosv = 0.0;
        if (n1 > 0.0 && n2 > 0.0) {
            cosv = dot45(w1, w2) / (n1 * n2);
            if (cosv > 1.0) cosv = 1.0;
            if (cosv < -1.0) cosv = -1.0;
        }
        else {
            cosv = -1.0;
        }
        S.Dcos = 1.0 - cosv;

        // 3) Drob
        double sumHuber = 0.0;
        for (int i = 0; i < kDim; ++i) sumHuber += huber(z[i], huber_delta);
        S.Drob = std::sqrt(sumHuber);

        // Combined
        S.Combined = alpha * S.DzL2 + beta * S.Dcos + gamma * S.Drob;
        return S;
    }

    inline double getShapeDifference(std::vector<float>& V1, std::vector<float>& V2)
    {
        if (V1.size() != V2.size()) return DBL_MIN;

        auto S = HandShapeDiff::Compute(V1, V2);


        return 1.0 - S.Dcos; // similarity
    }

} // namespace HandShapeDiff
