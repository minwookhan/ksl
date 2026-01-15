#include "pch.h"
#include "ETRI_KSL_Excel_DB-v1.4.h"
#include <iostream>
#include <opencv2\opencv.hpp>
#include  <cstring>

using namespace std;
using namespace OpenXLSX;

BOOL Excel_DB::_SearchSGIndexBySGID(UINT& index, UINT32 key)
{
	int start = 0;
	int end = (int)m_SignGlossDB.size() - 1;
	int mid;
	UINT32 sgID = 0;

	while (end - start >= 0) {
		mid = (start + end) / 2; // 중앙 값
		sgID = m_SignGlossDB[mid].sg_ID;

		if (sgID == key) { // key 값을 찾았을 때
			index = (UINT)mid;
			return TRUE;
		}
		else if (sgID > key) { // key 값이 mid의 값보다 작을때 (왼쪽으로)
			end = mid - 1;
		}
		else { // key 값이 mid의 값보다 클 때(오른쪽으로)
			start = mid + 1;
		}
	}
	return FALSE;
}

BOOL Excel_DB::_Load(CString strPath)
{
	//CString strExt = app::GetFileExt(strPath).MakeLower();
	//if (strExt != "xlsx") {
	//	AfxMessageBox("엑셀포맷이 아닙니다", MB_OK);
	//	return FALSE;
	//}
	//if (app::IsFileExist(strPath) == FALSE) {
	//	AfxMessageBox("엑셀 파일이 존재하지 않습니다", MB_OK);
	//	return FALSE;
	//}
	XLDocument doc;

	//strPath.Replace(_T("\\"), _T("/"));
//	CStringA strA = CStringA(strPath);
//	char* file_nm=strA.GetBuffer();

//	std::string str = m_util.StringToStdString(file_nm);
	std::string str = strPath.GetBuffer();
	doc.open(str);

	if (!doc.isOpen()) return FALSE;

	try {
		OutputDebugString("Sign_Gloss");
		auto wksSG = doc.workbook().worksheet("Sign_Gloss");
		_LoadSignGlossDB(wksSG);
		::SendMessage(m_hWndParent, m_uMsgID, eInfo_GlossDB, m_SignGlossDB.size());

		OutputDebugString("Sign_Gloss_Video");
		auto wksVideo = doc.workbook().worksheet("Sign_Gloss_Video");
		_LoadVideoDB(wksVideo);
		::SendMessage(m_hWndParent, m_uMsgID, eInfo_VideoDB, m_VideoDB.size());

		OutputDebugString("Sign_Corpus_Video");
		auto wksCorpusVideo = doc.workbook().worksheet("Sign_Corpus_Video");
		_LoadCorpusVideoDB(wksCorpusVideo);
		::SendMessage(m_hWndParent, m_uMsgID, eInfo_CorpusVideoDB, m_CorpusVideoDB.size());

		OutputDebugString("Sign_Video_Changepoint");
		auto wksChangepoint = doc.workbook().worksheet("Sign_Video_Changepoint");
		_LoadChangePointDB(wksChangepoint);

	//	_UpdateCPNumDB(m_CpDB);
		::SendMessage(m_hWndParent, m_uMsgID, eInfo_CpDB, m_CpDB.size());
		OutputDebugString("Finished");
		doc.close();
	}
	catch (...) {
		AfxMessageBox("엑셀 파일을 읽는동안 에러가 발생했습니다", MB_OK);
	}

	return TRUE;
}

BOOL Excel_DB::CheckSignGlossSheet(OpenXLSX::XLWorksheet& wks)
{
	std::string strVal;

	strVal = wks.cell(XLCellReference(1, 1)).value().get<std::string>();
	CString str_C = strVal.c_str();
	if (0 != str_C.Compare("Sign_Gloss_ID"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 2)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Origin_Number"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 3)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Gloss_Name"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 4)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Gloss_Description"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 5)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Gloss_Category"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 6)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Combination_Info"))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL Excel_DB::_LoadSignGlossDB(OpenXLSX::XLWorksheet& wks)
{
	if (FALSE == CheckSignGlossSheet(wks))
	{
		AfxMessageBox("Sign_Gloss 시트를 찾을 수 없습니다.", MB_OK);		
		return FALSE;
	}


	_typeSignGlossDB itemSG;

	m_SignGlossDB.clear();

	UINT32 nRowIndex = 0;
	std::vector<XLCellValue> values;
	for (auto& row : wks.rows())
	{
		values = row.values();
		uint64_t cell_idx = row.rowNumber();
		//if (3802 <= cell_idx) break;
		if (OpenXLSX::XLValueType::Empty == values.at(0).type())	// Exit loop at the end of row.
		{
			break;
		}

		if (1 < cell_idx)
		{
			itemSG.sg_ID = 0;
			itemSG.sg_OriginNumber = 0;
			itemSG.sg_Name = "";
			itemSG.sg_Category = "";
			itemSG.sg_Description = "";
			itemSG.sg_vtVideo.clear();
			itemSG.sg_key_frame_num = 0;
			itemSG.sg_HandMovement = "";
			itemSG.sg_KeyFrames = "";

			itemSG.index = nRowIndex++;
			itemSG.sg_ID  = values.at(0).get<UINT32>(); //ID
			itemSG.sg_OriginNumber = values.at(1).get<UINT32>(); //Sign_Gloss_Video_ID

			
			std::string sg_Name = values.at(2).get<std::string>(); //Name
			
			itemSG.sg_Name = convert_utf8_to_unicode_string( sg_Name); //한글있을경우에만 사용함.

			std::string sg_Description = values.at(3).get<std::string>(); //Description
			
			itemSG.sg_Description = convert_utf8_to_unicode_string(sg_Description); //한글있을경우에만 사용함.

			std::string sg_Category = values.at(4).get<std::string>(); //Category			
			itemSG.sg_Category = convert_utf8_to_unicode_string( sg_Category); //한글있을경우에만 사용함.

			if (6 <= values.size())// blank check
			{				
				std::string sg_Combination = values.at(5).get<std::string>(); //Combination				
				itemSG.sg_Combination = convert_utf8_to_unicode_string(sg_Combination); //한글있을경우에만 사용함.
			}

			if (7 <= values.size())// blank check
			{
				itemSG.sg_key_frame_num = values.at(6).get<UINT32>(); //sg_key_frame_num
			}

			if (8 <= values.size())// blank check
			{				
				std::string sg_HandMovement = values.at(7).get<std::string>(); //HM				
				itemSG.sg_HandMovement = convert_utf8_to_unicode_string( sg_HandMovement); //한글있을경우에만 사용함.
			}

			if (9 <= values.size())// blank check
			{				
				std::string sg_KeyFrames = values.at(8).get<std::string>(); //HM				
				itemSG.sg_KeyFrames = convert_utf8_to_unicode_string(sg_KeyFrames); //한글있을경우에만 사용함.
			}
			
			m_SignGlossDB.push_back(itemSG);
		}
	}
	return TRUE;
}

