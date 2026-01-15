#include "pch.h"
#include "ETRI_KSL_Excel_DB-v1.4.h"
//#include <OpenXLSX.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;
using namespace OpenXLSX;

Excel_DB::Excel_DB()
{

	//FILE* fp;
	//if (fopen_s(&fp, "origin_numbers.txt", "r") != 0)
	//{
	//	AfxMessageBox(_T("origin_numbers.txt 파일이 없습니다."));
	//	return;
	//}
	//int origin_number;

	//while (1)
	//{		
	//	if (fscanf(fp, "%d", &origin_number) == EOF) break;
	//	m_DB_origin_numbers.push_back(origin_number);
	//}
	//fclose(fp);
}

void Excel_DB::_SetMessageID(HWND hWnd, UINT uMSG) 
{ 
	m_hWndParent = hWnd;
	m_uMsgID = uMSG;
}

BOOL Excel_DB::_SearchAllVideosBySGID(std::vector<UINT>& vtVideo, UINT32 sgID)
{
//	int nFoundCount = 3;

	vtVideo.clear();	

	for (auto itemVideo : m_VideoDB)
	{		
		if (itemVideo.sgv_sgID == sgID)
		{
			vtVideo.push_back(itemVideo.index);	// Video Index를 저장함.
		//	nFoundCount--;
		//	if (0 == nFoundCount) break;
		}
	}

//	return (nFoundCount < 3);

	return (vtVideo.size());
}

BOOL Excel_DB::_SearchAllVideosBySGID(std::vector<UINT>& vtVideo, UINT32 sgID, CString person_id)
{
	//	int nFoundCount = 3;

	vtVideo.clear();

	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgID == sgID && itemVideo.sgv_Person == person_id)
		{
			vtVideo.push_back(itemVideo.index);	// Video Index를 저장함.
		//	nFoundCount--;
		//	if (0 == nFoundCount) break;
		}
	}

	//	return (nFoundCount < 3);

	return (vtVideo.size());
}


BOOL Excel_DB::_SearchAllVideosBySGID(std::vector<_typeVideoDB*>& vtVideoPtr, UINT32 sgID)
{
//	int nFoundCount = 3;

	vtVideoPtr.clear();

	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgID == sgID)
		{
			vtVideoPtr.push_back(&itemVideo);	// Video Index를 저장함.
	//		nFoundCount--;
	//		if (0 == nFoundCount) break;
		}
	}

//	return (nFoundCount < 3);
	return vtVideoPtr.size();
}


BOOL Excel_DB::_SearchAllVideosByOriginNumber(std::vector<UINT>& vtVideo, UINT32 OriginNumber)
{
//	int nFoundCount = 3;

	vtVideo.clear();

	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgOriginNumber == OriginNumber)
		{
			vtVideo.push_back(itemVideo.index);	// Video Index를 저장함.
	//		nFoundCount--;
	//		if (0 == nFoundCount) break;
		}
	}

//	return (nFoundCount < 3);
	return vtVideo.size();
}


BOOL Excel_DB::_SearchAllVideosByOriginNumber(std::vector<_typeVideoDB*>& vtVideoPtr, UINT32 OriginNumber)
{
//	int nFoundCount = 3;

	vtVideoPtr.clear();

	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgOriginNumber == OriginNumber)
		{
			vtVideoPtr.push_back(&itemVideo);	// Video Index를 저장함.
	//		nFoundCount--;
	//		if (0 == nFoundCount) break;
		}
	}

//	return (nFoundCount < 3);
	return vtVideoPtr.size();
}



// ----------------------------------------------
_typeSignGlossDB Excel_DB::GetSignGlossData(int index)
{
	_typeSignGlossDB _STRUCT;

	_STRUCT.sg_ID = 0;;
	_STRUCT.sg_OriginNumber = 0;;
	_STRUCT.sg_Name = "";
	_STRUCT.sg_Description = "";
	_STRUCT.sg_Category = "";
	_STRUCT.sg_vtVideo.clear();
	_STRUCT.sg_key_frame_num = 0;
	_STRUCT.sg_HandMovement = "";
	_STRUCT.sg_KeyFrames = "";

	if (m_SignGlossDB.size() <= index) return _STRUCT;
	_STRUCT = m_SignGlossDB[index];
	return _STRUCT;
}

