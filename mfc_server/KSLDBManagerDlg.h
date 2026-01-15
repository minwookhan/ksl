#pragma once

// KSLDBManagerDlg.h: 헤더 파일
//
// - 수어 DB 관리/표제어 탐색/AI 인식/시각화 등을 총괄하는 메인 다이얼로그 클래스
// - OpenXLSX, OpenCV, pybind11, CUDA optical flow, ChangePointDetector, InferSentence 등
//   여러 모듈을 묶어서 사용하는 상위 레벨 UI/로직을 담당

#include "ETRI_KSL_Excel_DB-v1.4.h"  // 수어 표제어/코퍼스/영상 DB 관리용 클래스
#include "VideoUtil-v1.1.h"          // MFC + OpenCV 기반 영상/텍스트 그리기 유틸
#include <OpenXLSX.hpp>              // Excel 파일 접근용(OpenXLSX)
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <limits>

// Python/pybind11 임베딩(ACR, 문장추론 등 파이썬 모델 호출)
#include <python.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <atomic>                    // std::atomic<bool> 등
#include <Windows.h>

#include "ChangePointDetector.h"     // 수어 동작 구간(체인지포인트) 탐지 관련
#include <afxrich.h>                 // CRichEditCtrl/CRichEditView
#include <chrono>
#include <opencv2/cudaoptflow.hpp>   // GPU Optical Flow
#include "InferSentence.h"           // 문장 추론(GlossInfered -> 문장) 모듈


// ===============================
// AI 모드/파라미터 상수 정의
// ===============================

#define _NONE_                      0   // AI 비활성/초기 상태
#define AI_MODE_HAND_DETECT_NEW_DB  7   // 새로운 Hand DB 기반 탐지 모드


// "준비 동작(ready pose)" 판별 관련 기준 값
#define READY_LOCATION              700
#define READY_HANDTIP_ANGLE         70.0
#define READY_ELBOW_DIRECTION_ANGLE 80
#define READY_ELBOW_ANGLE           90


// 모션 상태(정지/말하기/급격동작 등)
#define RESET_MOTION_STATUS         -1  // 상태 초기화
#define READY_MOTION_STATUS         0   // 준비 상태
#define SPEAK_MOTION_STATUS         1   // 수어 발화 상태
#define RAPID_MOTION_STATUS         2   // 급격 동작 상태

#define MIN_FRAME                   20  // 최소 프레임 수(모션 판별)
#define RAPID_DISTANCE              0.1 // 급격 동작 거리 기준(정규화 단위)

// Optical Flow-based Motion Detection 파라미터
#define OPTICALFLOW_THRESH          0.45
#define OPTICAL_FLOW_HOLD_FRAME     3   // 모션 유지 프레임 수

#define SIMILAR_FRAME_NUM           3   // 유사 포즈 탐색 시 최소 유사 프레임 수

// 유사도 계산 시 가중치 (Hand Shape / Orientation / Location)
#define HS_WEIGHT                   0.8
#define HO_WEIGHT                   0.7
#define HL_WEIGHT                   0.4

// ===============================
// ACR/손 검출 결과 구조체
// ===============================
//
// 한 손(또는 양손) 검출 결과를 담기 위한 구조체.
// - hand_type : 0 = Left, 1 = Right 등
// - rect      : 손 ROI 영역
// - jpts      : 2D joint (픽셀 좌표)
// - jpt3ds    : 3D joint
// - poses     : MANO pose 등 각도 벡터
// - conf      : 신뢰도
// - verts     : MANO 메쉬 vertex
// - faces     : MANO 메쉬 face index
// - orientaion: orientation 벡터(손 방향 벡터 등)
// - jpt3d_center : 중심 joint 위치
//
typedef struct
{
    int hand_type;
    cv::Rect rect;
    std::vector<Point2i>  jpts;
    std::vector<Point3f>  jpt3ds;
    std::vector<float>    poses;
    float                 conf;
    std::vector<Point3f>  verts;
    std::vector<Point3f>  faces;
    std::vector<Point3f>  orientaion;
    Point3f               jpt3d_center;
} Result;


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

using namespace std;
using namespace OpenXLSX;
namespace py = pybind11;


