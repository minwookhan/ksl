
// KSLDBManagerDlg.cpp: 구현 파일
//


#include "framework.h"
#include "KSLDBManager.h"
#include "KSLDBManagerDlg.h"
#include "afxdialogex.h"

#include <iostream>
#include <fstream>						// using for json file write/read
#include <io.h>

#include "HandShapeDiff.hpp" 

#include "gRPCThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")




UINT CKSLDBManagerDlg::AIThread(LPVOID pParam) //제일 늦음. 모든 속도가 AI 처리 부분에 종속됨
{

	CKSLDBManagerDlg* pDlg = (CKSLDBManagerDlg*)pParam;


	CRect frame_rect2;
	pDlg->m_ROIcctv_frame.GetWindowRect(frame_rect2);

	pybind11::scoped_interpreter guard{};
	
	try {
		auto exampleModule = pybind11::module_::import("ACR.detect");
		auto func_mano_hand_modelR = exampleModule.attr("mano_hand_parametersR");
		auto func_mano_hand_modelL = exampleModule.attr("mano_hand_parametersL");	
		auto func_single_hand_acr = exampleModule.attr("single_detect");
		auto func_single_hand_acr_no_faces = exampleModule.attr("single_detect_no_faces");
		
		auto exampleModule4 = pybind11::module_::import("ACR.mp_detect");
		auto func_mp_pose = exampleModule4.attr("mediapipe_pose_func");
		auto func_mp_hand = exampleModule4.attr("mediapipe_hand_func");


		int initF = 0;

		while (pDlg->m_exit.load() == 0)
		{
			if (initF == 0)
			{
				auto img = imread("init.jpg");
				auto result_Hand_acr1 = func_single_hand_acr_no_faces(pybind11::cast(img), 0);
				initF = 1;
				continue;
			}
			else 
				WaitForSingleObject(pDlg->hAIStart, INFINITE);

			if (pDlg->m_ai_mode == AI_MODE_HAND_DETECT_NEW_DB)
			{
				Result roi_l_hand1, roi_r_hand1, roi_l_hand2, roi_r_hand2;
				Result l_hand, r_hand;

				if (!pDlg->m_cap_img.empty())
				{
					cv::Mat l_img, r_img;
					cv::Mat dst_img = pDlg->m_cap_img.clone();

					pDlg->GetMPHandPointsFromPose(pDlg->m_result.sgcp_vtSkeletonMP, pDlg->m_cap_img, l_hand.jpts, r_hand.jpts);	

					pDlg->m_result.sgcp_vtHandStatus = pDlg->GetRoughHandStatusFromMP(pDlg->m_result.sgcp_vtSkeletonMP);

					float scale_val = 20.0;

					if (l_hand.jpts.size() > 0 )
					{
						scale_val = 10.0;
						while (1)
						{
							l_hand.rect = pDlg->GetCropSquareRect(dst_img, l_hand.jpts, scale_val);
							if (l_hand.rect.width > 0 && l_hand.rect.width > 0)
							{
								l_img = dst_img(l_hand.rect).clone();

								auto result_Hand_acr1 = func_single_hand_acr_no_faces(pybind11::cast(l_img), 0);
								std::vector<std::vector<std::vector <double>>> roi_stdVec_Hand_acr_Info1 = result_Hand_acr1.cast<std::vector<std::vector<std::vector <double>>>>();
								pDlg->DetectionResultParsingNofaces(roi_stdVec_Hand_acr_Info1, roi_l_hand1, roi_r_hand1); //roi_l_hand1							

							}

							if (roi_l_hand1.poses.size() > 0 )
							{
								break;
							}
							else scale_val = scale_val - 2;

							if (scale_val <= 2)
							{
								printf("\n\n**********************************\nACR error\n**********************************************\n\n");
								break;
							}
						}			

					}
					if (r_hand.jpts.size() > 0)
					{
						scale_val = 10.0;
						while (1)
						{
							r_hand.rect = pDlg->GetCropSquareRect(dst_img, r_hand.jpts, scale_val);
							if (r_hand.rect.width > 0 && r_hand.rect.width > 0)
							{
								r_img = dst_img(r_hand.rect).clone();

								auto result_Hand_acr2 = func_single_hand_acr_no_faces(pybind11::cast(r_img), 1);
								std::vector<std::vector<std::vector <double>>> roi_stdVec_Hand_acr_Info2 = result_Hand_acr2.cast<std::vector<std::vector<std::vector <double>>>>();
								pDlg->DetectionResultParsingNofaces(roi_stdVec_Hand_acr_Info2, roi_l_hand2, roi_r_hand2); //roi_r_hand2

								if (roi_r_hand2.poses.size() > 0 )
								{
									break;
								}
								else scale_val = scale_val - 2;

								if (scale_val <= 2)
								{
									printf("\n\n**********************************\nACR error\n**********************************************\n\n");
									break;
								}
							}					
						}
					}
					else
					{
						printf("\n\n\n********************************************************************\nR Hand roi err\n****************************************************************************\n\n\n");
					}

					if(roi_l_hand1.poses.size()>0 || roi_r_hand2.poses.size()>0)
						pDlg->GetCPFormACRResult(roi_l_hand1, roi_r_hand2, pDlg->m_util.CvRectToRECT(l_hand.rect), pDlg->m_util.CvRectToRECT(r_hand.rect));
					//		pDlg->m_result.sgcp_vtHandStatus = pDlg->GetHandStatusFromCP(pDlg->m_result);				
				}
			}			

			pDlg->m_ai_mode = _NONE_;
			SetEvent(pDlg->hAIFinish);
		}
	}
	catch (py::error_already_set& e) {
		std::cout << e.what() << std::endl;
	}

	pDlg->hAIThread = nullptr;

	return 0;
}



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


// CKSLDBManagerDlg 대화 상자



CKSLDBManagerDlg::CKSLDBManagerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_KSLDBMANAGER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	AfxInitRichEdit2();   // ★ RichEdit 2.0 이상 로딩

	hAIStart = CreateEvent(NULL, FALSE, FALSE, NULL);
	hAIFinish = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_exit.store(false);


}

void CKSLDBManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT5, m_info_edit);
	DDX_Control(pDX, IDC_MKV_VIDEO2, m_ROIcctv_frame);
}