// ----------------------------------------------
_typeVideoDB Excel_DB::GetVideoData(int index)
{
	_typeVideoDB _STRUCT;
	_STRUCT.sgv_ID = 0;
	_STRUCT.sgv_sgOriginNumber = 0;
	_STRUCT.sgv_ViewPoint = ESGV_ViewPoint::NONE;			// 'C', 'L', 'R'
	_STRUCT.sgv_Person = _T("");					// '1', '2'
	_STRUCT.dummy = 0;	
	_STRUCT.sg_vtCP.clear();
	_STRUCT.sgv_FileExt = _T("");
	_STRUCT.sgv_Width = 0;
	_STRUCT.sgv_Height = 0;
	_STRUCT.sgv_FPS = 0;

	if (m_VideoDB.size() <= index) return _STRUCT;
	_STRUCT = m_VideoDB[index];
	return _STRUCT;
}

// ----------------------------------------------
_typeChangepointDB Excel_DB::GetChangepointData(int index)
{
	_typeChangepointDB _STRUCT;
	_STRUCT.sgcp_ID = 1;
	_STRUCT.sgcp_sgvID = 1;
	_STRUCT.sgcp_TimeStamp = 0;
	_STRUCT.sgcp_FrameIndex = 0;
	_STRUCT.sgcp_Class = 0;
	_STRUCT.sgcp_rcLeftHand = { 0, 0, 0, 0 };
	_STRUCT.sgcp_rcRightHand = { 0, 0, 0, 0 };
	_STRUCT.sgcp_scvID = 0;
	_STRUCT.sgcp_scvFrameID = 0;

	if (m_CpDB.size() <= index) return _STRUCT;
	_STRUCT = m_CpDB[index];
	return _STRUCT;
}

// ----------------------------------------------
#define MAX_COLS_DBLIST  9
int Excel_DB::_Find_by_Expression(CString strFind, CListCtrl& listResult)
{
	_typeSignGlossDB itemSG;
	CString str;
	int nFindCount = 0;
	CString person_id;

	int nSGCount = (int)m_SignGlossDB.size();
	for (int i = 0; i < nSGCount; i++) {
		itemSG = GetSignGlossData(i);
		CString strExpression = itemSG.sg_Name;
	//	if (Find_CommaTokenString(strFind, strExpression)) {
		if (strExpression.Find(strFind)>=0) {
			//_SearchAllVideosByOriginNumber(itemSG.sg_vtVideo, itemSG.sg_OriginNumber);
			bool flag = _SearchAllVideosBySGID(itemSG.sg_vtVideo, itemSG.sg_ID);

			for( int k=0;k< itemSG.sg_vtVideo.size(); k+=3)
			{
				CString strVideoFiles;
				CString strSkeletonFiles;
				CString strVideoFilePath;

				if (0 < itemSG.sg_vtVideo.size())
				{
					CString strFile;

					int videoIndex = (int)itemSG.sg_vtVideo[k];
					GetSignGlossVideoFilePath(videoIndex, strFile);
					strVideoFilePath = strFile;

					GetSignGlossVideoFileName(videoIndex, strFile);
					strVideoFiles = strFile + ", ";
					m_VideoDB[videoIndex].sgv_sgOriginNumber = itemSG.sg_OriginNumber;

					videoIndex = (int)itemSG.sg_vtVideo[k+1];
					GetSignGlossVideoFileName(videoIndex, strFile);
					strVideoFiles += strFile + ", ";
					m_VideoDB[videoIndex].sgv_sgOriginNumber = itemSG.sg_OriginNumber;

					videoIndex = (int)itemSG.sg_vtVideo[k+2];
					GetSignGlossVideoFileName(videoIndex, strFile);
					strVideoFiles += strFile;
					m_VideoDB[videoIndex].sgv_sgOriginNumber = itemSG.sg_OriginNumber;

					videoIndex = (int)itemSG.sg_vtVideo[k];
					GetSignGlossSkeletonFileName(videoIndex, strFile);
					strSkeletonFiles = strFile + ", ";

					videoIndex = (int)itemSG.sg_vtVideo[k+1];
					GetSignGlossSkeletonFileName(videoIndex, strFile);
					strSkeletonFiles += strFile + ", ";

					videoIndex = (int)itemSG.sg_vtVideo[k+2];
					GetSignGlossSkeletonFileName(videoIndex, strFile);
					strSkeletonFiles += strFile;

					person_id =  GetSignGlossVideoPerson(videoIndex);
				}

				str.Format(_T("%d"), i + 1);
				listResult.InsertItem(nFindCount, str, -1);
				for (int col = 1; col < MAX_COLS_DBLIST; col++)
				{
					switch (col) {
					case 1: str.Format("%d", itemSG.sg_ID);	break;
					case 2: str.Format("%d", itemSG.sg_OriginNumber);	break;
					case 3: str = itemSG.sg_Name;				break;
					case 4: str = person_id; 	break;
					case 5: str = itemSG.sg_Category;			break;
	
	/*				case 5: str = strVideoFilePath;	break;		
					case 6: str = strVideoFiles;	break;			
					case 7: str = strSkeletonFiles;	break;
					case 8: str = person_id; 	break;*/
					}
					listResult.SetItemText(nFindCount, col, str);
				}
				nFindCount++;
			}
			
		}
	}

	return nFindCount;
}