// =============================================
// CKSLDBManagerDlg 대화 상자
//  - 수어 영상/표제어/체인지포인트/AI 인식 등 전체 워크플로우 UI
// =============================================
class CKSLDBManagerDlg : public CDialogEx
{
    // 생성입니다.
public:
    CKSLDBManagerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

    // 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_KSLDBMANAGER_DIALOG };
#endif

private:
    // =========================================================
    // 1) 일반 상수/좌표 관련 멤버
    // =========================================================

    // 1920x1080을 기준으로 1280x720 좌표를 스케일링하기 위한 비율
    double  x_scale = (1920.0 / 1280.0);
    double  y_scale = (1080.0 / 720.0);

    // 1080p 기준 참조 포즈 joint 위치(주로 정규화/기준 정렬에 사용)
    //  - MediaPipe Pose joint index 순서에 맞춰 세팅된 레퍼런스 포인트
    std::vector<Point2f> m_ref_joint_pts_1080p = {
        Point2f(640 * x_scale, 564 * y_scale),
        Point2f(640 * x_scale, 485 * y_scale),
        Point2f(640 * x_scale, 421 * y_scale),
        Point2f(640 * x_scale, 324 * y_scale),
        Point2f(650 * x_scale, 340 * y_scale),
        Point2f(712 * x_scale, 345 * y_scale),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(630 * x_scale, 340 * y_scale),
        Point2f(568 * x_scale, 345 * y_scale),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(665 * x_scale, 564 * y_scale),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(615 * x_scale, 564 * y_scale),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(-1, -1),
        Point2f(640 * x_scale, 286 * y_scale),
        Point2f(640 * x_scale, 247 * y_scale),
        Point2f(655 * x_scale, 231 * y_scale),
        Point2f(680 * x_scale, 253 * y_scale),
        Point2f(625 * x_scale, 231 * y_scale),
        Point2f(600 * x_scale, 253 * y_scale)
    };

    // 곡선/그래프/warping 등에 사용하는 삼각 코너 좌표(예시)
    vector<Point2f> m_warpCorners = {
        Point2f(720, 500),
        Point2f(1200, 500),
        Point2f(960, 400)
    };

    // =========================================================
    // 2) 포즈/Orientation/Ref 이미지 관련 멤버
    // =========================================================



    // =========================================================
    // 3) 백그라운드 스레드(Corpus 탐색, 기본 DB 로드 등)
    // =========================================================

 
    // Hand Location DB 업데이트 (준비 동작/위치 관련)
    void UpdateHandLocationDB();

    // 회전 행렬 -> Axis-Angle(축-각) 변환
    std::vector<double> RotationMatrixToAxisAngle(std::vector<float> orientation,
        int type);


    // MANO 모델용 포즈/방향 세팅 함수 (PCA component, orientation 포함)
    void SetPoseOrientationForManoModel(
        std::vector<float>& pose,
        std::vector<Point3f>& orientation,
        std::vector<float> sgcp_vtHandOrientation,
        std::vector<float> sgcp_vtPCACompoent,
        int type);

    // Ready Pose 판별용 각도 계산 함수들(손끝/팔꿈치 방향/각도)
    double GetDegreeAngleOfRHandTipDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfLHandTipDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfRElbowDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfLElbowDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfLElbowAngle(_typeChangepointDB cp);
    double GetDegreeAngleOfRElbowAngle(_typeChangepointDB cp);

    // MP Pose에서 Left/Right Hand 2D 위치 추출
    void GetMPHandPointsFromPose(std::vector<Point3f> skeleton,
        cv::Mat img,
        std::vector<cv::Point>& LHand,
        std::vector<cv::Point>& RHand);

    // Hand 상태를 문자열로 변환 (READY, SPEAK, RAPID 등)
    CString GetHandStateString(int hand_state);

    // 체인지포인트 간 유사도 계산(HandShape/Orientation/Location 별)
    void GetSimsFromCP(
        _tagChangepointDB cp1, _tagChangepointDB cp2,
        std::vector<double>& hsR, std::vector<double>& hoR, std::vector<double>& hlR,
        std::vector<double>& hsL, std::vector<double>& hoL, std::vector<double>& hlL
    );