BEGIN_MESSAGE_MAP(CKSLDBManagerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//	ON_BN_CLICKED(IDC_BUTTON1, &CKSLDBManagerDlg::OnBnClickedButton1)
//ON_BN_CLICKED(IDC_BUTTON6, &CKSLDBManagerDlg::OnBnClickedButton6)
ON_BN_CLICKED(IDOK, &CKSLDBManagerDlg::OnBnClickedOk)
//ON_BN_CLICKED(IDC_BUTTON56, &CKSLDBManagerDlg::OnBnClickedButton56)

//ON_BN_CLICKED(IDC_BUTTON10, &CKSLDBManagerDlg::OnBnClickedButton10)
//ON_BN_CLICKED(IDC_BUTTON11, &CKSLDBManagerDlg::OnBnClickedButton11)

END_MESSAGE_MAP()



// CKSLDBManagerDlg 메시지 처리기




BOOL CKSLDBManagerDlg::OnInitDialog()
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

	if (m_DB._Load(_T("..\\ETRI-KSL-DB\\ETRI_KSL_Dictionary_r36-corpus.xlsx") ) == FALSE)
	{
		AfxMessageBox("엑셀 파일이 없거나 읽는동안 에러가 발생했습니다", MB_OK);
	}

	UpdateHandLocationDB();


	if (hAIThread == NULL)
	{		
		hAIThread = AfxBeginThread(AIThread, this);
	}

	grpc_thread::RunThread(this);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CKSLDBManagerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CKSLDBManagerDlg::OnPaint()
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
HCURSOR CKSLDBManagerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void CKSLDBManagerDlg::UpdateHandLocationDB()
{

	for (int k = 0; k < m_DB.m_CpDB.size(); k++)
	{
		std::vector<Point2f> ref_hl_point;
		std::vector <cv::Point3f> AZ_ACR_hand_joints;
		AZ_ACR_hand_joints.insert(AZ_ACR_hand_joints.begin(), m_DB.m_CpDB[k].sgcp_vtSkeleton.begin(), m_DB.m_CpDB[k].sgcp_vtSkeleton.end());
		AZ_ACR_hand_joints.insert(AZ_ACR_hand_joints.end(), m_DB.m_CpDB[k].sgcp_vtLHSkeleton.begin(), m_DB.m_CpDB[k].sgcp_vtLHSkeleton.end());
		AZ_ACR_hand_joints.insert(AZ_ACR_hand_joints.end(), m_DB.m_CpDB[k].sgcp_vtRHSkeleton.begin(), m_DB.m_CpDB[k].sgcp_vtRHSkeleton.end());

		//Image가 1080p, 720p 를 구분하여, 각각에 맞는 m_sensor_calibration 선택
		
		//left right hands location
		//for (int mmm = 0; mmm < ref_hl_point.size(); mmm += 2)
		//{
		//	m_DB.m_CpDB[k].sgcp_vtHandLocation_ref_model.push_back(ref_hl_point[mmm]);
		//}
	}
}

//void CKSLDBManagerDlg::OnBnClickedButton1()
//{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

//	CString dialogKey = _T("DialogA_XLSX");
//
//	CString lastDir = m_util.LoadLastDir(dialogKey, _T("C:\\"));
//	lastDir = m_util.EnsureDirExists(lastDir, _T("C:\\"));	
//
//
//	char szFilter[] = "FileList (*.xlsx)|*.xlsx| All Files(*.*)|*.*||";
//
//	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, (CA2T)szFilter);
//
//	dlg.m_ofn.lpstrInitialDir = lastDir.GetBuffer();
//
//	if (dlg.DoModal() == IDOK)
//	{
//		auto str = dlg.GetPathName();
//
//		m_db_file_edit.SetWindowTextA(str);		
//
//		if (!str.IsEmpty())
//		{
//
//			if (hLoadDefaultDB == nullptr)
//				hLoadDefaultDB = AfxBeginThread(LoadDefaultDB, this);
//		}
//
//		m_util.SaveLastDir(dialogKey, m_util.DirFromPath(str));
//	}
//}


//void CKSLDBManagerDlg::OnBnClickedButton6()
//{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	// 


//}



int CKSLDBManagerDlg::GetRoughHandStatusFromMP(_cap_data cap_data)
{
	int status = -1; //-1:error, 0:ready, 1:right hand, 2:left hand, 3:all hands

	if (cap_data.mp_skeleton.size() <= 0) return -1;

	auto hl = GetRefHL2DPointsMP(cap_data.mp_skeleton);

	if (hl[1].y <= READY_LOCATION && hl[0].y <= READY_LOCATION) status = 3;
	else if (hl[1].y <= READY_LOCATION && hl[0].y > READY_LOCATION) status = 1;
	else if (hl[1].y > READY_LOCATION && hl[0].y <= READY_LOCATION) status = 2;
	else status = 0;


	return status;
}

int CKSLDBManagerDlg::GetRoughHandStatusFromMP(std::vector<Point3f> mp_pose)
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






void CKSLDBManagerDlg::OnBnClickedOk() //exit
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.


	CRect rect;
	GetWindowRect(&rect);

	// 레지스트리나 설정 파일 등에 위치 저장
	AfxGetApp()->WriteProfileInt(_T("DialogPos"), _T("X"), rect.left);
	AfxGetApp()->WriteProfileInt(_T("DialogPos"), _T("Y"), rect.top);

	m_exit.store(true);
	m_ai_mode = _NONE_;
	SetEvent(hAIStart);	

	Sleep(1000);

	CDialogEx::OnOK();	
}