BOOL Excel_DB::_UpdateSignGlossDB(OpenXLSX::XLWorksheet& wks, int col_num)
{
	if (FALSE == CheckSignGlossSheet(wks))
	{
		AfxMessageBox("Sign_Gloss 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	for (int k = 0; k < m_SignGlossDB.size(); k++)
	{
		auto sg = m_SignGlossDB[k];

		if (col_num == 7)
			wks.cell(XLCellReference(k + 2, col_num)).value() = sg.sg_key_frame_num;
		if (col_num == 8)			
			wks.cell(XLCellReference(k + 2, col_num)).value() = std::string(CT2CA(sg.sg_HandMovement));
		if (col_num == 9)
			wks.cell(XLCellReference(k + 2, col_num)).value() = std::string(CT2CA(sg.sg_KeyFrames));
	}
}

BOOL Excel_DB::_UpdateVideoDB(OpenXLSX::XLWorksheet& wks, int col_num)
{
	if (FALSE == CheckSignGlossVideoSheet(wks))
	{
		AfxMessageBox("Sign_Gloss 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	for (int k = 0; k < m_VideoDB.size(); k++)
	{
		auto video = m_VideoDB[k];

		if (col_num == 10)
			wks.cell(XLCellReference(k + 2, col_num)).value() = video.sgv_Width;
		if (col_num == 11)
			wks.cell(XLCellReference(k + 2, col_num)).value() = video.sgv_Height;
		if (col_num == 12)
			wks.cell(XLCellReference(k + 2, col_num)).value() = video.sgv_FPS;

	}
}

BOOL Excel_DB::_UpdateCorpusVideoDB(OpenXLSX::XLWorksheet& wks, int col_num)
{

	for (int k = 0; k < m_CorpusVideoDB.size(); k++)
	{
		auto corpus_video = m_CorpusVideoDB[k];

		if (col_num == 2)
		{
			std::vector<UINT32> sgIDs;
			for (auto sgindex : corpus_video.scv_sgIndex)
				sgIDs.push_back(sgindex + 1);
			auto cstring = m_util.Vector2CString(sgIDs);
			wks.cell(XLCellReference(k + 2, col_num)).value() = std::string(CT2CA(cstring));
		}

		if (col_num == 6)
		{
			auto cstring = m_util.Vector2CString(corpus_video.scv_key_frame_index);
			wks.cell(XLCellReference(k + 2, col_num)).value() = std::string(CT2CA(cstring));
		}
		if (col_num == 13)
		{
			auto cstring = m_util.Vector2CString(corpus_video.scv_handStatus);
			wks.cell(XLCellReference(k + 2, col_num)).value() = std::string(CT2CA(cstring));
		}
	}

	return TRUE;
}

BOOL Excel_DB::CheckSignGlossVideoSheet(OpenXLSX::XLWorksheet& wks)
{
	std::string strVal;

	strVal = wks.cell(XLCellReference(1, 1)).value().get<std::string>();
	CString str_C = strVal.c_str();
	if (0 != str_C.Compare("Sign_Gloss_Video_ID"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 2)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Sign_Gloss_ID"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 3)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("View_Point"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 4)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Person"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 5)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Video_File_Name"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 6)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Video_File_Location"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 7)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Skeleton_File_Name"))
	{
		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 8)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Skeleton_File_Location"))
	{
		return FALSE;
	}
	return TRUE;
}

VOID Excel_DB::_ClearVideoDB(_typeVideoDB& itemVideo)
{
	itemVideo.index = 0;
	itemVideo.sgv_ID = 0;
	itemVideo.sgv_sgOriginNumber = 0;
	itemVideo.sgv_ViewPoint = ESGV_ViewPoint::NONE;				// 'C', 'L', 'R'
	itemVideo.sgv_Person = _T("");					// '1', '2'
	itemVideo.dummy = 0;
	itemVideo.sg_vtCP.clear();
	itemVideo.sgv_FileExt = _T("");
	itemVideo.sgv_Width = 0;
	itemVideo.sgv_Height = 0;
	itemVideo.sgv_FPS = 0;
}

VOID Excel_DB::_ClearCorpusVideoDB(_typeCorpusVideoDB& itemVideo)
{
	itemVideo.index = 0;
	itemVideo.scv_ID = 0;
	itemVideo.scv_FileName = _T("");	
	itemVideo.scv_ViewPoint = ESGV_ViewPoint::NONE;				// 'C', 'L', 'R'
	itemVideo.scv_Person = _T("");					// '1', '2'
	itemVideo.dummy = 0;
	itemVideo.scv_sgOriginNumber.clear();
	itemVideo.scv_key_frame_index.clear();
	itemVideo.scv_sgIndex.clear();
	itemVideo.scv_unified_sgIndex.clear();
	itemVideo.scv_gloss_ranges.clear();
	itemVideo.scv_cpIndex.clear();
	itemVideo.scv_FileExt = _T("");
	itemVideo.scv_Width = 0;
	itemVideo.scv_Height = 0;
	itemVideo.scv_FPS = 0;
	itemVideo.scv_handStatus.clear();
}

BOOL Excel_DB::_LoadVideoDB(OpenXLSX::XLWorksheet& wks)
{
	if (FALSE == CheckSignGlossVideoSheet(wks))
	{
		AfxMessageBox("Sign_Gloss_Video 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	_typeVideoDB itemVideo;

	m_VideoDB.clear();

	UINT32 unSGID = 0xFFFFFFFF;
	UINT32 unSGIndex = 0xFFFFFFFF;

	UINT32 nRowIndex = 0;
	std::vector<XLCellValue> values;
	for (auto& row : wks.rows()) 
	{
		values = row.values();
		uint64_t cell_idx = row.rowNumber();
		//if (17369 <= cell_idx) break;
		if (OpenXLSX::XLValueType::Empty == values.at(0).type())	// Exit loop at the end of row.
		{
			break;
		}

		if (1 < cell_idx)
		{
			_ClearVideoDB(itemVideo);

			itemVideo.index = nRowIndex;
			itemVideo.sgv_ID = values.at(0).get<UINT32>(); //ChangePoint_ID
			
			if (OpenXLSX::XLValueType::Integer == values.at(1).type())
			{
				itemVideo.sgv_sgID = values.at(1).get<UINT32>(); //Sign_Gloss_Video_ID
			}
			else if (OpenXLSX::XLValueType::String == values.at(1).type())
			{
				std::string sgv_sgID = values.at(1).get<std::string>(); //Sign_Gloss_Video_ID
				itemVideo.sgv_sgID = stoi(sgv_sgID.c_str());
			}

			if (unSGID != itemVideo.sgv_sgID)
			{
				unSGID = itemVideo.sgv_sgID;
				_SearchSGIndexBySGID(unSGIndex, unSGID);
			}
			itemVideo.sgv_sgIndex = unSGIndex;
			itemVideo.sgv_sgOriginNumber = m_SignGlossDB[unSGIndex].sg_OriginNumber;

			std::string sgv_ViewPoint = values.at(2).get<std::string>(); //Changepoint_Timestamp
			if (sgv_ViewPoint[0] == 'f')			//"front"
			{
				itemVideo.sgv_ViewPoint = ESGV_ViewPoint::CENTER;
			}
			else if (sgv_ViewPoint[0] == 'l')		//"left"
			{
				itemVideo.sgv_ViewPoint = ESGV_ViewPoint::LEFT;
			}
			else if (sgv_ViewPoint[0] == 'r')		//"right"
			{
				itemVideo.sgv_ViewPoint = ESGV_ViewPoint::RIGHT;
			}
			else
			{
				itemVideo.sgv_ViewPoint = ESGV_ViewPoint::NONE;
			}

			std::string  sgv_Person = values.at(3).get<std::string>(); //Changepoint_frame_number
			CString cstr(sgv_Person.c_str());
			itemVideo.sgv_Person = cstr;

			std::string  sgv_FileExt = values.at(8).get<std::string>();
			CString cstr2(sgv_FileExt.c_str());
			itemVideo.sgv_FileExt = cstr2;

			//itemVideo.sgv_Width = values.at(9).get<UINT32>(); 
			//itemVideo.sgv_Height = values.at(10).get<UINT32>();
			//itemVideo.sgv_FPS = values.at(11).get<UINT32>();


			m_VideoDB.push_back(itemVideo);
			nRowIndex++;
		}
	}

	return TRUE;
}


BOOL Excel_DB::_LoadCorpusVideoDB(OpenXLSX::XLWorksheet& wks)
{

	_typeCorpusVideoDB itemVideo;

	m_CorpusVideoDB.clear();

	UINT32 nRowIndex = 0;
	std::vector<XLCellValue> values;
	for (auto& row : wks.rows())
	{
		values = row.values();
		uint64_t cell_idx = row.rowNumber();
		
		if (OpenXLSX::XLValueType::Empty == values.at(0).type())	// Exit loop at the end of row.
		{
			break;
		}

		if (1 < cell_idx)
		{
			_ClearCorpusVideoDB(itemVideo);

			itemVideo.index = nRowIndex;
			itemVideo.scv_ID = values.at(0).get<UINT32>();

			std::string scv_sgID = values.at(1).get<std::string>();
			CString scv_sgID_str(scv_sgID.c_str());
			auto scv_sgID_ = m_util.SplitCString(scv_sgID_str);
			for (auto scv_sgID__ : scv_sgID_)
				itemVideo.scv_sgIndex.push_back(_ttoi(scv_sgID__)-1);

			if (itemVideo.scv_sgIndex.size() > 0)
			{
				itemVideo.scv_unified_sgIndex = m_util.removeDuplicates(itemVideo.scv_sgIndex);
			}

			std::string sgv_ViewPoint = values.at(2).get<std::string>(); //Changepoint_Timestamp
			if (sgv_ViewPoint[0] == 'f')			//"front"
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::CENTER;
			}
			else if (sgv_ViewPoint[0] == 'l')		//"left"
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::LEFT;
			}
			else if (sgv_ViewPoint[0] == 'r')		//"right"
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::RIGHT;
			}
			else
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::NONE;
			}

			std::string  scv_Person = values.at(3).get<std::string>(); //Changepoint_frame_number
			CString cstr(scv_Person.c_str());
			itemVideo.scv_Person = cstr;
		

			std::string scv_FileName = values.at(4).get<std::string>(); 				
			itemVideo.scv_FileName = convert_utf8_to_unicode_string(scv_FileName); //한글있을경우에만 사용함.	

			std::string scv_key_frame_index = values.at(5).get<std::string>();
			CString scv_key_frame_index_str(scv_key_frame_index.c_str());
			auto scv_key_frame_index_ = m_util.SplitCString(scv_key_frame_index_str);
			for (auto scv_key_frame_index__ : scv_key_frame_index_)
				itemVideo.scv_key_frame_index.push_back(_ttoi(scv_key_frame_index__));

			std::string scv_gloss_ranges = values.at(6).get<std::string>();
			CString scv_gloss_ranges_str(scv_gloss_ranges.c_str());
			auto scv_gloss_ranges_group = m_util.SplitCString(scv_gloss_ranges_str);
			for (auto scv_gloss_ranges_group_ : scv_gloss_ranges_group)
			{
				auto scv_gloss_ranges_ = m_util.SplitCString(scv_gloss_ranges_group_, _T("-"));
				for (auto scv_gloss_ranges__ : scv_gloss_ranges_)
				{
					itemVideo.scv_gloss_ranges.push_back(_ttoi(scv_gloss_ranges__));
				}
			}			

			std::string scv_Sentence = values.at(7).get<std::string>();
			itemVideo.scv_Sentence = convert_utf8_to_unicode_string(scv_Sentence); //한글있을경우에만 사용함.		

			std::string  scv_FileExt = values.at(8).get<std::string>();
			CString cstr2(scv_FileExt.c_str());
			itemVideo.scv_FileExt = cstr2;

			itemVideo.scv_Width = values.at(9).get<UINT32>(); 
			itemVideo.scv_Height = values.at(10).get<UINT32>();
			itemVideo.scv_FPS = values.at(11).get<UINT32>();

			if (values.at(12).type() == OpenXLSX::XLValueType::String)
			{
				std::string hand_status = values.at(12).get<std::string>();
				CString hand_status_str(hand_status.c_str());
				auto hand_status_index_ = m_util.SplitCString(hand_status_str);
				for (auto hand_status_index__ : hand_status_index_)
					itemVideo.scv_handStatus.push_back(_ttoi(hand_status_index__));
			}	

			m_CorpusVideoDB.push_back(itemVideo);
			nRowIndex++;
		}
	}
	return TRUE;
}


BOOL Excel_DB::_WriteVideoDB(OpenXLSX::XLWorksheet& wks, _typeVideoDB itemVideo)
{
	if (FALSE == CheckSignGlossVideoSheet(wks))
	{
		AfxMessageBox("Sign_Gloss_Video 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	auto row_count = wks.rowCount();
	std::string str;

	wks.cell(XLCellReference(row_count+1, 1)).value()= itemVideo.sgv_ID;
	wks.cell(XLCellReference(row_count + 1, 2)).value() = itemVideo.sgv_sgID;
	if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::LEFT) str = "left";
	else if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::RIGHT) str = "right";
	else if(itemVideo.sgv_ViewPoint == ESGV_ViewPoint::CENTER) str = "front";
	wks.cell(XLCellReference(row_count + 1, 3)).value() = str;
	wks.cell(XLCellReference(row_count + 1, 4)).value() = std::string( CT2CA(itemVideo.sgv_Person) );
	wks.cell(XLCellReference(row_count + 1, 9)).value() = std::string(CT2CA(itemVideo.sgv_FileExt));

	wks.cell(XLCellReference(row_count + 1, 10)).value() = itemVideo.sgv_Width;
	wks.cell(XLCellReference(row_count + 1, 11)).value() = itemVideo.sgv_Height;
	wks.cell(XLCellReference(row_count + 1, 12)).value() = itemVideo.sgv_FPS;

	return TRUE;
}

BOOL Excel_DB::_AddVideoDB(OpenXLSX::XLWorksheet& wks, _typeVideoDB itemVideo)
{
	if (FALSE == CheckSignGlossVideoSheet(wks))
	{
		AfxMessageBox("Sign_Gloss_Video 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	auto row_count = wks.rowCount();
	std::string str;

	wks.cell(XLCellReference(row_count + 1, 1)).value() = itemVideo.sgv_ID;
	wks.cell(XLCellReference(row_count + 1, 2)).value() = itemVideo.sgv_sgID;
	if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::LEFT) str = "left";
	else if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::RIGHT) str = "right";
	else if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::CENTER) str = "front";
	wks.cell(XLCellReference(row_count + 1, 3)).value() = str;
	wks.cell(XLCellReference(row_count + 1, 4)).value() = std::string(CT2CA(itemVideo.sgv_Person));
	wks.cell(XLCellReference(row_count + 1, 9)).value() = std::string(CT2CA(itemVideo.sgv_FileExt));

	CString FileName;
	if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::CENTER)
		FileName.Format(_T("%d.%s.C.mkv"), itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person);
	else if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::LEFT)
		FileName.Format(_T("%d.%s.L.mkv"), itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person);
	else if (itemVideo.sgv_ViewPoint == ESGV_ViewPoint::RIGHT)
		FileName.Format(_T("%d.%s.R.mkv"), itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person);
	if(!itemVideo.sgv_FileExt.IsEmpty()) 	
	{
		CString str;
		str.Format(_T(".%s.mkv"), itemVideo.sgv_FileExt);
		FileName.Replace(_T(".mkv"), str);
	}

	wks.cell(XLCellReference(row_count + 1, 5)).value() = std::string(CT2CA(FileName));
	CString FileLocation;
	FileLocation.Format(_T("\\\\ETRI-KSL-DB\\etri_ksl_db\\%s\\MKV"), itemVideo.sgv_Person);
	wks.cell(XLCellReference(row_count + 1, 6)).value() = std::string(CT2CA(FileLocation));

	FileName.Replace(_T("mkv"), _T("json"));
	wks.cell(XLCellReference(row_count + 1, 7)).value() = std::string(CT2CA(FileName));

	CString SkeletonFileLocation;
	SkeletonFileLocation.Format(_T("\\\\ETRI-KSL-DB\\etri_ksl_db\\%s\\SKELETON"), itemVideo.sgv_Person);
	wks.cell(XLCellReference(row_count + 1, 8)).value() = std::string(CT2CA(SkeletonFileLocation));

	wks.cell(XLCellReference(row_count + 1, 9)).value() = std::string(CT2CA(itemVideo.sgv_FileExt));
	wks.cell(XLCellReference(row_count + 1, 10)).value() = itemVideo.sgv_Width;
	wks.cell(XLCellReference(row_count + 1, 11)).value() = itemVideo.sgv_Height;
	wks.cell(XLCellReference(row_count + 1, 12)).value() = itemVideo.sgv_FPS;

	return TRUE;
}

