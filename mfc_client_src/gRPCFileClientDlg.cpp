
// gRPCFileClientDlg.cpp: 구현 파일
//

#include "gRPCThread_.h"

//#include "pch.h"
#include "framework.h"
#include "gRPCFileClient.h"
#include "gRPCFileClientDlg.h"

#include "afxdialogex.h"

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif




// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CgRPCFileClientDlg 대화 상자



CgRPCFileClientDlg::CgRPCFileClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GRPCFILECLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	hPlayPause = CreateEvent(NULL, FALSE, FALSE, NULL);
	hAIStart = CreateEvent(NULL, FALSE, FALSE, NULL);
	hAIFinish = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_exit.store(false);
	m_pause.store(false);
	m_stop.store(false);
	m_back.store(false);
	m_move.store(false);

}

void CgRPCFileClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_video_file_name_edit);
	DDX_Control(pDX, IDC_CCTV, m_cctv_frame);
	DDX_Control(pDX, IDC_ROICCTV, m_ROIcctv_frame);
	DDX_Control(pDX, IDC_EDIT6, m_frame_index_edit);
	DDX_Control(pDX, IDC_EDIT2, m_roi_edit);
	DDX_Control(pDX, IDC_EDIT5, m_frame_range_edit);
	DDX_Control(pDX, IDC_EDIT3, m_info_edit);
}

BEGIN_MESSAGE_MAP(CgRPCFileClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CgRPCFileClientDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDOK, &CgRPCFileClientDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON2, &CgRPCFileClientDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CgRPCFileClientDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CgRPCFileClientDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CgRPCFileClientDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON7, &CgRPCFileClientDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CgRPCFileClientDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON10, &CgRPCFileClientDlg::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON9, &CgRPCFileClientDlg::OnBnClickedButton9)
END_MESSAGE_MAP()


// CgRPCFileClientDlg 메시지 처리기

BOOL CgRPCFileClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

		// UTF-8 콘솔 설정
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	// 안전: 실패하면 클래식 로캘로 대체
	try {
		std::wcout.imbue(std::locale(".UTF-8"));   // 또는 ".utf8" / "Korean_Korea.949"
	}
	catch (...) {
		std::wcout.imbue(std::locale::classic());
	}

	ReadCFG();

	OnBnClickedButton2(); //ROI 적용

	if (hAIThread == NULL) hAIThread = AfxBeginThread(AIThread, this);
	


	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CgRPCFileClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CgRPCFileClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CgRPCFileClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

grpc_client_::SequenceClient client(grpc::CreateChannel(TARGETIP, grpc::InsecureChannelCredentials()));

void CgRPCFileClientDlg::OnBnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	CString dialogKey = _T("DialogA_VIDEO");

	CString lastDir = m_util.LoadLastDir(dialogKey, _T("C:\\"));
	lastDir = m_util.EnsureDirExists(lastDir, _T("C:\\"));

	char szFilter[] = "FileList (*.mkv, *.mp4, *.avi)|*.mkv;*.mp4;*.avi| All Files(*.*)|*.*||";

	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, (CA2T)szFilter);

	dlg.m_ofn.lpstrInitialDir = lastDir.GetBuffer();

	if (dlg.DoModal() == IDOK)
	{
		auto str = dlg.GetPathName();

		m_video_file_name_edit.SetWindowText(str);

		m_util.SaveLastDir(dialogKey, m_util.DirFromPath(str));
	}

}