void CKSLDBManagerDlg::SetPoseOrientationForManoModel(std::vector<float>& pose, std::vector<Point3f>& orientation,
	std::vector<float> sgcp_vtHandOrientation, std::vector<float> sgcp_vtPCACompoent, int type)
{
	pose.clear();
	orientation.clear();
	if (sgcp_vtHandOrientation.size() <= 0 || sgcp_vtPCACompoent.size() <= 0 ) return;
	
	std::vector<Point3f> orientation_ret;
	for (int k = 0; k < sgcp_vtHandOrientation.size(); k += 3)
		orientation_ret.push_back(Point3f(sgcp_vtHandOrientation[k], sgcp_vtHandOrientation[k + 1], sgcp_vtHandOrientation[k + 2]));

	double norm1 = sqrt(sgcp_vtHandOrientation[0] * sgcp_vtHandOrientation[0] + sgcp_vtHandOrientation[1] * sgcp_vtHandOrientation[1] + sgcp_vtHandOrientation[2] * sgcp_vtHandOrientation[2]);
	double norm2 = sqrt(sgcp_vtHandOrientation[3] * sgcp_vtHandOrientation[3] + sgcp_vtHandOrientation[4] * sgcp_vtHandOrientation[4] + sgcp_vtHandOrientation[5] * sgcp_vtHandOrientation[5]);
	double norm3 = sqrt(sgcp_vtHandOrientation[6] * sgcp_vtHandOrientation[6] + sgcp_vtHandOrientation[7] * sgcp_vtHandOrientation[7] + sgcp_vtHandOrientation[8] * sgcp_vtHandOrientation[8]);

	auto axis_angle = RotationMatrixToAxisAngle( sgcp_vtHandOrientation, type);
	
	pose.clear();
	for (auto comp : sgcp_vtPCACompoent)
		pose.push_back(comp);
	
	pose.insert(pose.begin(), axis_angle[2]);
	pose.insert(pose.begin(), axis_angle[1]);
	pose.insert(pose.begin(), axis_angle[0]);



	orientation = orientation_ret;
}






std::vector<double> CKSLDBManagerDlg::RotationMatrixToAxisAngle(std::vector<float> orientation, int type)
{
	std::vector<double> axis_angle;

	if (orientation.size() != 9) return axis_angle;	

	// Rotation Matrix To Axis-angle Test
	float sign = 1.0;

	if (type == 1) sign = -1.0;

	cv::Mat R = (cv::Mat_<double>(3, 3) <<
		sign*orientation[0 * 3 + 0], sign*orientation[0 * 3 + 1], sign*orientation[0 * 3 + 2],
		orientation[1 * 3 + 0], orientation[1* 3 + 1], orientation[1 * 3 + 2],
		orientation[2* 3 + 0], orientation[2 * 3 + 1], orientation[2* 3 + 2]);

	// 결과를 저장할 회전 벡터 (axis-angle)
	cv::Mat rvec;
	cv::Rodrigues(R, rvec);
	//std::cout << "Axis-Angle (Rodrigues Vector):\n" << rvec << std::endl;	
	//std::cout << "R_norm:\n" << R_norm << std::endl;

	for (int k = 0;k < 3;k++) axis_angle.push_back(-rvec.at<double>(k, 0));

	return axis_angle;
	
}


void CKSLDBManagerDlg::DetectionResultParsing(std::vector<std::vector<std::vector <double>>> stdVec_Hand_acr_Info, Result& l_hand, Result& r_hand)
{
	l_hand.conf = 0.0;
	r_hand.conf = 0.0;
	l_hand.jpts.clear(); //2D
	r_hand.jpts.clear(); //2D
	l_hand.jpt3ds.clear();
	r_hand.jpt3ds.clear();
	l_hand.poses.clear();
	r_hand.poses.clear();

	l_hand.faces.clear();
	r_hand.faces.clear();
	l_hand.verts.clear();
	r_hand.verts.clear();
	l_hand.orientaion.clear();
	r_hand.orientaion.clear();


	for (int k = 1; k < stdVec_Hand_acr_Info.size(); k = k + 9)
	{
		if ((int)stdVec_Hand_acr_Info[k + 7][0][0] == 0) //Left Hand
		{
			std::vector<double>axis_angle;
			std::vector<float>orientation_;
			l_hand.hand_type = 0;
			for (int m = 0; m < stdVec_Hand_acr_Info[k].size(); m++)
			{
				l_hand.jpt3ds.push_back(Point3f(stdVec_Hand_acr_Info[k][m][0], stdVec_Hand_acr_Info[k][m][1], stdVec_Hand_acr_Info[k][m][2]));
			}
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 4].size(); m++)
				l_hand.jpts.push_back(Point2i(round(stdVec_Hand_acr_Info[k + 4][m][0]), round(stdVec_Hand_acr_Info[k + 4][m][1])));
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 5].size(); m++)
			{
				for (int mm = 3; mm < stdVec_Hand_acr_Info[k + 5][m].size(); mm++)
					l_hand.poses.push_back(stdVec_Hand_acr_Info[k + 5][m][mm]);
				for (int mm = 0; mm < 3; mm++)
					axis_angle.push_back(stdVec_Hand_acr_Info[k + 5][m][mm]);

			}

			l_hand.conf = stdVec_Hand_acr_Info[k + 6][0][0];

			Point3f ref_verts_joint = l_hand.jpt3ds[0];
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 1].size(); m++)
				l_hand.verts.push_back(Point3f(stdVec_Hand_acr_Info[k + 1][m][0], stdVec_Hand_acr_Info[k + 1][m][1], stdVec_Hand_acr_Info[k + 1][m][2]) - ref_verts_joint);
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 2].size(); m++)
				l_hand.faces.push_back(Point3f(stdVec_Hand_acr_Info[k + 2][m][0], stdVec_Hand_acr_Info[k + 2][m][1], stdVec_Hand_acr_Info[k + 2][m][2]));
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 3].size(); m++)
				l_hand.orientaion.push_back(Point3f(stdVec_Hand_acr_Info[k + 3][m][0], stdVec_Hand_acr_Info[k + 3][m][1], stdVec_Hand_acr_Info[k + 3][m][2]));
			l_hand.jpt3d_center.x = stdVec_Hand_acr_Info[k + 8][0][0];
			l_hand.jpt3d_center.y = stdVec_Hand_acr_Info[k + 8][0][1];
			l_hand.jpt3d_center.z = stdVec_Hand_acr_Info[k + 8][0][2];

			for (int k = 0;k < l_hand.orientaion.size();k++)
			{
				orientation_.push_back(l_hand.orientaion[k].x);
				orientation_.push_back(l_hand.orientaion[k].y);
				orientation_.push_back(l_hand.orientaion[k].z);
			}
			auto data = RotationMatrixToAxisAngle(orientation_, 0);

	//		printf("\n");

		

		}
		else //Right Hand
		{
			std::vector<double>axis_angle;
			std::vector<float>orientation_;
			r_hand.hand_type = 1;
			for (int m = 0; m < stdVec_Hand_acr_Info[k].size(); m++)
			{
				r_hand.jpt3ds.push_back(Point3f(stdVec_Hand_acr_Info[k][m][0], stdVec_Hand_acr_Info[k][m][1], stdVec_Hand_acr_Info[k][m][2]));
			}
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 4].size(); m++)
				r_hand.jpts.push_back(Point2i(round(stdVec_Hand_acr_Info[k + 4][m][0]), round(stdVec_Hand_acr_Info[k + 4][m][1])));
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 5].size(); m++)
			{
				for (int mm = 3; mm < stdVec_Hand_acr_Info[k + 5][m].size(); mm++)
					r_hand.poses.push_back(stdVec_Hand_acr_Info[k + 5][m][mm]);
				for (int mm = 0; mm < 3; mm++)
					axis_angle.push_back(stdVec_Hand_acr_Info[k + 5][m][mm]);

			}

			r_hand.conf = stdVec_Hand_acr_Info[k + 6][0][0];

			Point3f ref_verts_joint = r_hand.jpt3ds[0];
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 1].size(); m++)
				r_hand.verts.push_back(Point3f(stdVec_Hand_acr_Info[k + 1][m][0], stdVec_Hand_acr_Info[k + 1][m][1], stdVec_Hand_acr_Info[k + 1][m][2]) - ref_verts_joint);
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 2].size(); m++)
				r_hand.faces.push_back(Point3f(stdVec_Hand_acr_Info[k + 2][m][0], stdVec_Hand_acr_Info[k + 2][m][1], stdVec_Hand_acr_Info[k + 2][m][2]));
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 3].size(); m++)
				r_hand.orientaion.push_back(Point3f(stdVec_Hand_acr_Info[k + 3][m][0], stdVec_Hand_acr_Info[k + 3][m][1], stdVec_Hand_acr_Info[k + 3][m][2]));
			r_hand.jpt3d_center.x = stdVec_Hand_acr_Info[k + 8][0][0];
			r_hand.jpt3d_center.y = stdVec_Hand_acr_Info[k + 8][0][1];
			r_hand.jpt3d_center.z = stdVec_Hand_acr_Info[k + 8][0][2];

			for (int k = 0;k < r_hand.orientaion.size();k++)
			{
				orientation_.push_back(r_hand.orientaion[k].x);
				orientation_.push_back(r_hand.orientaion[k].y);
				orientation_.push_back(r_hand.orientaion[k].z);
			}
			auto data = RotationMatrixToAxisAngle(orientation_, 1);

		//	printf("\n");



		}
	}

}