BOOL Excel_DB::_AddCorpusVideoDB(OpenXLSX::XLWorksheet& wks, _typeCorpusVideoDB itemVideo)
{

	auto row_count = wks.rowCount();
	std::string str;

	wks.cell(XLCellReference(row_count + 1, 1)).value() = itemVideo.scv_ID;

	CString scv_sgIndex;
	for (auto index : itemVideo.scv_sgIndex)
	{
		CString add;
		add.Format(_T("%d "), index+1);
		scv_sgIndex.Append(add);
	}
	wks.cell(XLCellReference(row_count + 1, 2)).value() = std::string(CT2CA(scv_sgIndex));

	if (itemVideo.scv_ViewPoint == ESGV_ViewPoint::LEFT) str = "left";
	else if (itemVideo.scv_ViewPoint == ESGV_ViewPoint::RIGHT) str = "right";
	else if (itemVideo.scv_ViewPoint == ESGV_ViewPoint::CENTER) str = "front";
	wks.cell(XLCellReference(row_count + 1, 3)).value() = str;

	wks.cell(XLCellReference(row_count + 1, 4)).value() = std::string(CT2CA(itemVideo.scv_Person));

	wks.cell(XLCellReference(row_count + 1, 5)).value() = convert_unicode_to_utf8_string(itemVideo.scv_FileName);

	CString scv_key_frame_index;
	for (auto frame : itemVideo.scv_key_frame_index)
	{
		CString add;
		add.Format(_T("%d "), frame);
		scv_key_frame_index.Append(add);
	}
	wks.cell(XLCellReference(row_count + 1, 6)).value() = std::string(CT2CA(scv_key_frame_index));

	CString scv_gloss_ranges;
	for(int k=0;k< itemVideo.scv_gloss_ranges.size(); k +=3)
	{
		CString add;
		add.Format(_T("%d-%d-%d "), itemVideo.scv_gloss_ranges[k], itemVideo.scv_gloss_ranges[k+1], itemVideo.scv_gloss_ranges[k+2]);
		scv_gloss_ranges.Append(add);
	}
	wks.cell(XLCellReference(row_count + 1, 7)).value() = std::string(CT2CA(scv_gloss_ranges));


	wks.cell(XLCellReference(row_count + 1, 8)).value() = convert_unicode_to_utf8_string(itemVideo.scv_Sentence);

	wks.cell(XLCellReference(row_count + 1, 9)).value() = std::string(CT2CA(itemVideo.scv_FileExt));
	wks.cell(XLCellReference(row_count + 1, 10)).value() = itemVideo.scv_Width;
	wks.cell(XLCellReference(row_count + 1, 11)).value() = itemVideo.scv_Height;
	wks.cell(XLCellReference(row_count + 1, 12)).value() = itemVideo.scv_FPS;

	CString scv_handStatus;
	for (auto status : itemVideo.scv_handStatus)
	{
		CString add;
		add.Format(_T("%d "), status);
		scv_handStatus.Append(add);
	}
	wks.cell(XLCellReference(row_count + 1, 13)).value() = std::string(CT2CA(scv_handStatus));

	return TRUE;
}

