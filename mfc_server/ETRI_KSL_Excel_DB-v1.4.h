#pragma once
#include "pch.h"
#include <vector>
#include <OpenXLSX.hpp>

#include "VideoUtil-v1.1.h"

#include <string>
#include <atlstr.h> // CString

//////////////////////////////////////////////////////////////////////////
// Ver 1.2 Summary
// Video DB : Width, Height, FPS added
// CP DB: FPS deleted
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Ver 1.3 Summary
// Corpus define, Read, Write
//////////////////////////////////////////////////////////////////////////


typedef struct _cap_data {
	cv::Mat img;

	std::vector<Point3f> mp_skeleton;
	int num_bodies;
	int frame_idx;
	uint64_t timestamp_usec;	
}_cap_data;

class CSGItem;

class MediaPipe3DReconstructor;

enum eInfo {
	eInfo_GlossDB = 0,
	eInfo_VideoDB,
	eInfo_CorpusVideoDB,
	eInfo_CpDB
};

enum class ESGV_ViewPoint : UINT8 
{
	NONE=0,
	CENTER='C',
	LEFT='L',
	RIGHT='R',
};


enum class ESGCP_HandLocation : UINT8
{
	HEAD = 0,
	BODY,
	NONDOMINENT_HAND,
	MAX_HAND_POSITION_COUNT,
};

enum class ESGCP_HandLocation_Flag : UINT32
{
	FLAG_HAND_POSITION_HEAD = 0x00000001,
	FLAG_HAND_POSITION_BODY = 0x00000002,
	FLAG_HAND_POSITION_NONDOMINENT_HAND = 0x00000004,
};


typedef struct _tagSignGlossDB {
	UINT32 index;
	UINT32 sg_ID;
	UINT32 sg_OriginNumber;
	CString sg_Name;
	CString sg_Description;
	CString sg_Category;
	CString sg_Combination;
	std::vector<UINT32> sg_vtVideo;	// Video list
	INT sg_key_frame_num; //Added by hlee
	CString sg_HandMovement;
	CString sg_KeyFrames;	

} _typeSignGlossDB;

typedef struct _tagVideoDB {
	UINT32 index;
	UINT32 sgv_sgIndex;
	UINT32 sgv_ID;
	UINT32 sgv_sgID;
	UINT32 sgv_sgOriginNumber;
	ESGV_ViewPoint sgv_ViewPoint;				// 'C', 'L', 'R'
	CString sgv_Person;					// '1', '2'
	UINT16 dummy;
	//CString sgv_FileName;				// "1.001.C.mkv"
	//CString sgv_SkeletonFileName		// "1.001.C.json"
	std::vector<UINT32> sg_vtCP;		// Changepoint list

	CString sgv_FileExt; //Added 2025.1.16
	UINT32 sgv_Width; //Added 2025.8.22
	UINT32 sgv_Height;
	UINT32 sgv_FPS;
} _typeVideoDB;

typedef struct _tagCorpusVideoDB {
	UINT32 index;	
	UINT32 scv_ID;	
	
	CString scv_FileName;				// "1-10-120.001.C.000.mkv"
	CString scv_Sentence;
	ESGV_ViewPoint scv_ViewPoint;				// 'C', 'L', 'R'
	CString scv_Person;					// '1', '2'
	UINT16 dummy;

	//size same
	std::vector<UINT32> scv_sgOriginNumber;
	std::vector<UINT32> scv_key_frame_index;
	std::vector<UINT32> scv_sgIndex; // key frame 갯수 만큼 할당
	std::vector<UINT32> scv_unified_sgIndex; //추론시 스코어 계산을 위하여 중복 인덱스 제거한 유니크한 글로수
	std::vector<UINT32> scv_cpIndex;		// Changepoint list
	std::vector<UINT32> scv_handStatus;

	std::vector<UINT32> scv_gloss_ranges; //gloss origin number-start index-end index
		
	CString scv_FileExt; //Added 2025.1.16
	UINT32 scv_Width; //Added 2025.8.22
	UINT32 scv_Height;
	UINT32 scv_FPS;

	std::vector<cv::Mat> scv_img;

	// ======== [추가] corpus 간 gloss 희소도 기반 weight 사전 계산 결과 ========
// 한 corpus 안에서의 unique gloss 리스트
	std::vector<UINT32> scv_gloss_unique;          // 유니크 sgIndex
	std::vector<double> scv_gloss_unique_weight;   // 그 gloss의 IDF 기반 weight

} _typeCorpusVideoDB;