void CKSLDBManagerDlg::DetectionResultParsingNofaces(std::vector<std::vector<std::vector <double>>> stdVec_Hand_acr_Info, Result& l_hand, Result& r_hand)
{
	l_hand.conf = 0.0;
	r_hand.conf = 0.0;
	l_hand.jpts.clear(); //2D
	r_hand.jpts.clear(); //2D
	l_hand.jpt3ds.clear();
	r_hand.jpt3ds.clear();
	l_hand.poses.clear();
	r_hand.poses.clear();

	l_hand.faces.clear();
	r_hand.faces.clear();
	l_hand.verts.clear();
	r_hand.verts.clear();
	l_hand.orientaion.clear();
	r_hand.orientaion.clear();


	for (int k = 1; k < stdVec_Hand_acr_Info.size(); k = k + 9)
	{
		if ((int)stdVec_Hand_acr_Info[k + 5][0][0] == 0) //Left Hand
		{
			std::vector<double>axis_angle;
			std::vector<float>orientation_;
			l_hand.hand_type = 0;
			for (int m = 0; m < stdVec_Hand_acr_Info[k].size(); m++)
			{
				l_hand.jpt3ds.push_back(Point3f(stdVec_Hand_acr_Info[k][m][0], stdVec_Hand_acr_Info[k][m][1], stdVec_Hand_acr_Info[k][m][2]));
			}
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 2].size(); m++)
				l_hand.jpts.push_back(Point2i(round(stdVec_Hand_acr_Info[k + 2][m][0]), round(stdVec_Hand_acr_Info[k + 2][m][1])));
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 3].size(); m++)
			{
				for (int mm = 3; mm < stdVec_Hand_acr_Info[k + 3][m].size(); mm++)
					l_hand.poses.push_back(stdVec_Hand_acr_Info[k + 3][m][mm]);
				for (int mm = 0; mm < 3; mm++)
					axis_angle.push_back(stdVec_Hand_acr_Info[k + 3][m][mm]);

			}

			l_hand.conf = stdVec_Hand_acr_Info[k + 4][0][0];

			Point3f ref_verts_joint = l_hand.jpt3ds[0];
			/*		for (int m = 0; m < stdVec_Hand_acr_Info[k + 1].size(); m++)
						l_hand.verts.push_back(Point3f(stdVec_Hand_acr_Info[k + 1][m][0], stdVec_Hand_acr_Info[k + 1][m][1], stdVec_Hand_acr_Info[k + 1][m][2]) - ref_verts_joint);
					for (int m = 0; m < stdVec_Hand_acr_Info[k + 2].size(); m++)
						l_hand.faces.push_back(Point3f(stdVec_Hand_acr_Info[k + 2][m][0], stdVec_Hand_acr_Info[k + 2][m][1], stdVec_Hand_acr_Info[k + 2][m][2]));*/
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 1].size(); m++)
				l_hand.orientaion.push_back(Point3f(stdVec_Hand_acr_Info[k + 1][m][0], stdVec_Hand_acr_Info[k + 1][m][1], stdVec_Hand_acr_Info[k + 1][m][2]));
			l_hand.jpt3d_center.x = stdVec_Hand_acr_Info[k + 6][0][0];
			l_hand.jpt3d_center.y = stdVec_Hand_acr_Info[k + 6][0][1];
			l_hand.jpt3d_center.z = stdVec_Hand_acr_Info[k + 6][0][2];

			for (int k = 0;k < l_hand.orientaion.size();k++)
			{
				orientation_.push_back(l_hand.orientaion[k].x);
				orientation_.push_back(l_hand.orientaion[k].y);
				orientation_.push_back(l_hand.orientaion[k].z);
			}
			auto data = RotationMatrixToAxisAngle(orientation_, 0);

			//		printf("\n");



		}
		else //Right Hand
		{
			std::vector<double>axis_angle;
			std::vector<float>orientation_;
			r_hand.hand_type = 1;
			for (int m = 0; m < stdVec_Hand_acr_Info[k].size(); m++)
			{
				r_hand.jpt3ds.push_back(Point3f(stdVec_Hand_acr_Info[k][m][0], stdVec_Hand_acr_Info[k][m][1], stdVec_Hand_acr_Info[k][m][2]));
			}
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 2].size(); m++)
				r_hand.jpts.push_back(Point2i(round(stdVec_Hand_acr_Info[k + 2][m][0]), round(stdVec_Hand_acr_Info[k + 2][m][1])));
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 3].size(); m++)
			{
				for (int mm = 3; mm < stdVec_Hand_acr_Info[k + 3][m].size(); mm++)
					r_hand.poses.push_back(stdVec_Hand_acr_Info[k + 3][m][mm]);
				for (int mm = 0; mm < 3; mm++)
					axis_angle.push_back(stdVec_Hand_acr_Info[k + 3][m][mm]);

			}

			r_hand.conf = stdVec_Hand_acr_Info[k + 4][0][0];

			Point3f ref_verts_joint = r_hand.jpt3ds[0];
			/*		for (int m = 0; m < stdVec_Hand_acr_Info[k + 1].size(); m++)
						r_hand.verts.push_back(Point3f(stdVec_Hand_acr_Info[k + 1][m][0], stdVec_Hand_acr_Info[k + 1][m][1], stdVec_Hand_acr_Info[k + 1][m][2]) - ref_verts_joint);
					for (int m = 0; m < stdVec_Hand_acr_Info[k + 2].size(); m++)
						r_hand.faces.push_back(Point3f(stdVec_Hand_acr_Info[k + 2][m][0], stdVec_Hand_acr_Info[k + 2][m][1], stdVec_Hand_acr_Info[k + 2][m][2]));*/
			for (int m = 0; m < stdVec_Hand_acr_Info[k + 1].size(); m++)
				r_hand.orientaion.push_back(Point3f(stdVec_Hand_acr_Info[k + 1][m][0], stdVec_Hand_acr_Info[k + 1][m][1], stdVec_Hand_acr_Info[k + 1][m][2]));
			r_hand.jpt3d_center.x = stdVec_Hand_acr_Info[k + 6][0][0];
			r_hand.jpt3d_center.y = stdVec_Hand_acr_Info[k + 6][0][1];
			r_hand.jpt3d_center.z = stdVec_Hand_acr_Info[k + 6][0][2];

			for (int k = 0;k < r_hand.orientaion.size();k++)
			{
				orientation_.push_back(r_hand.orientaion[k].x);
				orientation_.push_back(r_hand.orientaion[k].y);
				orientation_.push_back(r_hand.orientaion[k].z);
			}
			auto data = RotationMatrixToAxisAngle(orientation_, 1);

			//	printf("\n");



		}
	}

}