BOOL Excel_DB::Find_CommaTokenString(CString& strFind, CString& strTarget)
{
	CString Token;
	int nPos = 0;
	Token = strTarget.Tokenize(",", nPos);
	if (Token == strFind) return TRUE;
	while (!Token.IsEmpty()) {
		Token = strTarget.Tokenize(",", nPos);
		if (Token == strFind) return TRUE;
	}
	return FALSE;
}


UINT Excel_DB::GetSignGlossID(int index)
{
	_typeSignGlossDB itemSG = GetSignGlossData(index);
	return itemSG.sg_ID;
}

UINT Excel_DB::GetSignGlossOriginNumber(int index)
{
	_typeSignGlossDB itemSG = GetSignGlossData(index);
	return itemSG.sg_OriginNumber;
}

INT Excel_DB::GetSignGlossIndexFromOriginNumber(int OriginNumber)
{
	for (int k = 0; k < m_SignGlossDB.size(); k++)
	{
		if (m_SignGlossDB[k].sg_OriginNumber == OriginNumber)
		{
			return m_SignGlossDB[k].index;
		}
	}
	return -1;
}

INT Excel_DB::GetSignGlossIndexFromVideoID(int VideoID)
{
	if (m_VideoDB.size() >= VideoID) return m_VideoDB[VideoID - 1].sgv_sgIndex;
	else return -1;
}



int Excel_DB::DecodeEntry(CString entry, CString& gloss_nm, int& origin_number)
{
	int ptr1 = entry.Find(_T("["));
	int ptr2 = entry.Find(_T("]"));
	int ptr3 = entry.Find(_T("_"));

	if (ptr1 >= 0 && ptr2 >= 0 && ptr3 >= 0)
	{
		origin_number = _ttoi(entry.Mid(ptr3 + 1, ptr2 - ptr3 - 1));
		gloss_nm = entry.Mid(ptr1 + 1, ptr3 - ptr1 - 1);
		return 1;
	}
	else return 0;
}

VOID Excel_DB::GetSignGlossName(int index, CString& str)
{
	_typeSignGlossDB itemSG = GetSignGlossData(index);
	str = itemSG.sg_Name;
}

VOID Excel_DB::GetSignGlossDescription(int index, CString& str)
{
	_typeSignGlossDB itemSG = GetSignGlossData(index);
	str = itemSG.sg_Description;
}

VOID Excel_DB::GetSignGlossCategory(int index, CString& str)
{
	_typeSignGlossDB itemSG = GetSignGlossData(index);
	str = itemSG.sg_Category;
}


UINT Excel_DB::GetSignGlossVideoID(int index)
{
	_typeVideoDB itemVideo = GetVideoData(index);
	return itemVideo.sgv_ID;
}

UINT Excel_DB::GetSignGlossVideoSGID(int index)
{
	_typeVideoDB itemVideo = GetVideoData(index);
	return itemVideo.sgv_sgID;
}

ESGV_ViewPoint Excel_DB::GetSignGlossVideoViewPoint(int index)
{
	_typeVideoDB itemVideo = GetVideoData(index);
	return itemVideo.sgv_ViewPoint;
}

CString Excel_DB::GetSignGlossVideoPerson(int index)
{
	_typeVideoDB itemVideo = GetVideoData(index);
	return itemVideo.sgv_Person;
}

VOID Excel_DB::GetSignGlossVideoFilePath(int indexVideo, CString& str)
{
	size_t dbSize = m_VideoDB.size();
	if ((0 < dbSize) && (0 <= indexVideo) && (indexVideo < (dbSize - 1)))
	{
		//str = gConfig.m_Config.strVideoPath;
		//if ("\\" != str.Right(1))
		//{
		//	str += "\\";
		//}

		_typeVideoDB itemVideo = GetVideoData((UINT)indexVideo);
		CString add_str;

		str += (itemVideo.sgv_Person+"\\MKV");
	}
}