typedef struct _tagChangepointDB {
	UINT32 index; // DB vector의 인덱스. 생략 가능
	UINT32 sgcp_sgIndex; //엑셀 파일의 인덱스 생략 없음
	UINT32 sgcp_ID;
	UINT32 sgcp_sgvID;
	UINT64 sgcp_TimeStamp;
	UINT32 sgcp_FrameIndex;
	INT32 sgcp_Class;
	RECT sgcp_rcLeftHand;
	RECT sgcp_rcRightHand;

	//New For corpus
	UINT32 sgcp_scvID;
	UINT32 sgcp_scvFrameID;


	std::vector<FLOAT> sgcp_vtLeftHandOrientation;
	std::vector<FLOAT> sgcp_vtRightHandOrientation;
	std::vector<UINT32>sgcp_vtHandLocation; //
	std::vector<FLOAT> sgcp_vtHandLocationConfidence;
	std::vector<FLOAT> sgcp_vtRelativeDistance;

	//added(2023.12.19)
	std::vector<FLOAT> sgcp_vtDominantPCACompoent;
	FLOAT sgcp_vtDominantPCACompoentConfidence;
	std::vector<FLOAT> sgcp_vtNondominantPCACompoent;
	FLOAT sgcp_vtNondominantPCACompoentConfidence;
	std::vector<cv::Point3f> sgcp_vtSkeleton;
	std::vector<cv::Point3f> sgcp_vtRHSkeleton;
	std::vector<cv::Point3f> sgcp_vtLHSkeleton;
	int sgcp_vtHandStatus;
	std::vector < FLOAT> sgcp_vtSkeleton_confidence;
	FLOAT sgcp_Confidence;
//	std::vector<cv::Point2f> sgcp_vtHandLocation_ref_model;
	std::vector<Point3f> LH_verts;
	std::vector<Point3f> LH_faces;
	std::vector<Point3f> RH_verts;
	std::vector<Point3f> RH_faces;
	std::vector<Point3f> origin_joints;
//	cv::Mat rot_mat;
	UINT32 sgcp_kIndex; //index:0 None, 1,2,3,4,...

	std::vector<cv::Point3f> sgcp_vtSkeletonMP;
	std::vector<double> sgcp_vtSkeletonVisiibilityMP;
	std::vector<cv::Point3f> sgcp_vtLHandSkeletonMP;
	std::vector<cv::Point3f> sgcp_vtRHandSkeletonMP;

	double RHandEstErr;
	double LHandEstErr;
	double SimilarScore;

} _typeChangepointDB;


typedef struct _tagChangepointTimePositionDB {
	UINT64 cptp_TimePosition;
	FLOAT cptp_fTimePositionSecond;
	CString cptp_strTimePosition;
} _typeCPTimePositionDB;



class Excel_DB
{
public:
	Excel_DB();

	HWND m_hWndParent;
	UINT m_uMsgID;
	void _SetMessageID(HWND hWnd, UINT uMSG);



	std::vector<int> m_DB_origin_numbers;


	//std::vector<_typeDicDB> m_MainDB;
	std::vector<_typeSignGlossDB> m_SignGlossDB;

	std::vector<_typeCorpusVideoDB> m_CorpusVideoDB;
	std::vector<_typeVideoDB> m_VideoDB;
	std::vector<_typeChangepointDB> m_CpDB;

	BOOL _Load(CString strPath);
	BOOL CheckSignGlossSheet(OpenXLSX::XLWorksheet& wks);
	BOOL _LoadSignGlossDB(OpenXLSX::XLWorksheet& wks);
	BOOL _UpdateSignGlossDB(OpenXLSX::XLWorksheet& wks, int col_num);

	BOOL CheckSignGlossVideoSheet(OpenXLSX::XLWorksheet& wks);
	VOID _ClearVideoDB(_typeVideoDB& itemVideo);
	VOID _ClearCorpusVideoDB(_typeCorpusVideoDB& itemVideo);
	BOOL _LoadVideoDB(OpenXLSX::XLWorksheet& wks);
	BOOL _LoadCorpusVideoDB(OpenXLSX::XLWorksheet& wks);

	BOOL _UpdateVideoDB(OpenXLSX::XLWorksheet& wks, int col_num);
	BOOL _UpdateCorpusVideoDB(OpenXLSX::XLWorksheet& wks, int col_num);