BOOL Excel_DB::CheckSignVideoChangepointSheet(OpenXLSX::XLWorksheet& wks)
{
	std::string strVal;

	strVal = wks.cell(XLCellReference(1, 1)).value().get<std::string>();
	CString str_C = strVal.c_str();
	if (0 != str_C.Compare("ChangePoint_ID"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 2)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Sign_Gloss_Video_ID"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 3)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Changepoint_Timestamp"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 4)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Changepoint_frame_number"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 5)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Confidence"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 6)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("LEFT_HAND_ROI"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 7)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RIGHT_HAND_ROI"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 8)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("FPS"))
	{
	//	return FALSE;
	}	

	strVal = wks.cell(XLCellReference(1, 9)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("HAND_POSITION"))
	{
//		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 10)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("HAND_POSITION_CONFIDENCE"))
	{
//		return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 11)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RELATIVE_DISTANCE"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 12)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("LEFT_HAND_SHAPE[]"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 13)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RIGHT_HAND_SHAPE[]"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 14)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("LEFT_HAND_SHAPE_CONFIDENCE[]"))
	{
	//	return FALSE;
	}

	strVal = wks.cell(XLCellReference(1, 15)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RIGHT_HAND_SHAPE_CONFIDENCE[]"))
	{
	//	return FALSE;
	}

	return TRUE;
}

VOID Excel_DB::_ClearChangepointDB(_typeChangepointDB& itemCP)
{
	itemCP.index = 0;
	itemCP.sgcp_ID = 1;
	itemCP.sgcp_sgvID = 1;
	itemCP.sgcp_TimeStamp = 0;
	itemCP.sgcp_FrameIndex = 0;
	itemCP.sgcp_Class = 0;
	itemCP.sgcp_rcLeftHand = { 0, 0, 0, 0 };
	itemCP.sgcp_rcRightHand = { 0, 0, 0, 0 };
	itemCP.sgcp_scvID = 0;
	itemCP.sgcp_FrameIndex = 0;
	itemCP.sgcp_vtHandLocation.clear();
	itemCP.sgcp_vtHandLocationConfidence.clear();
	itemCP.sgcp_vtRelativeDistance.clear();
	//itemCP.sgcp_vtLeftHandShape.clear();
	//itemCP.sgcp_vtLeftHandShapeConfidence.clear();
	//itemCP.sgcp_vtRightHandShape.clear();
	//itemCP.sgcp_vtRightHandShapeConfidence.clear();
	itemCP.sgcp_vtLeftHandOrientation.clear();
	itemCP.sgcp_vtRightHandOrientation.clear();

	itemCP.sgcp_vtDominantPCACompoent.clear();
	itemCP.sgcp_vtDominantPCACompoentConfidence = 0;
	itemCP.sgcp_vtNondominantPCACompoent.clear();
	itemCP.sgcp_vtNondominantPCACompoentConfidence = 0;
	itemCP.sgcp_vtSkeleton.clear();
	itemCP.sgcp_vtLHSkeleton.clear();
	itemCP.sgcp_vtRHSkeleton.clear();
	itemCP.sgcp_vtHandStatus=0;
	itemCP.sgcp_vtSkeleton_confidence.clear();
	itemCP.sgcp_Confidence = 0;	
	itemCP.sgcp_kIndex = 0;
//	itemCP.sgcp_vtHandLocation_ref_model.clear();
	itemCP.sgcp_vtSkeletonMP.clear();
	itemCP.sgcp_vtLHandSkeletonMP.clear();
	itemCP.sgcp_vtRHandSkeletonMP.clear();
}


BOOL Excel_DB::IsDBList(int query)
{
	if (m_DB_origin_numbers.size() == 0) return 1;

	for (int k = 0; k < m_DB_origin_numbers.size(); k++)
	{
		if (m_DB_origin_numbers[k] == query) return 1;
	}
	return 0;
}

BOOL Excel_DB::_LoadChangePointDB(OpenXLSX::XLWorksheet& wks)
{	

	if (FALSE == CheckSignVideoChangepointSheet(wks))
	{
		AfxMessageBox("Sign_Video_Changepoint 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	_typeChangepointDB itemCP;

	m_CpDB.clear();

	UINT32 unRowIndex = 0;
	std::vector<XLCellValue> values;
	for (auto& row : wks.rows())
	{
		_ClearChangepointDB(itemCP);

		values = row.values();
		uint64_t cell_idx = row.rowNumber();
		if (OpenXLSX::XLValueType::Empty == values.at(0).type())	// Exit loop at the end of row.
		{
			break;
		}

		if (1 < cell_idx)
		{
			UINT32 SGV_ID = values.at(1).get<UINT32>();

			itemCP.index = unRowIndex++;

			itemCP.sgcp_ID = values.at(0).get<UINT32>(); //ChangePoint_ID
			itemCP.sgcp_sgvID = values.at(1).get<UINT32>(); //Sign_Gloss_Video_ID

			itemCP.sgcp_scvID = values.at(7).get<UINT32>();
			itemCP.sgcp_scvFrameID = values.at(8).get<UINT32>();


			if (itemCP.sgcp_sgvID != 0)
			{
				m_VideoDB[itemCP.sgcp_sgvID - 1].sg_vtCP.push_back(itemCP.index);
				itemCP.sgcp_sgIndex = m_VideoDB[itemCP.sgcp_sgvID - 1].sgv_sgIndex;
			}
			else if (itemCP.sgcp_scvID != 0 && itemCP.sgcp_scvFrameID != 0)
			{
				m_CorpusVideoDB[itemCP.sgcp_scvID - 1].scv_cpIndex.push_back(itemCP.index);
				itemCP.sgcp_sgIndex = m_CorpusVideoDB[itemCP.sgcp_scvID - 1].scv_sgIndex[itemCP.sgcp_scvFrameID - 1];						
			}
			else itemCP.sgcp_sgIndex = UINT32_MAX;

			itemCP.sgcp_TimeStamp = values.at(2).get<UINT64>(); //Changepoint_Timestamp
			itemCP.sgcp_FrameIndex = values.at(3).get<UINT32>(); //Changepoint_frame_number
			itemCP.sgcp_Class = values.at(4).get<INT32>(); //Confidence

			if (1 == itemCP.sgcp_Class)
			{
				std::string sgcp_LeftHandROI = values.at(5).get<std::string>(); //LEFT_HAND_ROI
				if (0 < sgcp_LeftHandROI.length())
				{
					std::string token;
					std::stringstream ss(sgcp_LeftHandROI);

					ss >> token;
					itemCP.sgcp_rcLeftHand.left = stoi(token);
					ss >> token;
					itemCP.sgcp_rcLeftHand.top = stoi(token);
					ss >> token;
					itemCP.sgcp_rcLeftHand.right = stoi(token);
					ss >> token;
					itemCP.sgcp_rcLeftHand.bottom = stoi(token);
				}

				std::string sgcp_RightHandROI = values.at(6).get<std::string>(); //RIGHT_HAND_ROI
				if (0 < sgcp_RightHandROI.length())
				{
					std::string token;
					std::stringstream ss(sgcp_RightHandROI);

					ss >> token;
					itemCP.sgcp_rcRightHand.left = stoi(token);
					ss >> token;
					itemCP.sgcp_rcRightHand.top = stoi(token);
					ss >> token;
					itemCP.sgcp_rcRightHand.right = stoi(token);
					ss >> token;
					itemCP.sgcp_rcRightHand.bottom = stoi(token);
				}
			}			




			UINT unColumnCount = (UINT)values.size();

			if (1 == itemCP.sgcp_Class)
			{
		
				if (10 <= unColumnCount)
				{
	
					if (OpenXLSX::XLValueType::String == values.at(9).type())
					{
						std::string sgcp_vtLHandSkeletonMP = values.at(9).get<std::string>(); //LEFT_HAND_SHAPE_CONFIDENCE[]
						if (0 < sgcp_vtLHandSkeletonMP.length())
						{							
							std::istringstream ss(sgcp_vtLHandSkeletonMP);
							for (int i = 0; i < 21; i++)
							{
								cv::Point3f pt;
								ss >> pt.x;								
								ss >> pt.y;								
								ss >> pt.z;
								itemCP.sgcp_vtLHandSkeletonMP.push_back(pt);
							}
						}
					}
				}

				if (11 <= unColumnCount)
				{

					if (OpenXLSX::XLValueType::String == values.at(10).type())
					{
						std::string sgcp_vtRHandSkeletonMP = values.at(10).get<std::string>(); //LEFT_HAND_SHAPE_CONFIDENCE[]
						if (0 < sgcp_vtRHandSkeletonMP.length())
						{
							std::istringstream ss(sgcp_vtRHandSkeletonMP);
							for (int i = 0; i < 21; i++)
							{
								cv::Point3f pt;
								ss >> pt.x;
								ss >> pt.y;
								ss >> pt.z;
								itemCP.sgcp_vtRHandSkeletonMP.push_back(pt);
							}
						}
					}
				}

				
				if (16 <= unColumnCount)
				{
					itemCP.sgcp_Confidence = values.at(15).get<UINT32>(); 
					
				}

				if (17 <= unColumnCount)
				{
					std::string sgcp_vtDominantPCACompoent = values.at(16).get<std::string>();
					if (0 < sgcp_vtDominantPCACompoent.length())
					{
						std::string token;
						std::stringstream ss(sgcp_vtDominantPCACompoent);

						for (int i = 0; i < 45; i++)
						{
							ss >> token;
							if (token.length() <= 0) break;
							itemCP.sgcp_vtDominantPCACompoent.push_back(stof(token));
						}

					}
				}

				if (18 <= unColumnCount)
				{
					if (OpenXLSX::XLValueType::Float == values.at(17).type())
					{
						itemCP.sgcp_vtDominantPCACompoentConfidence = values.at(17).get<FLOAT>();
					}
				}


				if (19 <= unColumnCount)
				{
					std::string sgcp_vtNondominantPCACompoent = values.at(18).get<std::string>();
					if (0 < sgcp_vtNondominantPCACompoent.length())
					{
						std::string token;
						std::stringstream ss(sgcp_vtNondominantPCACompoent);

						for (int i = 0; i < 45; i++)
						{
							ss >> token;
							if (token.length() <= 0) break;
							itemCP.sgcp_vtNondominantPCACompoent.push_back(stof(token));
						}
					}
				}

				if (20 <= unColumnCount)
				{
					if (OpenXLSX::XLValueType::Float == values.at(19).type())
					{
						itemCP.sgcp_vtNondominantPCACompoentConfidence = values.at(19).get<FLOAT>();
					}
				}


				if (21 <= unColumnCount)
				{
					std::string sgcp_RightHandOrientaion = values.at(20).get<std::string>(); //R orientation
					if (0 < sgcp_RightHandOrientaion.length())
					{
						std::string token;
						std::stringstream ss(sgcp_RightHandOrientaion);

						for (int i = 0; i < 9; i++)
						{
							ss >> token;
							if (token.length() <= 0) break;
							itemCP.sgcp_vtRightHandOrientation.push_back(stof(token));							
						}				
					}
				}

				if (22 <= unColumnCount)
				{
					std::string sgcp_LeftHandOrientaion = values.at(21).get<std::string>(); //L orientation
					if (0 < sgcp_LeftHandOrientaion.length())
					{
						std::string token;
						std::stringstream ss(sgcp_LeftHandOrientaion);

						for (int i = 0; i < 9; i++)
						{
							ss >> token;
							if (token.length() <= 0) break;
							itemCP.sgcp_vtLeftHandOrientation.push_back(stof(token));
						}
					}
				}

				if (25 <= unColumnCount)
				{
					std::string sgcp_skeleton = values.at(24).get<std::string>(); //Skeleton
					CString sk_str(sgcp_skeleton.c_str());
					auto split_data = m_util.SplitCString(sk_str);
					for (int mm = 0; mm < split_data.size(); mm += 3)
					{
						cv::Point3f pt;
						pt.x = _ttof(split_data[mm]);
						pt.y = _ttof(split_data[mm + 1]);
						pt.z = _ttof(split_data[mm + 2]);
						itemCP.sgcp_vtSkeleton.push_back(pt);
					}
				}

				if (26 <= unColumnCount)
				{
					std::string sgcp_skeleton = values.at(25).get<std::string>(); //RHSkeleton
					CString sk_str(sgcp_skeleton.c_str());
					auto split_data = m_util.SplitCString(sk_str);
					for (int mm = 0; mm < split_data.size(); mm += 3)
					{
						cv::Point3f pt;
						pt.x = _ttof(split_data[mm]);
						pt.y = _ttof(split_data[mm + 1]);
						pt.z = _ttof(split_data[mm + 2]);
						itemCP.sgcp_vtRHSkeleton.push_back(pt);
					}

				}
				if (27 <= unColumnCount)
				{
					std::string sgcp_skeleton = values.at(26).get<std::string>(); //LHSkeleton
					CString sk_str(sgcp_skeleton.c_str());
					auto split_data = m_util.SplitCString(sk_str);
					for (int mm = 0; mm < split_data.size(); mm += 3)
					{
						cv::Point3f pt;
						pt.x = _ttof(split_data[mm]);
						pt.y = _ttof(split_data[mm + 1]);
						pt.z = _ttof(split_data[mm + 2]);
						itemCP.sgcp_vtLHSkeleton.push_back(pt);
					}
				}
				if (28 <= unColumnCount)
				{		
					itemCP.sgcp_vtHandStatus = values.at(27).get<int>();//HandStatus					
				}
				if (29 <= unColumnCount)
				{
					std::string sgcp_vtSkeleton_confidence = values.at(28).get<std::string>(); //Skeleton
					CString sk_str(sgcp_vtSkeleton_confidence.c_str());
					auto split_data = m_util.SplitCString(sk_str);
					for (int mm = 0; mm < split_data.size(); mm += 1)
					{
						float pt;
						pt = _ttof(split_data[mm]);				
						itemCP.sgcp_vtSkeleton_confidence.push_back(pt);
					}
				}
				if (30 <= unColumnCount)
				{
					itemCP.sgcp_kIndex = values.at(29).get<UINT32>(); //kIndex
				}
				if (31 <= unColumnCount)
				{
					std::string sgcp_skeleton = values.at(30).get<std::string>(); //SkeletonMP
					CString sk_str(sgcp_skeleton.c_str());
					auto split_data = m_util.SplitCString(sk_str);
					for (int mm = 0; mm < split_data.size(); mm += 3)
					{
						cv::Point3f pt;
						pt.x = _ttof(split_data[mm]);
						pt.y = _ttof(split_data[mm + 1]);
						pt.z = _ttof(split_data[mm + 2]);
						itemCP.sgcp_vtSkeletonMP.push_back(pt);
					}	
				}
				
			}
			if (itemCP.sgcp_vtSkeleton.size() > 0 && itemCP.sgcp_vtRHSkeleton.size() > 0 && itemCP.sgcp_vtLHSkeleton.size() > 0)
			{
				_ResizeACRHand(itemCP.sgcp_vtSkeleton, itemCP.sgcp_vtRHSkeleton, itemCP.sgcp_vtLHSkeleton);
			}

	//		if (m_SignGlossDB[itemCP.sgcp_sgIndex].sg_Combination.IsEmpty())
	//		if(itemCP.sgcp_Class==1)
			{
			//	int orign_number = m_SignGlossDB[itemCP.sgcp_sgIndex].sg_OriginNumber;
			//	printf("gloss=%d\n", orign_number);
			//	if (IsDBList(orign_number))
				{
					static int modified_index = 0;					
					itemCP.index = modified_index;
					m_CpDB.push_back(itemCP);
					modified_index++;
				}
			}
		//	else unRowIndex--;
		}
	}
	return TRUE;
}

BOOL Excel_DB::_WriteChangePointDB(OpenXLSX::XLWorksheet& wks, _typeChangepointDB itemCP)
{
	if (FALSE == CheckSignVideoChangepointSheet(wks))
	{
		AfxMessageBox("Sign_Video_Changepoint 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	auto row_count = wks.rowCount();
	
	CString cstring;

	wks.cell(XLCellReference(row_count + 1, 1)).value() = itemCP.sgcp_ID;
	wks.cell(XLCellReference(row_count + 1, 2)).value() = itemCP.sgcp_sgvID;
	wks.cell(XLCellReference(row_count + 1, 3)).value() = itemCP.sgcp_TimeStamp;
	wks.cell(XLCellReference(row_count + 1, 4)).value() = itemCP.sgcp_FrameIndex; 
	wks.cell(XLCellReference(row_count + 1, 5)).value() = itemCP.sgcp_Class;
	cstring.Format(_T("%d %d %d %d"), itemCP.sgcp_rcLeftHand.left, itemCP.sgcp_rcLeftHand.top,
		itemCP.sgcp_rcLeftHand.right, itemCP.sgcp_rcLeftHand.bottom);
	wks.cell(XLCellReference(row_count + 1, 6)).value() = std::string(CT2CA(cstring));
	cstring.Format(_T("%d %d %d %d"), itemCP.sgcp_rcRightHand.left, itemCP.sgcp_rcRightHand.top,
		itemCP.sgcp_rcRightHand.right, itemCP.sgcp_rcRightHand.bottom);
	wks.cell(XLCellReference(row_count + 1, 7)).value() = std::string(CT2CA(cstring));

	wks.cell(XLCellReference(row_count + 1, 8)).value() = itemCP.sgcp_scvID;
	wks.cell(XLCellReference(row_count + 1, 9)).value() = itemCP.sgcp_scvFrameID;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtLHandSkeletonMP);
	wks.cell(XLCellReference(row_count + 1, 10)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtRHandSkeletonMP);
	wks.cell(XLCellReference(row_count + 1, 11)).value() = std::string(CT2CA(cstring));

	wks.cell(XLCellReference(row_count + 1, 16)).value() = itemCP.sgcp_Confidence;
	cstring = m_util.Vector2CString(itemCP.sgcp_vtDominantPCACompoent);
	wks.cell(XLCellReference(row_count + 1, 17)).value() = std::string(CT2CA(cstring));
	wks.cell(XLCellReference(row_count + 1, 18)).value() = itemCP.sgcp_vtDominantPCACompoentConfidence;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtNondominantPCACompoent);
	wks.cell(XLCellReference(row_count + 1, 19)).value() = std::string(CT2CA(cstring));
	wks.cell(XLCellReference(row_count + 1, 20)).value() = itemCP.sgcp_vtNondominantPCACompoentConfidence;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtRightHandOrientation);
	wks.cell(XLCellReference(row_count + 1, 21)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtLeftHandOrientation);
	wks.cell(XLCellReference(row_count + 1, 22)).value() = std::string(CT2CA(cstring));

	//cstring.Format(_T("%d %d"), (itemCP.sgcp_rcLeftHand.left + itemCP.sgcp_rcLeftHand.right) / 2,
	//	(itemCP.sgcp_rcLeftHand.top + itemCP.sgcp_rcLeftHand.bottom) / 2);
	//wks.cell(XLCellReference(row_count + 1, 23)).value() = std::string(CT2CA(cstring));

	//cstring.Format(_T("%d %d"), (itemCP.sgcp_rcRightHand.left + itemCP.sgcp_rcRightHand.right) / 2,
	//	(itemCP.sgcp_rcRightHand.top + itemCP.sgcp_rcRightHand.bottom) / 2);
	//wks.cell(XLCellReference(row_count + 1, 24)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtSkeleton);
	wks.cell(XLCellReference(row_count + 1, 25)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtRHSkeleton);
	wks.cell(XLCellReference(row_count + 1, 26)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtLHSkeleton);
	wks.cell(XLCellReference(row_count + 1, 27)).value() = std::string(CT2CA(cstring));

	wks.cell(XLCellReference(row_count + 1, 28)).value() = itemCP.sgcp_vtHandStatus;
	
	cstring = m_util.Vector2CString(itemCP.sgcp_vtSkeleton_confidence);
	wks.cell(XLCellReference(row_count + 1, 29)).value() = std::string(CT2CA(cstring));
	
	wks.cell(XLCellReference(row_count + 1, 30)).value() = itemCP.sgcp_kIndex;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtSkeletonMP);
	wks.cell(XLCellReference(row_count + 1, 31)).value() = std::string(CT2CA(cstring));

	return TRUE;
}


BOOL Excel_DB::_AddChangePointDB(OpenXLSX::XLWorksheet& wks, _typeChangepointDB itemCP)
{
	if (FALSE == CheckSignVideoChangepointSheet(wks))
	{
		AfxMessageBox("Sign_Video_Changepoint 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	auto row_count = wks.rowCount();

	CString cstring;

	wks.cell(XLCellReference(row_count + 1, 1)).value() = itemCP.sgcp_ID;
	wks.cell(XLCellReference(row_count + 1, 2)).value() = itemCP.sgcp_sgvID;
	wks.cell(XLCellReference(row_count + 1, 3)).value() = itemCP.sgcp_TimeStamp;
	wks.cell(XLCellReference(row_count + 1, 4)).value() = itemCP.sgcp_FrameIndex;
	wks.cell(XLCellReference(row_count + 1, 5)).value() = itemCP.sgcp_Class;
	cstring.Format(_T("%d %d %d %d"), itemCP.sgcp_rcLeftHand.left, itemCP.sgcp_rcLeftHand.top,
		itemCP.sgcp_rcLeftHand.right, itemCP.sgcp_rcLeftHand.bottom);
	wks.cell(XLCellReference(row_count + 1, 6)).value() = std::string(CT2CA(cstring));
	cstring.Format(_T("%d %d %d %d"), itemCP.sgcp_rcRightHand.left, itemCP.sgcp_rcRightHand.top,
		itemCP.sgcp_rcRightHand.right, itemCP.sgcp_rcRightHand.bottom);
	wks.cell(XLCellReference(row_count + 1, 7)).value() = std::string(CT2CA(cstring));
	wks.cell(XLCellReference(row_count + 1, 8)).value() = itemCP.sgcp_scvID;
	wks.cell(XLCellReference(row_count + 1, 9)).value() = itemCP.sgcp_scvFrameID;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtLHandSkeletonMP);
	wks.cell(XLCellReference(row_count + 1, 10)).value() = std::string(CT2CA(cstring));
	cstring = m_util.Vector2CString(itemCP.sgcp_vtRHandSkeletonMP);
	wks.cell(XLCellReference(row_count + 1, 11)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtSkeletonMP);
	wks.cell(XLCellReference(row_count + 1, 31)).value() = std::string(CT2CA(cstring));

	wks.cell(XLCellReference(row_count + 1, 16)).value() = itemCP.sgcp_Confidence;
	cstring = m_util.Vector2CString(itemCP.sgcp_vtDominantPCACompoent);
	wks.cell(XLCellReference(row_count + 1, 17)).value() = std::string(CT2CA(cstring));
	wks.cell(XLCellReference(row_count + 1, 18)).value() = itemCP.sgcp_vtDominantPCACompoentConfidence;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtNondominantPCACompoent);
	wks.cell(XLCellReference(row_count + 1, 19)).value() = std::string(CT2CA(cstring));
	wks.cell(XLCellReference(row_count + 1, 20)).value() = itemCP.sgcp_vtNondominantPCACompoentConfidence;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtRightHandOrientation);
	wks.cell(XLCellReference(row_count + 1, 21)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtLeftHandOrientation);
	wks.cell(XLCellReference(row_count + 1, 22)).value() = std::string(CT2CA(cstring));

	cstring.Format(_T("%d %d"), (itemCP.sgcp_rcLeftHand.left + itemCP.sgcp_rcLeftHand.right) / 2,
		(itemCP.sgcp_rcLeftHand.top + itemCP.sgcp_rcLeftHand.bottom) / 2);
	wks.cell(XLCellReference(row_count + 1, 23)).value() = std::string(CT2CA(cstring));

	cstring.Format(_T("%d %d"), (itemCP.sgcp_rcRightHand.left + itemCP.sgcp_rcRightHand.right) / 2,
		(itemCP.sgcp_rcRightHand.top + itemCP.sgcp_rcRightHand.bottom) / 2);
	wks.cell(XLCellReference(row_count + 1, 24)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtSkeleton);
	wks.cell(XLCellReference(row_count + 1, 25)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtRHSkeleton);
	wks.cell(XLCellReference(row_count + 1, 26)).value() = std::string(CT2CA(cstring));

	cstring = m_util.Vector2CString(itemCP.sgcp_vtLHSkeleton);
	wks.cell(XLCellReference(row_count + 1, 27)).value() = std::string(CT2CA(cstring));

	wks.cell(XLCellReference(row_count + 1, 28)).value() = itemCP.sgcp_vtHandStatus;

	wks.cell(XLCellReference(row_count + 1, 30)).value() = itemCP.sgcp_kIndex;

	cstring = m_util.Vector2CString(itemCP.sgcp_vtSkeletonMP);
	wks.cell(XLCellReference(row_count + 1, 31)).value() = std::string(CT2CA(cstring));

	return TRUE;
}

BOOL Excel_DB::_UpdateChangePointDB(OpenXLSX::XLWorksheet& wks, int col_num)
{
	if (FALSE == CheckSignVideoChangepointSheet(wks))
	{
		AfxMessageBox("Sign_Video_Changepoint 시트를 찾을 수 없습니다.", MB_OK);
		return FALSE;
	}

	for (int k=0;k< m_CpDB.size();k++)
	{
		auto video = m_VideoDB[m_CpDB[k].sgcp_sgvID - 1];

		if(col_num==28)
			wks.cell(XLCellReference(k + 2, col_num)).value() = m_CpDB[k].sgcp_vtHandStatus;
		else if (col_num == 29)
		{
			auto cstring = m_util.Vector2CString(m_CpDB[k].sgcp_vtSkeleton_confidence);
			wks.cell(XLCellReference(k + 2, col_num)).value() = std::string(CT2CA(cstring));
		}
		else if (col_num == 5)
			wks.cell(XLCellReference(k + 2, col_num)).value() = m_CpDB[k].sgcp_Class;
		else if (col_num == 30)
			wks.cell(XLCellReference(k + 2, col_num)).value() = m_CpDB[k].sgcp_kIndex;
		else if (col_num == 31)
		{
			auto cstring = m_util.Vector2CString(m_CpDB[k].sgcp_vtSkeletonMP);
			wks.cell(XLCellReference(k + 2, col_num)).value() = std::string(CT2CA(cstring));
		}
	}


	return TRUE;
}


void Excel_DB::_ResizeACRHand(std::vector<Point3f> AZSkeleton, std::vector<Point3f>& RHSkeleton, std::vector<Point3f>& LHSkeleton )
{	
	double scale = 0.85;
	double sample_hand_lengthL = m_util.Distance(AZSkeleton[6], AZSkeleton[7])*scale;
	double sample_hand_lengthR = m_util.Distance(AZSkeleton[13], AZSkeleton[14])* scale;
	
	if (RHSkeleton.size() > 0)
	{
		double hand_len = m_util.Distance(RHSkeleton[0], RHSkeleton[9]) + m_util.Distance(RHSkeleton[9], RHSkeleton[10])
			+ m_util.Distance(RHSkeleton[10], RHSkeleton[11]) + m_util.Distance(RHSkeleton[11], RHSkeleton[12]);
		for (int k = 0; k < RHSkeleton.size(); k++) RHSkeleton[k] = sample_hand_lengthR /hand_len * RHSkeleton[k];
		auto ref_ptR = RHSkeleton[0];
		for (int k = 0; k < RHSkeleton.size(); k++)
			RHSkeleton[k] = RHSkeleton[k] - ref_ptR;

	}

	if (LHSkeleton.size() > 0)
	{	
		double hand_len = m_util.Distance(LHSkeleton[0], LHSkeleton[9]) + m_util.Distance(LHSkeleton[9], LHSkeleton[10])
			+ m_util.Distance(LHSkeleton[10], LHSkeleton[11]) + m_util.Distance(LHSkeleton[11], LHSkeleton[12]);
		for (int k = 0; k < LHSkeleton.size(); k++) LHSkeleton[k] = sample_hand_lengthL / hand_len * LHSkeleton[k];
		auto ref_ptL = LHSkeleton[0];
		for (int k = 0; k < LHSkeleton.size(); k++)
			LHSkeleton[k] = LHSkeleton[k] - ref_ptL;
	}
}

// CString(유니코드/멀티바이트 어떤 빌드든 OK) -> UTF-8 std::string
std::string Excel_DB::convert_unicode_to_utf8_string(const CString& s)
{
#ifdef _UNICODE
	// 유니코드 빌드: CString 내부가 UTF-16
	const wchar_t* u16 = static_cast<LPCWSTR>(s);
	int u16_len = static_cast<int>(wcslen(u16));

	// 필요한 바이트 수 계산
	int need = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		u16, u16_len,
		nullptr, 0, nullptr, nullptr);
	if (need <= 0) return {};

	std::string out(need, '\0');
	int written = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		u16, u16_len,
		out.data(), need,
		nullptr, nullptr);
	if (written <= 0) return {};
	return out;