UINT CgRPCFileClientDlg::VideoPlay(LPVOID pParam)
{
	CgRPCFileClientDlg* pDlg = (CgRPCFileClientDlg*)pParam;

	CString file_nm;

	pDlg->m_video_file_name_edit.GetWindowText(file_nm);

	if (!file_nm.IsEmpty())
	{
		CString file_str;

		pDlg->m_video_file_name_edit.GetWindowText(file_str);

		if (!file_str.IsEmpty() && !file_str.IsEmpty() && !file_str.IsEmpty())
		{

			CRect Rect1;
			pDlg->m_cctv_frame.GetClientRect(Rect1);
		

			VideoCapture cap(pDlg->m_util.StringToChar(file_str));

			Mat img;
			if (cap.isOpened())
			{
				auto [w, h, fps, totalFrames] = pDlg->m_util.GetVideoInfo(cap);

				CString str;
				str.Format(_T("[비디오파일 정보]\r\n%s\r\nwidht:%d height:%d fps:%.2f total frames:%d"), file_str, w, h, fps, totalFrames);
				pDlg->m_info_edit.SetWindowText(str);

				while (pDlg->m_stop.load() == false && pDlg->m_exit.load() == false)
				{
					cap >> img;
					int frameIndex = (int)cap.get(cv::CAP_PROP_POS_FRAMES);
					if (img.empty()) break;

					auto img_copy = img.clone();
					rectangle(img_copy, pDlg->m_roi, Scalar(0, 255, 0), 5);
					pDlg->m_util.DrawImageBMP(&pDlg->m_cctv_frame, img_copy, 0, 0, (double)frameIndex / (totalFrames - 1), frameIndex);

					cv::Mat roi_img = img(pDlg->m_roi).clone();
					pDlg->m_util.DrawImageBMP(&pDlg->m_ROIcctv_frame, roi_img, 0, 0);

					
					if (pDlg->m_pause.load() == true)
					{
						WaitForSingleObject(pDlg->hPlayPause, INFINITE);
					}
					if (pDlg->m_pause.load() == true && pDlg->m_back.load() == true)
					{
						int back_index = frameIndex - 2;
						if (back_index <= 0) back_index = 0;
						cap.set(cv::CAP_PROP_POS_FRAMES, back_index);
						pDlg->m_back.store(false);
					}
					if (pDlg->m_pause.load() == true && pDlg->m_move.load() == true)
					{
						CString str;
						pDlg->m_frame_index_edit.GetWindowText(str);
						if (!str.IsEmpty())
						{
							auto move_index = _ttoi(str) - 1;
							if (move_index < 0)move_index = 0;
							if (move_index >= totalFrames) move_index = totalFrames - 1;
							cap.set(cv::CAP_PROP_POS_FRAMES, move_index);
						}
						pDlg->m_move.store(false);
					}
					Sleep(30);
				}
				cap.release();
			}
		}
	}
	pDlg->m_stop.store(false);
	pDlg->m_pause.store(false);
	pDlg->GetDlgItem(IDC_BUTTON3)->EnableWindow(true);
	pDlg->GetDlgItem(IDC_BUTTON4)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON5)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON7)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON8)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON10)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_EDIT6)->EnableWindow(false);
	pDlg->hVideoPlay = nullptr;

	cv::Mat grayColor(480, 640, CV_8UC3, cv::Scalar(210, 210, 210));
	pDlg->m_util.DrawImageBMP(&pDlg->m_cctv_frame, grayColor, 0, 0);
	pDlg->m_util.DrawImageBMP(&pDlg->m_ROIcctv_frame, grayColor, 0, 0);

	return 0;
}
void CgRPCFileClientDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SaveCFG();

	m_exit.store(true);

	Sleep(100);

	CDialogEx::OnOK();
}


void CgRPCFileClientDlg::SaveCFG()
{
	CString str;
	USES_CONVERSION;
	FILE* fp;
	fopen_s(&fp, "cfg-new.txt", "w");
	m_roi_edit.GetWindowText(str);
	fprintf_s(fp, "%s\n", m_util.StringToChar(str));
	m_frame_range_edit.GetWindowText(str);
	fprintf_s(fp, "%s\n", m_util.StringToChar(str));
	fclose(fp);
}