	BOOL _WriteVideoDB(OpenXLSX::XLWorksheet& wks, _typeVideoDB v_db);
	BOOL _AddVideoDB(OpenXLSX::XLWorksheet& wks, _typeVideoDB v_db);
	BOOL _AddCorpusVideoDB(OpenXLSX::XLWorksheet& wks, _typeCorpusVideoDB itemVideo);

	BOOL CheckSignVideoChangepointSheet(OpenXLSX::XLWorksheet& wks);
	VOID _ClearChangepointDB(_typeChangepointDB& itemCP);
	BOOL _LoadChangePointDB(OpenXLSX::XLWorksheet& wks);
	BOOL _WriteChangePointDB(OpenXLSX::XLWorksheet& wks, _typeChangepointDB itemCP);
	BOOL _AddChangePointDB(OpenXLSX::XLWorksheet& wks, _typeChangepointDB itemCP);
	BOOL _UpdateChangePointDB(OpenXLSX::XLWorksheet& wks, int col_num);

	BOOL _SearchSGIndexBySGID(UINT& index, UINT32 key);
	BOOL _SearchAllVideosBySGID(std::vector<UINT>& vtVideo, UINT32 sgID);
	BOOL _SearchAllVideosBySGID(std::vector<UINT>& vtVideo, UINT32 sgID, CString person_id);
	BOOL _SearchAllVideosBySGID(std::vector<_typeVideoDB*>& vtVideoPtr, UINT32 sgID);
	BOOL _SearchAllVideosByOriginNumber(std::vector<UINT>& vtVideo, UINT32 key);
	BOOL _SearchAllVideosByOriginNumber(std::vector<_typeVideoDB*>& vtVideoPtr, UINT32 key);	


	//_typeDicDB GetData(int index);
	_typeSignGlossDB GetSignGlossData(int index);
	_typeVideoDB GetVideoData(int index);
	_typeChangepointDB GetChangepointData(int index);
	int _GetCount() {
		//return (int)m_MainDB.size();
		return (int)m_SignGlossDB.size();
	}
	int _Find_by_Expression(CString strFind, CListCtrl& listResult);

	BOOL Find_CommaTokenString(CString& strFind, CString& strTarget);

	BOOL IsDBList(int query);
	
	void recreateSheetWithHeader(OpenXLSX::XLDocument& doc, const std::string& sheetName);

	UINT GetSignGlossID(int index);
	UINT GetSignGlossOriginNumber(int index);
	VOID GetSignGlossName(int index, CString& str);
	VOID GetSignGlossDescription(int index, CString& str);
	VOID GetSignGlossCategory(int index, CString& str);

	INT GetSignGlossIndexFromOriginNumber(int OriginNumber);
	INT GetSignGlossIndexFromVideoID(int VideoID);

	UINT GetSignGlossVideoID(int index);
	UINT GetSignGlossVideoSGID(int index);
	ESGV_ViewPoint GetSignGlossVideoViewPoint(int index);
	CString GetSignGlossVideoPerson(int index);
	VOID GetSignGlossVideoFilePath(int indexVideo, CString& str);
	VOID GetSignGlossVideoFileName(int indexVideo, CString& str);
	VOID GetSignGlossVideoFileNameExt(int indexVideo, CString& str);
	VOID GetSignGlossSkeletonFilePath(int indexVideo, CString& str);
	VOID GetSignGlossSkeletonFileName(int indexVideo, CString& str);

	UINT GetChangepointID(int index);
	UINT GetChangepointVideoID(int index);
	UINT64 GetChangepointTimestamp(int index);
	UINT GetChangepointFramNumber(int index);
	INT GetChangepointConfidence(int index);
	RECT GetChangepointLeftHandRect(int index);
	RECT GetChangepointRightHandRect(int index);
	UINT GetVideoFPS(int index);
	_typeChangepointDB GetChangepoint(int sgcp_sgvID, int sgcp_ID);
	_typeChangepointDB GetChangepoint(int OriginNumber, CString person, int sgcp_ID);
	std::vector<_typeChangepointDB> GetChangepoint(int sgcp_sgvID);


	INT GetValidCPNum(_typeVideoDB videoDB);
	INT GetNewVideoID();
	CString GetMKVFIleNameFromCP(_typeChangepointDB cp, CString db_dir);
	CString GetMKVFIleFromVideo(_typeVideoDB video, CString db_dir);
	CString GetJsonFIleFromVideo(_typeVideoDB video, CString db_dir);
	void AddNewVideo(CString video_path_nm, int new_video_id, std::vector < _typeVideoDB >& video_db);
	void AddNewCorpusVideo(CString video_nm, int new_video_id, std::vector<UINT32> key_frames, std::vector<UINT32> origin_numbers,
		CString sentence, std::vector<UINT32> hand_status, std::vector<UINT32> gloss_ranges, std::vector < _typeCorpusVideoDB >& video_db);

