

// HandConfidence.h ? header?only C++ class
// Computes scalar and per?joint confidence from ACR vs MediaPipe 2D joints
// Requirements: C++17, OpenCV 4.x
// Usage:
//   #include "HandConfidence.h"
//   handconf::Hand2D acr{acr_pts, acr_vis}, mp{mp_pts, mp_vis};
//   handconf::Options opt; // defaults: identity mapping (0..20), Procrustes on, Huber on, bbox scaling
//   handconf::HandConfidence hc(opt);
//   auto out = hc.compute(acr, mp);
//   double conf = out.confidence; // exp(-alpha * error)
//   // See demo() at bottom for a minimal example

#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>

namespace handconf {

    struct Hand2D {
        std::vector<cv::Point2f> joints;  // size N (default 21)
        std::vector<float>        vis;     // optional, same size or empty (treated as 1.0)
    };

    struct Options {
        int num_joints = 21;               // assumes 21 for ACR/Mediapipe
        double alpha = 8.0;                // confidence scale in exp(-alpha * err)

        bool use_procrustes = true;        // align Q->P with similarity transform
        bool normalize_by_scale = true;    // divide distances by scale
        std::string scale_mode = "bbox";  // "bbox" or "anchor"
        int anchor_a = 0;                  // when scale_mode=="anchor"
        int anchor_b = 12;                 // wrist¡êmiddle_tip as default

        bool use_huber = true;             // robust loss
        double huber_delta = 0.02;         // after normalization (e.g., 2% of bbox diag)
        bool huber_per_joint = true;       // apply Huber per?joint before averaging

        bool try_mirror_x = false;         // also compare mirrored MP and take min error

        float visibility_thresh = 0.0f;    // ignore joints with vis < thresh
        std::vector<float> joint_weights;  // size==num_joints or empty (all 1)

        // mp index -> acr index mapping; if empty, identity(0..num_joints-1)
        std::vector<int> acr_index_of_mp;
    };

    struct Detail {
        double err_raw = 0.0;              // reserved
        double err_aligned = 0.0;          // error after alignment
        double scalar_error = 0.0;         // final error for confidence
        double confidence = 0.0;           // exp(-alpha * scalar_error)
        bool   used_mirror = false;        // true if mirrored path chosen

        cv::Matx22f R = cv::Matx22f::eye();
        float s = 1.0f;
        cv::Vec2f t = { 0.f, 0.f };

        std::vector<float> per_joint_err;  // normalized L2 per joint
        std::vector<float> per_joint_conf; // exp(-alpha * per_joint_err[i])
    };

    class HandConfidence {
    public:
        explicit HandConfidence(const Options& opt = Options{}) : opt_(opt) {
            if (opt_.num_joints <= 0) throw std::invalid_argument("num_joints must be > 0");
            if (opt_.acr_index_of_mp.empty()) {
                opt_.acr_index_of_mp.resize(opt_.num_joints);
                for (int i = 0;i < opt_.num_joints;++i) opt_.acr_index_of_mp[i] = i; // identity mapping
            }
        }

        const Options& options() const { return opt_; }
        Options& options() { return opt_; }

        // Main entry
        Detail compute(const Hand2D& acr, const Hand2D& mp) const {
            Detail out;
            std::vector<cv::Point2f> P, Q; // P: ACR, Q: MP
            std::vector<float> W;
            gatherMatched(acr, mp, opt_.acr_index_of_mp, P, Q, W);
            applyJointWeights(W, opt_.joint_weights);
            applyVisibilityThresh(W, opt_.visibility_thresh);

            const int valid = countPositive(W);
            if (P.empty() || Q.empty() || valid < 3) {
                out.scalar_error = 1.0; // conservative
                out.confidence = std::exp(-opt_.alpha * out.scalar_error);
                return out;
            }

            Detail baseCase; computePath(P, Q, W, baseCase);
            if (opt_.try_mirror_x) {
                std::vector<cv::Point2f> Qm = Q; mirrorX(Qm);
                Detail mirrorCase; computePath(P, Qm, W, mirrorCase);
                out = (mirrorCase.scalar_error < baseCase.scalar_error) ? mirrorCase : baseCase;
                out.used_mirror = (mirrorCase.scalar_error < baseCase.scalar_error);
            }
            else {
                out = baseCase;
            }
            out.confidence = std::exp(-opt_.alpha * out.scalar_error);
            return out;
        }

        // Quick helper to build identity mapping of given size
        static std::vector<int> identityMap(int n) { std::vector<int> m(n); for (int i = 0;i < n;++i) m[i] = i; return m; }