#else
	// 멀티바이트(ANSI) 빌드: CString 내부가 ACP(시스템 코드페이지) 바이트
	const char* mb = static_cast<LPCSTR>(s);

	// 1) 먼저 ACP -> UTF-16
	int needW = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
		mb, -1, nullptr, 0);
	if (needW <= 0) return {};
	std::wstring u16(needW - 1, L'\0'); // -1: 널 제외
	if (!::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
		mb, -1, u16.data(), needW)) return {};

	// 2) UTF-16 -> UTF-8
	int need8 = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		u16.c_str(), static_cast<int>(u16.size()),
		nullptr, 0, nullptr, nullptr);
	if (need8 <= 0) return {};
	std::string out(need8, '\0');
	if (!::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		u16.c_str(), static_cast<int>(u16.size()),
		out.data(), need8, nullptr, nullptr)) return {};
	return out;
#endif
}

CString Excel_DB::convert_utf8_to_unicode_string(std::string utf3_origin)
{
	const char* utf8 = utf3_origin.c_str();
	size_t utf8_size = utf3_origin.size();

	std::wstring unicode;
	DWORD error = 0;

	do {

		if ((nullptr == utf8) || (0 == utf8_size)) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}

		unicode.clear();

		//
		// getting required cch.
		//

		int required_cch = ::MultiByteToWideChar(
			CP_UTF8,
			MB_ERR_INVALID_CHARS,
			utf8, static_cast<int>(utf8_size),
			nullptr, 0
		);
		if (0 == required_cch) {
			error = ::GetLastError();
			break;
		}

		//
		// allocate.
		//

		unicode.resize(required_cch);

		//
		// convert.
		//

		if (0 == ::MultiByteToWideChar(
			CP_UTF8,
			MB_ERR_INVALID_CHARS,
			utf8, static_cast<int>(utf8_size),
			const_cast<wchar_t*>(unicode.c_str()), static_cast<int>(unicode.size())
		)) {
			error = ::GetLastError();
			break;
		}

	} while (false);

	return (CString)unicode.c_str();
}

