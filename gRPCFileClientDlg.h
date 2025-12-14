
// gRPCFileClientDlg.h: 헤더 파일
//

#include "VideoUtil-v1.1.h"

// Python/pybind11 임베딩(ACR, 문장추론 등 파이썬 모델 호출)
#include <python.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "HandTurnDetector.hpp"

#pragma once

//#define TARGETIP "localhost:50051"
#define TARGETIP "192.168.0.154:50051"

// 모션 상태(정지/말하기/급격동작 등)
#define RESET_MOTION_STATUS         -1  // 상태 초기화
#define READY_MOTION_STATUS         0   // 준비 상태
#define SPEAK_MOTION_STATUS         1   // 수어 발화 상태
#define RAPID_MOTION_STATUS         2   // 급격 동작 상태

// Optical Flow-based Motion Detection 파라미터
#define OPTICALFLOW_THRESH          0.45
#define OPTICAL_FLOW_HOLD_FRAME     3   // 모션 유지 프레임 수

#define READY_LOCATION              700

#define RAPID_DISTANCE              0.1 // 급격 동작 거리 기준(정규화 단위)

// ===============================
// pybind11 <-> cv::Mat 변환용 type_caster
// ===============================
//
// Python의 numpy.ndarray <-> OpenCV cv::Mat 간 상호 변환.
// 파이썬 ACR/AI 모듈과 OpenCV C++ 코드를 자연스럽게 연결하기 위해 사용.
//
namespace pybind11 {
    namespace detail {
        template<>
        struct type_caster<cv::Mat> {
        public:
            PYBIND11_TYPE_CASTER(cv::Mat, _("numpy.ndarray"));

            // 1. numpy.ndarray -> cv::Mat 변환
            bool load(handle obj, bool) {
                array b = reinterpret_borrow<array>(obj);
                buffer_info info = b.request();

                int nh = 1;
                int nw = 1;
                int nc = 1;
                int ndims = (int)info.ndim;

                if (ndims == 2) {
                    nh = (int)info.shape[0];
                    nw = (int)info.shape[1];
                }
                else if (ndims == 3) {
                    nh = (int)info.shape[0];
                    nw = (int)info.shape[1];
                    nc = (int)info.shape[2];
                }
                else {
                    char msg[64];
                    std::sprintf(msg,
                        "Unsupported dim %d, only support 2d, or 3-d", ndims);
                    throw std::logic_error(msg);
                    return false;
                }

                int dtype;
                if (info.format == format_descriptor<unsigned char>::format()) {
                    dtype = CV_8UC(nc);
                }
                else if (info.format == format_descriptor<int>::format()) {
                    dtype = CV_32SC(nc);
                }
                else if (info.format == format_descriptor<float>::format()) {
                    dtype = CV_32FC(nc);
                }
                else {
                    throw std::logic_error(
                        "Unsupported type, only support uchar, int32, float");
                    return false;
                }

                // data 포인터를 그대로 활용하는 얕은 래핑(복사X)
                value = cv::Mat(nh, nw, dtype, info.ptr);
                return true;
            }

            // 2. cv::Mat -> numpy.ndarray 변환
            static handle cast(const cv::Mat& mat,
                return_value_policy, handle defval) {
                UNUSED(defval);

                std::string format = format_descriptor<unsigned char>::format();
                size_t elemsize = sizeof(unsigned char);

                int nw = mat.cols;
                int nh = mat.rows;
                int nc = mat.channels();
                int depth = mat.depth();
                int type = mat.type();
                int dim = (depth == type) ? 2 : 3;

                if (depth == CV_8U) {
                    format = format_descriptor<unsigned char>::format();
                    elemsize = sizeof(unsigned char);
                }
                else if (depth == CV_32S) {
                    format = format_descriptor<int>::format();
                    elemsize = sizeof(int);
                }
                else if (depth == CV_32F) {
                    format = format_descriptor<float>::format();
                    elemsize = sizeof(float);
                }
                else {
                    throw std::logic_error(
                        "Unsupport type, only support uchar, int32, float");
                }

                std::vector<size_t> bufferdim;
                std::vector<size_t> strides;

                if (dim == 2) {
                    // 단일 채널: (h, w)
                    bufferdim = { (size_t)nh, (size_t)nw };
                    strides = { elemsize * (size_t)nw, elemsize };
                }
                else if (dim == 3) {
                    // 다채널: (h, w, c)
                    bufferdim = { (size_t)nh, (size_t)nw, (size_t)nc };
                    strides = {
                        (size_t)elemsize * nw * nc,
                        (size_t)elemsize * nc,
                        (size_t)elemsize
                    };
                }
                return array(buffer_info(
                    mat.data,
                    elemsize,
                    format,
                    dim,
                    bufferdim,
                    strides)).release();
            }
        };
    }
} // end namespace pybind11::detail