    private:
        Options opt_;

        static int countPositive(const std::vector<float>& W) { int c = 0; for (float w : W) if (w > 0) ++c; return c; }

        static void gatherMatched(const Hand2D& acr, const Hand2D& mp, const std::vector<int>& map,
            std::vector<cv::Point2f>& P, std::vector<cv::Point2f>& Q, std::vector<float>& W) {
            const int M = (int)map.size();
            P.clear(); Q.clear(); W.clear(); P.reserve(M); Q.reserve(M); W.reserve(M);
            for (int k = 0;k < M;++k) {
                int ai = map[k]; if (ai < 0) { continue; }
                if (ai >= (int)acr.joints.size() || k >= (int)mp.joints.size()) continue;
                cv::Point2f p = acr.joints[ai];
                cv::Point2f q = mp.joints[k];
                float vw = 1.f;
                float v_ac = (ai < (int)acr.vis.size() ? acr.vis[ai] : 1.f);
                float v_mp = (k < (int)mp.vis.size() ? mp.vis[k] : 1.f);
                vw *= v_ac * v_mp;
                P.push_back(p); Q.push_back(q); W.push_back(vw);
            }
        }

        static void applyJointWeights(std::vector<float>& W, const std::vector<float>& jw) {
            if (jw.empty()) return; const int n = std::min((int)W.size(), (int)jw.size());
            for (int i = 0;i < n;++i) W[i] *= jw[i];
        }
        static void applyVisibilityThresh(std::vector<float>& W, float thr) { if (thr <= 0) return; for (auto& w : W) if (w < thr) w = 0.f; }

        static void mirrorX(std::vector<cv::Point2f>& Q) {
            if (Q.empty()) return; float meanx = 0.f; for (auto& p : Q) meanx += p.x; meanx /= Q.size();
            for (auto& p : Q) p.x = 2.f * meanx - p.x;
        }

        void computePath(const std::vector<cv::Point2f>& P,
            const std::vector<cv::Point2f>& Q,
            const std::vector<float>& W,
            Detail& res) const {
            std::vector<cv::Point2f> Q_aligned;
            float s = 1.f; cv::Matx22f R = cv::Matx22f::eye(); cv::Vec2f t(0, 0);
            if (opt_.use_procrustes) procrustes2D(P, Q, W, s, R, t, Q_aligned);
            else Q_aligned = Q;

            std::vector<float> e_joint; double scalar_error = 0.0;
            computeErrors(P, Q_aligned, W, opt_.normalize_by_scale, opt_.scale_mode, opt_.anchor_a, opt_.anchor_b,
                opt_.use_huber, opt_.huber_delta, opt_.huber_per_joint,
                e_joint, scalar_error);
            res.per_joint_err = std::move(e_joint);
            res.scalar_error = scalar_error;
            res.err_aligned = scalar_error;
            res.R = R; res.s = s; res.t = t;
            res.per_joint_conf.resize(res.per_joint_err.size());
            for (size_t i = 0;i < res.per_joint_err.size();++i)
                res.per_joint_conf[i] = std::exp(-opt_.alpha * (double)res.per_joint_err[i]);
        }