	std::vector< _typeVideoDB > GetVideoFromOriginNumber(int origin_number);
	std::vector< _typeVideoDB > GetVideo(int origin_number, CString Person, ESGV_ViewPoint type);

	CString convert_utf8_to_unicode_string(std::string utf3_origin);
	std::string convert_unicode_to_utf8_string(const CString& u16_origin);

	BOOL SetSGItem(CSGItem& sgItem, int index);
	
	int DecodeEntry(CString entry, CString& gloss_nm, int& origin_number);

	CVideoUtil m_util;


	std::vector< _typeChangepointDB > GetCPFromOriginNumber(int origin_number);
	std::vector< _typeVideoDB > GetCenterVideo(UINT32 origin_number, CString person);

	void _ResizeACRHand(std::vector<Point3f> AZSkeleton, std::vector<Point3f>& RHSkeleton, std::vector<Point3f>& LHSkeleton);


private:

};


class CSGItem
{
public:
	CSGItem() {
		;
	}

	CSGItem(Excel_DB& excelDB, int index, CString strVideoFilePath = _T(""), CString strSkeletonFilePath = _T("")) {
		SetSGItem(excelDB, index);		
		SetVideoFilePath(strVideoFilePath);
		SetSkeletonFilePath(strSkeletonFilePath);
	}

	~CSGItem() {
		;
	}

	VOID SetSGItem(Excel_DB& excelDB, int index) {
		_typeSignGlossDB sgItem = excelDB.GetSignGlossData(index);
		m_SignGlossDB = sgItem;
		m_SignGlossDB.sg_vtVideo = sgItem.sg_vtVideo;

		int nSavedVideoCount = (int)sgItem.sg_vtVideo.size();
		if (nSavedVideoCount == 0) {
			//excelDB._SearchAllVideosByOriginNumber(m_SignGlossDB.sg_vtVideo, m_SignGlossDB.sg_OriginNumber);
			excelDB._SearchAllVideosBySGID(m_SignGlossDB.sg_vtVideo, m_SignGlossDB.sg_ID);
		}

		m_VideoDB.clear();

		int nVideoCount = (int)m_SignGlossDB.sg_vtVideo.size();
		for (int i = 0; i < nVideoCount; i++)
		{
			UINT32 unVideoIndex = m_SignGlossDB.sg_vtVideo[i];
			m_VideoDB.push_back(excelDB.m_VideoDB[unVideoIndex]);
		}

		m_CpDB.clear();

		if (0 < nVideoCount)
		{
			int nCPCount = (int)m_VideoDB[0].sg_vtCP.size();
			for (int i = 0; i < nCPCount; i++)
			{
				UINT32 unCPIndex = m_VideoDB[0].sg_vtCP[i];
				if (1 == excelDB.m_CpDB[unCPIndex].sgcp_Class)
				{
					m_CpDB.push_back(excelDB.m_CpDB[unCPIndex]);
				}				
			}

			CString strPath;
			excelDB.GetSignGlossVideoFilePath(m_VideoDB[0].index, strPath);
			SetVideoFilePath(strPath);
			excelDB.GetSignGlossSkeletonFilePath(m_VideoDB[0].index, strPath);
			SetSkeletonFilePath(strPath);
		}

		m_CPTimePosition.clear();
	}

	

	VOID SetSGItem(const CSGItem& sgItem) {
		CopyFrom(sgItem);
	}
	_typeSignGlossDB GetSignGlossData() {
		return m_SignGlossDB;
	}
	UINT GetID() {
		return m_SignGlossDB.sg_ID;
	}
	UINT GetOriginNumber() {
		return m_SignGlossDB.sg_OriginNumber;
	}
	VOID GetName(CString& str) {
		str = m_SignGlossDB.sg_Name;
	}
	VOID GetDescription(CString& str) {
		str = m_SignGlossDB.sg_Description;
	}
	VOID GetCategory(CString& str) {
		str = m_SignGlossDB.sg_Category;
	}
	VOID GetCombination(CString& str) {
		str = m_SignGlossDB.sg_Combination;
	}

	UINT GetVideoCount() {
		return ((UINT)m_VideoDB.size());
	}
	UINT GetVideoID(int index) {
		return m_VideoDB[index].sgv_ID;
	}
	ESGV_ViewPoint GetSVideoViewPoint(int index) {
		return m_VideoDB[index].sgv_ViewPoint;
	}
	CString GetVideoPerson(int index) {
		return m_VideoDB[index].sgv_Person;
	}