cv::Rect CKSLDBManagerDlg::GetCropSquareRect(cv::Mat img, std::vector<cv::Point2i> jpts, float scale)
{
	if (jpts.empty()) return cv::Rect(0, 0, img.cols, img.rows);

	// 1. joint들의 bounding box
	cv::Rect hand_rect = cv::boundingRect(jpts);

	// 2. 가장 긴 변을 기준으로 정사각형 크기 결정
	int std_size = std::max(hand_rect.width, hand_rect.height);

	// 3. scale 반영한 최종 크기
	int crop_size = static_cast<int>(std_size * scale);

//	int crop_size = 256/2;

	// 4. 중심점 계산
	cv::Point center(
		hand_rect.x + hand_rect.width / 2,
		hand_rect.y + hand_rect.height / 2
	);

	// 5. 정사각형 ROI 생성
	int x = center.x - crop_size / 2;
	int y = center.y - crop_size / 2;

	cv::Rect rect(x, y, crop_size, crop_size);

	// 6. 이미지 범위와 교집합 (밖으로 벗어나지 않게)
	cv::Rect img_rect(0, 0, img.cols, img.rows);
	rect = rect & img_rect;

	return rect;
}



void CKSLDBManagerDlg::GetCPFormACRResult(Result roi_l_hand1s, Result roi_r_hand2s, RECT l_hand_roi, RECT r_hand_roi)
{
	m_result.sgcp_rcLeftHand = l_hand_roi;

	m_result.sgcp_rcRightHand = r_hand_roi;

	if (roi_l_hand1s.faces.size() > 0)
	{
		m_result.LH_faces.resize(roi_l_hand1s.faces.size());
		copy(roi_l_hand1s.faces.begin(), roi_l_hand1s.faces.end(), m_result.LH_faces.begin());
	}
	if (roi_l_hand1s.verts.size() > 0)
	{
		m_result.LH_verts.resize(roi_l_hand1s.verts.size());
		copy(roi_l_hand1s.verts.begin(), roi_l_hand1s.verts.end(), m_result.LH_verts.begin());
	}
	if (roi_r_hand2s.faces.size() > 0)
	{
		m_result.RH_faces.resize(roi_r_hand2s.faces.size());
		copy(roi_r_hand2s.faces.begin(), roi_r_hand2s.faces.end(), m_result.RH_faces.begin());
	}
	if (roi_r_hand2s.verts.size() > 0)
	{
		m_result.RH_verts.resize(roi_r_hand2s.verts.size());
		copy(roi_r_hand2s.verts.begin(), roi_r_hand2s.verts.end(), m_result.RH_verts.begin());
	}


	for (int k = 0; k < roi_l_hand1s.jpt3ds.size(); k++)m_result.sgcp_vtLHSkeleton.push_back(roi_l_hand1s.jpt3ds[k]);
	for (int k = 0; k < roi_r_hand2s.jpt3ds.size(); k++)m_result.sgcp_vtRHSkeleton.push_back(roi_r_hand2s.jpt3ds[k]);

	for (int k = 0; k < roi_l_hand1s.poses.size(); k++)m_result.sgcp_vtNondominantPCACompoent.push_back(roi_l_hand1s.poses[k]);
	for (int k = 0; k < roi_r_hand2s.poses.size(); k++)m_result.sgcp_vtDominantPCACompoent.push_back(roi_r_hand2s.poses[k]);
	m_result.sgcp_vtDominantPCACompoentConfidence = roi_r_hand2s.conf;
	m_result.sgcp_vtNondominantPCACompoentConfidence = roi_l_hand1s.conf;

	for (int k = 0; k < roi_l_hand1s.orientaion.size(); k++)
	{
		auto ori = roi_l_hand1s.orientaion[k];
		m_result.sgcp_vtLeftHandOrientation.push_back(ori.x);
		m_result.sgcp_vtLeftHandOrientation.push_back(ori.y);
		m_result.sgcp_vtLeftHandOrientation.push_back(ori.z);
	}
	for (int k = 0; k < roi_r_hand2s.orientaion.size(); k++)
	{
		auto ori = roi_r_hand2s.orientaion[k];
		m_result.sgcp_vtRightHandOrientation.push_back(ori.x);
		m_result.sgcp_vtRightHandOrientation.push_back(ori.y);
		m_result.sgcp_vtRightHandOrientation.push_back(ori.z);
	}
	if (m_result.sgcp_vtSkeleton.size() > 14)
	{
		m_result.origin_joints.push_back(m_result.sgcp_vtSkeleton[7]);
		m_result.origin_joints.push_back(m_result.sgcp_vtSkeleton[14]);
	}	

}



