#pragma once

#include <grpcpp/grpcpp.h>
#include "ksl_sentence_recognition.grpc.pb.h"
#include <iostream>
#include <fstream>

#include "KSLDBManagerDlg.h"
//#include <opencv2/opencv.hpp>

//#include "HandTurnDetector.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReader;
using vision::raw::v1::Frame;
using vision::raw::v1::SubmitResultResponse;
using vision::raw::v1::SequenceService;

namespace grpc_thread {

    CKSLDBManagerDlg* pDlg;

	unsigned int WINAPI FileServerThread(void* arg);
	HANDLE hFileServerThread = NULL;

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

    inline bool MatToFrame(const cv::Mat& img,
        const std::string& session_id,
        int index, int flag,
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

        // 데이터 복사 (protobuf는 소유권을 가져야 하므로 copy)
        out->set_data(reinterpret_cast<const char*>(src.data), expected);
        return true;
    }

    /**
     * @brief Frame -> cv::Mat
     * @details Frame.data()는 행간 padding 없이 "꽉 찬" 형태로 직렬화되어 있다고 가정
     * @return 변환된 Mat (실패 시 empty Mat)
     */
    inline cv::Mat DecodeFrame(const Frame& f)
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

    class SequenceServiceImpl final : public SequenceService::Service {
    public:
        Status SendFrames(ServerContext* context,
            ServerReader<Frame>* reader,
            SubmitResultResponse* reply) override {

            Frame frame;
            int frame_count = 0;
            std::string session_id;
           
            pDlg->m_infered.clear();
         
            while (reader->Read(&frame)) {

                // 예: 프레임을 파일로 저장하고 싶다면
                if (frame.flag() == 0) {
                    std::cout << "  [START of stream]\n";
          
                }
                else if (frame.flag() == 2) {
                    std::cout << "  [END of stream]\n";
                }
                std::vector<cv::Point3f> pose3d;
                for (const auto& p : frame.pose_points()) {
                    cv::Point3f cp;
                    cp.x = p.x();
                    cp.y = p.y();
                    cp.z = p.z();
                    pose3d.push_back(cp);
                }

                cv::Mat img = DecodeFrame(frame);      

                frame_count++;

                if (!img.empty())
                {
                    pDlg->m_DB._ClearChangepointDB(pDlg->m_result);
                    pDlg->m_cap_img = img;
                    pDlg->m_result.sgcp_vtSkeletonMP = pose3d;
                    pDlg->m_ai_mode = AI_MODE_HAND_DETECT_NEW_DB;
                    SetEvent(pDlg->hAIStart);
                    WaitForSingleObject(pDlg->hAIFinish, INFINITE); //Wait for results	

                    pDlg->m_result.sgcp_vtHandStatus = pDlg->GetHandStatusFromCP(pDlg->m_result);

                    if (pDlg->m_result.sgcp_vtHandStatus != 0)
                    {

                        pDlg->m_util.DrawImageBMP(&pDlg->m_ROIcctv_frame, pDlg->m_cap_img, 0, 0);

                        std::vector<_tagChangepointDB> result_cps;

                        auto tmp_result_cps = pDlg->FindSimilarCPs(pDlg->m_result, pDlg->m_DB.m_CpDB);


                        if (tmp_result_cps.size() > 0 && tmp_result_cps[0].sgcp_Confidence > 0)
                        {
                            GlossInfered infer;
                            infer.frame_index = (int)frame.index();
                            infer.infer_order = 0;
                            infer.sgIndex = tmp_result_cps[0].sgcp_sgIndex;
                            infer.cpIndex = tmp_result_cps[0].index;
                            infer.prob = tmp_result_cps[0].SimilarScore;
                            pDlg->m_infered.push_back(infer);
                        }
                    }
                }
            }

            CString final_str=_T("수어영상인식 테스트");
     
            if (pDlg->m_infered.size() > 0)
            {
                //Infer Corpus

                auto corpus = pDlg->m_infer_sentence.InferCorpus(pDlg->m_infered, pDlg->m_DB.m_CorpusVideoDB);
              
                if (corpus.scv_Sentence.IsEmpty())
                {
                    final_str = _T("인식이 되지 않았습니다. 천천히 말씀해 주세요....");
                }
                else
                {
                    final_str = corpus.scv_Sentence;
                }             
            }   

            CString str;
            str.Format(_T("[인식서버 결과]\r\n%s\r\n%d 프레임수신"), final_str, frame_count);
            pDlg->m_info_edit.SetWindowTextA(str);

            reply->set_session_id(session_id);
            reply->set_frame_count(frame_count);
                     
            reply->set_message(CStringToUtf8(final_str));
            std::cout << "[Server] Finished receiving. Total: " << frame_count << " frames.\n";  

            return Status::OK;
        }
    };



	unsigned int WINAPI FileServerThread(void* arg)
	{
	    pDlg = (CKSLDBManagerDlg*)arg;

        std::string server_address("0.0.0.0:50051");
        SequenceServiceImpl service;

        ServerBuilder builder;

        const int kMaxMsg = 64 * 1920 * 1080;
        builder.SetMaxReceiveMessageSize(kMaxMsg); // 서버가 받을 수 있는 최대 메시지
        builder.SetMaxSendMessageSize(kMaxMsg);    // 서버가 보낼 수 있는 최대 메시지

        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "[Server] Listening on " << server_address << std::endl;

        server->Wait();

		return 1;
	}


	void RunThread(void *arg)
	{
		if (hFileServerThread == NULL)
		{
			hFileServerThread = (HANDLE)_beginthreadex(NULL, 0, FileServerThread, arg, 0, NULL);
		}
	}
}