	VOID SetVideoFilePath(CString& str) {
		m_strVideoFilePath = str;
	}
	VOID GetVideoFilePath(CString& str) {
		str = m_strVideoFilePath;
	}
	VOID GetVideoFileName(int index, CString& str) {
		_typeVideoDB itemVideo = m_VideoDB[index];
		str.Format(_T("%d.%s.%c.mkv"), itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person, itemVideo.sgv_ViewPoint);
	}
	VOID SetSkeletonFilePath(CString& str) {
		m_strSkeletonFilePath = str;
	}
	VOID GetSkeletonFilePath(CString& str) {
		str = m_strSkeletonFilePath;
	}
	VOID GetSkeletonFileName(int index, CString& str) {
		_typeVideoDB itemVideo = m_VideoDB[index];
		str.Format(_T("%d.%03d.%c.json"), itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person, itemVideo.sgv_ViewPoint);
	}
#if 1

#else
	VOID GetThumbnailPath(int index, CString& str) {
		CString strVideoFile;
		CString strThumbnailPath = m_strVideoFilePath;
		strThumbnailPath.TrimRight("\\MKV");
		strThumbnailPath += "\\THUMBNAIL\\";

		GetVideoFileName(index, strVideoFile);
		strThumbnailPath += strVideoFile;
		strThumbnailPath += "\\";
		
		str = strThumbnailPath;
	}
#endif

	UINT GetChangepointCount() {
		return ((UINT)m_CpDB.size());
	}
	UINT GetChangepointID(int index) {
		return m_CpDB[index].sgcp_ID;
	}
	UINT GetChangepointVideoID(int index) {
		return m_CpDB[index].sgcp_sgvID;
	}
	UINT64 GetChangepointTimestamp(int index) {
		return m_CpDB[index].sgcp_TimeStamp;
	}
	UINT GetChangepointFramNumber(int index) {
		return m_CpDB[index].sgcp_FrameIndex;
	}
	INT GetChangepointConfidence(int index) {
		return m_CpDB[index].sgcp_Class;
	}
	RECT GetChangepointLeftHandRect(int index) {
		return m_CpDB[index].sgcp_rcLeftHand;
	}
	RECT GetChangepointRightHandRect(int index) {
		return m_CpDB[index].sgcp_rcRightHand;
	}
	UINT GetVideoFPS(int index) {
		return m_VideoDB[index].sgv_FPS;
	}

	UINT GetCPTimePositionCount() {
		return ((UINT)m_CPTimePosition.size());
	}
	UINT64 GetCPTimePosition(int index) {
		return m_CPTimePosition[index].cptp_TimePosition;
	}
	FLOAT GetCPTimePositionSecond(int index) {
		return m_CPTimePosition[index].cptp_fTimePositionSecond;
	}
	VOID GetCPTimePositionString(int index, CString& str) {
		str = m_CPTimePosition[index].cptp_strTimePosition;
	}
	VOID CopyFrom(const CSGItem& sgItem) {
		m_SignGlossDB = sgItem.m_SignGlossDB;
		m_SignGlossDB.sg_vtVideo = sgItem.m_SignGlossDB.sg_vtVideo;
		m_VideoDB = sgItem.m_VideoDB;
		m_CpDB = sgItem.m_CpDB;
		m_CPTimePosition = sgItem.m_CPTimePosition;

		m_strVideoFilePath = sgItem.m_strVideoFilePath;
		m_strSkeletonFilePath = sgItem.m_strSkeletonFilePath;

		m_seletedPerson = sgItem.m_seletedPerson;
	}

	CSGItem& operator = (const CSGItem& sgItem) {
		CopyFrom(sgItem);
		return *this;
	}

public:
	_typeSignGlossDB				m_SignGlossDB;
	std::vector<_typeVideoDB>		m_VideoDB;
	std::vector<_typeChangepointDB> m_CpDB;
	std::vector<_typeCPTimePositionDB> m_CPTimePosition;

	CString m_strVideoFilePath;
	CString m_strSkeletonFilePath;
	CString m_seletedPerson;

	
};


class MediaPipe3DReconstructor {
public:
	using Vec3 = cv::Vec3f;
	using Mat3 = cv::Matx33f;
	using P2 = cv::Point2f;
	using P3 = cv::Point3f;