std::vector<Point2f> CKSLDBManagerDlg::GetRefHL2DPointsMP(std::vector <cv::Point3f> joints) //0 Left, 1 Right
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


std::vector<double> CKSLDBManagerDlg::SimHandLocation(_typeChangepointDB cp_db, _typeChangepointDB query)
{
	std::vector<double> sim_val;;

	auto hl_db = GetRefHL2DPointsMP(cp_db.sgcp_vtSkeletonMP); //0:Left, 1:Right
	auto hl_query = GetRefHL2DPointsMP(query.sgcp_vtSkeletonMP);

	double scale = m_util.Distance(cv::Point2f(0, 0), cv::Point2f(1920, 1080)) / 4.0;

	if (cp_db.sgcp_vtHandStatus == 3 && hl_db.size() == 2 && query.sgcp_vtHandStatus == 3 && hl_query.size() == 2)
	{
		auto simL = m_util.Distance(hl_db[0], hl_query[0]) / scale; //Left
		auto simR = m_util.Distance(hl_db[1], hl_query[1]) / scale; //Right

		sim_val.push_back(1.0 - simL);
		sim_val.push_back(1.0 - simR);

	}
	else if (cp_db.sgcp_vtHandStatus == 1 && hl_db.size() == 2 && query.sgcp_vtHandStatus == 1 && hl_query.size() == 2)
	{
		auto simL = m_util.Distance(hl_db[0], hl_query[0]) / scale; //Left
		auto simR = m_util.Distance(hl_db[1], hl_query[1]) / scale; //Right

		sim_val.push_back(-1.0);
		sim_val.push_back(1.0 - simR);
	}
	else if (cp_db.sgcp_vtHandStatus == 2 && hl_db.size() == 2 && query.sgcp_vtHandStatus == 2 && hl_query.size() == 2)
	{
		auto simL = m_util.Distance(hl_db[0], hl_query[0]) / scale; //Left
		auto simR = m_util.Distance(hl_db[1], hl_query[1]) / scale; //Right

		sim_val.push_back(1.0 - simL);
		sim_val.push_back(-1.0);
	}
	else
	{
		sim_val.push_back(-1.0);
		sim_val.push_back(-1.0);
	}
	return sim_val;
}



void CKSLDBManagerDlg::GetSimsFromCP(_tagChangepointDB cp1, _tagChangepointDB cp2,
	std::vector<double>& hsR, std::vector<double>& hoR, std::vector<double>& hlR,
	std::vector<double>& hsL, std::vector<double>& hoL, std::vector<double>& hlL
)
{
	double hs_simR = -1;
	double ho_simR = -1;
	std::vector<double> hl_simR = { -1,-1 };

	double hs_simL = -1;
	double ho_simL = -1;
	std::vector<double> hl_simL = { -1,-1 };

	if ( (cp1.sgcp_vtHandStatus == 3 && cp2.sgcp_vtHandStatus == 3) || (cp1.sgcp_vtHandStatus == 1 && cp2.sgcp_vtHandStatus == 1))
	{
		if (cp2.sgcp_vtDominantPCACompoent.size() > 0 && (cp2.sgcp_vtHandStatus == 1 || cp2.sgcp_vtHandStatus == 3))
			hs_simR = HandShapeDiff::getShapeDifference(cp1.sgcp_vtDominantPCACompoent, cp2.sgcp_vtDominantPCACompoent); //-1~1
		if (cp2.sgcp_vtRightHandOrientation.size() > 0 && (cp2.sgcp_vtHandStatus == 1 || cp2.sgcp_vtHandStatus == 3))
			ho_simR = 1.0 - fabs(m_util.getRotationAngle(cp1.sgcp_vtRightHandOrientation, cp2.sgcp_vtRightHandOrientation) / CV_PI); //-1~1
		if (cp2.sgcp_vtSkeletonMP.size() > 0)
			hl_simR = SimHandLocation(cp1, cp2); //-1~1	
	}
	if ((cp1.sgcp_vtHandStatus == 3 && cp2.sgcp_vtHandStatus == 3) || (cp1.sgcp_vtHandStatus == 2 && cp2.sgcp_vtHandStatus == 2))
	{
		if (cp2.sgcp_vtNondominantPCACompoent.size() > 0 && (cp2.sgcp_vtHandStatus == 2 || cp2.sgcp_vtHandStatus == 3))
			hs_simL = HandShapeDiff::getShapeDifference(cp1.sgcp_vtNondominantPCACompoent, cp2.sgcp_vtNondominantPCACompoent); //-1~1
		if (cp2.sgcp_vtLeftHandOrientation.size() > 0 && (cp2.sgcp_vtHandStatus == 2 || cp2.sgcp_vtHandStatus == 3))
			ho_simL = 1.0 - fabs(m_util.getRotationAngle(cp1.sgcp_vtLeftHandOrientation, cp2.sgcp_vtLeftHandOrientation) / CV_PI); //-1~1
		if (cp2.sgcp_vtSkeletonMP.size() > 0)
			hl_simL = SimHandLocation(cp1, cp2); //-1~1		
	}	

//	printf("\nSim:%f %f %f %f %f %f Hand Status:%d %d\n", hs_simR, ho_simR, hl_simR[1], hs_simL, ho_simL, hl_simL[0], cp1.sgcp_vtHandStatus, cp2.sgcp_vtHandStatus);

	hsR.push_back(hs_simR);
	hoR.push_back(ho_simR);
	hlR.push_back(hl_simR[1]);
	hsL.push_back(hs_simL);
	hoL.push_back(ho_simL);
	hlL.push_back(hl_simL[0]);
}