namespace py = pybind11;


// CgRPCFileClientDlg 대화 상자
class CgRPCFileClientDlg : public CDialogEx
{
// 생성입니다.
public:
	CgRPCFileClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.


// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GRPCFILECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	std::atomic<bool> m_exit;
	Rect m_roi;

	// CFG (환경설정, 최근 경로 등) 저장/로드
	void SaveCFG();
	void ReadCFG();

	//Video Player 관련
	std::atomic<bool> m_pause;
	std::atomic<bool> m_back;
	std::atomic<bool> m_move;
	std::atomic<bool> m_stop;
	HANDLE hPlayPause;
	CWinThread* hVideoPlay = nullptr;
	static UINT VideoPlay(LPVOID pParam);

	CVideoUtil m_util;

	afx_msg void OnBnClickedButton1();
	CEdit m_video_file_name_edit;
	CStatic m_cctv_frame;
	CStatic m_ROIcctv_frame;
	CEdit m_frame_index_edit;
	CEdit m_roi_edit;
	CEdit m_frame_range_edit;
	CEdit m_info_edit;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnBnClickedButton8();
	afx_msg void OnBnClickedButton10();

    cv::Mat m_cap_img;
    std::vector<cv::Point3f> m_sgcp_vtSkeletonMP;

    HandTurnDetector m_Rdetector;
    HandTurnDetector m_Ldetector;

	// AI 처리 시작/종료 이벤트 + AI 스레드 핸들
	HANDLE      hAIStart;
	HANDLE      hAIFinish;
	CWinThread* hAIThread = nullptr;
	// AI 스레드 실행 함수 (영상/프레임에 대해 ACR/MP 등 수행)
	static UINT AIThread(LPVOID pParam);

    CWinThread* hgRPCSend = nullptr;
    // AI 스레드 실행 함수 (영상/프레임에 대해 ACR/MP 등 수행)
    static UINT gRPCSend(LPVOID pParam);

    // Optical Flow 기반 모션 크기 계산(여러 오버로드)
    double GetMotionVauleWithOpticalFlow(
        cv::Mat prevGray,
        std::vector<cv::Point2f> prevPts,
        cv::Mat gray);

    double GetMotionVauleWithOpticalFlow(
        cv::Mat prev,
        cv::Mat frame,
        cv::Rect rect);

    double GetMotionVauleWithOpticalFlow(
        cv::Mat prev,
        cv::Mat frame,
        cv::Rect Rrect,
        cv::Rect Lrect);

    double GetMotionVauleWithOpticalFlow(cv::Mat frame);

    int GetRoughHandStatusFromMP(std::vector<Point3f> mp_pose);
    int GetMotionStatusFromMP(int cur_motion_status, std::vector<cv::Point3f> cur_mp);
    std::vector<Point2f> GetRefHL2DPointsMP(std::vector <cv::Point3f> joints); //0 Left, 1 Right

    // 곡선/그래프/warping 등에 사용하는 삼각 코너 좌표(예시)
    vector<Point2f> m_warpCorners = {
        Point2f(720, 500),
        Point2f(1200, 500),
        Point2f(960, 400)
    };
    
    afx_msg void OnBnClickedButton9();
};