	struct Intrinsics {
		float fx = 1000.f, fy = 1000.f, cx = 0.f, cy = 0.f;
		int   width = 0, height = 0;
	};
	struct Extrinsics {
		Mat3 R_cw = Mat3::eye(); // world->camera
		Vec3 C_w = { 0,0,0 };     // camera position
		bool valid = false;
	};
	struct Indices {
		int L_SH = 11, R_SH = 12;
		int L_EL = 13, R_EL = 14;
		int L_HIP = 23, R_HIP = 24;
	};
	enum class DistanceMode { Fixed, ShoulderScaled };
	enum class DepthMode { Scaler, Plane, Hybrid };

	struct Config {
		// 카메라 정면 추정(상체)
		bool  use_continuity = true;
		float ema_c = 0.20f, ema_r = 0.20f, ema_u = 0.20f, ema_f = 0.15f;
		DistanceMode distance_mode = DistanceMode::ShoulderScaled;
		float fixed_distance_m = 1.2f;
		float shoulder_scale_k = 10.0f; // d = k * |RS-LS|
		float up_offset_m = 0.10f;
		float side_offset_m = 0.00f;

		// image landmarks 깊이 복원
		DepthMode depth_mode = DepthMode::Hybrid;
		float shoulder_target_m = 0.40f;  // 실제 어깨폭
		float z_alpha_init = 1000.f;
		float z_beta_init = 0.0f;

		Indices idx{};
	};

	explicit MediaPipe3DReconstructor(const Config& cfg = {}, const Intrinsics& K = {})
		: cfg_(cfg), K_(K) {
	}

	void setIntrinsics(const Intrinsics& K) { K_ = K; }
	const Intrinsics& intrinsics() const { return K_; }
	void setConfig(const Config& c) { cfg_ = c; }
	const Config& config() const { return cfg_; }
	void setUseContinuity(bool on) { cfg_.use_continuity = on; }
	void resetFilters() { filt_.reset(); }
	void setExtrinsics(const Extrinsics& X) { X_ = X; }

	// --------- 상체(월드) → 카메라 포즈 추정 ---------
	// 입력: MediaPipe world landmarks (m, y-up)
	Extrinsics updateCameraFromUpperBodyWorld(const std::vector<P3>& mp_world) {
		BodyFrame bf = estimateUpperBodyFromWorld(mp_world, cfg_.idx);
		bf = filt_.smoothAndFix(bf, cfg_);
		last_body_ = bf;

		float d = computeViewDistanceFromWorld(mp_world, cfg_.idx);
		Vec3 C = bf.torso_center - bf.f * d + bf.u * cfg_.up_offset_m + bf.r * cfg_.side_offset_m;

		cv::Matx33f R_wc(
			bf.r[0], bf.u[0], bf.f[0],
			bf.r[1], bf.u[1], bf.f[1],
			bf.r[2], bf.u[2], bf.f[2]
		);
		X_.R_cw = R_wc.t();
		X_.C_w = C;
		X_.valid = true;
		return X_;
	}

	// --------- image landmarks → 3D 복원 ---------
	// 입력: (x,y in [0,1], z_mp)
	// 출력: 카메라 좌표, (Extrinsics 유효 시) 월드 좌표
	void reconstructFromImageLandmarks(
		const std::vector<P3>& mp_image,
		std::vector<P3>& pts_cam_out,
		std::vector<P3>* pts_world_out = nullptr
	) {
		DepthScaler scaler;
		scaler.alpha = cfg_.z_alpha_init;
		scaler.beta = cfg_.z_beta_init;
		scaler.fitShoulderAlpha(mp_image, cfg_.idx.L_SH, cfg_.idx.R_SH, K_, cfg_.shoulder_target_m);

		// 상체 평면(카메라 좌표)용 임시 점 3개
		P3 LS_c = unprojectZ(mp_image[cfg_.idx.L_SH], scaler);
		P3 RS_c = unprojectZ(mp_image[cfg_.idx.R_SH], scaler);
		P3 LH_c = unprojectZ(mp_image[cfg_.idx.L_HIP], scaler);
		P3 RH_c = unprojectZ(mp_image[cfg_.idx.R_HIP], scaler);
		P3 HIPc((LH_c.x + RH_c.x) * 0.5f, (LH_c.y + RH_c.y) * 0.5f, (LH_c.z + RH_c.z) * 0.5f);

		Plane torsoPlane = Plane::fit(LS_c, RS_c, HIPc);

		pts_cam_out.clear(); pts_cam_out.reserve(mp_image.size());
		if (pts_world_out) { pts_world_out->clear(); pts_world_out->reserve(mp_image.size()); }

		for (const auto& p : mp_image) {
			P2   uv = toPixel(p);
			Vec3 ray = rayFromPixel(uv);

			float Zc = std::numeric_limits<float>::quiet_NaN();
			if (cfg_.depth_mode != DepthMode::Scaler)
				Zc = torsoPlane.depthOnRay(ray);

			if (!(Zc > 0.f) || !std::isfinite(Zc)) // Plane 실패면 Scaler 사용
				Zc = std::max(0.02f, scaler.Zc_from_zmp(p.z));

			P3 Pc = unprojectCam(uv, Zc);
			pts_cam_out.push_back(Pc);

			if (pts_world_out && X_.valid) {
				Mat3 R_wc = X_.R_cw.t();
				Vec3 Pw = R_wc * Vec3(Pc.x, Pc.y, Pc.z) + X_.C_w;
				pts_world_out->emplace_back(Pw[0], Pw[1], Pw[2]);
			}
		}
	}