void CgRPCFileClientDlg::ReadCFG()
{
	USES_CONVERSION;
	SYSTEMTIME seleted1, seleted2;
	CString str;

	FILE* fp;
	char string[3840];
	if (fopen_s(&fp, "cfg-new.txt", "r") != 0)
	{
		AfxMessageBox(_T("cfg.txt 파일이 없습니다."));
		return;
	}
	fgets(string, sizeof(string), fp);
	str = A2T(string);
	str.Replace(_T("\n"), _T(""));
	str.Replace(_T("\r"), _T(""));
	m_roi_edit.SetWindowText(str);
	fgets(string, sizeof(string), fp);
	str = A2T(string);
	str.Replace(_T("\n"), _T(""));
	str.Replace(_T("\r"), _T(""));
	m_frame_range_edit.SetWindowText(str);
	fclose(fp);
}
void CgRPCFileClientDlg::OnBnClickedButton2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	CString roi_str;
	m_roi_edit.GetWindowText(roi_str);
	auto strs = m_util.SplitCString(roi_str);

	if (strs.size() >= 4)
	{
		m_roi = cv::Rect(_ttoi(strs[0]), _ttoi(strs[1]), _ttoi(strs[2]), _ttoi(strs[3]));
	}
}

void CgRPCFileClientDlg::OnBnClickedButton3() //Play
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (hVideoPlay == NULL) hVideoPlay = AfxBeginThread(VideoPlay, this);

	m_pause.store(false);
	SetEvent(hPlayPause);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON4)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON5)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON7)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON8)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON10)->EnableWindow(false);
	GetDlgItem(IDC_EDIT6)->EnableWindow(false);
}

void CgRPCFileClientDlg::OnBnClickedButton4() //Stop
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	m_stop.store(true);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON4)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON5)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON7)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON8)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON10)->EnableWindow(false);
	GetDlgItem(IDC_EDIT6)->EnableWindow(false);
}

void CgRPCFileClientDlg::OnBnClickedButton5() //Pause
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	m_pause.store(true);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON4)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON5)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON7)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON8)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON10)->EnableWindow(true);
	GetDlgItem(IDC_EDIT6)->EnableWindow(true);
}

void CgRPCFileClientDlg::OnBnClickedButton7() //Backward
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_back.store(true);
	SetEvent(hPlayPause);
}

void CgRPCFileClientDlg::OnBnClickedButton8() //Forward
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetEvent(hPlayPause);
}

void CgRPCFileClientDlg::OnBnClickedButton10() //Move to frame index
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_move.store(true);
	SetEvent(hPlayPause);
}


UINT CgRPCFileClientDlg::AIThread(LPVOID pParam) //제일 늦음. 모든 속도가 AI 처리 부분에 종속됨
{

	CgRPCFileClientDlg* pDlg = (CgRPCFileClientDlg*)pParam;

	CRect frame_rect;
	pDlg->m_cctv_frame.GetWindowRect(frame_rect);

	CRect frame_rect2;
	pDlg->m_ROIcctv_frame.GetWindowRect(frame_rect2);

	pybind11::scoped_interpreter guard{};

	try {

		auto exampleModule4 = pybind11::module_::import("ACR.mp_detect");
		auto func_mp_pose = exampleModule4.attr("mediapipe_pose_func");
		auto func_mp_hand = exampleModule4.attr("mediapipe_hand_func");


		while (pDlg->m_exit.load() == 0)
		{

			WaitForSingleObject(pDlg->hAIStart, INFINITE);			
	
		
			if (!pDlg->m_cap_img.empty())
			{
				cv::Mat dst_img = pDlg->m_cap_img.clone();
				if (dst_img.channels() == 4) cvtColor(dst_img, dst_img, COLOR_BGRA2BGR);
				auto result_pose_mp = func_mp_pose(pybind11::cast(dst_img));
				auto mp_result = result_pose_mp.cast<std::vector<std::vector <double>>>();

				pDlg->m_sgcp_vtSkeletonMP.clear();		

				if (mp_result.size() >= 3)
				{
					for (int k = 0; k < mp_result[0].size(); k++)
					{
						pDlg->m_sgcp_vtSkeletonMP.push_back(Point3f(mp_result[0][k], mp_result[1][k], mp_result[2][k]));						
					}
				}
			}
		
			SetEvent(pDlg->hAIFinish);
		}
	}
	catch (py::error_already_set& e) {
		std::cout << e.what() << std::endl;
	}

	pDlg->hAIThread = nullptr;

	return 0;
}