VOID Excel_DB::GetSignGlossVideoFileName(int indexVideo, CString& str)
{
	if ((0 <= indexVideo) && (indexVideo < m_VideoDB.size()))
	{
		_typeVideoDB itemVideo = m_VideoDB[indexVideo];

		str.Format("%d.%s.%c.mkv", itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person, itemVideo.sgv_ViewPoint);
	}
}

VOID Excel_DB::GetSignGlossVideoFileNameExt(int indexVideo, CString& str)
{
	if ((0 <= indexVideo) && (indexVideo < m_VideoDB.size()))
	{
		_typeVideoDB itemVideo = m_VideoDB[indexVideo];
		str.Format("%d.%s.%c.%s.mkv", itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person, itemVideo.sgv_ViewPoint, itemVideo.sgv_FileExt);
	}
}

VOID Excel_DB::GetSignGlossSkeletonFilePath(int indexVideo, CString& str)
{
	size_t dbSize = m_VideoDB.size();
	if ((0 < dbSize) && (0 <= indexVideo) && (indexVideo < (dbSize - 1)))
	{
		//str = gConfig.m_Config.strVideoPath;
		//if ("\\" != str.Right(1))
		//{
		//	str += "\\";
		//}

		_typeVideoDB itemVideo = GetVideoData((UINT)indexVideo);
		str += (itemVideo.sgv_Person+"\\SKELETON");
	}
}

VOID Excel_DB::GetSignGlossSkeletonFileName(int indexVideo, CString& str)
{
	if ((0 <= indexVideo) && (indexVideo < m_VideoDB.size()))
	{
		_typeVideoDB itemVideo = m_VideoDB[indexVideo];
		str.Format("%d.%s.%c.json", itemVideo.sgv_sgOriginNumber, itemVideo.sgv_Person, itemVideo.sgv_ViewPoint);
	}
}

UINT Excel_DB::GetChangepointID(int index)
{
	_typeChangepointDB itemCP = GetChangepointData(index);
	return itemCP.sgcp_ID;
}

UINT Excel_DB::GetChangepointVideoID(int index)
{
	_typeChangepointDB itemCP = GetChangepointData(index);
	return itemCP.sgcp_sgvID;
}

UINT64 Excel_DB::GetChangepointTimestamp(int index)
{
	_typeChangepointDB itemCP = GetChangepointData(index);
	return itemCP.sgcp_TimeStamp;
}

UINT Excel_DB::GetChangepointFramNumber(int index)
{
	_typeChangepointDB itemCP = GetChangepointData(index);
	return itemCP.sgcp_FrameIndex;
}

INT Excel_DB::GetChangepointConfidence(int index)
{
	_typeChangepointDB itemCP = GetChangepointData(index);
	return (INT)itemCP.sgcp_Class;
}

RECT Excel_DB::GetChangepointLeftHandRect(int index)
{
	_typeChangepointDB itemCP = GetChangepointData(index);
	return itemCP.sgcp_rcLeftHand;
}

RECT Excel_DB::GetChangepointRightHandRect(int index)
{
	_typeChangepointDB itemCP = GetChangepointData(index);
	return itemCP.sgcp_rcRightHand;
}

UINT Excel_DB::GetVideoFPS(int index)
{
	auto itemVideo = GetVideoData(index);
	return itemVideo.sgv_FPS;
}

_typeChangepointDB Excel_DB::GetChangepoint(int sgcp_sgvID, int sgcp_ID)
{
	for (auto cp : m_CpDB)
	{
		if (cp.sgcp_sgvID == sgcp_sgvID && cp.sgcp_ID == sgcp_ID)
			return cp;
	}
	_typeChangepointDB ret_cp;
	ret_cp.index = -1;
	return ret_cp;
}

std::vector<_typeChangepointDB> Excel_DB::GetChangepoint(int sgcp_sgvID)
{
	std::vector<_typeChangepointDB> ret_cps;
	for (auto cp : m_CpDB)
	{
		if (cp.sgcp_sgvID == sgcp_sgvID)
			ret_cps.push_back(cp);			
	}	
	return ret_cps;
}

_typeChangepointDB Excel_DB::GetChangepoint(int OriginNumber, CString person, int sgcp_ID)
{
	for (auto cp : m_CpDB)
	{
		auto sg = m_SignGlossDB[cp.sgcp_sgIndex];
		if (sg.sg_OriginNumber == OriginNumber && cp.sgcp_ID == sgcp_ID && m_VideoDB[cp.sgcp_sgvID - 1].sgv_Person.Compare(person)==0)
		{
			return cp;
		}			
	}
	_typeChangepointDB ret_cp;
	ret_cp.index = -1;
	return ret_cp;
}