	Extrinsics extrinsics() const { return X_; }

private:
	// ===== 수학 유틸 =====
	static inline float norm3(const Vec3& v) { return std::sqrt(v.dot(v)); }
	static inline Vec3  nz(const Vec3& v) { float n = norm3(v); return (n > 1e-8f) ? v * (1.0f / n) : Vec3(0, 0, 0); }
	static inline void  orthonormalize(Vec3& r, Vec3& u, Vec3& f) {
		f = nz(f); r = nz(r - f * f.dot(r)); u = nz(u - f * f.dot(u) - r * r.dot(u));
		if (norm3(r) < 1e-6f) r = nz(f.cross(u));
		if (norm3(u) < 1e-6f) u = nz(r.cross(f));
	}

	// ===== 상체 프레임 =====
	struct BodyFrame { Vec3 torso_center, r, u, f; };

	BodyFrame estimateUpperBodyFromWorld(const std::vector<P3>& mp, const Indices& I) const {
		auto V = [&](int i) { return Vec3(mp[i].x, mp[i].y, mp[i].z); };
		Vec3 LS = V(I.L_SH), RS = V(I.R_SH);
		Vec3 LH = V(I.L_HIP), RH = V(I.R_HIP);
		Vec3 sh_c = 0.5f * (LS + RS);
		Vec3 hip_c = 0.5f * (LH + RH);
		Vec3 torso_c = 0.5f * (sh_c + hip_c);

		Vec3 r = nz(RS - LS);
		Vec3 u = nz(sh_c - hip_c);
		Vec3 f = nz(r.cross(u));

		// 첫 프레임: 팔꿈치 방향으로 f 부호 힌트
		if (!filt_.hasPrev()) {
			Vec3 LEL = V(cfg_.idx.L_EL), REL = V(cfg_.idx.R_EL);
			Vec3 el_c = 0.5f * (LEL + REL);
			Vec3 hint = nz(el_c - sh_c);
			if (f.dot(hint) < 0.0f) f = -f;
		}
		orthonormalize(r, u, f);
		return { torso_c, r,u,f };
	}

	float computeViewDistanceFromWorld(const std::vector<P3>& mp, const Indices& I) const {
		if (cfg_.distance_mode == DistanceMode::Fixed) return cfg_.fixed_distance_m;
		Vec3 LS(mp[I.L_SH].x, mp[I.L_SH].y, mp[I.L_SH].z);
		Vec3 RS(mp[I.R_SH].x, mp[I.R_SH].y, mp[I.R_SH].z);
		float shoulder = norm3(RS - LS);
		return std::max(0.05f, cfg_.shoulder_scale_k * shoulder);
	}

	// ===== EMA + 연속성 =====
	struct EMA {
		float a; bool init = false; Vec3 v{ 0,0,0 };
		explicit EMA(float alpha = 0.2f) :a(alpha) {}
		void   setAlpha(float alpha) { a = alpha; }
		void   reset() { init = false; v = Vec3(0, 0, 0); }
		Vec3   update(const Vec3& x) { if (!init) { v = x;init = true;return v; } v = v * (1.0f - a) + x * a; return v; }
	};
	struct Filter {
		EMA e_c{ 0.2f }, e_r{ 0.2f }, e_u{ 0.2f }, e_f{ 0.15f };
		Vec3 f_prev{ 0,0,1 }; bool has_prev = false;