UINT CgRPCFileClientDlg::gRPCSend(LPVOID pParam) //제일 늦음. 모든 속도가 AI 처리 부분에 종속됨
{

	CgRPCFileClientDlg* pDlg = (CgRPCFileClientDlg*)pParam;

	client.SendFrames("SESSION_001");

	pDlg->hgRPCSend = nullptr;

	return 0;
}



double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat prevGray, std::vector<cv::Point2f> prevPts, cv::Mat gray)
{

	std::vector<cv::Point2f> nextPts;
	std::vector<unsigned char> status;
	std::vector<float> err;

	cv::calcOpticalFlowPyrLK(
		prevGray, gray,
		prevPts, nextPts,
		status, err
	);

	double sumLen = 0.0;
	int cnt = 0;
	for (size_t i = 0; i < nextPts.size(); ++i) {
		if (!status[i]) continue;
		double dx = nextPts[i].x - prevPts[i].x;
		double dy = nextPts[i].y - prevPts[i].y;
		double len = std::sqrt(dx * dx + dy * dy);
		sumLen += len;
		cnt++;
	}

	double avgMotion = (cnt > 0) ? (sumLen / cnt) : 0.0;
	std::cout << "avg motion (LK): " << avgMotion << std::endl;

	return avgMotion;
}


double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat frame)
{
	static cv::Mat prev;
	cv::Rect rect;

	if (frame.empty())
	{
		cv::Mat empty;
		prev = empty;

		return -1;
	}

	double avgMotion = 10000.0;

	if (!prev.empty()) avgMotion = GetMotionVauleWithOpticalFlow(prev, frame, rect);

	prev = frame.clone();

	return avgMotion;
}

double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat prev, cv::Mat frame, cv::Rect rect)
{
	cv::Mat prevGray, gray;

	cv::cvtColor(prev, prevGray, cv::COLOR_BGR2GRAY);
	cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

	if (!rect.empty())
	{
		prevGray = prevGray(rect);
		gray = gray(rect);
	}


	// 추적할 포인트 찾기
	std::vector<cv::Point2f> prevPts;
	cv::goodFeaturesToTrack(prevGray, prevPts, 300, 0.01, 7);



	std::vector<cv::Point2f> nextPts;
	std::vector<unsigned char> status;
	std::vector<float> err;

	cv::calcOpticalFlowPyrLK(
		prevGray, gray,
		prevPts, nextPts,
		status, err
	);

	double sumLen = 0.0;
	int cnt = 0;
	for (size_t i = 0; i < nextPts.size(); ++i) {
		if (!status[i]) continue;
		double dx = nextPts[i].x - prevPts[i].x;
		double dy = nextPts[i].y - prevPts[i].y;
		double len = std::sqrt(dx * dx + dy * dy);
		sumLen += len;
		cnt++;
	}

	double avgMotion = (cnt > 0) ? (sumLen / cnt) : 0.0;
	std::cout << "avg motion (Optical Flow): " << avgMotion << std::endl;

	return avgMotion;
}

double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat prev, cv::Mat frame, cv::Rect Rrect, cv::Rect Lrect)
{
	cv::Mat prevGray, gray;
	cv::Mat RprevGray, Rgray;
	cv::Mat LprevGray, Lgray;
	double RavgMotion = 0, LavgMotion = 0;


	cv::cvtColor(prev, prevGray, cv::COLOR_BGR2GRAY);
	cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

	if (!Rrect.empty())
	{
		cv::Rect rect;
		RprevGray = prevGray(Rrect);
		Rgray = gray(Rrect);
		RavgMotion = GetMotionVauleWithOpticalFlow(RprevGray, Rgray, rect);
	}

	if (!Lrect.empty())
	{
		cv::Rect rect;
		LprevGray = prevGray(Lrect);
		Lgray = gray(Lrect);
		LavgMotion = GetMotionVauleWithOpticalFlow(LprevGray, Lgray, rect);
	}

	double avgMotion = RavgMotion + LavgMotion;
	std::cout << "avg motion (LK): " << avgMotion << std::endl;

	return avgMotion;
}