    // 손 위치 유사도 계산 (location 기반)
    std::vector<double> SimHandLocation(_typeChangepointDB cp_db,
        _typeChangepointDB query);


    // =========================================================
    // 4) UI 컨트롤 (MFC CStatic/CEdit 등)
    // =========================================================



    // 전체 종료 플래그(스레드 안전하게 종료하기 위해 atomic 사용)
    std::atomic<bool> m_exit;
  

    // 전체 체인지포인트 목록 포인터(외부에서 관리)
  //  _typeChangepointDB* m_cp = NULL;


    // AI 스레드 실행 함수 (영상/프레임에 대해 ACR/MP 등 수행)
    static UINT AIThread(LPVOID pParam);

    // ACR 결과(numpy->std::vector 변환된 것)를 Result 구조체로 파싱
    void DetectionResultParsing(
        std::vector<std::vector<std::vector<double>>> stdVec_Hand_acr_Info,
        Result& l_hand,
        Result& r_hand);
    void DetectionResultParsingNofaces(std::vector<std::vector<std::vector <double>>> stdVec_Hand_acr_Info, Result& l_hand, Result& r_hand);

    // 불안정/과도 구간을 제거한 인퍼 결과 리스트 생성
    std::vector<GlossInfered> RemoveTransientHandState(
        std::vector<GlossInfered> infered);


    // 이미지 + joint 위치 기반으로 정사각 crop 영역 계산
    cv::Rect GetCropSquareRect(cv::Mat img,
        std::vector<cv::Point2i> jpts,
        float scale);

    // ACR 결과로부터 체인지포인트 DB 채우기 (ROI/pose/verts 등)
    void GetCPFormACRResult(
        Result roi_l_hand1s,
        Result roi_r_hand2s,
        RECT l_hand_roi,
        RECT r_hand_roi);

    // MP Pose joint -> HandLocation 2D 기준점 계산
    // (0: Left, 1: Right)
    std::vector<Point2f> GetRefHL2DPointsMP(
        std::vector<cv::Point3f> joints);


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

    // MP Pose를 이용하여 거친 손 상태(Ready/Speak 등) 판별
    int GetRoughHandStatusFromMP(_cap_data cap_data);
    int GetRoughHandStatusFromMP(std::vector<Point3f> mp_pose);

public:

  //  CString InferSentence(Mat img, int status, int frame_count);

        // 영상/텍스트 등 UI 그리기 유틸
    CVideoUtil m_util;
    CStatic m_ROIcctv_frame;

    // 수어 DB(표제어/코퍼스/영상/메타 정보)
    Excel_DB   m_DB;

    // 현재 프레임(캡처 영상)
    cv::Mat m_cap_img;

    // 최종 선택된 체인지포인트 결과(1개)
    _typeChangepointDB  m_result;

    // 현재 AI 모드 (MANO/MP/HandDetect 등)
    int m_ai_mode = _NONE_;

    // AI 처리 시작/종료 이벤트 + AI 스레드 핸들
    HANDLE      hAIStart;
    HANDLE      hAIFinish;
    CWinThread* hAIThread = nullptr;

    // 체인지포인트 DB 기반 최종 핸드 상태 결정
    int GetHandStatusFromCP(_typeChangepointDB cp);

    // 기준 CP와 유사한 CP 후보 찾기 (유사도 기반 검색)
    std::vector<_tagChangepointDB> FindSimilarCPs(
        _tagChangepointDB ref_cp,
        std::vector<_tagChangepointDB> cadidate_cps);

    // Gloss 인퍼 결과(한 문장 내 여러 글로스 후보)
    std::vector<GlossInfered> m_infered;

    // 문장 추론 객체 (글로스 시퀀스 -> 자연어/표제어 문장 후보)
    InferSentence m_infer_sentence;

    CEdit   m_info_edit;

protected:
    // =========================================
    // MFC 표준 멤버들
    // =========================================

    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
    HICON m_hIcon;                                      // 다이얼로그 아이콘

    // 생성된 메시지 맵 함수
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();

    // 버튼/이벤트 핸들러들 (번호는 리소스 ID에 대응)

    afx_msg void OnBnClickedOk();
//    afx_msg void OnBnClickedButton56();


    DECLARE_MESSAGE_MAP()
public:
  
};