int CKSLDBManagerDlg::GetHandStatusFromCP(_typeChangepointDB cp)
{
	int r_hand_status = 0;
	int l_hand_status = 0;

	if (cp.sgcp_vtRightHandOrientation.size() > 0)
	{
		double degree_angle = GetDegreeAngleOfRHandTipDirection(cp);		
		double degree_angle2 = GetDegreeAngleOfRElbowDirection(cp);
		double degree_angle3 = GetDegreeAngleOfRElbowAngle(cp);

		if (degree_angle > READY_HANDTIP_ANGLE || degree_angle2> READY_ELBOW_DIRECTION_ANGLE || degree_angle3< READY_ELBOW_ANGLE)
			r_hand_status = 1;

	}

	if (cp.sgcp_vtLeftHandOrientation.size() > 0)
	{
		double degree_angle = GetDegreeAngleOfLHandTipDirection(cp);
		double degree_angle2 = GetDegreeAngleOfLElbowDirection(cp);
		double degree_angle3 = GetDegreeAngleOfLElbowAngle(cp);

		if (degree_angle > READY_HANDTIP_ANGLE || degree_angle2 > READY_ELBOW_DIRECTION_ANGLE || degree_angle3 < READY_ELBOW_ANGLE)
			l_hand_status = 1;
	}

	int hand_status=0;

	if (r_hand_status && l_hand_status) hand_status = 3;
	else if(r_hand_status && !l_hand_status)hand_status = 1;
	else if (!r_hand_status && l_hand_status)hand_status = 2;

	return hand_status;
}



double CKSLDBManagerDlg::GetDegreeAngleOfRHandTipDirection(_typeChangepointDB cp)
{
	double degree_angle = 0;

	std::vector<Point3f> orientationR;
	std::vector<float> poseR;

	if (cp.sgcp_vtRightHandOrientation.size() > 0 && cp.sgcp_vtDominantPCACompoent.size() > 0)
	{
		SetPoseOrientationForManoModel(poseR, orientationR, cp.sgcp_vtRightHandOrientation, cp.sgcp_vtDominantPCACompoent, 1);
		degree_angle = fabs(m_util.GetAngle(orientationR[0], Point3f(0, 1, 0)));		
	}
	return degree_angle;
}

double CKSLDBManagerDlg::GetDegreeAngleOfLHandTipDirection(_typeChangepointDB cp)
{
	double degree_angle = 0;

	std::vector<Point3f> orientationL;
	std::vector<float> poseL;

	if (cp.sgcp_vtLeftHandOrientation.size() > 0 && cp.sgcp_vtNondominantPCACompoent.size() > 0)
	{
		SetPoseOrientationForManoModel(poseL, orientationL, cp.sgcp_vtLeftHandOrientation, cp.sgcp_vtNondominantPCACompoent, 0);
		degree_angle = fabs(m_util.GetAngle(orientationL[0], Point3f(0, 1, 0)));
	}
	return degree_angle;
}

double CKSLDBManagerDlg::GetDegreeAngleOfLElbowDirection(_typeChangepointDB cp)
{
	double degree_angle = 0;

	if (cp.sgcp_vtSkeletonMP.size() > 0 )
	{	
		auto elbow = cp.sgcp_vtSkeletonMP[15] - cp.sgcp_vtSkeletonMP[13];
		degree_angle = fabs(m_util.GetAngle(elbow, Point3f(0.0, 1.0, 0.0)));
	}
	return degree_angle;
}



double CKSLDBManagerDlg::GetDegreeAngleOfRElbowDirection(_typeChangepointDB cp)
{
	double degree_angle = 0;

	if (cp.sgcp_vtSkeletonMP.size() > 0)
	{
		auto elbow = cp.sgcp_vtSkeletonMP[16] - cp.sgcp_vtSkeletonMP[14];		
		degree_angle = fabs(m_util.GetAngle(elbow, Point3f(0.0, 1.0, 0.0)));
	}
	return degree_angle;
}

double CKSLDBManagerDlg::GetDegreeAngleOfLElbowAngle(_typeChangepointDB cp)
{
	double degree_angle = 0;

	if (cp.sgcp_vtSkeletonMP.size() > 0)
	{
		auto elbow = cp.sgcp_vtSkeletonMP[11] - cp.sgcp_vtSkeletonMP[13];
		auto elbow2 = cp.sgcp_vtSkeletonMP[15] - cp.sgcp_vtSkeletonMP[13];
		cv::Point2f p1 = cv::Point2f(elbow.x, elbow.y);
		cv::Point2f p2 = cv::Point2f(elbow2.x, elbow2.y);
		degree_angle = fabs(m_util.GetAngle(p1, p2));
	}
	return degree_angle;
}

