

#include "ksl_sentence_recognition.grpc.pb.h"  // 당신의 *_grpc.pb.h

#include <grpcpp/grpcpp.h>

#include <opencv2/opencv.hpp>

#include <iostream>
#include <fstream>
#include <thread>

#include "gRPCFileClientDlg.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;
using vision::raw::v1::Frame;
using vision::raw::v1::SubmitResultResponse;
using vision::raw::v1::SequenceService;

#pragma once

namespace grpc_client_ {


    // UTF-8 -> std::wstring
    inline std::wstring Utf8ToWstring(const std::string& s) {
        if (s.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
        std::wstring w(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], len);
        return w;
    }

    // std::wstring -> UTF-8
    inline std::string WstringToUtf8(const std::wstring& w) {
        if (w.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::string s(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], len, nullptr, nullptr);
        return s;
    }

    // CString <-> UTF-8
    inline std::string CStringToUtf8(const CString& cs) {
#ifdef UNICODE
        return WstringToUtf8(std::wstring(cs));
#else
        // CStringA일 때는 먼저 UTF-16으로 변환 후 UTF-8로
        int lenW = MultiByteToWideChar(CP_ACP, 0, cs, -1, nullptr, 0);
        std::wstring w(lenW - 1, 0);
        MultiByteToWideChar(CP_ACP, 0, cs, -1, &w[0], lenW - 1);
        return WstringToUtf8(w);
#endif
    }

    inline CString Utf8ToCString(const std::string& s) {
#ifdef UNICODE
        return CString(Utf8ToWstring(s).c_str());
#else
        // UTF-8 -> UTF-16 -> ANSI
        std::wstring w = Utf8ToWstring(s);
        int lenA = WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::string a(lenA, 0);
        WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), &a[0], lenA, nullptr, nullptr);
        return CString(a.c_str());