		void applyConfig(const Config& c) {
			e_c.setAlpha(c.ema_c); e_r.setAlpha(c.ema_r);
			e_u.setAlpha(c.ema_u); e_f.setAlpha(c.ema_f);
		}
		bool hasPrev() const { return has_prev; }
		void reset() { e_c.reset(); e_r.reset(); e_u.reset(); e_f.reset(); has_prev = false; f_prev = Vec3(0, 0, 1); }

		BodyFrame smoothAndFix(const BodyFrame& b, const Config& c) {
			applyConfig(c);
			BodyFrame s;
			s.torso_center = e_c.update(b.torso_center);
			s.r = nz(e_r.update(b.r));
			s.u = nz(e_u.update(b.u));
			Vec3 f_raw = b.f;
			if (c.use_continuity && has_prev && f_raw.dot(f_prev) < 0.0f) f_raw = -f_raw;
			s.f = nz(e_f.update(f_raw));
			orthonormalize(s.r, s.u, s.f);
			f_prev = s.f; has_prev = true;
			return s;
		}
	};

	// ===== image landmarks 투영/복원 =====
	inline P2 toPixel(const P3& p_norm) const {
		return { p_norm.x * K_.width, p_norm.y * K_.height };
	}
	inline Vec3 rayFromPixel(const P2& uv) const {
		return { (uv.x - K_.cx) / K_.fx, (uv.y - K_.cy) / K_.fy, 1.0f };
	}
	inline P3 unprojectCam(const P2& uv, float Zc) const {
		Vec3 r = rayFromPixel(uv);
		return { r[0] * Zc, r[1] * Zc, Zc };
	}

	// ---- DepthScaler: 먼저 선언(읽기 함수 const) ----
	struct DepthScaler {
		float alpha = 1000.f, beta = 0.f;
		inline float Zc_from_zmp(float z_mp) const { return alpha * z_mp + beta; }

		void fitShoulderAlpha(const std::vector<P3>& mp_img,
			int idxA, int idxB,
			const Intrinsics& K, float s_target_m)
		{
			beta = 0.0f; // 단순/안정
			auto toPixel = [&](const P3& p) { return P2(p.x * K.width, p.y * K.height); };
			auto ray = [&](const P2& uv) { return Vec3((uv.x - K.cx) / K.fx, (uv.y - K.cy) / K.fy, 1.f); };

			P2 uvA = toPixel(mp_img[idxA]);
			P2 uvB = toPixel(mp_img[idxB]);
			float za = mp_img[idxA].z, zb = mp_img[idxB].z;

			auto dist_given_alpha = [&](float a) {
				Vec3 ra = ray(uvA), rb = ray(uvB);
				Vec3 Pa(ra[0] * (a * za), ra[1] * (a * za), a * za);
				Vec3 Pb(rb[0] * (a * zb), rb[1] * (a * zb), a * zb);
				Vec3 d = Pa - Pb;
				return std::sqrt(d.dot(d));
				};
			float lo = 10.f, hi = 5000.f;
			for (int it = 0; it < 40; ++it) {
				float mid = 0.5f * (lo + hi);
				(dist_given_alpha(mid) < s_target_m) ? lo = mid : hi = mid;
			}
			alpha = 0.5f * (lo + hi);
		}
	};

	inline P3 unprojectZ(const P3& p_norm, const DepthScaler& scaler) const {
		P2 uv = toPixel(p_norm);
		float Zc = std::max(0.02f, scaler.Zc_from_zmp(p_norm.z));
		return unprojectCam(uv, Zc);
	}

	// ---- 평면 ----
	struct Plane {
		Vec3 n{ 0,0,1 }; float d = 0.f; // n·X + d = 0
		static Plane fit(const P3& A, const P3& B, const P3& C) {
			Vec3 a(A.x, A.y, A.z), b(B.x, B.y, B.z), c(C.x, C.y, C.z);
			Vec3 n = (b - a).cross(c - a);
			float len = std::sqrt(n.dot(n));
			if (len < 1e-6f) return { {0,0,1}, 0.0f };
			n *= (1.0f / len);
			float d = -n.dot(a);
			return { n,d };
		}
		float depthOnRay(const Vec3& ray) const {
			float denom = n.dot(ray);
			if (std::fabs(denom) < 1e-6f) return std::numeric_limits<float>::quiet_NaN();
			float t = -d / denom; // ray.z=1 가정 → Zc=t
			return t;
		}
	};

private:
	Config     cfg_;
	Intrinsics K_;
	Extrinsics X_{};
	Filter     filt_{};
	BodyFrame  last_body_{};
};