double CKSLDBManagerDlg::GetDegreeAngleOfRElbowAngle(_typeChangepointDB cp)
{
	double degree_angle = 0;

	if (cp.sgcp_vtSkeletonMP.size() > 0)
	{
		auto elbow = cp.sgcp_vtSkeletonMP[12] - cp.sgcp_vtSkeletonMP[14];
		auto elbow2 = cp.sgcp_vtSkeletonMP[16] - cp.sgcp_vtSkeletonMP[14];
		cv::Point2f p1 = cv::Point2f(elbow.x, elbow.y);
		cv::Point2f p2 = cv::Point2f(elbow2.x, elbow2.y);
		degree_angle = fabs(m_util.GetAngle(p1, p2));
	}
	return degree_angle;
}



std::vector<_tagChangepointDB> CKSLDBManagerDlg::FindSimilarCPs(
	_tagChangepointDB ref_cp,
	std::vector<_tagChangepointDB> cadidate_cps)
{
	std::vector<_tagChangepointDB> result_cps;

	// 1) 유사도 원시 점수들 채우기
	std::vector<double> hsR, hoR, hlR;
	std::vector<double> hsL, hoL, hlL;

	hsR.reserve(cadidate_cps.size()); hoR.reserve(cadidate_cps.size()); hlR.reserve(cadidate_cps.size());
	hsL.reserve(cadidate_cps.size()); hoL.reserve(cadidate_cps.size()); hlL.reserve(cadidate_cps.size());

	for (const auto& cand_cp : cadidate_cps) // CP
		GetSimsFromCP(ref_cp, cand_cp, hsR, hoR, hlR, hsL, hoL, hlL);

	if (!(ref_cp.sgcp_vtHandStatus == 1 || ref_cp.sgcp_vtHandStatus == 2 || ref_cp.sgcp_vtHandStatus == 3))
		return result_cps;

	// 2) 후보별 최종 점수 계산 (가중합)
	const size_t N = cadidate_cps.size();
	std::vector<double> score(N, -std::numeric_limits<double>::infinity());

	auto safe = [](double v) {
		return (std::isfinite(v) ? v : 0.0); // NaN/Inf 방지
		};

	for (size_t i = 0; i < N; ++i)
	{
		if (ref_cp.sgcp_vtHandStatus == 3) {
			score[i] =
				HS_WEIGHT * (safe(hsR[i]) + safe(hsL[i])) +
				HO_WEIGHT * (safe(hoR[i]) + safe(hoL[i])) +
				HL_WEIGHT * (safe(hlR[i]) + safe(hlL[i]));
		}
		else if (ref_cp.sgcp_vtHandStatus == 1) { // 오른손만
			score[i] =
				HS_WEIGHT * safe(hsR[i]) +
				HO_WEIGHT * safe(hoR[i]) +
				HL_WEIGHT * safe(hlR[i]);
		}
		else { // 2: 왼손만
			score[i] =
				HS_WEIGHT * safe(hsL[i]) +
				HO_WEIGHT * safe(hoL[i]) +
				HL_WEIGHT * safe(hlL[i]);
		}
	}

	// 3) 점수 기준 내림차순 정렬 (index 배열 정렬)
	std::vector<int> ord(N);
	std::iota(ord.begin(), ord.end(), 0);
	std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) {
		if (score[a] == score[b]) {
			// 동점 시 부드러운 타이브레이커 (예: 원본 인덱스 오름차순)
			return a < b;
		}
		return score[a] > score[b]; // 내림차순
		});

	// 4) 상위 SIMILAR_FRAME_NUM만 선택해 SimilarScore 세팅 후 반환
	const size_t K = std::min(static_cast<size_t>(SIMILAR_FRAME_NUM), N);
	result_cps.reserve(K);

	for (size_t k = 0; k < K; ++k)
	{
		int idx = ord[k];
		cadidate_cps[idx].SimilarScore = score[idx];
		result_cps.push_back(cadidate_cps[idx]);
	}

	return result_cps;
}




std::vector<GlossInfered> CKSLDBManagerDlg::RemoveTransientHandState(std::vector<GlossInfered> infered) 
{
	std::vector<GlossInfered> ret;
	
	for (int k = 0; k < infered.size(); k += 1)
	{
		if (infered[k].prob >= MIN_SIMILARITY)
		{
			ret.push_back(infered[k]);
		}
	}
	return ret;	
}

void CKSLDBManagerDlg::GetMPHandPointsFromPose(std::vector<Point3f> skeleton, cv::Mat img, std::vector<cv::Point> &LHand, std::vector<cv::Point>& RHand)
{
	LHand.clear();
	RHand.clear();

	if (skeleton.size() > 0)
	{
		LHand.push_back(cv::Point(skeleton[15].x * img.cols, skeleton[15].y * img.rows));
		LHand.push_back(cv::Point(skeleton[17].x * img.cols, skeleton[17].y * img.rows));
		LHand.push_back(cv::Point(skeleton[19].x * img.cols, skeleton[19].y * img.rows));
		LHand.push_back(cv::Point(skeleton[21].x * img.cols, skeleton[21].y * img.rows));

		RHand.push_back(cv::Point(skeleton[16].x * img.cols, skeleton[16].y * img.rows));
		RHand.push_back(cv::Point(skeleton[18].x * img.cols, skeleton[18].y * img.rows));
		RHand.push_back(cv::Point(skeleton[20].x * img.cols, skeleton[20].y * img.rows));
		RHand.push_back(cv::Point(skeleton[22].x * img.cols, skeleton[22].y * img.rows));
	}

}


CString CKSLDBManagerDlg::GetHandStateString(int hand_state)
{
	CString str;
	if (hand_state == 3)
		str.Format(_T("양손 수어"));
	else if (hand_state == 1)
		str.Format(_T("우세손 수어"));
	else if (hand_state == 2)
		str.Format(_T("비우세손 수어"));
	else str.Format(_T("인식불가"));
	return str;
}





double CKSLDBManagerDlg::GetMotionVauleWithOpticalFlow(cv::Mat prevGray, std::vector<cv::Point2f> prevPts, cv::Mat gray)
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


double CKSLDBManagerDlg::GetMotionVauleWithOpticalFlow(cv::Mat frame)
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

double CKSLDBManagerDlg::GetMotionVauleWithOpticalFlow(cv::Mat prev, cv::Mat frame, cv::Rect rect)
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

double CKSLDBManagerDlg::GetMotionVauleWithOpticalFlow(cv::Mat prev, cv::Mat frame, cv::Rect Rrect, cv::Rect Lrect)
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