#endif
    }

    inline size_t ElemSizeFromType(int cv_type) {
        return CV_ELEM_SIZE(cv_type); // OpenCV 매크로
    }

    inline bool EncodeFrame(const cv::Mat& img,
        const std::string& session_id,
        int index, int flag, std::vector<cv::Point3f>& mpPose3D,
        Frame* out)
    {
        if (!out) return false;
        if (img.empty()) return false;

        // 연속 메모리 보장 (protobuf로 직렬화할 때는 연속 버퍼가 편함)
        cv::Mat contiguous;
        const cv::Mat& src = img.isContinuous() ? img : (contiguous = img.clone());

        const int width = src.cols;
        const int height = src.rows;
        const int type = src.type();
        const size_t elem_size = ElemSizeFromType(type);
        const size_t expected = static_cast<size_t>(width) * height * elem_size;

        // Frame 필드 세팅
        out->Clear();
        out->set_session_id(session_id);
        out->set_index(index);
        out->set_flag(flag);
        out->set_width(width);
        out->set_height(height);
        out->set_type(type);
     
        for (const auto& p : mpPose3D) {
            auto* pt = out->add_pose_points();
            pt->set_x(p.x);
            pt->set_y(p.y);
            pt->set_z(p.z);
        }

        // 데이터 복사 (protobuf는 소유권을 가져야 하므로 copy)
        out->set_data(reinterpret_cast<const char*>(src.data), expected);
        return true;
    }

    /**
     * @brief Frame -> cv::Mat
     * @details Frame.data()는 행간 padding 없이 "꽉 찬" 형태로 직렬화되어 있다고 가정
     * @return 변환된 Mat (실패 시 empty Mat)
     */
    inline cv::Mat FrameToMat(const Frame& f)
    {
        const int width = f.width();
        const int height = f.height();
        const int type = f.type();

        if (width <= 0 || height <= 0) return {};

        const size_t elem_size = ElemSizeFromType(type);
        const size_t expected = static_cast<size_t>(width) * height * elem_size;

        const std::string& blob = f.data();
        if (blob.size() != expected) {
            // 크기 불일치: 프로토콜/전송 측 정합성 문제
            return {};
        }

        // data 포인터를 참조하는 Mat를 만들고, 바로 clone() 해서 독립 데이터로 보관
        // (protobuf 문자열 수명과 무관하게 사용 가능)
        cv::Mat wrapped(height, width, type,
            const_cast<char*>(blob.data())); // 참조만
        return wrapped.clone(); // 안전하게 소유 복사
    }

    /** (선택) BGR<->RGB 변환 헬퍼
     * OpenCV 기본은 BGR. 상대 쪽이 RGB를 기대한다면 사용.
     */
    inline cv::Mat BGR2RGB(const cv::Mat& bgr) {
        cv::Mat rgb;
        if (!bgr.empty()) cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
        return rgb;
    }

    inline cv::Mat RGB2BGR(const cv::Mat& rgb) {
        cv::Mat bgr;
        if (!rgb.empty()) cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    }

    class SequenceClient {
    public:
        SequenceClient(std::shared_ptr<Channel> channel)
            : stub_(SequenceService::NewStub(channel)) {
        }

        CgRPCFileClientDlg* pDlg;

        void SendFrames(const std::string& session_id) {
            ClientContext context;
            SubmitResultResponse response;
            std::unique_ptr<ClientWriter<Frame>> writer(
                stub_->SendFrames(&context, &response));
          
            CString file_str, frame_range_str;

            pDlg->m_video_file_name_edit.GetWindowText(file_str);
            pDlg->m_frame_range_edit.GetWindowText(frame_range_str);

            if (!file_str.IsEmpty() && !frame_range_str.IsEmpty())
            {

                CRect Rect1;
                pDlg->m_cctv_frame.GetClientRect(Rect1);

                CRect Rect2;
                pDlg->m_ROIcctv_frame.GetClientRect(Rect2);

                auto strs2 = pDlg->m_util.SplitCString(frame_range_str);

                if (strs2.size() >= 2)
                {
                    double dt = 1.0 / 30.0;
                    int frozenflag = 0;
                    int speakstartflag = 0;
                    int RTurncnt = 0;
                    int Lturncnt = 0;
                    int Holdcnt = 0;
                    int motion_status = READY_MOTION_STATUS;

                    cv::Mat empty;
                    pDlg->GetMotionVauleWithOpticalFlow(empty);
                    pDlg->GetMotionStatusFromMP(RESET_MOTION_STATUS, {});

                    pDlg->m_Rdetector.reset();
                    pDlg->m_Ldetector.reset();               

                    VideoCapture cap(pDlg->m_util.StringToChar(file_str));

                    Mat img;
                    if (cap.isOpened())
                    {
                        auto [w, h, fps, totalFrames] = pDlg->m_util.GetVideoInfo(cap);

                        CString str;
                        str.Format(_T("[비디오파일 정보]\r\n%s\r\nwidht:%d height:%d fps:%.2f total frames:%d"), file_str, w, h, fps, totalFrames);
                        pDlg->m_info_edit.SetWindowText(str);

                        cap.set(cv::CAP_PROP_POS_FRAMES, _ttoi(strs2[0]));  // targetIndex 프레임으로 이동

                        double frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);

                        while (pDlg->m_exit.load() == false)
                        {
                            cap >> img;
                            int frameIndex = (int)cap.get(cv::CAP_PROP_POS_FRAMES);
                            if (img.empty() || frameIndex > _ttoi(strs2[1])) break;
                            auto cap_img = img.clone();
                            rectangle(cap_img, pDlg->m_roi, Scalar(0, 255, 0), 3);

                            pDlg->m_util.DrawImageBMP(&pDlg->m_cctv_frame, cap_img, 0, 0, (double)frameIndex / (totalFrames - 1), frameIndex);
                            cv::Mat roi = img(pDlg->m_roi).clone();
                            pDlg->m_util.DrawImageBMP(&pDlg->m_ROIcctv_frame, roi, 0, 0);
                            if (roi.cols >= 320)
                            {
                                cv::Mat resize_roi_img;
                                double src_x = static_cast<double>(roi.cols);
                                double src_y = static_cast<double>(roi.rows);
                                // 목표 가로 크기
                                double target_x = 320.0;
                                // 비율 계산
                                double scale = target_x / src_x;    // x축 기준 스케일
                                double dst_x_rate = scale;
                                double dst_y_rate = scale;          // y축 동일 비율 적용
                                // 이미지 리사이즈
                                cv::resize(roi, resize_roi_img, cv::Size(), dst_x_rate, dst_y_rate, cv::INTER_LINEAR);
                                pDlg->m_cap_img = resize_roi_img;
                            }
                            else pDlg->m_cap_img = roi;

                            SetEvent(pDlg->hAIStart);
                            WaitForSingleObject(pDlg->hAIFinish, INFINITE); //Wait for results		

                            if (pDlg->m_sgcp_vtSkeletonMP.size() > 0)
                            {
                                int hand_status = pDlg->GetRoughHandStatusFromMP(pDlg->m_sgcp_vtSkeletonMP); //손의 위치로 준비, 우세손, 비우세손, 양손 결정
                                motion_status = pDlg->GetMotionStatusFromMP(motion_status, pDlg->m_sgcp_vtSkeletonMP); //준비단계인지 수어 단계인지 구분

                                //             if (hand_status !=0 && (motion_status==SPEAK_MOTION_STATUS || frame_count > MIN_FRAME) )
                                if (hand_status != 0)
                                {
                                    double  avgMotion = pDlg->GetMotionVauleWithOpticalFlow(roi);

                                    bool Rhit = pDlg->m_Rdetector.update({ pDlg->m_sgcp_vtSkeletonMP[16].x, pDlg->m_sgcp_vtSkeletonMP[16].y }, dt);
                                    bool Lhit = pDlg->m_Ldetector.update({ pDlg->m_sgcp_vtSkeletonMP[15].x, pDlg->m_sgcp_vtSkeletonMP[15].y }, dt);

                                    if (Rhit)
                                    {
                                        RTurncnt++;
                                        speakstartflag = 1;
                                        //		m_cp_indexs.push_back((int)(frame_count));
                                    }
                                    else if (Lhit)
                                    {
                                        Lturncnt++;
                                        speakstartflag = 1;
                                        //		m_cp_indexs.push_back((int)(frame_count));
                                    }
                                    else if (speakstartflag && avgMotion < OPTICALFLOW_THRESH && frozenflag == 0)
                                    {
                                        Holdcnt++;                             

                                        Frame frame;
                                        EncodeFrame(roi, session_id, frameIndex, 1, pDlg->m_sgcp_vtSkeletonMP,&frame);
                                        writer->Write(frame);

                                        frozenflag = OPTICAL_FLOW_HOLD_FRAME;
                                    }
                                }
                            }
                            if (frozenflag > 0) frozenflag--;
                        }
                        cap.release();
                    }                    
                }
            }
            writer->WritesDone();
            Status status = writer->Finish();

            if (status.ok()) {
               
                std::wstring msg = Utf8ToWstring(response.message());
                CStringW cws(msg.c_str());
                CStringW str;
                str.Format(_T("[문장추론결과]\r\n%s"), cws);
                pDlg->m_info_edit.SetWindowTextW(str);              
            }
            else {
                std::cerr << "[Client] RPC failed: " << status.error_message() << "\n";
            }
        }

    private:
        std::unique_ptr<SequenceService::Stub> stub_;
    };
}