INT Excel_DB::GetValidCPNum(_typeVideoDB videoDB)
{
	int valid_key_cp = 0;
	for (int m = 0; m < videoDB.sg_vtCP.size(); m++)
	{
		int cp_index = videoDB.sg_vtCP[m];

		_typeChangepointDB cp_db = GetChangepointData(cp_index);

		if (cp_db.sgcp_vtHandStatus == 1 || cp_db.sgcp_vtHandStatus == 3)
			valid_key_cp++;
	}
	return valid_key_cp;
}

INT Excel_DB::GetNewVideoID()
{
	int max_vID = 0;
	for (auto v : m_VideoDB)
	{
		if (v.sgv_ID > max_vID) max_vID = v.sgv_ID;
	}
	return (max_vID + 1);
}



std::vector< _typeVideoDB > Excel_DB::GetVideoFromOriginNumber(int origin_number)
{
	std::vector< _typeVideoDB > videos;
	for (auto video : m_VideoDB)
	{
		if (m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber == origin_number)
			videos.push_back(video);
	}
	return videos;
}

std::vector< _typeVideoDB > Excel_DB::GetVideo(int origin_number, CString Person, ESGV_ViewPoint type)
{
	std::vector< _typeVideoDB > videos;
	for (auto video : m_VideoDB)
	{
		if (m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber == origin_number && video.sgv_Person.Compare(Person) == 0 && video.sgv_ViewPoint == type)
		{
			videos.push_back(video);
		}			
	}
	return videos;
}

CString Excel_DB::GetMKVFIleFromVideo(_typeVideoDB video, CString db_dir)
{
	CString mkv_file;

	if (video.sgv_FileExt.IsEmpty())
	{
		if (video.sgv_ViewPoint == ESGV_ViewPoint::CENTER)
			mkv_file.Format(_T("%s\\%s\\MKV\\%d.%s.C.mkv"), db_dir, video.sgv_Person, m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber, video.sgv_Person);
		else if (video.sgv_ViewPoint == ESGV_ViewPoint::RIGHT)
			mkv_file.Format(_T("%s\\%s\\MKV\\%d.%s.R.mkv"), db_dir, video.sgv_Person, m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber, video.sgv_Person);
		else mkv_file.Format(_T("%s\\%s\\MKV\\%d.%s.L.mkv"), db_dir, video.sgv_Person, m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber, video.sgv_Person);
	}
	else
	{
		if (video.sgv_ViewPoint == ESGV_ViewPoint::CENTER)
			mkv_file.Format(_T("%s\\%s\\MKV\\%d.%s.C.%s.mkv"), db_dir, video.sgv_Person, m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber, video.sgv_Person, video.sgv_FileExt);
		else if (video.sgv_ViewPoint == ESGV_ViewPoint::RIGHT)
			mkv_file.Format(_T("%s\\%s\\MKV\\%d.%s.R.%s.mkv"), db_dir, video.sgv_Person, m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber, video.sgv_Person, video.sgv_FileExt);
		else mkv_file.Format(_T("%s\\%s\\MKV\\%d.%s.L.%s.mkv"), db_dir, video.sgv_Person, m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber, video.sgv_Person, video.sgv_FileExt);
	}

	return mkv_file;
}

CString Excel_DB::GetJsonFIleFromVideo(_typeVideoDB video, CString db_dir)
{
	CString json_file;

	json_file.Format(_T("%s\\%s\\SKELETON\\%d.%s.C.json"), db_dir, video.sgv_Person, m_SignGlossDB[video.sgv_sgIndex].sg_OriginNumber, video.sgv_Person);

	return json_file;
}



CString Excel_DB::GetMKVFIleNameFromCP(_typeChangepointDB cp, CString db_dir)
{
	CString mkv_file;

	auto video = GetVideoData(cp.sgcp_sgvID - 1);

	if( video.index >= 0 )
		mkv_file = GetMKVFIleFromVideo(video, db_dir);

	return mkv_file;
}


BOOL Excel_DB::SetSGItem(CSGItem& sgItem, int index) 
{
	if ((index < 0) || (m_SignGlossDB.size() < index))
	{
		return FALSE;
	}

	_SearchAllVideosByOriginNumber(m_SignGlossDB[index].sg_vtVideo, m_SignGlossDB[index].sg_OriginNumber);

//	sgItem.SetSGItem(*this, index);
	return TRUE;
}