std::vector< _typeChangepointDB > Excel_DB::GetCPFromOriginNumber(int origin_number)
{
	std::vector< _typeChangepointDB > cps;

	for (auto cp : m_CpDB)
	{
		int db_origin_number = m_SignGlossDB[cp.sgcp_sgIndex].sg_OriginNumber;
		if (db_origin_number == origin_number && (cp.sgcp_vtHandStatus == 1 || cp.sgcp_vtHandStatus == 3))
			cps.push_back(cp);
	}

	return cps;

}

std::vector< _typeVideoDB > Excel_DB::GetCenterVideo(UINT32 origin_number, CString person)
{
	std::vector< _typeVideoDB > Videos;

	for (auto video : m_VideoDB)
	{
		int db_origin_number = m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber;
		if (db_origin_number == origin_number && video.sgv_Person.Compare(person)==0 && video.sgv_ViewPoint== ESGV_ViewPoint::CENTER)
			Videos.push_back(video);
	}
	return Videos;
}



void Excel_DB::AddNewVideo(CString video_nm, int new_video_id, std::vector < _typeVideoDB >& video_db)
{
	if (!video_nm.IsEmpty())
	{
		auto data = m_util.SplitCString(video_nm, _T("."));

		_typeVideoDB video;
		video.sgv_sgOriginNumber = _ttoi(data[0]);
		int sg_index = GetSignGlossIndexFromOriginNumber(video.sgv_sgOriginNumber);
		video.sgv_sgID = GetSignGlossID(sg_index);
		video.sgv_Person = data[1];
		if (data[2].Compare(_T("C")) == 0) video.sgv_ViewPoint = ESGV_ViewPoint::CENTER;
		else if (data[2].Compare(_T("L")) == 0) video.sgv_ViewPoint = ESGV_ViewPoint::LEFT;
		else if (data[2].Compare(_T("R")) == 0) video.sgv_ViewPoint = ESGV_ViewPoint::RIGHT;
		if (data.size() >= 5) video.sgv_FileExt = data[3];
		else video.sgv_FileExt.Empty();
		video.sgv_ID = new_video_id++;		
		video_db.push_back(video);
	}
}