int CgRPCFileClientDlg::GetRoughHandStatusFromMP(std::vector<Point3f> mp_pose)
{
	int status = -1; //-1:error, 0:ready, 1:right hand, 2:left hand, 3:all hands

	if (mp_pose.size() <= 0) return -1;

	auto hl = GetRefHL2DPointsMP(mp_pose);

	if (hl[1].y <= READY_LOCATION && hl[0].y <= READY_LOCATION) status = 3;
	else if (hl[1].y <= READY_LOCATION && hl[0].y > READY_LOCATION) status = 1;
	else if (hl[1].y > READY_LOCATION && hl[0].y <= READY_LOCATION) status = 2;
	else status = 0;
	return status;
}


int CgRPCFileClientDlg::GetMotionStatusFromMP(int cur_motion_status, std::vector<cv::Point3f> cur_mp)
{
	static std::vector<cv::Point3f> prev_mp;

	if (cur_motion_status == RESET_MOTION_STATUS)
	{
		prev_mp.clear();
		return RESET_MOTION_STATUS;
	}

	int motion_status = cur_motion_status;

	if (prev_mp.size() <= 0)
	{
		prev_mp = cur_mp;
		return READY_MOTION_STATUS;
	}

	auto Rdev = m_util.Distance(cv::Point2f(prev_mp[16].x - cur_mp[16].x, prev_mp[16].y - cur_mp[16].y));
	auto Ldev = m_util.Distance(cv::Point2f(prev_mp[15].x - cur_mp[15].x, prev_mp[15].y - cur_mp[15].y));

	cout << "\n\n Rdev  Ldev: " << Rdev << " " << Ldev << "\n\n";

	if (Rdev > RAPID_DISTANCE || Ldev > RAPID_DISTANCE) return RAPID_MOTION_STATUS;

	if (cur_motion_status == READY_MOTION_STATUS)
	{
		int hand_status = GetRoughHandStatusFromMP(cur_mp);
		if (hand_status > 0) motion_status = SPEAK_MOTION_STATUS;
	}
	else if (cur_motion_status == SPEAK_MOTION_STATUS)
	{
		if (prev_mp.size() > 0 && cur_mp.size() > 0)
		{
			motion_status = READY_MOTION_STATUS;
		}
	}
	prev_mp = cur_mp;
	return motion_status;
}

std::vector<Point2f> CgRPCFileClientDlg::GetRefHL2DPointsMP(std::vector <cv::Point3f> joints) //0 Left, 1 Right
{
	std::vector<Point2f> ref_hl_points;


	if (joints.size() <= 0) return ref_hl_points;

	//2D Hand Location
	std::vector<Point2f> pixels;
	for (int k = 0; k < joints.size(); k++)
	{
		Point2f pt;
		pt.x = joints[k].x * 1920.0;
		pt.y = joints[k].y * 1080.0;
		pixels.push_back(pt);
	}

	//Geometric Transformation	
	vector<Point2f> corners = { pixels[12] , pixels[11] ,pixels[0] };

	Mat trans = cv::getAffineTransform(corners, m_warpCorners);

	vector<Point2f> hl_point = { pixels[15],pixels[16] }; //Left wrist, Right wrist
	std::vector<cv::Point2f> dst;

	cv::transform(hl_point, dst, trans);

	for (auto& pt : dst)
	{
		pt.x = std::clamp(pt.x, 0.0f, 1920.0f);
		pt.y = std::clamp(pt.y, 0.0f, 1080.0f);
		double offset = 0;
		pt.y += offset;
	}

	ref_hl_points.push_back(dst[0]);
	ref_hl_points.push_back(dst[1]);

	return ref_hl_points;
}

void CgRPCFileClientDlg::OnBnClickedButton9() //gRPC Send
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	client.pDlg = this;

	if (hgRPCSend == NULL) hgRPCSend = AfxBeginThread(gRPCSend, this);

	
}