        static void procrustes2D(const std::vector<cv::Point2f>& P,
            const std::vector<cv::Point2f>& Q,
            const std::vector<float>& W,
            float& s, cv::Matx22f& R, cv::Vec2f& t,
            std::vector<cv::Point2f>& Q_aligned) {
            auto wmean = [&](const std::vector<cv::Point2f>& X) { double sx = 0, sy = 0, sw = 0; for (size_t i = 0;i < X.size();++i) { double w = std::max(1e-8f, W[i]); sx += w * X[i].x; sy += w * X[i].y; sw += w; } return (sw > 0) ? cv::Point2f((float)(sx / sw), (float)(sy / sw)) : cv::Point2f(0, 0); };
            cv::Point2f muP = wmean(P), muQ = wmean(Q);
            std::vector<cv::Point2f> Pc(P.size()), Qc(Q.size());
            for (size_t i = 0;i < P.size();++i) { Pc[i] = P[i] - muP; Qc[i] = Q[i] - muQ; }
            cv::Mat H = cv::Mat::zeros(2, 2, CV_32F); double denom = 0.0;
            for (size_t i = 0;i < P.size();++i) {
                float w = std::max(1e-8f, W[i]); cv::Vec2f q(Qc[i].x, Qc[i].y), p(Pc[i].x, Pc[i].y);
                H.at<float>(0, 0) += w * q[0] * p[0]; H.at<float>(0, 1) += w * q[0] * p[1]; H.at<float>(1, 0) += w * q[1] * p[0]; H.at<float>(1, 1) += w * q[1] * p[1];
                denom += w * (q[0] * q[0] + q[1] * q[1]);
            }
            cv::SVD svd(H, cv::SVD::FULL_UV); cv::Mat U = svd.u, Vt = svd.vt; cv::Mat Rm = U * Vt; if (cv::determinant(Rm) < 0) { cv::Mat B = cv::Mat::eye(2, 2, CV_32F); B.at<float>(1, 1) = -1.f; Rm = U * B * Vt; }
            R = cv::Matx22f(Rm); double tr = svd.w.at<float>(0) + svd.w.at<float>(1); s = (denom > 0) ? (float)(tr / denom) : 1.f;
            cv::Vec2f muP_v(muP.x, muP.y), muQ_v(muQ.x, muQ.y); t = muP_v - s * (R * muQ_v);
            Q_aligned.resize(Q.size()); for (size_t i = 0;i < Q.size();++i) { cv::Vec2f q(Q[i].x, Q[i].y); cv::Vec2f qp = s * (R * q) + t; Q_aligned[i] = cv::Point2f(qp[0], qp[1]); }
        }

        static float computeScale(const std::vector<cv::Point2f>& P, const std::vector<float>& W,
            const std::string& mode, int a, int b) {
            if (P.empty()) return 1.f;
            if (mode == "anchor" && a >= 0 && b >= 0 && a < (int)P.size() && b < (int)P.size()) {
                float d = cv::norm(P[a] - P[b]); return std::max(1.f, d);
            }
            float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f; for (size_t i = 0;i < P.size();++i) { if (W[i] <= 0) continue; minx = std::min(minx, P[i].x); miny = std::min(miny, P[i].y); maxx = std::max(maxx, P[i].x); maxy = std::max(maxy, P[i].y); }
            if (maxx < minx + 1e-6f || maxy < miny + 1e-6f) return 1.f; float dx = maxx - minx, dy = maxy - miny; float diag = std::sqrt(dx * dx + dy * dy); return std::max(1.f, diag);
        }

        static inline float huber(float r, float d) { float ar = std::fabs(r); return (ar <= d) ? 0.5f * r * r : d * (ar - 0.5f * d); }

        static void computeErrors(const std::vector<cv::Point2f>& P, const std::vector<cv::Point2f>& Q,
            const std::vector<float>& W, bool norm_scale, const std::string& scale_mode,
            int a, int b, bool use_huber, double huber_delta, bool huber_per_joint,
            std::vector<float>& per_joint_err, double& scalar_error) {
            per_joint_err.assign(P.size(), 0.f);
            float scale = 1.f; if (norm_scale) scale = computeScale(P, W, scale_mode, a, b);
            double sw = 0.0, acc = 0.0;
            for (size_t i = 0;i < P.size();++i) { if (W[i] <= 0) continue; float d = cv::norm(P[i] - Q[i]) / scale; per_joint_err[i] = d; if (use_huber && huber_per_joint) acc += W[i] * huber(d, (float)huber_delta); else acc += W[i] * (d * d); sw += W[i]; }
            if (sw <= 0) { scalar_error = 1.0; return; }
            if (use_huber && huber_per_joint) scalar_error = acc / sw; else { scalar_error = std::sqrt(acc / sw); if (use_huber && !huber_per_joint) scalar_error = huber((float)scalar_error, (float)huber_delta); }
        }
    };

    // --- Minimal demo (define HANDCONF_DEMO to build) ---
#ifdef HANDCONF_DEMO
#include <iostream>
    inline int demo() {
        Hand2D acr, mp; acr.joints.resize(21); mp.joints.resize(21); acr.vis.assign(21, 1.f); mp.vis.assign(21, 1.f);
        for (int i = 0;i < 21;++i) { acr.joints[i] = cv::Point2f(100 + i * 2, 100 + i * 1); mp.joints[i] = cv::Point2f(102 + i * 2, 98 + i * 1); }
        Options opt; opt.alpha = 8.0; opt.try_mirror_x = false; // identity mapping by default
        HandConfidence HC(opt); auto out = HC.compute(acr, mp);
        std::cout << "err=" << out.scalar_error << " conf=" << out.confidence << " mirror=" << out.used_mirror << "\n";
        return 0;
    }
#endif

} // namespace handconf