void Excel_DB::AddNewCorpusVideo(CString video_nm, int new_video_id, std::vector<UINT32> key_frames, std::vector<UINT32> key_frame_origin_numbers,
	CString sentence, std::vector<UINT32> hand_status, std::vector<UINT32> gloss_ranges, std::vector < _typeCorpusVideoDB >& video_db)
{
	if (!video_nm.IsEmpty())
	{
//		auto data = m_util.SplitCString(video_nm, _T(".")); // "1-10-120.001.C.000.mkv"

//		if (data.size() == 5)
		{
			_typeCorpusVideoDB video;

			video.scv_ID = new_video_id;
			video.scv_FileName = m_util.GetFileName(video_nm);
			video.scv_Sentence = sentence;

			if (video_nm.Find(_T(".mkv")) >= 0)
			{
				auto strs = m_util.SplitCString(video_nm, _T("."));
				video.scv_Person = strs[strs.size()-4];
				video.scv_FileExt = strs[strs.size() - 2];
			}
			else
			{
				video.scv_Person = _T("1000");
				video.scv_FileExt = _T("000");
			}	

			video.scv_ViewPoint = ESGV_ViewPoint::CENTER;


			auto [w, h, fps, count] = m_util.GetVideoInfo((LPSTR)(LPCSTR)video_nm);
			video.scv_Width = w;
			video.scv_Height = h;
			video.scv_FPS = fps;
			

			video.scv_sgOriginNumber.clear();
			video.scv_key_frame_index.clear();
			video.scv_cpIndex.clear();
			video.scv_sgIndex.clear();
			video.scv_unified_sgIndex.clear();
			video.scv_gloss_ranges.clear();
			video.scv_handStatus.clear();

			for (auto key_frame : key_frames) video.scv_key_frame_index.push_back(key_frame);

			for (auto origin_number : key_frame_origin_numbers)
			{
				int sg_index = GetSignGlossIndexFromOriginNumber(origin_number);
				video.scv_sgIndex.push_back(sg_index);
				video.scv_sgOriginNumber.push_back(origin_number);
			}

			for (auto status : hand_status) video.scv_handStatus.push_back(status);	

			for (auto range : gloss_ranges)
				video.scv_gloss_ranges.push_back(range);

			video_db.push_back(video);
		}		
	}
}




void Excel_DB::recreateSheetWithHeader(OpenXLSX::XLDocument& doc, const std::string& sheetName) {
	// 1. 기존 시트 열기
	auto& oldSheet = doc.workbook().worksheet(sheetName);

	// 2. 첫 번째 행(헤더) 복사
	std::vector<std::string> headerRow;
	uint64_t maxCol = oldSheet.columnCount();

	for (uint64_t col = 1; col <= maxCol; ++col) {
		auto cell = oldSheet.cell(XLCellReference(1, col));
		if (cell.value().type() != XLValueType::Empty) {
			headerRow.push_back(cell.value().get<std::string>());
		}
		else {
			headerRow.push_back("");
		}
	}

	// 3. 기존 시트 삭제
	doc.workbook().deleteSheet(sheetName);

	// 4. 새 시트 추가
	doc.workbook().addWorksheet(sheetName);

	auto& newSheet = doc.workbook().worksheet(sheetName);

	// 5. 복사한 헤더 붙여넣기
	for (uint64_t col = 1; col <= headerRow.size(); ++col) {
		newSheet.cell(XLCellReference(1, col)).value() = headerRow[col - 1];
	}
}