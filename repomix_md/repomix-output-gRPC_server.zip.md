This file is a merged representation of the entire codebase, combined into a single document by Repomix.
The content has been processed where comments have been removed, empty lines have been removed, security check has been disabled.

# File Summary

## Purpose
This file contains a packed representation of the entire repository's contents.
It is designed to be easily consumable by AI systems for analysis, code review,
or other automated processes.

## File Format
The content is organized as follows:
1. This summary section
2. Repository information
3. Directory structure
4. Repository files (if enabled)
5. Multiple file entries, each consisting of:
  a. A header with the file path (## File: path/to/file)
  b. The full contents of the file in a code block

## Usage Guidelines
- This file should be treated as read-only. Any changes should be made to the
  original repository files, not this packed version.
- When processing this file, use the file path to distinguish
  between different files in the repository.
- Be aware that this file may contain sensitive information. Handle it with
  the same level of security as you would the original repository.

## Notes
- Some files may have been excluded based on .gitignore rules and Repomix's configuration
- Binary files are not included in this packed representation. Please refer to the Repository Structure section for a complete list of file paths, including binary files
- Files matching patterns in .gitignore are excluded
- Files matching default ignore patterns are excluded
- Code comments have been removed from supported file types
- Empty lines have been removed from all files
- Security check has been disabled - content may contain sensitive information
- Files are sorted by Git change count (files with more changes are at the bottom)

# Directory Structure
```
ChangePointDetector.cpp
ETRI_KSL_Excel_DB_Load-v1.4.cpp
ETRI_KSL_Excel_DB-v1.4.cpp
ETRI_KSL_Excel_DB-v1.4.h
framework.h
gRPCThread.h
HandConfidence.h
InferSentence.cpp
ksl_sentence_recognition.grpc.pb.cc
ksl_sentence_recognition.grpc.pb.h
ksl_sentence_recognition.pb.cc
ksl_sentence_recognition.pb.h
ksl_sentence_recognition.proto
KSLDBManager.cpp
KSLDBManager.h
KSLDBManagerDlg.cpp
KSLDBManagerDlg.h
ManoPoseDiff.cpp
ManoPoseDiff.h
pch.cpp
pch.h
resource.h
targetver.h
VideoUtil-v1.1.cpp
VideoUtil-v1.1.h
```

# Files

## File: ChangePointDetector.cpp
```cpp
#include "pch.h"
#include "ChangePointDetector.h"
#include <algorithm>
#include <cmath>
#include <limits>
ChangePointDetector::ChangePointDetector(const Params& p)
    : params_(p), joint_indices_(p.joint_indices) {
}
void ChangePointDetector::reset() {
    frame_idx_ = -1;
    prev_.clear(); has_prev_ = false;
    motion_hist_.clear();
    ema_motion_ = 0.f; ema_init_ = false;
    state_ = State::MOVING;
    dwell_counter_in_ = dwell_counter_out_ = 0;
    candidate_start_ = -1;
    last_stable_start_ = last_stable_end_ = -1;
    have_ema1_ = have_ema2_ = false;
    last_valley_idx_ = -1000000;
}
float ChangePointDetector::l2(const cv::Point3f& a, const cv::Point3f& b, bool useZ) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    if (useZ) {
        const float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    return std::sqrt(dx * dx + dy * dy);
}
float ChangePointDetector::median(std::deque<float> v) {
    if (v.empty()) return 0.f;
    std::nth_element(v.begin(), v.begin() + static_cast<long>(v.size() / 2), v.end());
    return v[v.size() / 2];
}
void ChangePointDetector::computeThresholds(float& thr_low, float& thr_high) const {
    float med = 0.f;
    if (!motion_hist_.empty()) {
        std::deque<float> tmp = motion_hist_;
        med = median(tmp);
    }
    thr_low = std::max(params_.floor_low, params_.rel_low_scale * med);
    thr_high = std::max(params_.floor_high, params_.rel_high_scale * med);
    if (thr_high < thr_low) thr_high = thr_low;
}
ChangePointDetector::Event ChangePointDetector::step(float motion_raw) {
    Event ev{};
    ev.motion_raw = motion_raw;
    if (!ema_init_) { ema_motion_ = motion_raw; ema_init_ = true; }
    else {
        ema_motion_ = params_.ema_alpha * motion_raw
            + (1.f - params_.ema_alpha) * ema_motion_;
    }
    ev.motion_smooth = ema_motion_;
    motion_hist_.push_back(motion_raw);
    while ((int)motion_hist_.size() > params_.win_median)
        motion_hist_.pop_front();
    float thr_low = 0.f, thr_high = 0.f;
    computeThresholds(thr_low, thr_high);
    ev.thr_low = thr_low;
    ev.thr_high = thr_high;
    const bool below = (ema_motion_ < thr_low);
    const bool above = (ema_motion_ > thr_high);
    if (state_ == State::MOVING) {
        if (below) {
            if (dwell_counter_in_ == 0) candidate_start_ = frame_idx_;
            if (++dwell_counter_in_ >= params_.dwell_in_frames) {
                state_ = State::STABLE;
                ev.segment_started = true;
                last_stable_start_ = candidate_start_;
                ev.stable_start = last_stable_start_;
                dwell_counter_out_ = 0;
            }
        }
        else {
            dwell_counter_in_ = 0;
            candidate_start_ = -1;
        }
    }
    else {
        ev.is_stable_now = true;
        if (above) {
            if (++dwell_counter_out_ >= params_.dwell_out_frames) {
                state_ = State::MOVING;
                ev.segment_ended = true;
                last_stable_end_ = frame_idx_ - (dwell_counter_out_ - 1);
                ev.stable_end = last_stable_end_;
                if (last_stable_start_ >= 0 && last_stable_end_ >= last_stable_start_) {
                    ev.representative = (last_stable_start_ + last_stable_end_) / 2;
                }
                dwell_counter_in_ = 0;
            }
        }
        else {
            dwell_counter_out_ = 0;
        }
    }
    {
        bool in_roi = true;
        if (params_.valley_roi_start >= 0 && params_.valley_roi_end >= 0) {
            in_roi = (frame_idx_ >= params_.valley_roi_start &&
                frame_idx_ <= params_.valley_roi_end);
        }
        if (in_roi && have_ema1_ && have_ema2_) {
            float y2 = ema_prev2_, y1 = ema_prev1_, y0 = ema_motion_;
            bool is_valley_shape = (y2 > y1 && y1 <= y0);
            float left_drop = y2 - y1;
            float right_drop = y0 - y1;
            float prom = std::min(left_drop, right_drop);
            float gate = ev.thr_low * params_.valley_below_scale;
            if (is_valley_shape &&
                std::isfinite(prom) &&
                prom >= std::max(params_.valley_prominence, 0.2f * ev.thr_low) &&
                y1 <= gate &&
                (frame_idx_ - 1 - last_valley_idx_) >= params_.valley_min_dist)
            {
                ev.local_min = true;
                ev.local_min_index = frame_idx_ - 1;
                ev.local_min_value = y1;
                last_valley_idx_ = ev.local_min_index;
            }
        }
        have_ema2_ = have_ema1_;
        ema_prev2_ = ema_prev1_;
        have_ema1_ = true;
        ema_prev1_ = ema_motion_;
    }
    ev.is_stable_now = (state_ == State::STABLE);
    return ev;
}
ChangePointDetector::Event
ChangePointDetector::update(const std::vector<cv::Point3f>& mp_skeleton) {
    frame_idx_++;
    if (mp_skeleton.empty()) {
        Event ev{};
        ev.motion_raw = std::numeric_limits<float>::quiet_NaN();
        ev.motion_smooth = ema_motion_;
        computeThresholds(ev.thr_low, ev.thr_high);
        ev.is_stable_now = (state_ == State::STABLE);
        return ev;
    }
    float motion_raw = 0.f;
    if (!has_prev_) {
        prev_ = mp_skeleton;
        has_prev_ = true;
        motion_raw = 0.f;
        return step(motion_raw);
    }
    const size_t n = std::min(prev_.size(), mp_skeleton.size());
    std::vector<int> sel;
    sel.reserve(joint_indices_.empty() ? n : joint_indices_.size());
    if (joint_indices_.empty()) {
        for (size_t i = 0; i < n; ++i) sel.push_back((int)i);
    }
    else {
        for (int idx : joint_indices_) {
            if (idx >= 0 && (size_t)idx < n) sel.push_back(idx);
        }
    }
    std::vector<float> dists; dists.reserve(sel.size());
    for (int idx : sel) {
        float d = l2(mp_skeleton[(size_t)idx], prev_[(size_t)idx], params_.use_z);
        if (std::isfinite(d)) dists.push_back(d);
    }
    prev_ = mp_skeleton;
    const int effective_min = std::max(1, std::min(params_.min_joints, (int)sel.size()));
    if ((int)dists.size() < effective_min) {
        Event ev{};
        ev.motion_raw = std::numeric_limits<float>::quiet_NaN();
        ev.motion_smooth = ema_motion_;
        computeThresholds(ev.thr_low, ev.thr_high);
        ev.is_stable_now = (state_ == State::STABLE);
        return ev;
    }
    std::nth_element(dists.begin(), dists.begin() + (long)(dists.size() / 2), dists.end());
    motion_raw = dists[dists.size() / 2];
    return step(motion_raw);
}
ChangePointDetector::Event ChangePointDetector::finalize() {
    Event ev{};
    if (state_ == State::STABLE && last_stable_start_ >= 0) {
        ev.segment_ended = true;
        last_stable_end_ = frame_idx_;
        ev.stable_end = last_stable_end_;
        ev.stable_start = last_stable_start_;
        ev.representative = (last_stable_start_ + last_stable_end_) / 2;
        state_ = State::MOVING;
    }
    return ev;
}
```

## File: ETRI_KSL_Excel_DB_Load-v1.4.cpp
```cpp
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
		mid = (start + end) / 2;
		sgID = m_SignGlossDB[mid].sg_ID;
		if (sgID == key) {
			index = (UINT)mid;
			return TRUE;
		}
		else if (sgID > key) {
			end = mid - 1;
		}
		else {
			start = mid + 1;
		}
	}
	return FALSE;
}
BOOL Excel_DB::_Load(CString strPath)
{
	XLDocument doc;
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
		if (OpenXLSX::XLValueType::Empty == values.at(0).type())
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
	itemVideo.sgv_ViewPoint = ESGV_ViewPoint::NONE;
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
			else if (sgv_ViewPoint[0] == 'l')
			{
				itemVideo.sgv_ViewPoint = ESGV_ViewPoint::LEFT;
			}
			else if (sgv_ViewPoint[0] == 'r')
			{
				itemVideo.sgv_ViewPoint = ESGV_ViewPoint::RIGHT;
			}
			else
			{
				itemVideo.sgv_ViewPoint = ESGV_ViewPoint::NONE;
			}
			std::string  sgv_Person = values.at(3).get<std::string>();
			CString cstr(sgv_Person.c_str());
			itemVideo.sgv_Person = cstr;
			std::string  sgv_FileExt = values.at(8).get<std::string>();
			CString cstr2(sgv_FileExt.c_str());
			itemVideo.sgv_FileExt = cstr2;
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
		if (OpenXLSX::XLValueType::Empty == values.at(0).type())
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
			std::string sgv_ViewPoint = values.at(2).get<std::string>();
			if (sgv_ViewPoint[0] == 'f')
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::CENTER;
			}
			else if (sgv_ViewPoint[0] == 'l')
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::LEFT;
			}
			else if (sgv_ViewPoint[0] == 'r')
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::RIGHT;
			}
			else
			{
				itemVideo.scv_ViewPoint = ESGV_ViewPoint::NONE;
			}
			std::string  scv_Person = values.at(3).get<std::string>();
			CString cstr(scv_Person.c_str());
			itemVideo.scv_Person = cstr;
			std::string scv_FileName = values.at(4).get<std::string>();
			itemVideo.scv_FileName = convert_utf8_to_unicode_string(scv_FileName);
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
			itemVideo.scv_Sentence = convert_utf8_to_unicode_string(scv_Sentence);
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
	}
	strVal = wks.cell(XLCellReference(1, 2)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Sign_Gloss_Video_ID"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 3)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Changepoint_Timestamp"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 4)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Changepoint_frame_number"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 5)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("Confidence"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 6)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("LEFT_HAND_ROI"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 7)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RIGHT_HAND_ROI"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 8)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("FPS"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 9)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("HAND_POSITION"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 10)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("HAND_POSITION_CONFIDENCE"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 11)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RELATIVE_DISTANCE"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 12)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("LEFT_HAND_SHAPE[]"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 13)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RIGHT_HAND_SHAPE[]"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 14)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("LEFT_HAND_SHAPE_CONFIDENCE[]"))
	{
	}
	strVal = wks.cell(XLCellReference(1, 15)).value().get<std::string>();
	str_C = strVal.c_str();
	if (0 != str_C.Compare("RIGHT_HAND_SHAPE_CONFIDENCE[]"))
	{
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
		if (OpenXLSX::XLValueType::Empty == values.at(0).type())
		{
			break;
		}
		if (1 < cell_idx)
		{
			UINT32 SGV_ID = values.at(1).get<UINT32>();
			itemCP.index = unRowIndex++;
			itemCP.sgcp_ID = values.at(0).get<UINT32>();
			itemCP.sgcp_sgvID = values.at(1).get<UINT32>();
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
			itemCP.sgcp_TimeStamp = values.at(2).get<UINT64>();
			itemCP.sgcp_FrameIndex = values.at(3).get<UINT32>();
			itemCP.sgcp_Class = values.at(4).get<INT32>();
			if (1 == itemCP.sgcp_Class)
			{
				std::string sgcp_LeftHandROI = values.at(5).get<std::string>();
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
				std::string sgcp_RightHandROI = values.at(6).get<std::string>();
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
						std::string sgcp_vtLHandSkeletonMP = values.at(9).get<std::string>();
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
						std::string sgcp_vtRHandSkeletonMP = values.at(10).get<std::string>();
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
					std::string sgcp_RightHandOrientaion = values.at(20).get<std::string>();
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
					std::string sgcp_LeftHandOrientaion = values.at(21).get<std::string>();
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
					std::string sgcp_skeleton = values.at(24).get<std::string>();
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
					std::string sgcp_skeleton = values.at(25).get<std::string>();
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
					std::string sgcp_skeleton = values.at(26).get<std::string>();
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
					itemCP.sgcp_vtHandStatus = values.at(27).get<int>();
				}
				if (29 <= unColumnCount)
				{
					std::string sgcp_vtSkeleton_confidence = values.at(28).get<std::string>();
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
					itemCP.sgcp_kIndex = values.at(29).get<UINT32>();
				}
				if (31 <= unColumnCount)
				{
					std::string sgcp_skeleton = values.at(30).get<std::string>();
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
			{
				{
					static int modified_index = 0;
					itemCP.index = modified_index;
					m_CpDB.push_back(itemCP);
					modified_index++;
				}
			}
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
std::string Excel_DB::convert_unicode_to_utf8_string(const CString& s)
{
#ifdef _UNICODE
	const wchar_t* u16 = static_cast<LPCWSTR>(s);
	int u16_len = static_cast<int>(wcslen(u16));
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
	const char* mb = static_cast<LPCSTR>(s);
	int needW = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
		mb, -1, nullptr, 0);
	if (needW <= 0) return {};
	std::wstring u16(needW - 1, L'\0');
	if (!::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
		mb, -1, u16.data(), needW)) return {};
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
		unicode.resize(required_cch);
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
	auto& oldSheet = doc.workbook().worksheet(sheetName);
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
	doc.workbook().deleteSheet(sheetName);
	doc.workbook().addWorksheet(sheetName);
	auto& newSheet = doc.workbook().worksheet(sheetName);
	for (uint64_t col = 1; col <= headerRow.size(); ++col) {
		newSheet.cell(XLCellReference(1, col)).value() = headerRow[col - 1];
	}
}
```

## File: ETRI_KSL_Excel_DB-v1.4.cpp
```cpp
#include "pch.h"
#include "ETRI_KSL_Excel_DB-v1.4.h"
#include <iostream>
#include <algorithm>
#include <cmath>
using namespace std;
using namespace OpenXLSX;
Excel_DB::Excel_DB()
{
}
void Excel_DB::_SetMessageID(HWND hWnd, UINT uMSG)
{
	m_hWndParent = hWnd;
	m_uMsgID = uMSG;
}
BOOL Excel_DB::_SearchAllVideosBySGID(std::vector<UINT>& vtVideo, UINT32 sgID)
{
	vtVideo.clear();
	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgID == sgID)
		{
			vtVideo.push_back(itemVideo.index);
		}
	}
	return (vtVideo.size());
}
BOOL Excel_DB::_SearchAllVideosBySGID(std::vector<UINT>& vtVideo, UINT32 sgID, CString person_id)
{
	vtVideo.clear();
	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgID == sgID && itemVideo.sgv_Person == person_id)
		{
			vtVideo.push_back(itemVideo.index);
		}
	}
	return (vtVideo.size());
}
BOOL Excel_DB::_SearchAllVideosBySGID(std::vector<_typeVideoDB*>& vtVideoPtr, UINT32 sgID)
{
	vtVideoPtr.clear();
	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgID == sgID)
		{
			vtVideoPtr.push_back(&itemVideo);
		}
	}
	return vtVideoPtr.size();
}
BOOL Excel_DB::_SearchAllVideosByOriginNumber(std::vector<UINT>& vtVideo, UINT32 OriginNumber)
{
	vtVideo.clear();
	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgOriginNumber == OriginNumber)
		{
			vtVideo.push_back(itemVideo.index);
		}
	}
	return vtVideo.size();
}
BOOL Excel_DB::_SearchAllVideosByOriginNumber(std::vector<_typeVideoDB*>& vtVideoPtr, UINT32 OriginNumber)
{
	vtVideoPtr.clear();
	for (auto itemVideo : m_VideoDB)
	{
		if (itemVideo.sgv_sgOriginNumber == OriginNumber)
		{
			vtVideoPtr.push_back(&itemVideo);
		}
	}
	return vtVideoPtr.size();
}
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
	return TRUE;
}
```

## File: ETRI_KSL_Excel_DB-v1.4.h
```
#pragma once
#include "pch.h"
#include <vector>
#include <OpenXLSX.hpp>
#include "VideoUtil-v1.1.h"
#include <string>
#include <atlstr.h>
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
	std::vector<UINT32> sg_vtVideo;
	INT sg_key_frame_num;
	CString sg_HandMovement;
	CString sg_KeyFrames;
} _typeSignGlossDB;
typedef struct _tagVideoDB {
	UINT32 index;
	UINT32 sgv_sgIndex;
	UINT32 sgv_ID;
	UINT32 sgv_sgID;
	UINT32 sgv_sgOriginNumber;
	ESGV_ViewPoint sgv_ViewPoint;
	CString sgv_Person;
	UINT16 dummy;
	std::vector<UINT32> sg_vtCP;
	CString sgv_FileExt;
	UINT32 sgv_Width;
	UINT32 sgv_Height;
	UINT32 sgv_FPS;
} _typeVideoDB;
typedef struct _tagCorpusVideoDB {
	UINT32 index;
	UINT32 scv_ID;
	CString scv_FileName;
	CString scv_Sentence;
	ESGV_ViewPoint scv_ViewPoint;
	CString scv_Person;
	UINT16 dummy;
	std::vector<UINT32> scv_sgOriginNumber;
	std::vector<UINT32> scv_key_frame_index;
	std::vector<UINT32> scv_sgIndex;
	std::vector<UINT32> scv_unified_sgIndex;
	std::vector<UINT32> scv_cpIndex;
	std::vector<UINT32> scv_handStatus;
	std::vector<UINT32> scv_gloss_ranges;
	CString scv_FileExt;
	UINT32 scv_Width;
	UINT32 scv_Height;
	UINT32 scv_FPS;
	std::vector<cv::Mat> scv_img;
	std::vector<UINT32> scv_gloss_unique;
	std::vector<double> scv_gloss_unique_weight;
} _typeCorpusVideoDB;
typedef struct _tagChangepointDB {
	UINT32 index;
	UINT32 sgcp_sgIndex;
	UINT32 sgcp_ID;
	UINT32 sgcp_sgvID;
	UINT64 sgcp_TimeStamp;
	UINT32 sgcp_FrameIndex;
	INT32 sgcp_Class;
	RECT sgcp_rcLeftHand;
	RECT sgcp_rcRightHand;
	UINT32 sgcp_scvID;
	UINT32 sgcp_scvFrameID;
	std::vector<FLOAT> sgcp_vtLeftHandOrientation;
	std::vector<FLOAT> sgcp_vtRightHandOrientation;
	std::vector<UINT32>sgcp_vtHandLocation;
	std::vector<FLOAT> sgcp_vtHandLocationConfidence;
	std::vector<FLOAT> sgcp_vtRelativeDistance;
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
	std::vector<Point3f> LH_verts;
	std::vector<Point3f> LH_faces;
	std::vector<Point3f> RH_verts;
	std::vector<Point3f> RH_faces;
	std::vector<Point3f> origin_joints;
	UINT32 sgcp_kIndex;
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
	_typeSignGlossDB GetSignGlossData(int index);
	_typeVideoDB GetVideoData(int index);
	_typeChangepointDB GetChangepointData(int index);
	int _GetCount() {
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
		Mat3 R_cw = Mat3::eye();
		Vec3 C_w = { 0,0,0 };
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
		bool  use_continuity = true;
		float ema_c = 0.20f, ema_r = 0.20f, ema_u = 0.20f, ema_f = 0.15f;
		DistanceMode distance_mode = DistanceMode::ShoulderScaled;
		float fixed_distance_m = 1.2f;
		float shoulder_scale_k = 10.0f;
		float up_offset_m = 0.10f;
		float side_offset_m = 0.00f;
		DepthMode depth_mode = DepthMode::Hybrid;
		float shoulder_target_m = 0.40f;
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
	void reconstructFromImageLandmarks(
		const std::vector<P3>& mp_image,
		std::vector<P3>& pts_cam_out,
		std::vector<P3>* pts_world_out = nullptr
	) {
		DepthScaler scaler;
		scaler.alpha = cfg_.z_alpha_init;
		scaler.beta = cfg_.z_beta_init;
		scaler.fitShoulderAlpha(mp_image, cfg_.idx.L_SH, cfg_.idx.R_SH, K_, cfg_.shoulder_target_m);
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
			if (!(Zc > 0.f) || !std::isfinite(Zc))
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
	static inline float norm3(const Vec3& v) { return std::sqrt(v.dot(v)); }
	static inline Vec3  nz(const Vec3& v) { float n = norm3(v); return (n > 1e-8f) ? v * (1.0f / n) : Vec3(0, 0, 0); }
	static inline void  orthonormalize(Vec3& r, Vec3& u, Vec3& f) {
		f = nz(f); r = nz(r - f * f.dot(r)); u = nz(u - f * f.dot(u) - r * r.dot(u));
		if (norm3(r) < 1e-6f) r = nz(f.cross(u));
		if (norm3(u) < 1e-6f) u = nz(r.cross(f));
	}
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
	struct DepthScaler {
		float alpha = 1000.f, beta = 0.f;
		inline float Zc_from_zmp(float z_mp) const { return alpha * z_mp + beta; }
		void fitShoulderAlpha(const std::vector<P3>& mp_img,
			int idxA, int idxB,
			const Intrinsics& K, float s_target_m)
		{
			beta = 0.0f;
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
	struct Plane {
		Vec3 n{ 0,0,1 }; float d = 0.f;
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
			float t = -d / denom;
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
```

## File: framework.h
```
#pragma once
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#include "targetver.h"
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _AFX_ALL_WARNINGS
#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif
#include <afxcontrolbars.h>
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
```

## File: gRPCThread.h
```
#pragma once
#include <grpcpp/grpcpp.h>
#include "ksl_sentence_recognition.grpc.pb.h"
#include <iostream>
#include <fstream>
#include "KSLDBManagerDlg.h"
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
                    WaitForSingleObject(pDlg->hAIFinish, INFINITE);
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
        builder.SetMaxReceiveMessageSize(kMaxMsg);
        builder.SetMaxSendMessageSize(kMaxMsg);
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
```

## File: HandConfidence.h
```
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
        std::vector<cv::Point2f> joints;
        std::vector<float>        vis;
    };
    struct Options {
        int num_joints = 21;
        double alpha = 8.0;
        bool use_procrustes = true;
        bool normalize_by_scale = true;
        std::string scale_mode = "bbox";
        int anchor_a = 0;
        int anchor_b = 12;
        bool use_huber = true;
        double huber_delta = 0.02;
        bool huber_per_joint = true;
        bool try_mirror_x = false;
        float visibility_thresh = 0.0f;
        std::vector<float> joint_weights;
        std::vector<int> acr_index_of_mp;
    };
    struct Detail {
        double err_raw = 0.0;
        double err_aligned = 0.0;
        double scalar_error = 0.0;
        double confidence = 0.0;
        bool   used_mirror = false;
        cv::Matx22f R = cv::Matx22f::eye();
        float s = 1.0f;
        cv::Vec2f t = { 0.f, 0.f };
        std::vector<float> per_joint_err;
        std::vector<float> per_joint_conf;
    };
    class HandConfidence {
    public:
        explicit HandConfidence(const Options& opt = Options{}) : opt_(opt) {
            if (opt_.num_joints <= 0) throw std::invalid_argument("num_joints must be > 0");
            if (opt_.acr_index_of_mp.empty()) {
                opt_.acr_index_of_mp.resize(opt_.num_joints);
                for (int i = 0;i < opt_.num_joints;++i) opt_.acr_index_of_mp[i] = i;
            }
        }
        const Options& options() const { return opt_; }
        Options& options() { return opt_; }
        Detail compute(const Hand2D& acr, const Hand2D& mp) const {
            Detail out;
            std::vector<cv::Point2f> P, Q;
            std::vector<float> W;
            gatherMatched(acr, mp, opt_.acr_index_of_mp, P, Q, W);
            applyJointWeights(W, opt_.joint_weights);
            applyVisibilityThresh(W, opt_.visibility_thresh);
            const int valid = countPositive(W);
            if (P.empty() || Q.empty() || valid < 3) {
                out.scalar_error = 1.0;
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
#ifdef HANDCONF_DEMO
#include <iostream>
    inline int demo() {
        Hand2D acr, mp; acr.joints.resize(21); mp.joints.resize(21); acr.vis.assign(21, 1.f); mp.vis.assign(21, 1.f);
        for (int i = 0;i < 21;++i) { acr.joints[i] = cv::Point2f(100 + i * 2, 100 + i * 1); mp.joints[i] = cv::Point2f(102 + i * 2, 98 + i * 1); }
        Options opt; opt.alpha = 8.0; opt.try_mirror_x = false;
        HandConfidence HC(opt); auto out = HC.compute(acr, mp);
        std::cout << "err=" << out.scalar_error << " conf=" << out.confidence << " mirror=" << out.used_mirror << "\n";
        return 0;
    }
#endif
}
```

## File: InferSentence.cpp
```cpp
#include "InferSentence.h"
void InferSentence::BuildCorpusGlossWeight(std::vector<_typeCorpusVideoDB> &CorpusVideoDB)
{
	auto& corpusDB = CorpusVideoDB;
	const size_t N = corpusDB.size();
	if (N == 0) return;
	std::unordered_map<UINT32, int> docFreq;
	for (auto& corpus : corpusDB)
	{
		std::unordered_set<UINT32> uniqueSet;
		if (!corpus.scv_unified_sgIndex.empty())
		{
			uniqueSet.insert(corpus.scv_unified_sgIndex.begin(),
				corpus.scv_unified_sgIndex.end());
		}
		else
		{
			uniqueSet.insert(corpus.scv_sgIndex.begin(),
				corpus.scv_sgIndex.end());
		}
		corpus.scv_gloss_unique.assign(uniqueSet.begin(), uniqueSet.end());
		for (UINT32 g : uniqueSet)
			docFreq[g]++;
	}
	const double Ndouble = static_cast<double>(N);
	for (auto& corpus : corpusDB)
	{
		const auto& G = corpus.scv_gloss_unique;
		corpus.scv_gloss_unique_weight.resize(G.size());
		for (size_t i = 0; i < G.size(); ++i)
		{
			UINT32 g = G[i];
			int df = 0;
			auto it = docFreq.find(g);
			if (it != docFreq.end()) df = it->second;
			double idf = std::log((Ndouble + 1.0) / (df + 0.5));
			if (idf < 0.0) idf = 0.0;
			corpus.scv_gloss_unique_weight[i] = idf;
		}
	}
}
_typeCorpusVideoDB InferSentence::InferCorpus(std::vector<GlossInfered> inferList,
    std::vector<_typeCorpusVideoDB>& CorpusVideoDB)
{
    _typeCorpusVideoDB emptyCorpus;
    _typeCorpusVideoDB bestCorpus;
    if (inferList.empty() || CorpusVideoDB.empty())
        return emptyCorpus;
    std::unordered_map<int, int> sgCount;
    int totalCount = 0;
    for (const auto& g : inferList)
    {
        if (g.infer_order < 0 || g.infer_order > 2)
            continue;
        sgCount[g.sgIndex] += 1;
        totalCount += 1;
    }
    if (sgCount.empty() || totalCount <= 0)
        return emptyCorpus;
    const double eps = 1e-12;
    struct SentInfo
    {
        const _typeCorpusVideoDB* corpus;
        double  prob;
        size_t  glossCount;
        size_t  seqLen;
        int     matchedGlossCount;
    };
    std::vector<SentInfo> group2plus;
    std::vector<SentInfo> group1;
    for (const auto& corpus : CorpusVideoDB)
    {
        const auto& seq = corpus.scv_sgIndex;
        if (seq.empty())
            continue;
        std::unordered_set<UINT32> uniqueGloss(seq.begin(), seq.end());
        size_t glossCount = uniqueGloss.size();
        if (glossCount == 0)
            continue;
        double sentenceProb = 0.0;
        int matchedGlossCount = 0;
        for (UINT32 sg : uniqueGloss)
        {
            auto it = sgCount.find(static_cast<int>(sg));
            if (it == sgCount.end())
                continue;
            matchedGlossCount++;
            double p = static_cast<double>(it->second) /
                static_cast<double>(totalCount);
            if (p > sentenceProb)
                sentenceProb = p;
        }
        if (matchedGlossCount <= 0 || sentenceProb <= 0.0)
            continue;
        SentInfo info;
        info.corpus = &corpus;
        info.prob = sentenceProb;
        info.glossCount = glossCount;
        info.seqLen = seq.size();
        info.matchedGlossCount = matchedGlossCount;
        if (matchedGlossCount >= 2)
            group2plus.push_back(info);
        else
            group1.push_back(info);
    }
    if (group2plus.empty() && group1.empty())
        return emptyCorpus;
    const std::vector<SentInfo>* targetGroup = nullptr;
    if (!group2plus.empty())
        targetGroup = &group2plus;
    else
        targetGroup = &group1;
    bool   hasBest = false;
    double bestProb = -1.0;
    size_t bestGlossCount = 0;
    size_t bestSeqLen = 0;
    for (const auto& s : *targetGroup)
    {
        bool isBetter = false;
        if (!hasBest || s.prob > bestProb + eps)
        {
            isBetter = true;
        }
        else if (std::fabs(s.prob - bestProb) <= eps)
        {
            if (s.glossCount < bestGlossCount)
            {
                isBetter = true;
            }
            else if (s.glossCount == bestGlossCount)
            {
                if (!hasBest || s.seqLen < bestSeqLen)
                    isBetter = true;
            }
        }
        if (isBetter)
        {
            hasBest = true;
            bestProb = s.prob;
            bestGlossCount = s.glossCount;
            bestSeqLen = s.seqLen;
            bestCorpus = *(s.corpus);
        }
    }
    if (!hasBest)
        return emptyCorpus;
    return bestCorpus;
}
```

## File: ksl_sentence_recognition.grpc.pb.cc
```
#include "ksl_sentence_recognition.pb.h"
#include "ksl_sentence_recognition.grpc.pb.h"
#include <functional>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/impl/channel_interface.h>
#include <grpcpp/impl/client_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/rpc_service_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/sync_stream.h>
namespace vision {
namespace raw {
namespace v1 {
static const char* SequenceService_method_names[] = {
  "/vision.raw.v1.SequenceService/SendFrames",
};
std::unique_ptr< SequenceService::Stub> SequenceService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< SequenceService::Stub> stub(new SequenceService::Stub(channel, options));
  return stub;
}
SequenceService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_SendFrames_(SequenceService_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::CLIENT_STREAMING, channel)
  {}
::grpc::ClientWriter< ::vision::raw::v1::Frame>* SequenceService::Stub::SendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) {
  return ::grpc::internal::ClientWriterFactory< ::vision::raw::v1::Frame>::Create(channel_.get(), rpcmethod_SendFrames_, context, response);
}
void SequenceService::Stub::async::SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::ClientWriteReactor< ::vision::raw::v1::Frame>* reactor) {
  ::grpc::internal::ClientCallbackWriterFactory< ::vision::raw::v1::Frame>::Create(stub_->channel_.get(), stub_->rpcmethod_SendFrames_, context, response, reactor);
}
::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* SequenceService::Stub::AsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::vision::raw::v1::Frame>::Create(channel_.get(), cq, rpcmethod_SendFrames_, context, response, true, tag);
}
::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* SequenceService::Stub::PrepareAsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::vision::raw::v1::Frame>::Create(channel_.get(), cq, rpcmethod_SendFrames_, context, response, false, nullptr);
}
SequenceService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      SequenceService_method_names[0],
      ::grpc::internal::RpcMethod::CLIENT_STREAMING,
      new ::grpc::internal::ClientStreamingHandler< SequenceService::Service, ::vision::raw::v1::Frame, ::vision::raw::v1::SubmitResultResponse>(
          [](SequenceService::Service* service,
             ::grpc::ServerContext* ctx,
             ::grpc::ServerReader<::vision::raw::v1::Frame>* reader,
             ::vision::raw::v1::SubmitResultResponse* resp) {
               return service->SendFrames(ctx, reader, resp);
             }, this)));
}
SequenceService::Service::~Service() {
}
::grpc::Status SequenceService::Service::SendFrames(::grpc::ServerContext* context, ::grpc::ServerReader< ::vision::raw::v1::Frame>* reader, ::vision::raw::v1::SubmitResultResponse* response) {
  (void) context;
  (void) reader;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}
}
}
}
```

## File: ksl_sentence_recognition.grpc.pb.h
```
#ifndef GRPC_ksl_5fsentence_5frecognition_2eproto__INCLUDED
#define GRPC_ksl_5fsentence_5frecognition_2eproto__INCLUDED
#include "ksl_sentence_recognition.pb.h"
#include <functional>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/proto_utils.h>
#include <grpcpp/impl/rpc_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <grpcpp/support/sync_stream.h>
namespace vision {
namespace raw {
namespace v1 {
class SequenceService final {
 public:
  static constexpr char const* service_full_name() {
    return "vision.raw.v1.SequenceService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    std::unique_ptr< ::grpc::ClientWriterInterface< ::vision::raw::v1::Frame>> SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) {
      return std::unique_ptr< ::grpc::ClientWriterInterface< ::vision::raw::v1::Frame>>(SendFramesRaw(context, response));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>> AsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>>(AsyncSendFramesRaw(context, response, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>> PrepareAsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>>(PrepareAsyncSendFramesRaw(context, response, cq));
    }
    class async_interface {
     public:
      virtual ~async_interface() {}
      virtual void SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::ClientWriteReactor< ::vision::raw::v1::Frame>* reactor) = 0;
    };
    typedef class async_interface experimental_async_interface;
    virtual class async_interface* async() { return nullptr; }
    class async_interface* experimental_async() { return async(); }
   private:
    virtual ::grpc::ClientWriterInterface< ::vision::raw::v1::Frame>* SendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) = 0;
    virtual ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>* AsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>* PrepareAsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
    std::unique_ptr< ::grpc::ClientWriter< ::vision::raw::v1::Frame>> SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) {
      return std::unique_ptr< ::grpc::ClientWriter< ::vision::raw::v1::Frame>>(SendFramesRaw(context, response));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>> AsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>>(AsyncSendFramesRaw(context, response, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>> PrepareAsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>>(PrepareAsyncSendFramesRaw(context, response, cq));
    }
    class async final :
      public StubInterface::async_interface {
     public:
      void SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::ClientWriteReactor< ::vision::raw::v1::Frame>* reactor) override;
     private:
      friend class Stub;
      explicit async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class async* async() override { return &async_stub_; }
   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class async async_stub_{this};
    ::grpc::ClientWriter< ::vision::raw::v1::Frame>* SendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) override;
    ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* AsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* PrepareAsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_SendFrames_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status SendFrames(::grpc::ServerContext* context, ::grpc::ServerReader< ::vision::raw::v1::Frame>* reader, ::vision::raw::v1::SubmitResultResponse* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* ) {}
   public:
    WithAsyncMethod_SendFrames() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    ::grpc::Status SendFrames(::grpc::ServerContext* , ::grpc::ServerReader< ::vision::raw::v1::Frame>* , ::vision::raw::v1::SubmitResultResponse* ) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendFrames(::grpc::ServerContext* context, ::grpc::ServerAsyncReader< ::vision::raw::v1::SubmitResultResponse, ::vision::raw::v1::Frame>* reader, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncClientStreaming(0, context, reader, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_SendFrames<Service > AsyncService;
  template <class BaseClass>
  class WithCallbackMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_SendFrames() {
      ::grpc::Service::MarkMethodCallback(0,
          new ::grpc::internal::CallbackClientStreamingHandler< ::vision::raw::v1::Frame, ::vision::raw::v1::SubmitResultResponse>(
            [this](
                   ::grpc::CallbackServerContext* context, ::vision::raw::v1::SubmitResultResponse* response) { return this->SendFrames(context, response); }));
    }
    ~WithCallbackMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerReadReactor< ::vision::raw::v1::Frame>* SendFrames(
      ::grpc::CallbackServerContext* /*context*/, ::vision::raw::v1::SubmitResultResponse* /*response*/)  { return nullptr; }
  };
  typedef WithCallbackMethod_SendFrames<Service > CallbackService;
  typedef CallbackService ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_SendFrames() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_SendFrames() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendFrames(::grpc::ServerContext* context, ::grpc::ServerAsyncReader< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* reader, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncClientStreaming(0, context, reader, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_SendFrames() {
      ::grpc::Service::MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackClientStreamingHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, ::grpc::ByteBuffer* response) { return this->SendFrames(context, response); }));
    }
    ~WithRawCallbackMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerReadReactor< ::grpc::ByteBuffer>* SendFrames(
      ::grpc::CallbackServerContext* , ::grpc::ByteBuffer* )  { return nullptr; }
  };
  typedef Service StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef Service StreamedService;
};
}
}
}
#endif
```

## File: ksl_sentence_recognition.pb.cc
```
#include "ksl_sentence_recognition.pb.h"
#include <algorithm>
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/port_def.inc"
PROTOBUF_PRAGMA_INIT_SEG
namespace _pb = ::google::protobuf;
namespace _pbi = ::google::protobuf::internal;
namespace _fl = ::google::protobuf::internal::field_layout;
namespace vision {
namespace raw {
namespace v1 {
inline constexpr SubmitResultResponse::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : session_id_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        message_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        frame_count_{0},
        _cached_size_{0} {}
template <typename>
PROTOBUF_CONSTEXPR SubmitResultResponse::SubmitResultResponse(::_pbi::ConstantInitialized)
    : _impl_(::_pbi::ConstantInitialized()) {}
struct SubmitResultResponseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR SubmitResultResponseDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~SubmitResultResponseDefaultTypeInternal() {}
  union {
    SubmitResultResponse _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 SubmitResultResponseDefaultTypeInternal _SubmitResultResponse_default_instance_;
inline constexpr Point3::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : x_{0},
        y_{0},
        z_{0},
        _cached_size_{0} {}
template <typename>
PROTOBUF_CONSTEXPR Point3::Point3(::_pbi::ConstantInitialized)
    : _impl_(::_pbi::ConstantInitialized()) {}
struct Point3DefaultTypeInternal {
  PROTOBUF_CONSTEXPR Point3DefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~Point3DefaultTypeInternal() {}
  union {
    Point3 _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 Point3DefaultTypeInternal _Point3_default_instance_;
inline constexpr Frame::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : pose_points_{},
        session_id_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        data_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        index_{0},
        flag_{0},
        width_{0},
        height_{0},
        type_{0},
        _cached_size_{0} {}
template <typename>
PROTOBUF_CONSTEXPR Frame::Frame(::_pbi::ConstantInitialized)
    : _impl_(::_pbi::ConstantInitialized()) {}
struct FrameDefaultTypeInternal {
  PROTOBUF_CONSTEXPR FrameDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~FrameDefaultTypeInternal() {}
  union {
    Frame _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 FrameDefaultTypeInternal _Frame_default_instance_;
}
}
}
static ::_pb::Metadata file_level_metadata_ksl_5fsentence_5frecognition_2eproto[3];
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_ksl_5fsentence_5frecognition_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_ksl_5fsentence_5frecognition_2eproto = nullptr;
const ::uint32_t TableStruct_ksl_5fsentence_5frecognition_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(
    protodesc_cold) = {
    ~0u,
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _internal_metadata_),
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _impl_.x_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _impl_.y_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _impl_.z_),
    ~0u,
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _internal_metadata_),
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.session_id_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.index_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.flag_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.width_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.height_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.type_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.data_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.pose_points_),
    ~0u,
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _internal_metadata_),
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    ~0u,
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _impl_.session_id_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _impl_.frame_count_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _impl_.message_),
};
static const ::_pbi::MigrationSchema
    schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
        {0, -1, -1, sizeof(::vision::raw::v1::Point3)},
        {11, -1, -1, sizeof(::vision::raw::v1::Frame)},
        {27, -1, -1, sizeof(::vision::raw::v1::SubmitResultResponse)},
};
static const ::_pb::Message* const file_default_instances[] = {
    &::vision::raw::v1::_Point3_default_instance_._instance,
    &::vision::raw::v1::_Frame_default_instance_._instance,
    &::vision::raw::v1::_SubmitResultResponse_default_instance_._instance,
};
const char descriptor_table_protodef_ksl_5fsentence_5frecognition_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
    "\n\036ksl_sentence_recognition.proto\022\rvision"
    ".raw.v1\")\n\006Point3\022\t\n\001x\030\001 \001(\002\022\t\n\001y\030\002 \001(\002\022"
    "\t\n\001z\030\003 \001(\002\"\237\001\n\005Frame\022\022\n\nsession_id\030\001 \001(\t"
    "\022\r\n\005index\030\002 \001(\005\022\014\n\004flag\030\003 \001(\005\022\r\n\005width\030\004"
    " \001(\005\022\016\n\006height\030\005 \001(\005\022\014\n\004type\030\006 \001(\005\022\014\n\004da"
    "ta\030\007 \001(\014\022*\n\013pose_points\030\010 \003(\0132\025.vision.r"
    "aw.v1.Point3\"P\n\024SubmitResultResponse\022\022\n\n"
    "session_id\030\001 \001(\t\022\023\n\013frame_count\030\002 \001(\005\022\017\n"
    "\007message\030\003 \001(\t2\\\n\017SequenceService\022I\n\nSen"
    "dFrames\022\024.vision.raw.v1.Frame\032#.vision.r"
    "aw.v1.SubmitResultResponse(\001b\006proto3"
};
static ::absl::once_flag descriptor_table_ksl_5fsentence_5frecognition_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_ksl_5fsentence_5frecognition_2eproto = {
    false,
    false,
    436,
    descriptor_table_protodef_ksl_5fsentence_5frecognition_2eproto,
    "ksl_sentence_recognition.proto",
    &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
    nullptr,
    0,
    3,
    schemas,
    file_default_instances,
    TableStruct_ksl_5fsentence_5frecognition_2eproto::offsets,
    file_level_metadata_ksl_5fsentence_5frecognition_2eproto,
    file_level_enum_descriptors_ksl_5fsentence_5frecognition_2eproto,
    file_level_service_descriptors_ksl_5fsentence_5frecognition_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter() {
  return &descriptor_table_ksl_5fsentence_5frecognition_2eproto;
}
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2
static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_ksl_5fsentence_5frecognition_2eproto(&descriptor_table_ksl_5fsentence_5frecognition_2eproto);
namespace vision {
namespace raw {
namespace v1 {
class Point3::_Internal {
 public:
};
Point3::Point3(::google::protobuf::Arena* arena)
    : ::google::protobuf::Message(arena) {
  SharedCtor(arena);
}
Point3::Point3(
    ::google::protobuf::Arena* arena, const Point3& from)
    : Point3(arena) {
  MergeFrom(from);
}
inline PROTOBUF_NDEBUG_INLINE Point3::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : _cached_size_{0} {}
inline void Point3::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  ::memset(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, x_),
           0,
           offsetof(Impl_, z_) -
               offsetof(Impl_, x_) +
               sizeof(Impl_::z_));
}
Point3::~Point3() {
  _internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  SharedDtor();
}
inline void Point3::SharedDtor() {
  ABSL_DCHECK(GetArena() == nullptr);
  _impl_.~Impl_();
}
PROTOBUF_NOINLINE void Point3::Clear() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  ::memset(&_impl_.x_, 0, static_cast<::size_t>(
      reinterpret_cast<char*>(&_impl_.z_) -
      reinterpret_cast<char*>(&_impl_.x_)) + sizeof(_impl_.z_));
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}
const char* Point3::_InternalParse(
    const char* ptr, ::_pbi::ParseContext* ctx) {
  ptr = ::_pbi::TcParser::ParseLoop(this, ptr, ctx, &_table_.header);
  return ptr;
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<2, 3, 0, 0, 2> Point3::_table_ = {
  {
    0,
    0,
    3, 24,
    offsetof(decltype(_table_), field_lookup_table),
    4294967288,
    offsetof(decltype(_table_), field_entries),
    3,
    0,
    offsetof(decltype(_table_), field_names),
    &_Point3_default_instance_._instance,
    ::_pbi::TcParser::GenericFallback,
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
    {::_pbi::TcParser::FastF32S1,
     {13, 63, 0, PROTOBUF_FIELD_OFFSET(Point3, _impl_.x_)}},
    {::_pbi::TcParser::FastF32S1,
     {21, 63, 0, PROTOBUF_FIELD_OFFSET(Point3, _impl_.y_)}},
    {::_pbi::TcParser::FastF32S1,
     {29, 63, 0, PROTOBUF_FIELD_OFFSET(Point3, _impl_.z_)}},
  }}, {{
    65535, 65535
  }}, {{
    {PROTOBUF_FIELD_OFFSET(Point3, _impl_.x_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kFloat)},
    {PROTOBUF_FIELD_OFFSET(Point3, _impl_.y_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kFloat)},
    {PROTOBUF_FIELD_OFFSET(Point3, _impl_.z_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kFloat)},
  }},
  {{
  }},
};
::uint8_t* Point3::_InternalSerialize(
    ::uint8_t* target,
    ::google::protobuf::io::EpsCopyOutputStream* stream) const {
  ::uint32_t cached_has_bits = 0;
  (void)cached_has_bits;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_x = this->_internal_x();
  ::uint32_t raw_x;
  memcpy(&raw_x, &tmp_x, sizeof(tmp_x));
  if (raw_x != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFloatToArray(
        1, this->_internal_x(), target);
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_y = this->_internal_y();
  ::uint32_t raw_y;
  memcpy(&raw_y, &tmp_y, sizeof(tmp_y));
  if (raw_y != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFloatToArray(
        2, this->_internal_y(), target);
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_z = this->_internal_z();
  ::uint32_t raw_z;
  memcpy(&raw_z, &tmp_z, sizeof(tmp_z));
  if (raw_z != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFloatToArray(
        3, this->_internal_z(), target);
  }
  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target =
        ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
            _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
  }
  return target;
}
::size_t Point3::ByteSizeLong() const {
  ::size_t total_size = 0;
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_x = this->_internal_x();
  ::uint32_t raw_x;
  memcpy(&raw_x, &tmp_x, sizeof(tmp_x));
  if (raw_x != 0) {
    total_size += 5;
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_y = this->_internal_y();
  ::uint32_t raw_y;
  memcpy(&raw_y, &tmp_y, sizeof(tmp_y));
  if (raw_y != 0) {
    total_size += 5;
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_z = this->_internal_z();
  ::uint32_t raw_z;
  memcpy(&raw_z, &tmp_z, sizeof(tmp_z));
  if (raw_z != 0) {
    total_size += 5;
  }
  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}
const ::google::protobuf::Message::ClassData Point3::_class_data_ = {
    Point3::MergeImpl,
    nullptr,
};
const ::google::protobuf::Message::ClassData* Point3::GetClassData() const {
  return &_class_data_;
}
void Point3::MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg) {
  auto* const _this = static_cast<Point3*>(&to_msg);
  auto& from = static_cast<const Point3&>(from_msg);
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_x = from._internal_x();
  ::uint32_t raw_x;
  memcpy(&raw_x, &tmp_x, sizeof(tmp_x));
  if (raw_x != 0) {
    _this->_internal_set_x(from._internal_x());
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_y = from._internal_y();
  ::uint32_t raw_y;
  memcpy(&raw_y, &tmp_y, sizeof(tmp_y));
  if (raw_y != 0) {
    _this->_internal_set_y(from._internal_y());
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_z = from._internal_z();
  ::uint32_t raw_z;
  memcpy(&raw_z, &tmp_z, sizeof(tmp_z));
  if (raw_z != 0) {
    _this->_internal_set_z(from._internal_z());
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}
void Point3::CopyFrom(const Point3& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}
PROTOBUF_NOINLINE bool Point3::IsInitialized() const {
  return true;
}
::_pbi::CachedSize* Point3::AccessCachedSize() const {
  return &_impl_._cached_size_;
}
void Point3::InternalSwap(Point3* PROTOBUF_RESTRICT other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::google::protobuf::internal::memswap<
      PROTOBUF_FIELD_OFFSET(Point3, _impl_.z_)
      + sizeof(Point3::_impl_.z_)
      - PROTOBUF_FIELD_OFFSET(Point3, _impl_.x_)>(
          reinterpret_cast<char*>(&_impl_.x_),
          reinterpret_cast<char*>(&other->_impl_.x_));
}
::google::protobuf::Metadata Point3::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter, &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
      file_level_metadata_ksl_5fsentence_5frecognition_2eproto[0]);
}
class Frame::_Internal {
 public:
};
Frame::Frame(::google::protobuf::Arena* arena)
    : ::google::protobuf::Message(arena) {
  SharedCtor(arena);
}
inline PROTOBUF_NDEBUG_INLINE Frame::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from)
      : pose_points_{visibility, arena, from.pose_points_},
        session_id_(arena, from.session_id_),
        data_(arena, from.data_),
        _cached_size_{0} {}
Frame::Frame(
    ::google::protobuf::Arena* arena,
    const Frame& from)
    : ::google::protobuf::Message(arena) {
  Frame* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_);
  ::memcpy(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, index_),
           reinterpret_cast<const char *>(&from._impl_) +
               offsetof(Impl_, index_),
           offsetof(Impl_, type_) -
               offsetof(Impl_, index_) +
               sizeof(Impl_::type_));
}
inline PROTOBUF_NDEBUG_INLINE Frame::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : pose_points_{visibility, arena},
        session_id_(arena),
        data_(arena),
        _cached_size_{0} {}
inline void Frame::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  ::memset(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, index_),
           0,
           offsetof(Impl_, type_) -
               offsetof(Impl_, index_) +
               sizeof(Impl_::type_));
}
Frame::~Frame() {
  _internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  SharedDtor();
}
inline void Frame::SharedDtor() {
  ABSL_DCHECK(GetArena() == nullptr);
  _impl_.session_id_.Destroy();
  _impl_.data_.Destroy();
  _impl_.~Impl_();
}
PROTOBUF_NOINLINE void Frame::Clear() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  _impl_.pose_points_.Clear();
  _impl_.session_id_.ClearToEmpty();
  _impl_.data_.ClearToEmpty();
  ::memset(&_impl_.index_, 0, static_cast<::size_t>(
      reinterpret_cast<char*>(&_impl_.type_) -
      reinterpret_cast<char*>(&_impl_.index_)) + sizeof(_impl_.type_));
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}
const char* Frame::_InternalParse(
    const char* ptr, ::_pbi::ParseContext* ctx) {
  ptr = ::_pbi::TcParser::ParseLoop(this, ptr, ctx, &_table_.header);
  return ptr;
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<3, 8, 1, 46, 2> Frame::_table_ = {
  {
    0,
    0,
    8, 56,
    offsetof(decltype(_table_), field_lookup_table),
    4294967040,
    offsetof(decltype(_table_), field_entries),
    8,
    1,
    offsetof(decltype(_table_), aux_entries),
    &_Frame_default_instance_._instance,
    ::_pbi::TcParser::GenericFallback,
  }, {{
    {::_pbi::TcParser::FastMtR1,
     {66, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.pose_points_)}},
    {::_pbi::TcParser::FastUS1,
     {10, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.session_id_)}},
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.index_), 63>(),
     {16, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.index_)}},
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.flag_), 63>(),
     {24, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.flag_)}},
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.width_), 63>(),
     {32, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.width_)}},
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.height_), 63>(),
     {40, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.height_)}},
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.type_), 63>(),
     {48, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.type_)}},
    {::_pbi::TcParser::FastBS1,
     {58, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.data_)}},
  }}, {{
    65535, 65535
  }}, {{
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.session_id_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.index_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.flag_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.width_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.height_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.type_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.data_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kBytes | ::_fl::kRepAString)},
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.pose_points_), 0, 0,
    (0 | ::_fl::kFcRepeated | ::_fl::kMessage | ::_fl::kTvTable)},
  }}, {{
    {::_pbi::TcParser::GetTable<::vision::raw::v1::Point3>()},
  }}, {{
    "\23\12\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "vision.raw.v1.Frame"
    "session_id"
  }},
};
::uint8_t* Frame::_InternalSerialize(
    ::uint8_t* target,
    ::google::protobuf::io::EpsCopyOutputStream* stream) const {
  ::uint32_t cached_has_bits = 0;
  (void)cached_has_bits;
  if (!this->_internal_session_id().empty()) {
    const std::string& _s = this->_internal_session_id();
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
        _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "vision.raw.v1.Frame.session_id");
    target = stream->WriteStringMaybeAliased(1, _s, target);
  }
  if (this->_internal_index() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<2>(
            stream, this->_internal_index(), target);
  }
  if (this->_internal_flag() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<3>(
            stream, this->_internal_flag(), target);
  }
  if (this->_internal_width() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<4>(
            stream, this->_internal_width(), target);
  }
  if (this->_internal_height() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<5>(
            stream, this->_internal_height(), target);
  }
  if (this->_internal_type() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<6>(
            stream, this->_internal_type(), target);
  }
  if (!this->_internal_data().empty()) {
    const std::string& _s = this->_internal_data();
    target = stream->WriteBytesMaybeAliased(7, _s, target);
  }
  for (unsigned i = 0,
      n = static_cast<unsigned>(this->_internal_pose_points_size()); i < n; i++) {
    const auto& repfield = this->_internal_pose_points().Get(i);
    target = ::google::protobuf::internal::WireFormatLite::
        InternalWriteMessage(8, repfield, repfield.GetCachedSize(), target, stream);
  }
  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target =
        ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
            _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
  }
  return target;
}
::size_t Frame::ByteSizeLong() const {
  ::size_t total_size = 0;
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  total_size += 1UL * this->_internal_pose_points_size();
  for (const auto& msg : this->_internal_pose_points()) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSize(msg);
  }
  if (!this->_internal_session_id().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                    this->_internal_session_id());
  }
  if (!this->_internal_data().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::BytesSize(
                                    this->_internal_data());
  }
  if (this->_internal_index() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_index());
  }
  if (this->_internal_flag() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_flag());
  }
  if (this->_internal_width() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_width());
  }
  if (this->_internal_height() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_height());
  }
  if (this->_internal_type() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_type());
  }
  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}
const ::google::protobuf::Message::ClassData Frame::_class_data_ = {
    Frame::MergeImpl,
    nullptr,
};
const ::google::protobuf::Message::ClassData* Frame::GetClassData() const {
  return &_class_data_;
}
void Frame::MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg) {
  auto* const _this = static_cast<Frame*>(&to_msg);
  auto& from = static_cast<const Frame&>(from_msg);
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  _this->_internal_mutable_pose_points()->MergeFrom(
      from._internal_pose_points());
  if (!from._internal_session_id().empty()) {
    _this->_internal_set_session_id(from._internal_session_id());
  }
  if (!from._internal_data().empty()) {
    _this->_internal_set_data(from._internal_data());
  }
  if (from._internal_index() != 0) {
    _this->_internal_set_index(from._internal_index());
  }
  if (from._internal_flag() != 0) {
    _this->_internal_set_flag(from._internal_flag());
  }
  if (from._internal_width() != 0) {
    _this->_internal_set_width(from._internal_width());
  }
  if (from._internal_height() != 0) {
    _this->_internal_set_height(from._internal_height());
  }
  if (from._internal_type() != 0) {
    _this->_internal_set_type(from._internal_type());
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}
void Frame::CopyFrom(const Frame& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}
PROTOBUF_NOINLINE bool Frame::IsInitialized() const {
  return true;
}
::_pbi::CachedSize* Frame::AccessCachedSize() const {
  return &_impl_._cached_size_;
}
void Frame::InternalSwap(Frame* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  _impl_.pose_points_.InternalSwap(&other->_impl_.pose_points_);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.session_id_, &other->_impl_.session_id_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.data_, &other->_impl_.data_, arena);
  ::google::protobuf::internal::memswap<
      PROTOBUF_FIELD_OFFSET(Frame, _impl_.type_)
      + sizeof(Frame::_impl_.type_)
      - PROTOBUF_FIELD_OFFSET(Frame, _impl_.index_)>(
          reinterpret_cast<char*>(&_impl_.index_),
          reinterpret_cast<char*>(&other->_impl_.index_));
}
::google::protobuf::Metadata Frame::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter, &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
      file_level_metadata_ksl_5fsentence_5frecognition_2eproto[1]);
}
class SubmitResultResponse::_Internal {
 public:
};
SubmitResultResponse::SubmitResultResponse(::google::protobuf::Arena* arena)
    : ::google::protobuf::Message(arena) {
  SharedCtor(arena);
}
inline PROTOBUF_NDEBUG_INLINE SubmitResultResponse::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from)
      : session_id_(arena, from.session_id_),
        message_(arena, from.message_),
        _cached_size_{0} {}
SubmitResultResponse::SubmitResultResponse(
    ::google::protobuf::Arena* arena,
    const SubmitResultResponse& from)
    : ::google::protobuf::Message(arena) {
  SubmitResultResponse* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_);
  _impl_.frame_count_ = from._impl_.frame_count_;
}
inline PROTOBUF_NDEBUG_INLINE SubmitResultResponse::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : session_id_(arena),
        message_(arena),
        _cached_size_{0} {}
inline void SubmitResultResponse::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  _impl_.frame_count_ = {};
}
SubmitResultResponse::~SubmitResultResponse() {
  _internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  SharedDtor();
}
inline void SubmitResultResponse::SharedDtor() {
  ABSL_DCHECK(GetArena() == nullptr);
  _impl_.session_id_.Destroy();
  _impl_.message_.Destroy();
  _impl_.~Impl_();
}
PROTOBUF_NOINLINE void SubmitResultResponse::Clear() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  _impl_.session_id_.ClearToEmpty();
  _impl_.message_.ClearToEmpty();
  _impl_.frame_count_ = 0;
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}
const char* SubmitResultResponse::_InternalParse(
    const char* ptr, ::_pbi::ParseContext* ctx) {
  ptr = ::_pbi::TcParser::ParseLoop(this, ptr, ctx, &_table_.header);
  return ptr;
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<2, 3, 0, 60, 2> SubmitResultResponse::_table_ = {
  {
    0,
    0,
    3, 24,
    offsetof(decltype(_table_), field_lookup_table),
    4294967288,
    offsetof(decltype(_table_), field_entries),
    3,
    0,
    offsetof(decltype(_table_), field_names),
    &_SubmitResultResponse_default_instance_._instance,
    ::_pbi::TcParser::GenericFallback,
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
    {::_pbi::TcParser::FastUS1,
     {10, 63, 0, PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.session_id_)}},
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(SubmitResultResponse, _impl_.frame_count_), 63>(),
     {16, 63, 0, PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.frame_count_)}},
    {::_pbi::TcParser::FastUS1,
     {26, 63, 0, PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.message_)}},
  }}, {{
    65535, 65535
  }}, {{
    {PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.session_id_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
    {PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.frame_count_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    {PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.message_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
  }},
  {{
    "\42\12\0\7\0\0\0\0"
    "vision.raw.v1.SubmitResultResponse"
    "session_id"
    "message"
  }},
};
::uint8_t* SubmitResultResponse::_InternalSerialize(
    ::uint8_t* target,
    ::google::protobuf::io::EpsCopyOutputStream* stream) const {
  ::uint32_t cached_has_bits = 0;
  (void)cached_has_bits;
  if (!this->_internal_session_id().empty()) {
    const std::string& _s = this->_internal_session_id();
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
        _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "vision.raw.v1.SubmitResultResponse.session_id");
    target = stream->WriteStringMaybeAliased(1, _s, target);
  }
  if (this->_internal_frame_count() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<2>(
            stream, this->_internal_frame_count(), target);
  }
  if (!this->_internal_message().empty()) {
    const std::string& _s = this->_internal_message();
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
        _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "vision.raw.v1.SubmitResultResponse.message");
    target = stream->WriteStringMaybeAliased(3, _s, target);
  }
  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target =
        ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
            _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
  }
  return target;
}
::size_t SubmitResultResponse::ByteSizeLong() const {
  ::size_t total_size = 0;
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  if (!this->_internal_session_id().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                    this->_internal_session_id());
  }
  if (!this->_internal_message().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                    this->_internal_message());
  }
  if (this->_internal_frame_count() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_frame_count());
  }
  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}
const ::google::protobuf::Message::ClassData SubmitResultResponse::_class_data_ = {
    SubmitResultResponse::MergeImpl,
    nullptr,
};
const ::google::protobuf::Message::ClassData* SubmitResultResponse::GetClassData() const {
  return &_class_data_;
}
void SubmitResultResponse::MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg) {
  auto* const _this = static_cast<SubmitResultResponse*>(&to_msg);
  auto& from = static_cast<const SubmitResultResponse&>(from_msg);
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;
  if (!from._internal_session_id().empty()) {
    _this->_internal_set_session_id(from._internal_session_id());
  }
  if (!from._internal_message().empty()) {
    _this->_internal_set_message(from._internal_message());
  }
  if (from._internal_frame_count() != 0) {
    _this->_internal_set_frame_count(from._internal_frame_count());
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}
void SubmitResultResponse::CopyFrom(const SubmitResultResponse& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}
PROTOBUF_NOINLINE bool SubmitResultResponse::IsInitialized() const {
  return true;
}
::_pbi::CachedSize* SubmitResultResponse::AccessCachedSize() const {
  return &_impl_._cached_size_;
}
void SubmitResultResponse::InternalSwap(SubmitResultResponse* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.session_id_, &other->_impl_.session_id_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.message_, &other->_impl_.message_, arena);
        swap(_impl_.frame_count_, other->_impl_.frame_count_);
}
::google::protobuf::Metadata SubmitResultResponse::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter, &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
      file_level_metadata_ksl_5fsentence_5frecognition_2eproto[2]);
}
}
}
}
namespace google {
namespace protobuf {
}
}
#include "google/protobuf/port_undef.inc"
```

## File: ksl_sentence_recognition.pb.h
```
#ifndef GOOGLE_PROTOBUF_INCLUDED_ksl_5fsentence_5frecognition_2eproto_2epb_2eh
#define GOOGLE_PROTOBUF_INCLUDED_ksl_5fsentence_5frecognition_2eproto_2epb_2eh
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include "google/protobuf/port_def.inc"
#if PROTOBUF_VERSION < 4025000
#error "This file was generated by a newer version of protoc which is"
#error "incompatible with your Protocol Buffer headers. Please update"
#error "your headers."
#endif
#if 4025001 < PROTOBUF_MIN_PROTOC_VERSION
#error "This file was generated by an older version of protoc which is"
#error "incompatible with your Protocol Buffer headers. Please"
#error "regenerate this file with a newer version of protoc."
#endif
#include "google/protobuf/port_undef.inc"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/message.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/port_def.inc"
#define PROTOBUF_INTERNAL_EXPORT_ksl_5fsentence_5frecognition_2eproto
namespace google {
namespace protobuf {
namespace internal {
class AnyMetadata;
}
}
}
struct TableStruct_ksl_5fsentence_5frecognition_2eproto {
  static const ::uint32_t offsets[];
};
extern const ::google::protobuf::internal::DescriptorTable
    descriptor_table_ksl_5fsentence_5frecognition_2eproto;
namespace vision {
namespace raw {
namespace v1 {
class Frame;
struct FrameDefaultTypeInternal;
extern FrameDefaultTypeInternal _Frame_default_instance_;
class Point3;
struct Point3DefaultTypeInternal;
extern Point3DefaultTypeInternal _Point3_default_instance_;
class SubmitResultResponse;
struct SubmitResultResponseDefaultTypeInternal;
extern SubmitResultResponseDefaultTypeInternal _SubmitResultResponse_default_instance_;
}
}
}
namespace google {
namespace protobuf {
}
}
namespace vision {
namespace raw {
namespace v1 {
class SubmitResultResponse final :
    public ::google::protobuf::Message  {
 public:
  inline SubmitResultResponse() : SubmitResultResponse(nullptr) {}
  ~SubmitResultResponse() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR SubmitResultResponse(::google::protobuf::internal::ConstantInitialized);
  inline SubmitResultResponse(const SubmitResultResponse& from)
      : SubmitResultResponse(nullptr, from) {}
  SubmitResultResponse(SubmitResultResponse&& from) noexcept
    : SubmitResultResponse() {
    *this = ::std::move(from);
  }
  inline SubmitResultResponse& operator=(const SubmitResultResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline SubmitResultResponse& operator=(SubmitResultResponse&& from) noexcept {
    if (this == &from) return *this;
    if (GetArena() == from.GetArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetArena() != nullptr
  #endif
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }
  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const SubmitResultResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const SubmitResultResponse* internal_default_instance() {
    return reinterpret_cast<const SubmitResultResponse*>(
               &_SubmitResultResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;
  friend void swap(SubmitResultResponse& a, SubmitResultResponse& b) {
    a.Swap(&b);
  }
  inline void Swap(SubmitResultResponse* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() != nullptr &&
        GetArena() == other->GetArena()) {
   #else
    if (GetArena() == other->GetArena()) {
  #endif
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(SubmitResultResponse* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }
  SubmitResultResponse* New(::google::protobuf::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<SubmitResultResponse>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const SubmitResultResponse& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom( const SubmitResultResponse& from) {
    SubmitResultResponse::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;
  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::google::protobuf::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }
  private:
  ::google::protobuf::internal::CachedSize* AccessCachedSize() const final;
  void SharedCtor(::google::protobuf::Arena* arena);
  void SharedDtor();
  void InternalSwap(SubmitResultResponse* other);
  private:
  friend class ::google::protobuf::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "vision.raw.v1.SubmitResultResponse";
  }
  protected:
  explicit SubmitResultResponse(::google::protobuf::Arena* arena);
  SubmitResultResponse(::google::protobuf::Arena* arena, const SubmitResultResponse& from);
  public:
  static const ClassData _class_data_;
  const ::google::protobuf::Message::ClassData*GetClassData() const final;
  ::google::protobuf::Metadata GetMetadata() const final;
  enum : int {
    kSessionIdFieldNumber = 1,
    kMessageFieldNumber = 3,
    kFrameCountFieldNumber = 2,
  };
  void clear_session_id() ;
  const std::string& session_id() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_session_id(Arg_&& arg, Args_... args);
  std::string* mutable_session_id();
  PROTOBUF_NODISCARD std::string* release_session_id();
  void set_allocated_session_id(std::string* value);
  private:
  const std::string& _internal_session_id() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_session_id(
      const std::string& value);
  std::string* _internal_mutable_session_id();
  public:
  void clear_message() ;
  const std::string& message() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_message(Arg_&& arg, Args_... args);
  std::string* mutable_message();
  PROTOBUF_NODISCARD std::string* release_message();
  void set_allocated_message(std::string* value);
  private:
  const std::string& _internal_message() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_message(
      const std::string& value);
  std::string* _internal_mutable_message();
  public:
  void clear_frame_count() ;
  ::int32_t frame_count() const;
  void set_frame_count(::int32_t value);
  private:
  ::int32_t _internal_frame_count() const;
  void _internal_set_frame_count(::int32_t value);
  public:
 private:
  class _Internal;
  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      2, 3, 0,
      60, 2>
      _table_;
  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {
        inline explicit constexpr Impl_(
            ::google::protobuf::internal::ConstantInitialized) noexcept;
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena);
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena, const Impl_& from);
    ::google::protobuf::internal::ArenaStringPtr session_id_;
    ::google::protobuf::internal::ArenaStringPtr message_;
    ::int32_t frame_count_;
    mutable ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ksl_5fsentence_5frecognition_2eproto;
};
class Point3 final :
    public ::google::protobuf::Message  {
 public:
  inline Point3() : Point3(nullptr) {}
  ~Point3() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR Point3(::google::protobuf::internal::ConstantInitialized);
  inline Point3(const Point3& from)
      : Point3(nullptr, from) {}
  Point3(Point3&& from) noexcept
    : Point3() {
    *this = ::std::move(from);
  }
  inline Point3& operator=(const Point3& from) {
    CopyFrom(from);
    return *this;
  }
  inline Point3& operator=(Point3&& from) noexcept {
    if (this == &from) return *this;
    if (GetArena() == from.GetArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetArena() != nullptr
  #endif
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }
  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const Point3& default_instance() {
    return *internal_default_instance();
  }
  static inline const Point3* internal_default_instance() {
    return reinterpret_cast<const Point3*>(
               &_Point3_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;
  friend void swap(Point3& a, Point3& b) {
    a.Swap(&b);
  }
  inline void Swap(Point3* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() != nullptr &&
        GetArena() == other->GetArena()) {
   #else
    if (GetArena() == other->GetArena()) {
  #endif
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Point3* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }
  Point3* New(::google::protobuf::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Point3>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const Point3& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom( const Point3& from) {
    Point3::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;
  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::google::protobuf::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }
  private:
  ::google::protobuf::internal::CachedSize* AccessCachedSize() const final;
  void SharedCtor(::google::protobuf::Arena* arena);
  void SharedDtor();
  void InternalSwap(Point3* other);
  private:
  friend class ::google::protobuf::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "vision.raw.v1.Point3";
  }
  protected:
  explicit Point3(::google::protobuf::Arena* arena);
  Point3(::google::protobuf::Arena* arena, const Point3& from);
  public:
  static const ClassData _class_data_;
  const ::google::protobuf::Message::ClassData*GetClassData() const final;
  ::google::protobuf::Metadata GetMetadata() const final;
  enum : int {
    kXFieldNumber = 1,
    kYFieldNumber = 2,
    kZFieldNumber = 3,
  };
  void clear_x() ;
  float x() const;
  void set_x(float value);
  private:
  float _internal_x() const;
  void _internal_set_x(float value);
  public:
  void clear_y() ;
  float y() const;
  void set_y(float value);
  private:
  float _internal_y() const;
  void _internal_set_y(float value);
  public:
  void clear_z() ;
  float z() const;
  void set_z(float value);
  private:
  float _internal_z() const;
  void _internal_set_z(float value);
  public:
 private:
  class _Internal;
  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      2, 3, 0,
      0, 2>
      _table_;
  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {
        inline explicit constexpr Impl_(
            ::google::protobuf::internal::ConstantInitialized) noexcept;
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena);
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena, const Impl_& from);
    float x_;
    float y_;
    float z_;
    mutable ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ksl_5fsentence_5frecognition_2eproto;
};
class Frame final :
    public ::google::protobuf::Message  {
 public:
  inline Frame() : Frame(nullptr) {}
  ~Frame() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR Frame(::google::protobuf::internal::ConstantInitialized);
  inline Frame(const Frame& from)
      : Frame(nullptr, from) {}
  Frame(Frame&& from) noexcept
    : Frame() {
    *this = ::std::move(from);
  }
  inline Frame& operator=(const Frame& from) {
    CopyFrom(from);
    return *this;
  }
  inline Frame& operator=(Frame&& from) noexcept {
    if (this == &from) return *this;
    if (GetArena() == from.GetArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetArena() != nullptr
  #endif
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }
  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const Frame& default_instance() {
    return *internal_default_instance();
  }
  static inline const Frame* internal_default_instance() {
    return reinterpret_cast<const Frame*>(
               &_Frame_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;
  friend void swap(Frame& a, Frame& b) {
    a.Swap(&b);
  }
  inline void Swap(Frame* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() != nullptr &&
        GetArena() == other->GetArena()) {
   #else
    if (GetArena() == other->GetArena()) {
  #endif
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Frame* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }
  Frame* New(::google::protobuf::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Frame>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const Frame& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom( const Frame& from) {
    Frame::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;
  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::google::protobuf::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }
  private:
  ::google::protobuf::internal::CachedSize* AccessCachedSize() const final;
  void SharedCtor(::google::protobuf::Arena* arena);
  void SharedDtor();
  void InternalSwap(Frame* other);
  private:
  friend class ::google::protobuf::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "vision.raw.v1.Frame";
  }
  protected:
  explicit Frame(::google::protobuf::Arena* arena);
  Frame(::google::protobuf::Arena* arena, const Frame& from);
  public:
  static const ClassData _class_data_;
  const ::google::protobuf::Message::ClassData*GetClassData() const final;
  ::google::protobuf::Metadata GetMetadata() const final;
  enum : int {
    kPosePointsFieldNumber = 8,
    kSessionIdFieldNumber = 1,
    kDataFieldNumber = 7,
    kIndexFieldNumber = 2,
    kFlagFieldNumber = 3,
    kWidthFieldNumber = 4,
    kHeightFieldNumber = 5,
    kTypeFieldNumber = 6,
  };
  int pose_points_size() const;
  private:
  int _internal_pose_points_size() const;
  public:
  void clear_pose_points() ;
  ::vision::raw::v1::Point3* mutable_pose_points(int index);
  ::google::protobuf::RepeatedPtrField< ::vision::raw::v1::Point3 >*
      mutable_pose_points();
  private:
  const ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>& _internal_pose_points() const;
  ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>* _internal_mutable_pose_points();
  public:
  const ::vision::raw::v1::Point3& pose_points(int index) const;
  ::vision::raw::v1::Point3* add_pose_points();
  const ::google::protobuf::RepeatedPtrField< ::vision::raw::v1::Point3 >&
      pose_points() const;
  void clear_session_id() ;
  const std::string& session_id() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_session_id(Arg_&& arg, Args_... args);
  std::string* mutable_session_id();
  PROTOBUF_NODISCARD std::string* release_session_id();
  void set_allocated_session_id(std::string* value);
  private:
  const std::string& _internal_session_id() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_session_id(
      const std::string& value);
  std::string* _internal_mutable_session_id();
  public:
  void clear_data() ;
  const std::string& data() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_data(Arg_&& arg, Args_... args);
  std::string* mutable_data();
  PROTOBUF_NODISCARD std::string* release_data();
  void set_allocated_data(std::string* value);
  private:
  const std::string& _internal_data() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_data(
      const std::string& value);
  std::string* _internal_mutable_data();
  public:
  void clear_index() ;
  ::int32_t index() const;
  void set_index(::int32_t value);
  private:
  ::int32_t _internal_index() const;
  void _internal_set_index(::int32_t value);
  public:
  void clear_flag() ;
  ::int32_t flag() const;
  void set_flag(::int32_t value);
  private:
  ::int32_t _internal_flag() const;
  void _internal_set_flag(::int32_t value);
  public:
  void clear_width() ;
  ::int32_t width() const;
  void set_width(::int32_t value);
  private:
  ::int32_t _internal_width() const;
  void _internal_set_width(::int32_t value);
  public:
  void clear_height() ;
  ::int32_t height() const;
  void set_height(::int32_t value);
  private:
  ::int32_t _internal_height() const;
  void _internal_set_height(::int32_t value);
  public:
  void clear_type() ;
  ::int32_t type() const;
  void set_type(::int32_t value);
  private:
  ::int32_t _internal_type() const;
  void _internal_set_type(::int32_t value);
  public:
 private:
  class _Internal;
  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      3, 8, 1,
      46, 2>
      _table_;
  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {
        inline explicit constexpr Impl_(
            ::google::protobuf::internal::ConstantInitialized) noexcept;
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena);
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena, const Impl_& from);
    ::google::protobuf::RepeatedPtrField< ::vision::raw::v1::Point3 > pose_points_;
    ::google::protobuf::internal::ArenaStringPtr session_id_;
    ::google::protobuf::internal::ArenaStringPtr data_;
    ::int32_t index_;
    ::int32_t flag_;
    ::int32_t width_;
    ::int32_t height_;
    ::int32_t type_;
    mutable ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ksl_5fsentence_5frecognition_2eproto;
};
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
inline void Point3::clear_x() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.x_ = 0;
}
inline float Point3::x() const {
  return _internal_x();
}
inline void Point3::set_x(float value) {
  _internal_set_x(value);
}
inline float Point3::_internal_x() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.x_;
}
inline void Point3::_internal_set_x(float value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.x_ = value;
}
inline void Point3::clear_y() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.y_ = 0;
}
inline float Point3::y() const {
  return _internal_y();
}
inline void Point3::set_y(float value) {
  _internal_set_y(value);
}
inline float Point3::_internal_y() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.y_;
}
inline void Point3::_internal_set_y(float value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.y_ = value;
}
inline void Point3::clear_z() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.z_ = 0;
}
inline float Point3::z() const {
  return _internal_z();
}
inline void Point3::set_z(float value) {
  _internal_set_z(value);
}
inline float Point3::_internal_z() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.z_;
}
inline void Point3::_internal_set_z(float value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.z_ = value;
}
inline void Frame::clear_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.ClearToEmpty();
}
inline const std::string& Frame::session_id() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  return _internal_session_id();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void Frame::set_session_id(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
}
inline std::string* Frame::mutable_session_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_session_id();
  return _s;
}
inline const std::string& Frame::_internal_session_id() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.session_id_.Get();
}
inline void Frame::_internal_set_session_id(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(value, GetArena());
}
inline std::string* Frame::_internal_mutable_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.session_id_.Mutable( GetArena());
}
inline std::string* Frame::release_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  return _impl_.session_id_.Release();
}
inline void Frame::set_allocated_session_id(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.session_id_.IsDefault()) {
          _impl_.session_id_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.Frame.session_id)
}
// int32 index = 2;
inline void Frame::clear_index() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.index_ = 0;
}
inline ::int32_t Frame::index() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.index)
  return _internal_index();
}
inline void Frame::set_index(::int32_t value) {
  _internal_set_index(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.index)
}
inline ::int32_t Frame::_internal_index() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.index_;
}
inline void Frame::_internal_set_index(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.index_ = value;
}
// int32 flag = 3;
inline void Frame::clear_flag() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.flag_ = 0;
}
inline ::int32_t Frame::flag() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.flag)
  return _internal_flag();
}
inline void Frame::set_flag(::int32_t value) {
  _internal_set_flag(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.flag)
}
inline ::int32_t Frame::_internal_flag() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.flag_;
}
inline void Frame::_internal_set_flag(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.flag_ = value;
}
// int32 width = 4;
inline void Frame::clear_width() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.width_ = 0;
}
inline ::int32_t Frame::width() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.width)
  return _internal_width();
}
inline void Frame::set_width(::int32_t value) {
  _internal_set_width(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.width)
}
inline ::int32_t Frame::_internal_width() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.width_;
}
inline void Frame::_internal_set_width(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.width_ = value;
}
// int32 height = 5;
inline void Frame::clear_height() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.height_ = 0;
}
inline ::int32_t Frame::height() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.height)
  return _internal_height();
}
inline void Frame::set_height(::int32_t value) {
  _internal_set_height(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.height)
}
inline ::int32_t Frame::_internal_height() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.height_;
}
inline void Frame::_internal_set_height(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.height_ = value;
}
// int32 type = 6;
inline void Frame::clear_type() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.type_ = 0;
}
inline ::int32_t Frame::type() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.type)
  return _internal_type();
}
inline void Frame::set_type(::int32_t value) {
  _internal_set_type(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.type)
}
inline ::int32_t Frame::_internal_type() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.type_;
}
inline void Frame::_internal_set_type(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.type_ = value;
}
// bytes data = 7;
inline void Frame::clear_data() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.data_.ClearToEmpty();
}
inline const std::string& Frame::data() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.data)
  return _internal_data();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void Frame::set_data(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.data_.SetBytes(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.data)
}
inline std::string* Frame::mutable_data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_data();
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.Frame.data)
  return _s;
}
inline const std::string& Frame::_internal_data() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.data_.Get();
}
inline void Frame::_internal_set_data(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.data_.Set(value, GetArena());
}
inline std::string* Frame::_internal_mutable_data() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.data_.Mutable( GetArena());
}
inline std::string* Frame::release_data() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  // @@protoc_insertion_point(field_release:vision.raw.v1.Frame.data)
  return _impl_.data_.Release();
}
inline void Frame::set_allocated_data(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.data_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.data_.IsDefault()) {
          _impl_.data_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.Frame.data)
}
// repeated .vision.raw.v1.Point3 pose_points = 8;
inline int Frame::_internal_pose_points_size() const {
  return _internal_pose_points().size();
}
inline int Frame::pose_points_size() const {
  return _internal_pose_points_size();
}
inline void Frame::clear_pose_points() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.pose_points_.Clear();
}
inline ::vision::raw::v1::Point3* Frame::mutable_pose_points(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.Frame.pose_points)
  return _internal_mutable_pose_points()->Mutable(index);
}
inline ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>* Frame::mutable_pose_points()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable_list:vision.raw.v1.Frame.pose_points)
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  return _internal_mutable_pose_points();
}
inline const ::vision::raw::v1::Point3& Frame::pose_points(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.pose_points)
  return _internal_pose_points().Get(index);
}
inline ::vision::raw::v1::Point3* Frame::add_pose_points() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::vision::raw::v1::Point3* _add = _internal_mutable_pose_points()->Add();
  // @@protoc_insertion_point(field_add:vision.raw.v1.Frame.pose_points)
  return _add;
}
inline const ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>& Frame::pose_points() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_list:vision.raw.v1.Frame.pose_points)
  return _internal_pose_points();
}
inline const ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>&
Frame::_internal_pose_points() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.pose_points_;
}
inline ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>*
Frame::_internal_mutable_pose_points() {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return &_impl_.pose_points_;
}
// -------------------------------------------------------------------
// SubmitResultResponse
// string session_id = 1;
inline void SubmitResultResponse::clear_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.ClearToEmpty();
}
inline const std::string& SubmitResultResponse::session_id() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.SubmitResultResponse.session_id)
  return _internal_session_id();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void SubmitResultResponse::set_session_id(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:vision.raw.v1.SubmitResultResponse.session_id)
}
inline std::string* SubmitResultResponse::mutable_session_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_session_id();
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.SubmitResultResponse.session_id)
  return _s;
}
inline const std::string& SubmitResultResponse::_internal_session_id() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.session_id_.Get();
}
inline void SubmitResultResponse::_internal_set_session_id(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(value, GetArena());
}
inline std::string* SubmitResultResponse::_internal_mutable_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.session_id_.Mutable( GetArena());
}
inline std::string* SubmitResultResponse::release_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  // @@protoc_insertion_point(field_release:vision.raw.v1.SubmitResultResponse.session_id)
  return _impl_.session_id_.Release();
}
inline void SubmitResultResponse::set_allocated_session_id(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.session_id_.IsDefault()) {
          _impl_.session_id_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.SubmitResultResponse.session_id)
}
// int32 frame_count = 2;
inline void SubmitResultResponse::clear_frame_count() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.frame_count_ = 0;
}
inline ::int32_t SubmitResultResponse::frame_count() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.SubmitResultResponse.frame_count)
  return _internal_frame_count();
}
inline void SubmitResultResponse::set_frame_count(::int32_t value) {
  _internal_set_frame_count(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.SubmitResultResponse.frame_count)
}
inline ::int32_t SubmitResultResponse::_internal_frame_count() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.frame_count_;
}
inline void SubmitResultResponse::_internal_set_frame_count(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.frame_count_ = value;
}
// string message = 3;
inline void SubmitResultResponse::clear_message() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.message_.ClearToEmpty();
}
inline const std::string& SubmitResultResponse::message() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.SubmitResultResponse.message)
  return _internal_message();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void SubmitResultResponse::set_message(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.message_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:vision.raw.v1.SubmitResultResponse.message)
}
inline std::string* SubmitResultResponse::mutable_message() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_message();
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.SubmitResultResponse.message)
  return _s;
}
inline const std::string& SubmitResultResponse::_internal_message() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.message_.Get();
}
inline void SubmitResultResponse::_internal_set_message(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.message_.Set(value, GetArena());
}
inline std::string* SubmitResultResponse::_internal_mutable_message() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.message_.Mutable( GetArena());
}
inline std::string* SubmitResultResponse::release_message() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  // @@protoc_insertion_point(field_release:vision.raw.v1.SubmitResultResponse.message)
  return _impl_.message_.Release();
}
inline void SubmitResultResponse::set_allocated_message(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.message_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.message_.IsDefault()) {
          _impl_.message_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.SubmitResultResponse.message)
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__
// @@protoc_insertion_point(namespace_scope)
}  // namespace v1
}  // namespace raw
}  // namespace vision
// @@protoc_insertion_point(global_scope)
#include "google/protobuf/port_undef.inc"
#endif
```

## File: ksl_sentence_recognition.proto
```protobuf
syntax = "proto3";
package vision.raw.v1;

service SequenceService {
  rpc SendFrames (stream Frame) returns (SubmitResultResponse);
}

message Point3 {
  float x = 1;
  float y = 2;
  float z = 3;
}

message Frame {
  string session_id = 1;
  int32 index = 2;
  int32 flag = 3; // 0: START, 1: NORMAL, 2: END
  int32 width = 4;
  int32 height = 5;
  int32 type = 6;
  bytes data = 7;
  repeated Point3 pose_points = 8;
}

message SubmitResultResponse {
  string session_id = 1;
  int32 frame_count = 2;
  string message = 3;
}
```

## File: KSLDBManager.cpp
```cpp
#include "pch.h"
#include "framework.h"
#include "KSLDBManager.h"
#include "KSLDBManagerDlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
BEGIN_MESSAGE_MAP(CKSLDBManagerApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()
CKSLDBManagerApp::CKSLDBManagerApp()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}
CKSLDBManagerApp theApp;
BOOL CKSLDBManagerApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	CWinApp::InitInstance();
	AfxEnableControlContainer();
	CShellManager *pShellManager = new CShellManager;
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	SetRegistryKey(_T("로컬 애플리케이션 마법사에서 생성된 애플리케이션"));
	CKSLDBManagerDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "경고: 대화 상자를 만들지 못했으므로 애플리케이션이 예기치 않게 종료됩니다.\n");
		TRACE(traceAppMsg, 0, "경고: 대화 상자에서 MFC 컨트롤을 사용하는 경우 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS를 수행할 수 없습니다.\n");
	}
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif
	return FALSE;
}
```

## File: KSLDBManager.h
```
#pragma once
#ifndef __AFXWIN_H__
	#error "PCH에 대해 이 파일을 포함하기 전에 'pch.h'를 포함합니다."
#endif
#include "resource.h"
class CKSLDBManagerApp : public CWinApp
{
public:
	CKSLDBManagerApp();
public:
	virtual BOOL InitInstance();
	DECLARE_MESSAGE_MAP()
};
extern CKSLDBManagerApp theApp;
```

## File: KSLDBManagerDlg.cpp
```cpp
#include "framework.h"
#include "KSLDBManager.h"
#include "KSLDBManagerDlg.h"
#include "afxdialogex.h"
#include <iostream>
#include <fstream>
#include <io.h>
#include "HandShapeDiff.hpp"
#include "gRPCThread.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
UINT CKSLDBManagerDlg::AIThread(LPVOID pParam)
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
								pDlg->DetectionResultParsingNofaces(roi_stdVec_Hand_acr_Info1, roi_l_hand1, roi_r_hand1);
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
								pDlg->DetectionResultParsingNofaces(roi_stdVec_Hand_acr_Info2, roi_l_hand2, roi_r_hand2);
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
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
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
CKSLDBManagerDlg::CKSLDBManagerDlg(CWnd* pParent )
	: CDialogEx(IDD_KSLDBMANAGER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	AfxInitRichEdit2();
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
ON_BN_CLICKED(IDOK, &CKSLDBManagerDlg::OnBnClickedOk)
END_MESSAGE_MAP()
BOOL CKSLDBManagerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
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
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
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
	return TRUE;
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
void CKSLDBManagerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}
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
	}
}
int CKSLDBManagerDlg::GetRoughHandStatusFromMP(_cap_data cap_data)
{
	int status = -1;
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
	int status = -1;
	if (mp_pose.size() <= 0) return -1;
	auto hl = GetRefHL2DPointsMP(mp_pose);
	if (hl[1].y <= READY_LOCATION && hl[0].y <= READY_LOCATION) status = 3;
	else if (hl[1].y <= READY_LOCATION && hl[0].y > READY_LOCATION) status = 1;
	else if (hl[1].y > READY_LOCATION && hl[0].y <= READY_LOCATION) status = 2;
	else status = 0;
	return status;
}
void CKSLDBManagerDlg::OnBnClickedOk()
{
	CRect rect;
	GetWindowRect(&rect);
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
	float sign = 1.0;
	if (type == 1) sign = -1.0;
	cv::Mat R = (cv::Mat_<double>(3, 3) <<
		sign*orientation[0 * 3 + 0], sign*orientation[0 * 3 + 1], sign*orientation[0 * 3 + 2],
		orientation[1 * 3 + 0], orientation[1* 3 + 1], orientation[1 * 3 + 2],
		orientation[2* 3 + 0], orientation[2 * 3 + 1], orientation[2* 3 + 2]);
	cv::Mat rvec;
	cv::Rodrigues(R, rvec);
	for (int k = 0;k < 3;k++) axis_angle.push_back(-rvec.at<double>(k, 0));
	return axis_angle;
}
void CKSLDBManagerDlg::DetectionResultParsing(std::vector<std::vector<std::vector <double>>> stdVec_Hand_acr_Info, Result& l_hand, Result& r_hand)
{
	l_hand.conf = 0.0;
	r_hand.conf = 0.0;
	l_hand.jpts.clear();
	r_hand.jpts.clear();
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
		if ((int)stdVec_Hand_acr_Info[k + 7][0][0] == 0)
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
		}
		else
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
		}
	}
}
void CKSLDBManagerDlg::DetectionResultParsingNofaces(std::vector<std::vector<std::vector <double>>> stdVec_Hand_acr_Info, Result& l_hand, Result& r_hand)
{
	l_hand.conf = 0.0;
	r_hand.conf = 0.0;
	l_hand.jpts.clear();
	r_hand.jpts.clear();
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
		if ((int)stdVec_Hand_acr_Info[k + 5][0][0] == 0)
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
		}
		else
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
		}
	}
}
cv::Rect CKSLDBManagerDlg::GetCropSquareRect(cv::Mat img, std::vector<cv::Point2i> jpts, float scale)
{
	if (jpts.empty()) return cv::Rect(0, 0, img.cols, img.rows);
	cv::Rect hand_rect = cv::boundingRect(jpts);
	int std_size = std::max(hand_rect.width, hand_rect.height);
	int crop_size = static_cast<int>(std_size * scale);
	cv::Point center(
		hand_rect.x + hand_rect.width / 2,
		hand_rect.y + hand_rect.height / 2
	);
	int x = center.x - crop_size / 2;
	int y = center.y - crop_size / 2;
	cv::Rect rect(x, y, crop_size, crop_size);
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
std::vector<Point2f> CKSLDBManagerDlg::GetRefHL2DPointsMP(std::vector <cv::Point3f> joints)
{
	std::vector<Point2f> ref_hl_points;
	if (joints.size() <= 0) return ref_hl_points;
	std::vector<Point2f> pixels;
	for (int k = 0; k < joints.size(); k++)
	{
		Point2f pt;
		pt.x = joints[k].x * 1920.0;
		pt.y = joints[k].y * 1080.0;
		pixels.push_back(pt);
	}
	vector<Point2f> corners = { pixels[12] , pixels[11] ,pixels[0] };
	Mat trans = cv::getAffineTransform(corners, m_warpCorners);
	vector<Point2f> hl_point = { pixels[15],pixels[16] };
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
	auto hl_db = GetRefHL2DPointsMP(cp_db.sgcp_vtSkeletonMP);
	auto hl_query = GetRefHL2DPointsMP(query.sgcp_vtSkeletonMP);
	double scale = m_util.Distance(cv::Point2f(0, 0), cv::Point2f(1920, 1080)) / 4.0;
	if (cp_db.sgcp_vtHandStatus == 3 && hl_db.size() == 2 && query.sgcp_vtHandStatus == 3 && hl_query.size() == 2)
	{
		auto simL = m_util.Distance(hl_db[0], hl_query[0]) / scale;
		auto simR = m_util.Distance(hl_db[1], hl_query[1]) / scale;
		sim_val.push_back(1.0 - simL);
		sim_val.push_back(1.0 - simR);
	}
	else if (cp_db.sgcp_vtHandStatus == 1 && hl_db.size() == 2 && query.sgcp_vtHandStatus == 1 && hl_query.size() == 2)
	{
		auto simL = m_util.Distance(hl_db[0], hl_query[0]) / scale;
		auto simR = m_util.Distance(hl_db[1], hl_query[1]) / scale;
		sim_val.push_back(-1.0);
		sim_val.push_back(1.0 - simR);
	}
	else if (cp_db.sgcp_vtHandStatus == 2 && hl_db.size() == 2 && query.sgcp_vtHandStatus == 2 && hl_query.size() == 2)
	{
		auto simL = m_util.Distance(hl_db[0], hl_query[0]) / scale;
		auto simR = m_util.Distance(hl_db[1], hl_query[1]) / scale;
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
			hs_simR = HandShapeDiff::getShapeDifference(cp1.sgcp_vtDominantPCACompoent, cp2.sgcp_vtDominantPCACompoent);
		if (cp2.sgcp_vtRightHandOrientation.size() > 0 && (cp2.sgcp_vtHandStatus == 1 || cp2.sgcp_vtHandStatus == 3))
			ho_simR = 1.0 - fabs(m_util.getRotationAngle(cp1.sgcp_vtRightHandOrientation, cp2.sgcp_vtRightHandOrientation) / CV_PI);
		if (cp2.sgcp_vtSkeletonMP.size() > 0)
			hl_simR = SimHandLocation(cp1, cp2);
	}
	if ((cp1.sgcp_vtHandStatus == 3 && cp2.sgcp_vtHandStatus == 3) || (cp1.sgcp_vtHandStatus == 2 && cp2.sgcp_vtHandStatus == 2))
	{
		if (cp2.sgcp_vtNondominantPCACompoent.size() > 0 && (cp2.sgcp_vtHandStatus == 2 || cp2.sgcp_vtHandStatus == 3))
			hs_simL = HandShapeDiff::getShapeDifference(cp1.sgcp_vtNondominantPCACompoent, cp2.sgcp_vtNondominantPCACompoent);
		if (cp2.sgcp_vtLeftHandOrientation.size() > 0 && (cp2.sgcp_vtHandStatus == 2 || cp2.sgcp_vtHandStatus == 3))
			ho_simL = 1.0 - fabs(m_util.getRotationAngle(cp1.sgcp_vtLeftHandOrientation, cp2.sgcp_vtLeftHandOrientation) / CV_PI);
		if (cp2.sgcp_vtSkeletonMP.size() > 0)
			hl_simL = SimHandLocation(cp1, cp2);
	}
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
	std::vector<double> hsR, hoR, hlR;
	std::vector<double> hsL, hoL, hlL;
	hsR.reserve(cadidate_cps.size()); hoR.reserve(cadidate_cps.size()); hlR.reserve(cadidate_cps.size());
	hsL.reserve(cadidate_cps.size()); hoL.reserve(cadidate_cps.size()); hlL.reserve(cadidate_cps.size());
	for (const auto& cand_cp : cadidate_cps)
		GetSimsFromCP(ref_cp, cand_cp, hsR, hoR, hlR, hsL, hoL, hlL);
	if (!(ref_cp.sgcp_vtHandStatus == 1 || ref_cp.sgcp_vtHandStatus == 2 || ref_cp.sgcp_vtHandStatus == 3))
		return result_cps;
	const size_t N = cadidate_cps.size();
	std::vector<double> score(N, -std::numeric_limits<double>::infinity());
	auto safe = [](double v) {
		return (std::isfinite(v) ? v : 0.0);
		};
	for (size_t i = 0; i < N; ++i)
	{
		if (ref_cp.sgcp_vtHandStatus == 3) {
			score[i] =
				HS_WEIGHT * (safe(hsR[i]) + safe(hsL[i])) +
				HO_WEIGHT * (safe(hoR[i]) + safe(hoL[i])) +
				HL_WEIGHT * (safe(hlR[i]) + safe(hlL[i]));
		}
		else if (ref_cp.sgcp_vtHandStatus == 1) {
			score[i] =
				HS_WEIGHT * safe(hsR[i]) +
				HO_WEIGHT * safe(hoR[i]) +
				HL_WEIGHT * safe(hlR[i]);
		}
		else {
			score[i] =
				HS_WEIGHT * safe(hsL[i]) +
				HO_WEIGHT * safe(hoL[i]) +
				HL_WEIGHT * safe(hlL[i]);
		}
	}
	std::vector<int> ord(N);
	std::iota(ord.begin(), ord.end(), 0);
	std::stable_sort(ord.begin(), ord.end(), [&](int a, int b) {
		if (score[a] == score[b]) {
			return a < b;
		}
		return score[a] > score[b];
		});
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
```

## File: KSLDBManagerDlg.h
```
#pragma once
#include "ETRI_KSL_Excel_DB-v1.4.h"
#include "VideoUtil-v1.1.h"
#include <OpenXLSX.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <limits>
#include <python.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <atomic>
#include <Windows.h>
#include "ChangePointDetector.h"
#include <afxrich.h>
#include <chrono>
#include <opencv2/cudaoptflow.hpp>
#include "InferSentence.h"
#define _NONE_                      0
#define AI_MODE_HAND_DETECT_NEW_DB  7
#define READY_LOCATION              700
#define READY_HANDTIP_ANGLE         70.0
#define READY_ELBOW_DIRECTION_ANGLE 80
#define READY_ELBOW_ANGLE           90
#define RESET_MOTION_STATUS         -1
#define READY_MOTION_STATUS         0
#define SPEAK_MOTION_STATUS         1
#define RAPID_MOTION_STATUS         2
#define MIN_FRAME                   20
#define RAPID_DISTANCE              0.1
#define OPTICALFLOW_THRESH          0.45
#define OPTICAL_FLOW_HOLD_FRAME     3
#define SIMILAR_FRAME_NUM           3
#define HS_WEIGHT                   0.8
#define HO_WEIGHT                   0.7
#define HL_WEIGHT                   0.4
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
namespace pybind11 {
    namespace detail {
        template<>
        struct type_caster<cv::Mat> {
        public:
            PYBIND11_TYPE_CASTER(cv::Mat, _("numpy.ndarray"));
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
                value = cv::Mat(nh, nw, dtype, info.ptr);
                return true;
            }
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
                    bufferdim = { (size_t)nh, (size_t)nw };
                    strides = { elemsize * (size_t)nw, elemsize };
                }
                else if (dim == 3) {
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
}
using namespace std;
using namespace OpenXLSX;
namespace py = pybind11;
class CKSLDBManagerDlg : public CDialogEx
{
public:
    CKSLDBManagerDlg(CWnd* pParent = nullptr);
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_KSLDBMANAGER_DIALOG };
#endif
private:
    double  x_scale = (1920.0 / 1280.0);
    double  y_scale = (1080.0 / 720.0);
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
    vector<Point2f> m_warpCorners = {
        Point2f(720, 500),
        Point2f(1200, 500),
        Point2f(960, 400)
    };
    void UpdateHandLocationDB();
    std::vector<double> RotationMatrixToAxisAngle(std::vector<float> orientation,
        int type);
    void SetPoseOrientationForManoModel(
        std::vector<float>& pose,
        std::vector<Point3f>& orientation,
        std::vector<float> sgcp_vtHandOrientation,
        std::vector<float> sgcp_vtPCACompoent,
        int type);
    double GetDegreeAngleOfRHandTipDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfLHandTipDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfRElbowDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfLElbowDirection(_typeChangepointDB cp);
    double GetDegreeAngleOfLElbowAngle(_typeChangepointDB cp);
    double GetDegreeAngleOfRElbowAngle(_typeChangepointDB cp);
    void GetMPHandPointsFromPose(std::vector<Point3f> skeleton,
        cv::Mat img,
        std::vector<cv::Point>& LHand,
        std::vector<cv::Point>& RHand);
    CString GetHandStateString(int hand_state);
    void GetSimsFromCP(
        _tagChangepointDB cp1, _tagChangepointDB cp2,
        std::vector<double>& hsR, std::vector<double>& hoR, std::vector<double>& hlR,
        std::vector<double>& hsL, std::vector<double>& hoL, std::vector<double>& hlL
    );
    std::vector<double> SimHandLocation(_typeChangepointDB cp_db,
        _typeChangepointDB query);
    std::atomic<bool> m_exit;
    static UINT AIThread(LPVOID pParam);
    void DetectionResultParsing(
        std::vector<std::vector<std::vector<double>>> stdVec_Hand_acr_Info,
        Result& l_hand,
        Result& r_hand);
    void DetectionResultParsingNofaces(std::vector<std::vector<std::vector <double>>> stdVec_Hand_acr_Info, Result& l_hand, Result& r_hand);
    std::vector<GlossInfered> RemoveTransientHandState(
        std::vector<GlossInfered> infered);
    cv::Rect GetCropSquareRect(cv::Mat img,
        std::vector<cv::Point2i> jpts,
        float scale);
    void GetCPFormACRResult(
        Result roi_l_hand1s,
        Result roi_r_hand2s,
        RECT l_hand_roi,
        RECT r_hand_roi);
    std::vector<Point2f> GetRefHL2DPointsMP(
        std::vector<cv::Point3f> joints);
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
    int GetRoughHandStatusFromMP(_cap_data cap_data);
    int GetRoughHandStatusFromMP(std::vector<Point3f> mp_pose);
public:
    CVideoUtil m_util;
    CStatic m_ROIcctv_frame;
    Excel_DB   m_DB;
    cv::Mat m_cap_img;
    _typeChangepointDB  m_result;
    int m_ai_mode = _NONE_;
    HANDLE      hAIStart;
    HANDLE      hAIFinish;
    CWinThread* hAIThread = nullptr;
    int GetHandStatusFromCP(_typeChangepointDB cp);
    std::vector<_tagChangepointDB> FindSimilarCPs(
        _tagChangepointDB ref_cp,
        std::vector<_tagChangepointDB> cadidate_cps);
    std::vector<GlossInfered> m_infered;
    InferSentence m_infer_sentence;
    CEdit   m_info_edit;
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    HICON m_hIcon;
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedOk();
    DECLARE_MESSAGE_MAP()
public:
};
```

## File: ManoPoseDiff.cpp
```cpp
#include <vector>
#include <array>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <numeric>
#include <opencv2/core.hpp>
#include "ManoPoseDiff.h"
namespace shapediff {
    static inline double _norm3(const cv::Point3f& v) {
        return std::sqrt((double)v.x * v.x + (double)v.y * v.y + (double)v.z * v.z);
    }
    static inline double _dot3(const cv::Point3f& a, const cv::Point3f& b) {
        return (double)a.x * b.x + (double)a.y * b.y + (double)a.z * b.z;
    }
    static inline double _angle_deg(const cv::Point3f& a, const cv::Point3f& b, double eps = 1e-8) {
        double na = _norm3(a), nb = _norm3(b);
        if (na < eps || nb < eps) return 0.0;
        double c = _dot3(a, b) / (na * nb);
        c = std::max(-1.0, std::min(1.0, c));
        return std::acos(c) * 180.0 / CV_PI;
    }
    static inline cv::Point3f _vec(const std::vector<cv::Point3f>& P, int i, int j) {
        return { P[j].x - P[i].x, P[j].y - P[i].y, P[j].z - P[i].z };
    }
    double ManoPoseL2(const std::vector<float>& V1, const std::vector<float>& V2) {
        if (V1.size() != V2.size() || V1.empty()) return DBL_MAX;
        double s = 0.0;
        for (size_t i = 0; i < V1.size(); ++i) {
            const double d = (double)V1[i] - (double)V2[i];
            s += d * d;
        }
        return std::sqrt(s)/ V1.size();
    }
    FingerDiff PerFingerAngleDiff3D(const std::vector<cv::Point3f>& A,
        const std::vector<cv::Point3f>& B)
    {
        FingerDiff D;
        if (A.size() < 21 || B.size() < 21) return D;
        auto avg4 = [&](int a0, int a1, int a2, int a3, int a4)->double {
            cv::Point3f a01 = _vec(A, a0, a1), b01 = _vec(B, a0, a1);
            cv::Point3f a12 = _vec(A, a1, a2), b12 = _vec(B, a1, a2);
            cv::Point3f a23 = _vec(A, a2, a3), b23 = _vec(B, a2, a3);
            cv::Point3f a34 = _vec(A, a3, a4), b34 = _vec(B, a3, a4);
            const double d01 = _angle_deg(a01, b01);
            const double d12 = _angle_deg(a12, b12);
            const double d23 = _angle_deg(a23, b23);
            const double d34 = _angle_deg(a34, b34);
            return (d01 + d12 + d23 + d34) / 4.0;
            };
        D.T = avg4(0, 1, 2, 3, 4);
        D.I = avg4(0, 5, 6, 7, 8);
        D.M = avg4(0, 9, 10, 11, 12);
        D.R = avg4(0, 13, 14, 15, 16);
        D.P = avg4(0, 17, 18, 19, 20);
        return D;
    }
    static inline std::array<double, 5> _softmaxWeights(const FingerDiff& d, const Params& prm) {
        const double v[5] = { d.T,d.I,d.M,d.R,d.P };
        const double vmax = *std::max_element(v, v + 5);
        std::array<double, 5> w{};
        double denom = 0.0;
        for (int k = 0; k < 5; ++k) {
            const double z = (v[k] - vmax) / std::max(1e-8, prm.tau);
            const double e = prm.user_w[k] * std::exp(prm.alpha * z);
            w[k] = e; denom += e;
        }
        for (int k = 0; k < 5; ++k) w[k] /= std::max(1e-12, denom);
        return w;
    }
    static inline double _fingerWeightedScore(const FingerDiff& d, const std::array<double, 5>& w) {
        const double v[5] = { d.T,d.I,d.M,d.R,d.P };
        double s = 0.0;
        for (int k = 0; k < 5; ++k) s += w[k] * v[k] * v[k];
        return std::sqrt(s)/5;
    }
    double FinalScore3D(const std::vector<float>& poseA,
        const std::vector<float>& poseB,
        const std::vector<cv::Point3f>& jointsA,
        const std::vector<cv::Point3f>& jointsB,
        const Params& prm)
    {
        if (poseA.size() != poseB.size()) return DBL_MAX;
        const double d_pca = ManoPoseL2(poseA, poseB);
        if (!std::isfinite(d_pca)) return DBL_MAX;
        const FingerDiff d = PerFingerAngleDiff3D(jointsA, jointsB);
        const auto w = _softmaxWeights(d, prm);
        const double d_f = _fingerWeightedScore(d, w);
        if (!std::isfinite(d_f)) return DBL_MAX;
        const double vals[5] = { d.T,d.I,d.M,d.R,d.P };
        const double vmax = *std::max_element(vals, vals + 5);
        const double mean = (d.T + d.I + d.M + d.R + d.P) / 5.0;
        const double booster = 1.0 + prm.pca_boost_k * (vmax / std::max(prm.eps, mean));
        const double lambda = std::max(0.0, std::min(1.0, prm.lambda));
        return lambda * (d_pca * booster) + (1.0 - lambda) * d_f;
    }
    double FinalScore2D(const std::vector<float>& poseA,
        const std::vector<float>& poseB,
        const std::vector<cv::Point2f>& A2,
        const std::vector<cv::Point2f>& B2,
        const Params& prm)
    {
        if (A2.size() < 21 || B2.size() < 21) return DBL_MAX;
        std::vector<cv::Point3f> A3; A3.reserve(A2.size());
        std::vector<cv::Point3f> B3; B3.reserve(B2.size());
        for (auto& p : A2) A3.emplace_back(p.x, p.y, 0.0f);
        for (auto& p : B2) B3.emplace_back(p.x, p.y, 0.0f);
        return FinalScore3D(poseA, poseB, A3, B3, prm);
    }
}
```

## File: ManoPoseDiff.h
```
#pragma once
#include <vector>
#include <array>
#include <opencv2/core.hpp>
namespace shapediff {
    struct Params {
        double lambda = 0.4;
        double tau = 0.25;
        double alpha = 1.0;
        double eps = 1e-8;
        std::array<double, 5> user_w{ 1,1,1,1,1 };
        double pca_boost_k = 0.5;
    };
    double FinalScore3D(const std::vector<float>& poseA,
        const std::vector<float>& poseB,
        const std::vector<cv::Point3f>& jointsA,
        const std::vector<cv::Point3f>& jointsB,
        const Params& prm = {});
    double FinalScore2D(const std::vector<float>& poseA,
        const std::vector<float>& poseB,
        const std::vector<cv::Point2f>& jointsA2D,
        const std::vector<cv::Point2f>& jointsB2D,
        const Params& prm = {});
    double ManoPoseL2(const std::vector<float>& V1, const std::vector<float>& V2);
    struct FingerDiff { double T = 0, I = 0, M = 0, R = 0, P = 0; };
    FingerDiff PerFingerAngleDiff3D(const std::vector<cv::Point3f>& A,
        const std::vector<cv::Point3f>& B);
}
```

## File: pch.cpp
```cpp
#include "pch.h"
```

## File: pch.h
```
#ifndef PCH_H
#define PCH_H
#include "framework.h"
#endif
```

## File: resource.h
```
#define IDM_ABOUTBOX                    0x0010
#define IDD_ABOUTBOX                    100
#define IDS_ABOUTBOX                    101
#define IDD_KSLDBMANAGER_DIALOG         102
#define IDR_MAINFRAME                   128
#define IDD_DIALOG1                     130
#define IDD_GL_DIALOG                   130
#define IDC_BUTTON1                     1000
#define IDC_BUTTON2                     1001
#define IDC_EDIT1                       1002
#define IDC_PROGRESS1                   1003
#define IDC_EDIT2                       1004
#define IDC_BUTTON3                     1005
#define IDC_EDIT3                       1006
#define IDC_LIST1                       1007
#define IDC_EDIT4                       1008
#define IDC_BUTTON4                     1009
#define IDC_BUTTON5                     1010
#define IDC_VIDEO                       1011
#define IDC_BUTTON6                     1012
#define IDC_BUTTON7                     1013
#define IDC_BUTTON8                     1014
#define IDC_BUTTON9                     1015
#define IDC_BUTTON10                    1016
#define IDC_BUTTON11                    1017
#define IDC_BUTTON12                    1018
#define IDC_EDIT5                       1019
#define IDC_BUTTON13                    1020
#define IDC_VIDEO2                      1021
#define IDC_VIDEO3                      1022
#define IDC_VIDEO_ILL                   1023
#define IDC_MKV_VIDEO                   1024
#define IDC_BUTTON14                    1025
#define IDC_BUTTON15                    1026
#define IDC_BUTTON16                    1027
#define IDC_EDIT6                       1028
#define IDC_EDIT7                       1029
#define IDC_BUTTON17                    1030
#define IDC_BUTTON18                    1031
#define IDC_BUTTON19                    1032
#define IDC_BUTTON20                    1033
#define IDC_BUTTON21                    1034
#define IDC_EDIT8                       1035
#define IDC_BUTTON22                    1036
#define IDC_EDIT9                       1037
#define IDC_BUTTON23                    1038
#define IDC_BUTTON24                    1039
#define IDC_BUTTON25                    1040
#define IDC_BUTTON26                    1041
#define IDC_BUTTON27                    1042
#define IDC_BUTTON28                    1043
#define IDC_EDIT10                      1044
#define IDC_BUTTON29                    1045
#define IDC_SLIDER1                     1048
#define IDC_BUTTON30                    1049
#define IDC_EDIT11                      1050
#define IDC_BUTTON31                    1051
#define IDC_COMBO1                      1052
#define IDC_COMBO2                      1053
#define IDC_COMBO3                      1054
#define IDC_COMBO4                      1056
#define IDC_COMBO5                      1057
#define IDC_BUTTON32                    1058
#define IDC_EDIT12                      1059
#define IDC_EDIT13                      1060
#define IDC_EDIT14                      1061
#define IDC_EDIT15                      1063
#define IDC_BUTTON33                    1064
#define IDC_K_CPS                       1065
#define IDC_BUTTON34                    1066
#define IDC_BUTTON35                    1067
#define IDC_BUTTON36                    1068
#define IDC_BUTTON37                    1069
#define IDC_BUTTON38                    1070
#define IDC_BUTTON39                    1071
#define IDC_BUTTON40                    1072
#define IDC_EDIT17                      1074
#define IDC_BUTTON41                    1075
#define IDC_SECOND_VIDEO                1076
#define IDC_BUTTON42                    1077
#define IDC_BUTTON43                    1078
#define IDC_BUTTON44                    1079
#define IDC_BUTTON45                    1080
#define IDC_RESULT                      1082
#define IDC_LIST2                       1083
#define IDC_BUTTON46                    1084
#define IDC_MKV_VIDEO2                  1085
#define IDC_REF_2D_MODEL1               1086
#define IDC_REF_2D_MODEL2               1087
#define IDC_BUTTON47                    1088
#define IDC_EDIT16                      1089
#define IDC_BUTTON48                    1090
#define IDC_BUTTON49                    1091
#define IDC_EDIT18                      1092
#define IDC_BUTTON50                    1093
#define IDC_BUTTON51                    1094
#define IDC_EDIT19                      1095
#define IDC_EDIT20                      1096
#define IDC_EDIT21                      1097
#define IDC_BUTTON52                    1098
#define IDC_EDIT22                      1099
#define IDC_BUTTON53                    1100
#define IDC_BUTTON54                    1101
#define IDC_BUTTON55                    1102
#define IDC_EDIT23                      1103
#define IDC_BUTTON56                    1104
#define IDC_EDIT24                      1105
#define IDC_BUTTON57                    1106
#define IDC_BUTTON58                    1107
#define IDC_BUTTON59                    1108
#define IDC_BUTTON60                    1109
#define IDC_BUTTON61                    1110
#define IDC_BUTTON62                    1116
#define IDC_EDIT25                      1117
#define IDC_BUTTON63                    1118
#define IDC_BUTTON64                    1119
#define IDC_EDIT26                      1120
#define IDC_BUTTON65                    1121
#define IDC_BUTTON66                    1122
#define IDC_CHECK1                      1123
#define IDC_CHECK2                      1124
#define IDC_EDIT27                      1125
#define IDC_BUTTON67                    1126
#define IDC_BUTTON68                    1127
#define IDC_BUTTON69                    1128
#define IDC_RECOG_TEXT                  1129
#define IDC_RECOG_TEXT1                 1129
#define IDC_RECOG_TEXT2                 1130
#define IDC_RECOG_TEXT3                 1131
#define IDC_LIST_TEXT                   1132
#define IDC_ANNOT_VIEW                  1133
#define IDC_BUTTON70                    1134
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        132
#define _APS_NEXT_COMMAND_VALUE         32771
#define _APS_NEXT_CONTROL_VALUE         1135
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
```

## File: targetver.h
```
#pragma once
#include <SDKDDKVer.h>
```

## File: VideoUtil-v1.1.cpp
```cpp
#include "pch.h"
#include "VideoUtil-v1.1.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <algorithm>
using namespace cv;
using namespace std;
CVideoUtil::CVideoUtil()
{
}
CVideoUtil::~CVideoUtil()
{
}
BITMAPINFO* CVideoUtil::MakeBMPHeader(int width, int height)
{
	BITMAPINFO* pBmp;
	pBmp = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)];
	pBmp->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBmp->bmiHeader.biWidth = width;
	pBmp->bmiHeader.biHeight = -height;
	pBmp->bmiHeader.biPlanes = 1;
	pBmp->bmiHeader.biBitCount = 24;
	pBmp->bmiHeader.biCompression = BI_RGB;
	pBmp->bmiHeader.biSizeImage = 0;
	pBmp->bmiHeader.biXPelsPerMeter = 0;
	pBmp->bmiHeader.biYPelsPerMeter = 0;
	pBmp->bmiHeader.biClrUsed = 0;
	pBmp->bmiHeader.biClrImportant = 0;
	return pBmp;
}
BITMAPINFO* CVideoUtil::MakeBMPHeader(int width, int height, int channel)
{
	BITMAPINFO* pBmp;
	pBmp = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)];
	pBmp->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBmp->bmiHeader.biWidth = width;
	pBmp->bmiHeader.biHeight = -height;
	pBmp->bmiHeader.biPlanes = 1;
	pBmp->bmiHeader.biBitCount = channel * 8;
	if (channel == 1) pBmp->bmiHeader.biCompression = BI_RGB;
	else pBmp->bmiHeader.biCompression = BI_RGB;
	pBmp->bmiHeader.biSizeImage = 0;
	pBmp->bmiHeader.biXPelsPerMeter = 0;
	pBmp->bmiHeader.biYPelsPerMeter = 0;
	pBmp->bmiHeader.biClrUsed = 0;
	pBmp->bmiHeader.biClrImportant = 0;
	return pBmp;
}
vector<string> CVideoUtil::SplitString(string str)
{
	istringstream ss(str);
	string stringBuffer;
	vector<string> x;
	x.clear();
	while (getline(ss, stringBuffer, ' ')) {
		x.push_back(stringBuffer);
	}
	return x;
}
int CVideoUtil::GetKoreaCharNum(CString str)
{
	int len = str.GetLength();
	int cnt = 0;
	for (int k = 0; k < len; k++)
	{
		if (str.GetAt(k) <= 0 || str.GetAt(k) < 127) cnt++;
	}
	return len - cnt;
}
void CVideoUtil::DrawImageBMPwKeepRationFast(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	if (frame.empty()) return;
	Mat white_img(height, width, CV_8UC3, Scalar(255, 255, 255));
	Mat src_resize_img;
	int src_width = frame.cols* height / frame.rows;
	resize(frame, src_resize_img, Size(src_width, height));
	src_resize_img.copyTo(white_img(Range(0, height), Range(width/2-src_width /2, width/2 + src_width / 2)));
	DrawImageBMP(wnd, white_img, x, y, width, height);
}
void CVideoUtil::DrawImageBMPwKeepRatio(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	if (frame.empty()) return;
	double src_ratio = (double)frame.rows / (double)frame.cols;
	double out_ratio = (double)height / (double)width;
	int blank_width;
	if (src_ratio > out_ratio) blank_width = width - (double)height / (double)src_ratio;
	else blank_width = 0;
	if (blank_width != 0)
	{
		Mat white_img(height, width, CV_8UC3, Scalar(255, 255, 255));
		Mat re_img;
		resize(white_img, re_img, Size(width, height));
		CImage m_img;
		Mat2CImage(&re_img, m_img);
		CClientDC dc(wnd);
		m_img.Draw(dc, x, y);
	}
	Mat re_img;
	resize(frame, re_img, Size(width - blank_width, height));
	CImage m_img;
	Mat2CImage(&re_img, m_img);
	CClientDC dc(wnd);
	m_img.Draw(dc, x + blank_width / 2, y);
}
void CVideoUtil::DrawImageBMP(CDC& dc, Mat frame, int x, int y, int width, int height)
{
	if (frame.empty()) return;
	Mat img;
	switch (frame.type())
	{
	case CV_8UC4: img = frame; break;
	case CV_8UC3: cvtColor(frame, img, COLOR_BGR2BGRA); break;
	case CV_8UC1: cvtColor(frame, img, COLOR_GRAY2BGRA); break;
	default: return;
	}
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = img.cols;
	bmi.bmiHeader.biHeight = -img.rows;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	dc.RealizePalette();
	::SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
	::StretchDIBits(dc.GetSafeHdc(),
		x, y, width, height,
		0, 0, img.cols, img.rows,
		img.data, &bmi, DIB_RGB_COLORS, SRCCOPY);
	::GdiFlush();
}
void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	if (!wnd) return;
	CClientDC dc(wnd);
	DrawImageBMP(dc, frame, x, y, width, height);
}
void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y)
{
	auto img = frame.clone();
	if (!img.empty())
	{
		CRect frame_rect;
		wnd->GetWindowRect(frame_rect);
		DrawImageBMP(wnd, img, x, y, frame_rect.Width(), frame_rect.Height());
	}
}
void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent)
{
	auto img = frame.clone();
	if (!img.empty())
	{
		CRect frame_rect;
		wnd->GetWindowRect(frame_rect);
		rectangle(img, Rect(5, img.rows-15, img.cols-10, 10), Scalar(255, 255, 255), FILLED);
		int width = (img.cols - 10) * time_line_percent;
		rectangle(img, Rect(5, img.rows - 15+2, width, 6), Scalar(0, 255, 0), FILLED);
		DrawImageBMP(wnd, img, x, y, frame_rect.Width(), frame_rect.Height());
	}
}
void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent, int frame_index)
{
	auto img = frame.clone();
	if (!img.empty())
	{
		CRect frame_rect;
		wnd->GetWindowRect(frame_rect);
		rectangle(img, Rect(5, img.rows - 15, img.cols - 10, 10), Scalar(255, 255, 255), FILLED);
		int width = (img.cols - 10) * time_line_percent;
		rectangle(img, Rect(5, img.rows - 15 + 2, width, 6), Scalar(0, 255, 0), FILLED);
		DrawImageBMP(wnd, img, x, y, frame_rect.Width(), frame_rect.Height());
		CString str;
		str.Format("%d", frame_index);
		DrawText_(wnd, 0, frame_rect.Height()-40, 10, str, RGB(255, 255, 0), 0, CRect(0, 0, frame_rect.Width(), frame_rect.Height()));
	}
}
void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height, CRect rec)
{
	if (frame.empty()) return;
	CClientDC dc(wnd);
	CRgn rgn;
	rgn.CreateRectRgnIndirect(rec);
	dc.SelectClipRgn(&rgn);
	if (frame.channels() != 1)
	{
		BITMAPINFO* pBmp = MakeBMPHeader(frame.cols, frame.rows, frame.channels());
		dc.RealizePalette();
		SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
		StretchDIBits(dc.GetSafeHdc(), x, y, width, height, 0, 0, frame.cols, frame.rows, frame.data, pBmp, DIB_RGB_COLORS, SRCCOPY);
		dc.SelectClipRgn(NULL);
		delete[] pBmp;
	}
	else
	{
		BITMAPINFO* pBmp = MakeBMPHeader(frame.cols, frame.rows, frame.channels());
		Mat cimg;
		cvtColor(frame, cimg, COLOR_GRAY2RGB);
		dc.RealizePalette();
		SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
		StretchDIBits(dc.GetSafeHdc(), x, y, width, height, 0, 0, cimg.cols, cimg.rows, cimg.data, pBmp, DIB_RGB_COLORS, SRCCOPY);
		dc.SelectClipRgn(NULL);
		delete[] pBmp;
	}
}
void CVideoUtil::CopyImg(Mat src, Mat& dst, int x, int  y)
{
	if (!src.empty())
	{
		Mat roi = src(Rect(x, y, dst.cols, dst.rows));
		dst.copyTo(roi);
	}
}
void CVideoUtil::CopyImg2(Mat src, Mat& dst, int x, int  y)
{
	if (!src.empty())
	{
		if ((y + src.rows) <= dst.rows && (x + src.cols) <= dst.cols)
		{
			Mat roi = dst(Rect(x, y, src.cols, src.rows));
			src.copyTo(roi);
		}
	}
}
void CVideoUtil::CopyImg3(Mat src, Mat& dst, int dst_x, int  dst_y, int width, int height)
{
	if (!src.empty())
	{
		if ((dst_y + height) <= dst.rows && (dst_x + width) <= dst.cols)
		{
			Mat resize_img;
			resize(src, resize_img, Size(width, height), INTER_CUBIC);
			CopyImg2(resize_img, dst, dst_x, dst_y);
		}
	}
}
void CVideoUtil::DrawScalarImg(CWnd* wnd, Scalar color, int x, int y, int width, int height)
{
	Mat frame(height, width, CV_8UC3, color);
	DrawImageBMP(wnd, frame, x, y, width, height);
}
void CVideoUtil::DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index)
{
	CRect frame_rect;
	wnd->GetWindowRect(frame_rect);
	int width_num = x_num;
	int height_num = y_num;
	int width = frame_rect.Width() / width_num;
	int height = frame_rect.Height() / height_num;
	Mat white_img(frame_rect.Height(), frame_rect.Width(), CV_8UC3, Scalar(240, 240, 240));
	for (int k = 0;k < imgs.size();k++)
	{
		int col = k % x_num;
		int row = k / x_num;
		if (k < x_num * y_num)
		{
			CopyImg3(imgs[k], white_img, width * col, height * row, width, height);
		}
	}
	DrawImageBMP(wnd, white_img, 0, 0, frame_rect.Width(), frame_rect.Height());
	if (ptr_index >= 0 && ptr_index < imgs.size())
	{
		int col = ptr_index % x_num;
		int row = ptr_index / x_num;
		DrawRect(wnd, CRect(width * col + 1, height * row + 1, width * (col + 1) - 1, height * (row + 1) - 1), RGB(255, 0, 0));
	}
}
void CVideoUtil::DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index, vector<CString> labels)
{
	CRect frame_rect;
	wnd->GetWindowRect(frame_rect);
	int width_num = x_num;
	int height_num = y_num;
	int width = frame_rect.Width() / width_num;
	int height = frame_rect.Height() / height_num;
	Mat white_img(frame_rect.Height(), frame_rect.Width(), CV_8UC3, Scalar(240, 240, 240));
	for (int k = 0; k < imgs.size(); k++)
	{
		int col = k % x_num;
		int row = k / x_num;
		if (k < x_num * y_num)
		{
			CopyImg3(imgs[k], white_img, width * col, height * row, width, height);
		}
	}
	DrawImageBMP(wnd, white_img, 0, 0, frame_rect.Width(), frame_rect.Height());
	for (int k = 0; k < labels.size(); k++)
	{
		int col = k % x_num;
		int row = k / x_num;
		DrawText_(wnd, col*width+4, row*height+4, 8, labels[k], RGB(0, 0, 0), 1, CRect(0, 0, frame_rect.Width(), frame_rect.Height()));
	}
	if (ptr_index >= 0 && ptr_index < imgs.size())
	{
		int col = ptr_index % x_num;
		int row = ptr_index / x_num;
		DrawRect(wnd, CRect(width * col + 1, height * row + 1, width * (col + 1) - 1, height * (row + 1) - 1), RGB(255, 0, 0));
	}
}
void CVideoUtil::DrawIndexList(CWnd* wnd, int x_num, int y_num, vector<int> index)
{
	CRect frame_rect;
	wnd->GetWindowRect(frame_rect);
	int width_num = x_num;
	int height_num = y_num;
	int width = frame_rect.Width() / width_num;
	int height = frame_rect.Height() / height_num;
	for (int k = 0;k < index.size();k++)
	{
		int col = k % x_num;
		int row = k / x_num;
		if (k < x_num * y_num)
		{
			CString str;
			str.Format(_T("%d"), index[k]);
			DrawText_(wnd, width * col,height * row, 80, str, RGB(255, 255, 255), 0, 0, CRect(0, 0, frame_rect.Width(), frame_rect.Height()));
		}
	}
}
CString CVideoUtil::Vector2CString(vector<float> dat)
{
	CString str;
	for (int k=0;k< dat.size(); k++)
	{
		CString add_str;
		if( k== (dat.size()-1)) add_str.Format(_T("%.6f"), dat[k]);
		else add_str.Format(_T("%.6f "), dat[k]);
		str.Append(add_str);
	}
	return str;
}
CString CVideoUtil::Vector2CString(vector<UINT32> dat)
{
	CString str;
	for (int k = 0;k < dat.size(); k++)
	{
		CString add_str;
		if (k == (dat.size() - 1)) add_str.Format(_T("%d"), dat[k]);
		else add_str.Format(_T("%d "), dat[k]);
		str.Append(add_str);
	}
	return str;
}
CString CVideoUtil::Vector2CString(vector<Point3f> dat)
{
	CString str;
	for (int k = 0; k < dat.size(); k++)
	{
		CString add_str;
		add_str.Format(_T("%.6f "), dat[k].x);
		str.Append(add_str);
		add_str.Format(_T("%.6f "), dat[k].y);
		str.Append(add_str);
		if (k == (dat.size() - 1)) add_str.Format(_T("%.6f"), dat[k].z);
		else add_str.Format(_T("%.6f "), dat[k].z);
		str.Append(add_str);
	}
	return str;
}
void CVideoUtil::DrawImageTransparentBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	if (frame.empty()) return;
	BITMAPINFO* pBmp = MakeBMPHeader(frame.cols, frame.rows, frame.channels());
	CClientDC dc(wnd);
	HDC hdc = dc.GetSafeHdc();
	HDC hMemDC;
	HBITMAP hImage, hOldBitmap;
	hMemDC = CreateCompatibleDC(hdc);
	BITMAPINFOHEADER   bmih;
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	bmih.biWidth = frame.cols;
	bmih.biHeight = -frame.rows;
	bmih.biBitCount = 24;
	bmih.biCompression = BI_RGB;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biPlanes = 1;
	BITMAPINFO* bmi = (BITMAPINFO*)&bmih;
	hImage = CreateDIBitmap(hdc, &bmih, CBM_INIT, frame.data, bmi, DIB_RGB_COLORS);
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hImage);
	TransparentBlt(hdc, x, y, width, height, hMemDC, 0, 0, frame.cols, frame.rows, RGB(0, 0, 0));
	SelectObject(hMemDC, hOldBitmap);
	DeleteObject(hImage);
	DeleteDC(hMemDC);
	delete[] pBmp;
}
void CVideoUtil::DrawText_Hangul(CWnd* wnd,
	int x, int y,
	int ptSize,
	const CString& txt,
	COLORREF textColor,
	int type,
	int mode,
	const CRect& win_rec)
{
	if (!wnd || !::IsWindow(wnd->GetSafeHwnd()) || txt.IsEmpty())
		return;
	CClientDC dc(wnd);
	const int saved = dc.SaveDC();
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);
	dc.SelectClipRgn(&rgn);
	LOGFONT lf = {};
	lf.lfWeight = FW_BOLD;
	lf.lfCharSet = DEFAULT_CHARSET;
	int dpiY = dc.GetDeviceCaps(LOGPIXELSY);
	lf.lfHeight = -MulDiv(ptSize, dpiY, 72);
	CFont font;
	font.CreateFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&font);
	dc.SetTextColor(textColor);
	int oldBkMode = TRANSPARENT;
	COLORREF oldBkColor = 0;
	if (type == 0) {
		oldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else {
		oldBkColor = dc.SetBkColor(RGB(255, 255, 255));
		oldBkMode = dc.SetBkMode(OPAQUE);
	}
	UINT fmt = DT_LEFT | DT_TOP | DT_NOPREFIX;
	if (mode == 0) {
		fmt |= DT_WORDBREAK;
	}
	else {
		fmt |= DT_SINGLELINE | DT_END_ELLIPSIS;
	}
	CRect rcDraw(x, y, win_rec.right, win_rec.bottom);
	CRect rcCalc = rcDraw;
	dc.DrawText(txt, rcCalc, fmt | DT_CALCRECT);
	if (mode == 1) {
		if (rcCalc.right > win_rec.right)
			rcCalc.right = win_rec.right;
		if (rcCalc.bottom > win_rec.bottom)
			rcCalc.bottom = y + (rcCalc.Height());
	}
	else {
		if (rcCalc.right > win_rec.right) rcCalc.right = win_rec.right;
		if (rcCalc.bottom > win_rec.bottom) rcCalc.bottom = win_rec.bottom;
	}
	dc.DrawText(txt, rcCalc, fmt);
	if (type == 1) {
		dc.Draw3dRect(rcCalc, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	}
	dc.SetBkMode(oldBkMode);
	if (type == 1) dc.SetBkColor(oldBkColor);
	dc.SelectObject(pOldFont);
	dc.SelectClipRgn(nullptr);
	dc.RestoreDC(saved);
}
void CVideoUtil::DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, int mode, CRect win_rec)
{
	CClientDC dc(wnd);
	CRect rec;
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);
	LOGFONT lf;
	::ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = size;
	lf.lfWeight = DWRITE_FONT_WEIGHT_BOLD;
	CFont          NewFont;
	NewFont.CreatePointFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&NewFont);
	dc.SetTextColor(color);
	if (type == 0) dc.SetBkMode(TRANSPARENT);
	else dc.SetBkColor(RGB(255, 255, 255));
	dc.SelectClipRgn(&rgn);
	int len = txt.GetLength();
	int korea_len = GetKoreaCharNum(txt);
	len = len + korea_len;
	if (mode == 1)
	{
		if (len <= 10)
		{
			rec.left = x;
			rec.top = y;
			rec.right = size * (len)+rec.left;
			rec.bottom = size + rec.top;
			dc.DrawText(txt, rec, DT_SINGLELINE);
		}
		else
		{
			CString txt1;
			for (int idx = 0; idx < len; idx += 10)
			{
				txt1 = txt.Mid(idx, 10);
				rec.left = x;
				rec.top = y + idx / 10 * (size + 1);
				rec.right = size * (10) + rec.left;
				rec.bottom = size + 3 + rec.top;
				dc.DrawText(txt1, rec, DT_SINGLELINE);
			}
		}
	}
	else
	{
		rec.left = x;
		rec.top = y;
		rec.right = size * (len)+rec.left;
		rec.bottom = size + rec.top;
		dc.DrawText(txt, rec, DT_SINGLELINE);
	}
	if (type == 1)
	{
		rec.right = rec.left + rec.Width() / 2 - 2;
		dc.Draw3dRect(rec, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	}
	dc.SelectClipRgn(NULL);
	dc.SelectObject(pOldFont);
}
void CVideoUtil::DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, CRect win_rec)
{
	CClientDC dc(wnd);
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);
	dc.SelectClipRgn(&rgn);
	const int dpi = dc.GetDeviceCaps(LOGPIXELSY);
	LOGFONT lf = {};
	lf.lfHeight = -MulDiv(size, dpi, 72);
	lf.lfWeight = FW_BOLD;
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI"));
	CFont font; font.CreateFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&font);
	CRect calc = CRect(0, 0, 0, 0);
	dc.DrawText(txt, &calc, DT_CALCRECT | DT_SINGLELINE);
	const int padX = MulDiv(size, dpi, 72) / 2;
	const int padY = MulDiv(size, dpi, 72) / 4;
	CRect rec;
	rec.left = x;
	rec.top = y;
	rec.right = x + calc.Width() + padX * 2;
	rec.bottom = y + calc.Height() + padY * 2;
	dc.SetBkMode(TRANSPARENT);
	if (type == 1) {
		dc.FillSolidRect(&rec, color);
		auto luminance = [](COLORREF c) {
			double r = GetRValue(c), g = GetGValue(c), b = GetBValue(c);
			return 0.2126 * r + 0.7152 * g + 0.0722 * b;
			};
		COLORREF textCol = (luminance(color) < 128.0) ? RGB(255, 255, 255) : RGB(0, 0, 0);
		dc.SetTextColor(textCol);
	}
	else {
		dc.SetTextColor(color);
	}
	CRect textRc = rec;
	textRc.DeflateRect(padX, padY);
	dc.DrawText(txt, &textRc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
	dc.SelectClipRgn(NULL);
	dc.SelectObject(pOldFont);
}
vector<Point3f> CVideoUtil::DevideByN(vector<Point3f> pt, double N)
{
	vector<Point3f> ret;
	for (auto p : pt) ret.push_back(p / N);
	return ret;
}
CString CVideoUtil::GetFilePrefix(CString src)
{
	CString prefix;
	int len = src.GetLength();
	int dot_pos = src.ReverseFind('.');
	int dir_pos = src.ReverseFind('\\');
	if (dot_pos == -1 || dir_pos > dot_pos) prefix.Empty();
	else if (dir_pos == -1)
	{
		prefix = src.Mid(0, dot_pos);
	}
	else prefix = src.Mid(dir_pos + 1, dot_pos - dir_pos - 1);
	return prefix;
}
CString CVideoUtil::GetFileExt(CString src)
{
	CString ext;
	int len = src.GetLength();
	int dot_pos = src.ReverseFind('.');
	if (dot_pos == -1 ) ext.Empty();
	else ext = src.Mid(dot_pos + 1, len-dot_pos - 1);
	return ext;
}
vector<CString> CVideoUtil::GetAllFile(CString srcDir2)
{
	CFileFind finder;
	CString srcDir;
	CString end_srcDir2 = srcDir2.Right(1);
	if (end_srcDir2.Compare(_T("\\")) == 0) srcDir = srcDir2.Left(srcDir2.GetLength() - 1);
	else srcDir = srcDir2;
	vector<CString> ret;
	BOOL bWorking = finder.FindFile(srcDir + "/*.*");
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots()) continue;
		if (finder.IsDirectory()) continue;
		CString strFileName = finder.GetFileName();
		CString tmp = srcDir + _T("\\") + strFileName;
		if(tmp.Find(_T("Thumbs.db"))<0) ret.push_back(tmp);
	}
	finder.Close();
	sort(ret.begin(), ret.end());
	return ret;
}
vector<CString> CVideoUtil::GetAllDir(CString srcDir2)
{
	CFileFind finder;
	CString srcDir;
	CString end_srcDir2 = srcDir2.Right(1);
	if (end_srcDir2.Compare(_T("\\")) == 0) srcDir = srcDir2.Left(srcDir2.GetLength() - 1);
	else srcDir = srcDir2;
	vector<CString> ret;
	BOOL bWorking = finder.FindFile(srcDir + "/*.*");
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots()) continue;
		if (finder.IsDirectory())
		{
			ret.push_back(finder.GetFilePath() + _T("\\"));
		}
	}
	finder.Close();
	return ret;
}
double CVideoUtil::Distance(vector<Point> pts1, vector<Point> pts2)
{
	double dist = 0.0;
	if (pts1.size() != pts2.size()) return -1.0;
	else
	{
		for (int k = 0;k < pts1.size();k++)
			dist += Distance(pts1[k], pts2[k]);
	}
	return dist / pts1.size();
}
double CVideoUtil::Distance(double x1, double y1, double x2, double y2)
{
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
double CVideoUtil::Distance(double x1, double y1, double z1, double x2, double y2, double z2)
{
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1-z2) * (z1 - z2));
}
double CVideoUtil::Distance(Point3f p1, Point3f p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) + (p1.z - p2.z) * (p1.z - p2.z));
}
double CVideoUtil::Distance(Point3f p1)
{
	return sqrt((p1.x ) * (p1.x ) + (p1.y ) * (p1.y ) + (p1.z ) * (p1.z ));
}
double CVideoUtil::Distance(Point2d p1, Point2d p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) );
}
double CVideoUtil::Distance(Point2f p1, Point2f p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}
double CVideoUtil::Distance(Point p1, Point p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}
double CVideoUtil::Distance(Point2f p1)
{
	return sqrt((p1.x ) * (p1.x ) + (p1.y ) * (p1.y ));
}
double CVideoUtil::Distance(Mat p1, Mat p2)
{
	Mat dif = p2 - p1;
	return norm(dif, NORM_L2);
}
double CVideoUtil::GetAngle(Point3f p1, Point3f p2)
{
	double angle=0;
	Point3f A = p1;
	Point3f B = p2;
	double n1 = A.x * B.x + A.y * B.y + A.z * B.z;
	double n2 = Distance(A) * Distance(B);
	double n = n1 / n2;
	if (n > 1.0)n = 1.0;
	if (n < -1.0)n = -1.0;
	angle = acos(n);
	return angle * 180 / CV_PI;
}
double CVideoUtil::GetAngle(Point2f p1, Point2f p2)
{
	double d1 = sqrt(p1.x * p1.x + p1.y * p1.y);
	double d2 = sqrt(p2.x * p2.x + p2.y * p2.y);
	if (d1 < 1e-8 || d2 < 1e-8) {
		return 0.0;
	}
	double dot = p1.x * p2.x + p1.y * p2.y;
	double cross = p1.x * p2.y - p1.y * p2.x;
	double angle = atan2(fabs(cross), dot) * 180.0 / CV_PI;
	return angle;
}
double CVideoUtil::GetAngle(Point2f p1)
{
	if (p1.x == 0 && p1.y == 0) return 0;
	else return atan2(p1.x, p1.y) * 180 / CV_PI;
}
double CVideoUtil::GetAngle(Point3f p1, Point3f p2, Point3f p3)
{
	double angle;
	Point3f A = p1 - p2;
	Point3f B = p3 - p2;
	return GetAngle(A,B);
}
Point3f CVideoUtil::MatToPoint3f(Mat m)
{
	return Point3f(m.at<double>(0, 0), m.at<double>(1, 0), m.at<double>(2, 0));
}
int CVideoUtil::Mat2CImage(Mat* mat, CImage& img)
{
	if (!mat || mat->empty())
		return -1;
	int nBPP = mat->channels() * 8;
	img.Create(mat->cols, mat->rows, nBPP);
	if (nBPP == 8)
	{
		static RGBQUAD pRGB[256];
		for (int i = 0; i < 256; i++)
			pRGB[i].rgbBlue = pRGB[i].rgbGreen = pRGB[i].rgbRed = i;
		img.SetColorTable(0, 256, pRGB);
	}
	uchar* psrc = mat->data;
	uchar* pdst = (uchar*)img.GetBits();
	int imgPitch = img.GetPitch();
	for (int y = 0; y < mat->rows; y++)
	{
		memcpy(pdst, psrc, mat->cols * mat->channels());
		psrc += mat->step;
		pdst += imgPitch;
	}
	return 0;
}
void CVideoUtil::DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness)
{
	CClientDC dc(wnd);
	CPen pen;
	pen.CreatePen(PS_SOLID, thickness, color);
	CPen* oldPen = dc.SelectObject(&pen);
	dc.MoveTo(p1.x, p1.y);
	dc.LineTo(p2.x, p2.y);
	dc.SelectObject(oldPen);
	pen.DeleteObject();
}
void CVideoUtil::DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness, int line_type)
{
	CClientDC dc(wnd);
	CPen pen;
	pen.CreatePen(line_type, thickness, color);
	CPen* oldPen = dc.SelectObject(&pen);
	dc.MoveTo(p1.x, p1.y);
	dc.LineTo(p2.x, p2.y);
	dc.SelectObject(oldPen);
	pen.DeleteObject();
}
void CVideoUtil::DrawRect(CWnd* wnd, CRect rect, COLORREF color)
{
	if (!wnd || !::IsWindow(wnd->GetSafeHwnd())) return;
	const int thickness = 3;
	CClientDC dc(wnd);
	CRect clientRect;
	wnd->GetClientRect(&clientRect);
	dc.IntersectClipRect(&clientRect);
	CPen pen;
	pen.CreatePen(PS_SOLID, thickness, color);
	CPen* pOldPen = dc.SelectObject(&pen);
	CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(HOLLOW_BRUSH);
	dc.Rectangle(rect);
	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBrush);
	pen.DeleteObject();
}
void CVideoUtil::DrawFillRect(CWnd* wnd, const CRect& rect, COLORREF color)
{
	if (!wnd) return;
	CClientDC dc(wnd);
	CRect client;
	wnd->GetClientRect(&client);
	dc.IntersectClipRect(&client);
	dc.FillSolidRect(&rect, color);
}
void CVideoUtil::DrawFillCircle(CWnd* wnd, CPoint center, int radius, COLORREF color)
{
	if (!wnd || radius <= 0) return;
	CClientDC dc(wnd);
	CRect client;
	wnd->GetClientRect(&client);
	dc.IntersectClipRect(&client);
	HPEN hOldPen = (HPEN)dc.SelectObject(GetStockObject(NULL_PEN));
	CBrush brush;
	brush.CreateSolidBrush(color);
	CBrush* pOldBrush = dc.SelectObject(&brush);
	CRect r(center.x - radius, center.y - radius,
		center.x + radius, center.y + radius);
	dc.Ellipse(&r);
	dc.SelectObject(pOldBrush);
	dc.SelectObject(hOldPen);
	brush.DeleteObject();
}
bool CVideoUtil::IsExistFile(string filename)
{
	if (filesystem::exists(filename)) return 1;
	return 0;
}
bool CVideoUtil::IsExistFile(CString srcDir, CString file)
{
	CString _strFile = srcDir + file;
	CFileStatus fs;
	if (CFile::GetStatus(_strFile, fs))
	{
		return 1;
	}
	else
	{
		return 0;
	}
	return 0;
}
bool CVideoUtil::IsExistIndex(vector<int> indexs, int query_insex)
{
	for (int k = 0; k < indexs.size(); k++)
	{
		if (indexs[k] == query_insex) return 1;
	}
	return 0;
}
char* CVideoUtil::StringToChar(CString str)
{
	char* szStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = str.GetLength() + 1;
	TCHAR* tszTemp = NULL;
	tszTemp = new TCHAR[nLen];
	memset(tszTemp, 0x00, nLen * sizeof(TCHAR));
	_tcscpy_s(tszTemp, nLen, str);
	int nSize = WideCharToMultiByte(CP_ACP, 0, tszTemp, -1, NULL, NULL, NULL, NULL);
	szStr = new char[nSize];
	memset(szStr, 0x00, nSize);
	WideCharToMultiByte(CP_ACP, 0, tszTemp, -1, szStr, nSize, NULL, NULL);
	if (tszTemp)
	{
		delete[] tszTemp;
		tszTemp = NULL;
	}
#else
	int nLen = str.GetLength() + 1;
	szStr = new char[nLen];
	memset(szStr, 0x00, nLen);
	strcpy(szStr, str);
#endif
	return szStr;
}
TCHAR* CVideoUtil::StringToTCHAR(CString str)
{
	TCHAR* tszStr = NULL;
	int nLen = str.GetLength() + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	_tcscpy_s(tszStr, nLen, str);
	return tszStr;
}
string CVideoUtil::StringToStdString(CString str)
{
	string stdStr;
	char* szStr = StringToChar(str);
	if (szStr)
	{
		stdStr = szStr;
		delete[] szStr;
	}
	return stdStr;
}
CString CVideoUtil::CharToString(char* str)
{
	CString cStr;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = strlen(str) + 1;
	TCHAR* tszTemp = NULL;
	tszTemp = new TCHAR[nLen];
	memset(tszTemp, 0x00, nLen * sizeof(TCHAR));
	MultiByteToWideChar(CP_ACP, 0, str, -1, tszTemp, nLen * sizeof(TCHAR));
	cStr.Format(_T("%s"), tszTemp);
	if (tszTemp)
	{
		delete[] tszTemp;
		tszTemp = NULL;
	}
#else
	cStr.Format("%s", str);
#endif
	return cStr;
}
TCHAR* CVideoUtil::CharToTCHAR(char* str)
{
	TCHAR* tszStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = strlen(str) + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	MultiByteToWideChar(CP_ACP, 0, str, -1, tszStr, nLen * sizeof(TCHAR));
#else
	int nLen = strlen(str) + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	_tcscpy(tszStr, str);
#endif
	return tszStr;
}
CString CVideoUtil::TCHARToString(TCHAR* str)
{
	CString cStr;
	cStr.Format(_T("%s"), str);
	return cStr;
}
char* CVideoUtil::TCHARToChar(TCHAR* str)
{
	char* szStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nSize = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, NULL, NULL, NULL);
	szStr = new char[nSize];
	memset(szStr, 0x00, nSize);
	WideCharToMultiByte(CP_ACP, 0, str, -1, szStr, nSize, NULL, NULL);
#else
	int nLen = strlen(str) + 1;
	szStr = new char[nLen];
	memset(szStr, 0x00, nLen);
	strcpy(szStr, str);
#endif
	return szStr;
}
CString CVideoUtil::GetFileName(CString src)
{
	CString prefix;
	int len = src.GetLength();
	int dot_pos = src.GetLength();
	int dir_pos = src.ReverseFind('\\');
	if (dot_pos == -1 || dir_pos > dot_pos) prefix.Empty();
	else if (dir_pos == -1)
	{
		prefix = src.Mid(0, dot_pos);
	}
	else prefix = src.Mid(dir_pos + 1, dot_pos - dir_pos - 1);
	return prefix;
}
CString CVideoUtil::GetDirName(CString src)
{
	CString file;
	int len = src.GetLength();
	int dot_pos = len;
	int dir_pos = src.ReverseFind('\\');
	if (dot_pos == -1 || dir_pos == -1 || dir_pos > dot_pos) file.Empty();
	else file = src.Mid(0, dir_pos);
	return file;
}
vector<CString> CVideoUtil::SplitCString(CString str)
{
	vector<CString> ret;
	CString   resToken;
	int curPos = 0;
	if (str.IsEmpty()) return ret;
	resToken = str.Tokenize(_T(" "), curPos);
	ret.push_back(resToken);
	while (resToken != "")
	{
		resToken = str.Tokenize(_T(" "), curPos);
		if (!resToken.IsEmpty()) ret.push_back(resToken);
	};
	if (ret.size() > 0)
	{
		ret[ret.size() - 1].Remove('\n');
	}
	return ret;
}
vector<CString> CVideoUtil::SplitCString(CString str, CString space)
{
	vector<CString> ret;
	// 구분자 집합이 비어 있으면 전체를 하나로
	if (space.IsEmpty()) {
		ret.push_back(str);
		return ret;
	}
	CString cur; // 현재 토큰
	for (int i = 0; i < str.GetLength(); ++i) {
		const TCHAR ch = str[i];
		// space에 포함된 문자는 모두 구분자로 취급
		if (space.Find(ch) != -1) {
			// cur이 빈 문자열이면 _T("")가 push_back 됩니다.
			ret.push_back(cur);
			cur.Empty();
		}
		else {
			cur.AppendChar(ch);
		}
	}
	// 마지막 토큰(비어 있을 수도 있음) 추가
	ret.push_back(cur);
	return ret;
}
//
// convert_ansi_to_unicode_string.
//
DWORD CVideoUtil::convert_ansi_to_unicode_string(
	__out wstring& unicode,
	__in const char* ansi,
	__in const size_t ansi_size
) {
	DWORD error = 0;
	do {
		if ((nullptr == ansi) || (0 == ansi_size)) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}
		unicode.clear();
		//
		// getting required cch.
		//
		int required_cch = ::MultiByteToWideChar(
			CP_ACP,
			0,
			ansi, static_cast<int>(ansi_size),
			nullptr, 0
		);
		if (0 == required_cch) {
			error = ::GetLastError();
			break;
		}
		unicode.resize(required_cch);
		//
		// convert.
		//
		if (0 == ::MultiByteToWideChar(
			CP_ACP,
			0,
			ansi, static_cast<int>(ansi_size),
			const_cast<wchar_t*>(unicode.c_str()), static_cast<int>(unicode.size())
		)) {
			error = ::GetLastError();
			break;
		}
	} while (false);
	return error;
}
//
// convert_unicode_to_ansi_string.
//
DWORD CVideoUtil::convert_unicode_to_ansi_string(
	__out string& ansi,
	__in const wchar_t* unicode,
	__in const size_t unicode_size
) {
	DWORD error = 0;
	do {
		if ((nullptr == unicode) || (0 == unicode_size)) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}
		ansi.clear();
		//
		// getting required cch.
		//
		int required_cch = ::WideCharToMultiByte(
			CP_ACP,
			0,
			unicode, static_cast<int>(unicode_size),
			nullptr, 0,
			nullptr, nullptr
		);
		if (0 == required_cch) {
			error = ::GetLastError();
			break;
		}
		//
		// allocate.
		//
		ansi.resize(required_cch);
		//
		// convert.
		//
		if (0 == ::WideCharToMultiByte(
			CP_ACP,
			0,
			unicode, static_cast<int>(unicode_size),
			const_cast<char*>(ansi.c_str()), static_cast<int>(ansi.size()),
			nullptr, nullptr
		)) {
			error = ::GetLastError();
			break;
		}
	} while (false);
	return error;
}
//
// convert_unicode_to_utf8_string
//
DWORD CVideoUtil::convert_unicode_to_utf8_string(
	__out string& utf8,
	__in const wchar_t* unicode,
	__in const size_t unicode_size
) {
	DWORD error = 0;
	do {
		if ((nullptr == unicode) || (0 == unicode_size)) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}
		utf8.clear();
		//
		// getting required cch.
		//
		int required_cch = ::WideCharToMultiByte(
			CP_UTF8,
			WC_ERR_INVALID_CHARS,
			unicode, static_cast<int>(unicode_size),
			nullptr, 0,
			nullptr, nullptr
		);
		if (0 == required_cch) {
			error = ::GetLastError();
			break;
		}
		//
		// allocate.
		//
		utf8.resize(required_cch);
		//
		// convert.
		//
		if (0 == ::WideCharToMultiByte(
			CP_UTF8,
			WC_ERR_INVALID_CHARS,
			unicode, static_cast<int>(unicode_size),
			const_cast<char*>(utf8.c_str()), static_cast<int>(utf8.size()),
			nullptr, nullptr
		)) {
			error = ::GetLastError();
			break;
		}
	} while (false);
	return error;
}
//
// convert_utf8_to_unicode_string
//
vector<CString> CVideoUtil::SPlitCString(CString str)
{
	vector<CString>	vecText;
	CString strText(_T(""));
	//1. AfxExtractSubString 사용
	int nP = 0;
	while (FALSE != AfxExtractSubString(strText, str, nP++, _T(' ')))
	{
		vecText.push_back(strText);
	}
	return vecText;
}
vector<CString> CVideoUtil::SPlitBracket(CString str)
{
	CString vecText1=str;
	vector<CString>	vecText2;
	vector<int>ptr1, ptr2;
	for (int k = 0; k < str.GetLength(); k++)
	{
		if (str.Mid(k, 1).Compare(_T("("))==0) ptr1.push_back(k);
		else if(str.Mid(k, 1).Compare(_T(")")) == 0) ptr2.push_back(k);
	}
	for (int k = 0; k < ptr1.size(); k++)
	{
		if (k < ptr2.size())
		{
			vecText1.Replace(str.Mid(ptr1[k], ptr2[k] - ptr1[k] + 1), _T(""));
			vecText2.push_back(str.Mid(ptr1[k] + 1, ptr2[k] - ptr1[k] - 1));
		}
	}
	vecText2.insert(vecText2.begin(), vecText1);
	return vecText2;
}
void CVideoUtil::DrawFillRect_(CWnd* wnd, int x, int y, int width, int height, COLORREF clr)
{
	CClientDC dc(wnd);
	CRect r;
	CPen pen;
	CBrush brush, * pOldBrush;
	dc.SetBkMode(TRANSPARENT);
	//	brush.CreateSolidBrush(RGB(0, 255, 0));
	brush.CreateSolidBrush(clr);
	pOldBrush = dc.SelectObject(&brush);
	r.top = y;
	r.bottom = r.top + height;
	r.left = x;
	r.right = x + width;
	dc.FillRect(&r, &brush);
	dc.SelectObject(pOldBrush);
	brush.DeleteObject();
}
int CVideoUtil::PlayVideoFile(CWnd* wnd, CString file)
{
	CRect Rect;
	wnd->GetClientRect(Rect);
	VideoCapture cap(StringToChar(file));
	Mat img;
	if (cap.isOpened())
	{
		while (1)
		{
			cap >> img;
			if (img.empty()) break;
			DrawImageBMP(wnd, img, 0, 0, Rect.Width(), Rect.Height());
			Sleep(6);
		}
		cap.release();
	}
	return 1;
}
int CVideoUtil::PlayVideoFile(CWnd* wnd, CString file, CString txt)
{
	CRect Rect;
	wnd->GetClientRect(Rect);
	VideoCapture cap(StringToChar(file));
	Mat img;
	if (cap.isOpened())
	{
		while (1)
		{
			cap >> img;
			if (img.empty()) break;
			DrawImageBMP(wnd, img.clone(), 0, 0, Rect.Width(), Rect.Height());
			DrawText_(wnd, 0, 0, 100, txt, RGB(255, 255, 255), 0, 0, CRect(0, 0, Rect.Width(), Rect.Height()));
			Sleep(10);
		}
		cap.release();
	}
	return 1;
}
int CVideoUtil::PlayImgs(CWnd* wnd,  CString txt, vector<Mat> imgs)
{
	CRect Rect;
	wnd->GetClientRect(Rect);
	for(auto img :imgs )
	{
		DrawImageBMP(wnd, img, 0, 0, Rect.Width(), Rect.Height());
		DrawText_(wnd, 10, 10, 100, txt, RGB(255, 255, 255), 0, 0, CRect(0, 0, Rect.Width(), Rect.Height()));
		Sleep(30);
	}
}
Mat CVideoUtil::GetImageFromVideowithframeIndex(CString video_file, int index)
{
	// VideoCapture 열기
	VideoCapture cap(StringToChar(video_file));
	if (!cap.isOpened()) {
		cerr << "비디오 파일 열기 실패: " << video_file << endl;
		return Mat();
	}
	// 전체 프레임 개수 확인
	int total_frames = static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT));
	if (index < 0 || index >= total_frames) {
		cerr << "잘못된 프레임 인덱스: " << index << " (총 프레임: " << total_frames << ")\n";
		return Mat();
	}
	cap.set(CAP_PROP_POS_FRAMES, index);
	Mat frame;
	if (!cap.read(frame)) {
		cerr << "프레임 읽기 실패 (index=" << index << ")\n";
		return Mat();
	}
	return frame.clone();
}
void CVideoUtil::DrawXYChart(CWnd* wnd, vector<vector<float>>dat)
{
	float min_val = FLT_MAX;
	float max_val = FLT_MIN;
	for (int k = 0; k < dat.size(); k++)
	{
		auto min_val_ = *min_element(dat[k].begin(), dat[k].end());
		auto max_val_ = *max_element(dat[k].begin(), dat[k].end());
		if (min_val_ < min_val) min_val = min_val_;
		if (max_val_ > max_val) max_val = max_val_;
	}
	CRect rect;
	wnd->GetClientRect(rect);
	Mat bg_img(rect.Height(), rect.Width(), CV_8UC3, Scalar(0, 0, 0));
	char str[50];
	int limit_y = bg_img.rows * 0.8;
	int base_y = bg_img.rows * 0.2 / 2;
	int x_step = round((double)bg_img.cols / (dat[0].size() + 1));
	Scalar fixed_color[10] = { Scalar(0, 255, 0), Scalar(0, 255, 255), Scalar(255, 255, 0), Scalar(255, 255, 100),
								 Scalar(0, 255, 255),Scalar(255, 0, 255), Scalar(255, 255, 255),
								 Scalar(100, 255, 100),Scalar(255, 100, 100),Scalar(100, 100, 255)
	};
	double y_step = (max_val - min_val) / 10;
	for (double m = (int)min_val; m <= max_val; m += y_step)
	{
		int x1 = x_step;
		int x2 = x_step * (dat[0].size() - 1);
		int y = base_y + limit_y * (max_val - m) / (max_val - min_val);
		line(bg_img, Point(x1, y), Point(x2, y), Scalar(80, 80, 80), 1, 8, 0);
		sprintf(str, "%.2f", m);
		putText(bg_img, str, Point(x2, y), FONT_HERSHEY_DUPLEX, 0.3, Scalar(0, 255, 255), 1, 1);
	}
	for (int m = 0; m < dat[0].size(); m += (dat[0].size() / 20))
	{
		int x = x_step * (m + 1);
		int y = base_y + limit_y * (max_val - dat[0][m]) / (max_val - min_val);
		line(bg_img, Point(x, base_y), Point(x, base_y + limit_y), Scalar(80, 80, 80), 1, 8, 0);
	}
	for (int k = 0; k < dat.size(); k++)
	{
		for (int m = 0; m < dat[k].size(); m++)
		{
			int x = x_step * (m + 1);
			int y = base_y + limit_y * (max_val - dat[k][m]) / (max_val - min_val);
			rectangle(bg_img, Point(x - 1, y - 1), Point(x + 1, y + 1), fixed_color[k % 10], FILLED);
			if (m != (dat[k].size() - 1))
			{
				int x2 = x_step * (m + 2);
				int y2 = base_y + limit_y * (max_val - dat[k][m + 1]) / (max_val - min_val);
				line(bg_img, Point(x, y), Point(x2, y2), fixed_color[k % 10], 1);
			}
		}
	}
	for (int k = 0; k < dat.size(); k++)
	{
		rectangle(bg_img, Point(1 + k * 5, 1), Point(1 + (k + 1) * 5, 6), fixed_color[k % 10], FILLED);
	}
	DrawImageBMP(wnd, bg_img, 0, 0, rect.Width(), rect.Height());
}
int CVideoUtil::GetNearIndex(vector<int> data, int query)
{
	if (data.empty()) return -1;
	auto it = min_element(data.begin(), data.end(),
		[query](int a, int b) {
			return abs(a - query) < abs(b - query);
		});
	return distance(data.begin(), it);
}
int CVideoUtil::GetInsertIndex(vector<int> data, int query)
{
	if (data.empty()) return -1;
	auto it = lower_bound(data.begin(), data.end(), query);
	return static_cast<int>(it - data.begin());
}
vector<int> CVideoUtil::DrawXYChart(CWnd* wnd, vector<vector<float>>dat, vector<vector<int>> index, Mat &bg_img_, int cur_index)
{
	float min_val = FLT_MAX;
	float max_val = FLT_MIN;
	vector<int> x_steps;
	for (int k = 0; k < dat.size(); k++)
	{
		if (dat[k].size() > 0)
		{
			auto min_val_ = *min_element(dat[k].begin(), dat[k].end());
			auto max_val_ = *max_element(dat[k].begin(), dat[k].end());
			if (min_val_ < min_val) min_val = min_val_;
			if (max_val_ > max_val) max_val = max_val_;
		}
		else
			return x_steps;
	}
	CRect rect;
	wnd->GetClientRect(rect);
	Mat bg_img(rect.Height(), rect.Width(), CV_8UC3, Scalar(0, 0, 0));
	char str[50];
	int limit_y = bg_img.rows * 0.8;
	int base_y = bg_img.rows * 0.2 / 2;
	int text_mrg = 50;
	double x_step = (double)(bg_img.cols- text_mrg) / (double)(dat[0].size());
	Scalar fixed_color[10] = { Scalar(0, 255, 0), Scalar(0, 0, 255), Scalar(255, 0, 0), Scalar(255, 255, 0),
								 Scalar(0, 255, 255),Scalar(255, 0, 255), Scalar(255, 255, 255),
								 Scalar(100, 255, 100),Scalar(255, 100, 100),Scalar(100, 100, 255)
	};
	int last_m = 0;
	for (int m = 0; m < (dat[0].size() ); m += (dat[0].size() / 20))
	{
		int x = round(x_step * (m + 1));
		int y = base_y + limit_y * (max_val - dat[0][m]) / (max_val - min_val);
		line(bg_img, Point(x, base_y), Point(x, base_y + limit_y), Scalar(80, 80, 80), 1, 8, 0);
		sprintf(str, "%d", m);
		putText(bg_img, str, Point(x, base_y + limit_y +15), FONT_HERSHEY_DUPLEX, 0.4, Scalar(0, 255, 255), 1, 1);
		last_m = m;
	}
	double y_step = (max_val - min_val) / 10;
	for (double m = (int)min_val; m <= max_val; m += y_step)
	{
		int x1 = round(x_step);
		int x2 = bg_img.cols - text_mrg;
		int y = base_y + limit_y * (max_val - m) / (max_val - min_val);
		line(bg_img, Point(x1, y), Point(x2, y), Scalar(80, 80, 80), 1, 8, 0);
		sprintf(str, "%.4f", m);
		putText(bg_img, str, Point(x2+5, y), FONT_HERSHEY_DUPLEX, 0.4, Scalar(0, 255, 255), 1, 1);
	}
	line(bg_img, Point(x_step, base_y), Point(bg_img.cols - text_mrg, base_y), Scalar(80, 80, 80), 1, 8, 0);
	for (int k = 0; k < dat.size(); k++)
	{
		for (int m = 0; m < dat[k].size(); m++)
		{
			int x = round(x_step * (m + 1));
			int y = base_y + limit_y * (max_val - dat[k][m]) / (max_val - min_val);
			rectangle(bg_img, Point(x - 1, y - 1), Point(x + 1, y + 1), fixed_color[k % 10], FILLED);
			if (m != (dat[k].size() - 1))
			{
				int x2 = round(x_step * (m + 2));
				int y2 = base_y + limit_y * (max_val - dat[k][m + 1]) / (max_val - min_val);
				line(bg_img, Point(x, y), Point(x2, y2), fixed_color[k % 10], 1);
			}
			if(k==0) x_steps.push_back(x);
		}
	}
	for (int k = 0; k < dat.size(); k++)
	{
		rectangle(bg_img, Point(1 + k * 5, 1), Point(1 + (k + 1) * 5, 6), fixed_color[k % 10], FILLED);
	}
	for (int k = 0; k < index.size(); k++)
	{
		rectangle(bg_img, Point(1 + k * 5+100, 1), Point(1 + (k + 1) * 5+100, 6), fixed_color[ (k+dat.size()) % 10], FILLED);
	}
	for (int k = 0; k < index.size(); k++)
	{
		for (int m = 0; m < index[k].size(); m++)
		{
			int x = round(x_step * (index[k][m]+1));
			int y1 = base_y ;
			int y2 = base_y + limit_y ;
			line(bg_img, Point(x, y1), Point(x, y2), fixed_color[(k + dat.size()) % 10], 1);
			int target_x_text_num = 10;
			int skip_num = index[k].size() / target_x_text_num;
			if (skip_num == 0 || (m % skip_num)==0)
			{
				CString str;
				str.Format(_T("%d"), index[k][m]);
				putText(bg_img, StringToChar(str), Point(x, y1), FONT_HERSHEY_DUPLEX, 0.4, Scalar(255, 255, 255), 1, 1);
			}
		}
	}
	DrawImageBMP(wnd, bg_img, 0, 0, rect.Width(), rect.Height());
	bg_img_ = bg_img;
	return x_steps;
}
void CVideoUtil::CreateFolder(CString csPath)
{
	CString csPrefix(_T("")), csToken(_T(""));
	int nStart = 0, nEnd;
	while ((nEnd = csPath.Find('/', nStart)) >= 0)
	{
		CString csToken = csPath.Mid(nStart, nEnd - nStart);
		CreateDirectory(csPrefix + csToken, NULL);
		csPrefix += csToken;
		csPrefix += _T("/");
		nStart = nEnd + 1;
	}
	csToken = csPath.Mid(nStart);
	CreateDirectory(csPrefix + csToken, NULL);
}
Mat  CVideoUtil::DrawHist(int w, int h, vector<float> data, float* channel_rangle, int number_bins)
{
	float step = (channel_rangle[1] - channel_rangle[0]) / number_bins;
	int* num_bin = new int[number_bins];
	for (int k = 0; k < number_bins; k++)num_bin[k] = 0;
	int max_num_bin = 0;
	for (int k = 0; k < data.size(); k++)
	{
		int bin_idx = (data[k] - channel_rangle[0]) / step;
		if (bin_idx >= 0 && bin_idx < number_bins)
		{
			num_bin[bin_idx]++;
			if (num_bin[bin_idx] > max_num_bin) max_num_bin = num_bin[bin_idx];
		}
	}
	int hist_w = w;
	int hist_h = h;
	int bin_w = cvRound((double)hist_w / number_bins);
	Mat hist_img(hist_h, hist_w, CV_8UC3, Scalar::all(0));
	for (int i = 0; i < number_bins; i++) {
		line(hist_img, Point(bin_w * (i ), hist_h), Point(bin_w * (i), hist_h - cvRound(hist_h*(double)num_bin[i])/ max_num_bin), Scalar(0, 255, 0), 1, 8, 0);
	}
	delete[] num_bin;
	return hist_img;
}
void CVideoUtil::calcNormal(double v0[3], double v1[3], double v2[3], double out[3])
{
	double V1[3], V2[3];
	static const int x = 0;
	static const int y = 1;
	static const int z = 2;
	// Calculate two vectors from the three points
	V1[x] = v0[x] - v1[x];
	V1[y] = v0[y] - v1[y];
	V1[z] = v0[z] - v1[z];
	V2[x] = v1[x] - v2[x];
	V2[y] = v1[y] - v2[y];
	V2[z] = v1[z] - v2[z];
	// Take the cross product of the two vectors to get
	// the normal vector which will be stored in out[]
	out[x] = V1[y] * V2[z] - V1[z] * V2[y];
	out[y] = V1[z] * V2[x] - V1[x] * V2[z];
	out[z] = V1[x] * V2[y] - V1[y] * V2[x];
	// Normalize the vector (shorten length to one)
	double nor = sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
	out[0] = out[0] / nor;
	out[1] = out[1] / nor;
	out[2] = out[2] / nor;
}
void CVideoUtil::calcNormal(Point3f v0, Point3f v1, Point3f v2, Point3f &out)
{
	double V1[3], V2[3];
	static const int x = 0;
	static const int y = 1;
	static const int z = 2;
	// Calculate two vectors from the three points
	V1[x] = v0.x - v1.x;
	V1[y] = v0.y - v1.y;
	V1[z] = v0.z - v1.z;
	V2[x] = v1.x - v2.x;
	V2[y] = v1.y - v2.y;
	V2[z] = v1.z - v2.z;
	// Take the cross product of the two vectors to get
	// the normal vector which will be stored in out[]
	out.x = V1[y] * V2[z] - V1[z] * V2[y];
	out.y = V1[z] * V2[x] - V1[x] * V2[z];
	out.z = V1[x] * V2[y] - V1[y] * V2[x];
	// Normalize the vector (shorten length to one)
	double nor = sqrt(out.x * out.x + out.y * out.y + out.z * out.z);
	out.x = out.x / nor;
	out.y = out.y / nor;
	out.z = out.z / nor;
}
double CVideoUtil::GetAngle(Point3f p11, Point3f p12, Point3f p21, Point3f p22)
{
	double angle;
	Point3f A = p11 - p12;
	Point3f B = p21 - p22;
	return GetAngle(A,B);
}
void CVideoUtil::DrawResetImage(CWnd* wnd)
{
	CRect rect;
	wnd->GetWindowRect(rect);
	Mat white_img(rect.Height(), rect.Width(), CV_8UC3, Scalar(240, 240, 240)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
	DrawImageBMP(wnd, white_img, 0, 0, rect.Width(), rect.Height());
//	DrawText_(wnd, rect.Width() * 3 / 10, rect.Height() / 2 - 50, 200, _T("Preocessing..."), RGB(210, 210, 210), 0, 0, CRect(0, 0, rect.Width(), rect.Height()));
}
int CVideoUtil::IsExist(int index, vector<int>data)
{
	for (int k = 0; k < data.size(); k++)
	{
		if (index == data[k]) return 1;
	}
	return 0;
}
int CVideoUtil::IsExist(int index, vector<UINT32>data)
{
	for (int k = 0; k < data.size(); k++)
	{
		if (index == (int)data[k]) return 1;
	}
	return 0;
}
bool CVideoUtil::IsExistDir(CString dir)
{
	if (GetFileAttributes(dir) != 0xFFFFFFFF)
	{
		return true;
	}
	else return false;
}
// 특정 바이트 크기(n)마다 개행(\n) 추가하는 함수
string CVideoUtil::InsertLineFeed_(string& input, int n)
{
	string result;
	int byteCount = 0; // 바이트 카운터
	for (size_t i = 0; i < input.length(); i++) {
		unsigned char ch = input[i]; // 현재 문자
		// 한글 (멀티바이트, 2바이트 문자)
		if (ch & 0x80) { // 첫 바이트가 1xxx xxxx 형태이면 한글
			if (i + 1 < input.length()) { // 한글은 2바이트이므로 한 글자 더 추가
				result += input.substr(i, 2);
				byteCount += 2;
				i++; // 한글이므로 다음 문자로 이동
			}
		}
		else { // 영문, 숫자, 특수문자 (1바이트)
			result += ch;
			byteCount += 1;
		}
		// 지정된 바이트 수(n) 초과 시 개행 추가
		if (byteCount >= n) {
			result += '\r';
			result += '\n';
			byteCount = 0;
		}
	}
	return result;
}
Rect CVideoUtil::GetBox(vector <Point2i> pts, double scale, Mat img) // margin은 rect의 스케일
{
	// pts가 비어있으면 빈 rect 반환
	if (pts.empty())
		return Rect();
	// pts의 모든 점에서 최소/최대 좌표 계산
	int minX = pts[0].x, minY = pts[0].y;
	int maxX = pts[0].x, maxY = pts[0].y;
	for (size_t i = 1; i < pts.size(); i++)
	{
		minX = min(minX, pts[i].x);
		minY = min(minY, pts[i].y);
		maxX = max(maxX, pts[i].x);
		maxY = max(maxY, pts[i].y);
	}
	// 최소 bounding box의 가로, 세로 길이 계산
	int width = maxX - minX;
	int height = maxY - minY;
	// 정사각형을 위해 두 길이 중 큰 값을 사용
	int maxDim = max(width, height);
	// scale을 적용한 정사각형의 사이즈
	int squareSize = static_cast<int>(maxDim * scale);
	// 만약 확대된 정사각형이 이미지 크기보다 크다면 이미지 크기로 제한
	if (squareSize > img.cols)
		squareSize = img.cols;
	if (squareSize > img.rows)
		squareSize = img.rows;
	// 원래 bounding box의 중심 계산
	int centerX = (minX + maxX) / 2;
	int centerY = (minY + maxY) / 2;
	// 중심을 기준으로 정사각형의 좌상단 좌표 계산
	int x = centerX - squareSize / 2;
	int y = centerY - squareSize / 2;
	// 이미지 범위 내에 위치하도록 보정
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x + squareSize > img.cols)
		x = img.cols - squareSize;
	if (y + squareSize > img.rows)
		y = img.rows - squareSize;
	return Rect(x, y, squareSize, squareSize);
}
Rect CVideoUtil::getRectWidth(Rect rect, int width, Mat img)
{
	auto center_pt = getRectCenter(rect);
	Rect r_rect = Rect(center_pt.x - width / 2, center_pt.y - width / 2, width, width);
	r_rect = r_rect & Rect(0, 0, img.cols, img.rows);
	return r_rect;
}
Point CVideoUtil::getRectCenter(const Rect& rect)
{
	return Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
}
RECT CVideoUtil::CvRectToRECT(const Rect& cvRect) {
	RECT rc;
	rc.left = cvRect.x;
	rc.top = cvRect.y;
	rc.right = cvRect.x + cvRect.width;
	rc.bottom = cvRect.y + cvRect.height;
	return rc;
}
double CVideoUtil::getRotationAngle(const Mat& R1, const Mat& R2)
{
	Mat R_diff = R1.t() * R2;
	Mat rvec;
	Rodrigues(R_diff, rvec);
	return norm(rvec);  // radians
}
double CVideoUtil::getRotationAngle(vector<float> dat1, vector<float> dat2)
{
	if (dat1.size() != 9 || dat2.size() != 9) return -CV_PI;
	Mat R1 = (Mat_<float>(3, 3) <<
		dat1[0], dat1[3], dat1[6],
		dat1[1], dat1[4], dat1[7],
		dat1[2], dat1[5], dat1[8]);
	Mat R2 = (Mat_<float>(3, 3) <<
		dat2[0], dat2[3], dat2[6],
		dat2[1], dat2[4], dat2[7],
		dat2[2], dat2[5], dat2[8]);
	Mat R_diff = R1.t() * R2;
	Mat rvec;
	Rodrigues(R_diff, rvec);
	return norm(rvec);  // radians
}
// 비디오 정보 가져오기 함수
tuple<int, int, double, int> CVideoUtil::GetVideoInfo(VideoCapture cap)
{
	if (!cap.isOpened()) {
		// 실패 시 -1 반환
		return make_tuple(-1, -1, -1.0, -1);
	}
	int width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
	double fps = cap.get(CAP_PROP_FPS);
	int frame_count = static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT));
	return make_tuple(width, height, fps, frame_count);
}
tuple<int, int, double, int> CVideoUtil::GetVideoInfo(const string& filename)
{
	VideoCapture cap(filename);
	if (!cap.isOpened()) {
		// 실패 시 -1 반환
		return make_tuple(-1, -1, -1.0, -1);
	}
	int width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
	double fps = cap.get(CAP_PROP_FPS);
	int frame_count = static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT));
	return make_tuple(width, height, fps, frame_count);
}
CString CVideoUtil::LoadLastDir(LPCTSTR dialogKey, LPCTSTR defDir = _T("C:\\")) {
	CString section = _T("Recent");
	CString key; key.Format(_T("LastDir_%s"), dialogKey);
	return AfxGetApp()->GetProfileString(section, key, defDir);
}
void CVideoUtil::SaveLastDir(LPCTSTR dialogKey, const CString& dir) {
	if (dir.IsEmpty()) return;
	CString section = _T("Recent");
	CString key; key.Format(_T("LastDir_%s"), dialogKey);
	AfxGetApp()->WriteProfileString(section, key, dir);
}
CString CVideoUtil::DirFromPath(const CString& path) {
	int pos = path.ReverseFind(_T('\\'));
	return (pos >= 0) ? path.Left(pos) : path;
}
CString CVideoUtil::EnsureDirExists(const CString& dir, LPCTSTR fallback = _T("C:\\")) {
	DWORD attr = GetFileAttributes(dir);
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		return dir;
	return fallback;
}
void CVideoUtil::DrawRactangles(Mat& img, vector<Point> pts, Scalar s, int width)
{
	for (auto pt : pts)
		rectangle(img, Rect(pt.x - width / 2, pt.y - width / 2, width, width), s, FILLED);
}
void CVideoUtil::DrawLines(Mat& img, vector<Point> pts, Scalar s, int len)
{
	for (auto pt : pts)
	{
		line(img, pt + Point(-len / 2, -len / 2), pt + Point(len / 2, len / 2), s, 1, 8, 0);
		line(img, pt + Point(len / 2, -len / 2), pt + Point(-len / 2, len / 2), s, 1, 8, 0);
	}
}
Rect CVideoUtil::ConvRect(Rect2f norm_rect, int img_w, int img_h)
{
	Rect pixel_rect(
		static_cast<int>(norm_rect.x * img_w),
		static_cast<int>(norm_rect.y * img_h),
		static_cast<int>(norm_rect.width * img_w),
		static_cast<int>(norm_rect.height * img_h)
	);
	return pixel_rect;
}
Rect2f CVideoUtil::ConvRect2f(Rect pixel_rect, int img_w, int img_h)
{
	Rect2f norm_rect;
	norm_rect.x = static_cast<float>(pixel_rect.x) / img_w;
	norm_rect.y = static_cast<float>(pixel_rect.y) / img_h;
	norm_rect.width = static_cast<float>(pixel_rect.width) / img_w;
	norm_rect.height = static_cast<float>(pixel_rect.height) / img_h;
	return norm_rect;
}
CRect CVideoUtil::ConvCRect(Rect rc)
{
	CRect rect;
	rect.left = rc.x;
	rect.right = rc.x + rc.width;
	rect.top = rc.y;
	rect.bottom = rc.y + rc.height;
	return rect;
}
Rect CVideoUtil::ConvRect(CRect rc)
{
	return Rect(rc.left, rc.top, rc.Width(), rc.Height());
}
size_t CVideoUtil::argmax_index_nan_safe(const vector<double>& v) {
	if (v.empty()) throw runtime_error("argmax on empty vector");
	auto it = max_element(v.begin(), v.end(),
		[](double a, double b) {
			if (isnan(a)) return true;
			if (isnan(b)) return false;
			return a < b;
		});
	return static_cast<size_t>(distance(v.begin(), it));
}
vector<UINT32> CVideoUtil::removeDuplicates(vector<UINT32> vec) {
	sort(vec.begin(), vec.end());
	vec.erase(unique(vec.begin(), vec.end()), vec.end());
	return vec;
}
CString CVideoUtil::FindMatchingFileName(vector<CString> file_nms, vector<CString> conditions)
{
	for (auto file : file_nms)
	{
		int flag = 0;
		for (auto condition : conditions)
		{
			if (file.Find(condition) < 0)
			{
				flag = 1;
				break;
			}
		}
		if (flag == 0) return file;
	}
	return _T("");
}
vector<size_t> CVideoUtil::removeExtraGroups(const vector<UINT32>& data, size_t N)
{
	vector<size_t> removedIndices;
	if (data.empty() || N == 0) {
		for (size_t i = 0; i < data.size(); ++i)
			removedIndices.push_back(i);
		return removedIndices;
	}
	struct Group { size_t start, end; uint32_t value; };
	vector<Group> groups;
	size_t start = 0;
	for (size_t i = 1; i <= data.size(); ++i) {
		if (i == data.size() || data[i] != data[i - 1]) {
			groups.push_back({ start, i - 1, data[start] });
			start = i;
		}
	}
	for (const auto& g : groups) {
		size_t len = g.end - g.start + 1;
		if (len <= N)
			continue;
		vector<size_t> keepIdx;
		keepIdx.reserve(N);
		double step = static_cast<double>(len - 1) / static_cast<double>(N - 1);
		for (size_t i = 0; i < N; ++i) {
			size_t idx = g.start + static_cast<size_t>(llround(i * step));
			keepIdx.push_back(idx);
		}
		for (size_t i = g.start; i <= g.end; ++i) {
			if (find(keepIdx.begin(), keepIdx.end(), i) == keepIdx.end())
				removedIndices.push_back(i);
		}
	}
	return removedIndices;
}
bool CVideoUtil::IsExist(size_t kk, const vector<size_t>& remove_index)
{
	return find(remove_index.begin(), remove_index.end(), kk) != remove_index.end();
}
```

## File: VideoUtil-v1.1.h
```
#pragma once
#include "afxdialogex.h"
#include <opencv2\opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <vector>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <tuple>
using namespace cv;
using namespace std;
class CVideoUtil :
	public CDialogEx
{
public:
	CVideoUtil();
	~CVideoUtil();
	void DrawImageBMP(CDC& dc, Mat frame, int x, int y, int width, int height);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent, int frame_index);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height, CRect rec);
	void DrawImageBMPwKeepRatio(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawImageBMPwKeepRationFast(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawImageTransparentBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness);
	void DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness, int line_type);
	void DrawLines(Mat& img, vector<Point> pts, Scalar s, int len);
	void DrawRect(CWnd* wnd, CRect rect, COLORREF color);
	void DrawFillRect(CWnd* wnd, const CRect& rect, COLORREF color);
	void DrawFillRect_(CWnd* wnd, int x, int y, int width, int height, COLORREF clr);
	void DrawFillCircle(CWnd* wnd, CPoint center, int radius, COLORREF color);
	void DrawScalarImg(CWnd* wnd, Scalar color, int x, int y, int width, int height);
	void DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index);
	void DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index, vector<CString> labels);
	void DrawIndexList(CWnd* wnd, int x_num, int y_num, vector<int> index);
	void DrawRactangles(Mat& img, vector<Point> pts, Scalar s, int width);
	void DrawResetImage(CWnd* wnd);
	Mat  DrawHist(int w, int h, vector<float> data, float* channel_rangle, int number_bins);
	void DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, int mode, CRect win_rec);
	void DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, CRect win_rec);
	void DrawText_Hangul(CWnd* wnd,
		int x, int y,
		int ptSize,
		const CString& txt,
		COLORREF textColor,
		int type,
		int mode,
		const CRect& win_rec);
	void DrawXYChart(CWnd* wnd, vector<vector<float>>dat);
	vector<int>  DrawXYChart(CWnd* wnd, vector<vector<float>>dat, vector<vector<int>> index, Mat& bg_img_, int cur_index);
	int PlayVideoFile(CWnd* wnd, CString file);
	int PlayVideoFile(CWnd* wnd, CString file, CString txt);
	int PlayImgs(CWnd* wnd, CString txt, vector<Mat> imgs);
	void CopyImg(Mat src, Mat& dst, int x, int y);
	void CopyImg2(Mat src, Mat& dst, int x, int y);
	void CopyImg3(Mat src, Mat& dst, int x, int  y, int width, int height);
	Mat GetImageFromVideowithframeIndex(CString video_file, int index);
	tuple<int, int, double, int> GetVideoInfo(VideoCapture cap);
	tuple<int, int, double, int> GetVideoInfo(const string& filename);
	Rect getRectWidth(Rect rect, int width, Mat img);
	Point getRectCenter(const Rect& rect);
	Rect GetBox(vector <Point2i> pts, double scale, Mat img);
	CString Vector2CString(vector<float> dat);
	CString Vector2CString(vector<Point3f> dat);
	CString Vector2CString(vector<UINT32> dat);
	vector<CString> SplitCString(CString str);
	vector<CString> SplitCString(CString str, CString space);
	vector<string> SplitString(string str);
	vector<CString> SPlitCString(CString str);
	vector<CString> SPlitBracket(CString str);
	string InsertLineFeed_(string& input, int n);
	void CreateFolder(CString csPath);
	vector<CString> GetAllFile(CString srcDir);
	vector<CString> GetAllDir(CString srcDir);
	CString GetFilePrefix(CString dirName);
	CString GetFileExt(CString src);
	CString GetDirName(CString src);
	CString GetFileName(CString dirName);
	bool IsExistFile(CString srcDir, CString file);
	bool IsExistFile(string filename);
	bool IsExistDir(CString dir);
	CString FindMatchingFileName(vector<CString> file_nms, vector<CString> conditions);
	CString LoadLastDir(LPCTSTR dialogKey, LPCTSTR defDir);
	void SaveLastDir(LPCTSTR dialogKey, const CString& dir);
	CString DirFromPath(const CString& path);
	CString EnsureDirExists(const CString& dir, LPCTSTR fallback);
	double GetAngle(Point3f p11, Point3f p12, Point3f p21, Point3f p22);
	double GetAngle(Point3f p1, Point3f p2, Point3f p3);
	double GetAngle(Point3f p1, Point3f p2);
	double GetAngle(Point2f p1, Point2f p2);
	double GetAngle(Point2f p1);
	double Distance(vector<Point> pts1, vector<Point> pts2);
	double Distance(double x1, double y1, double x2, double y2);
	double Distance(double x1, double y1, double z1, double x2, double y2, double z2);
	double Distance(Point3f p1, Point3f p2);
	double Distance(Point3f p1);
	double Distance(Point2d p1, Point2d p2);
	double Distance(Point2f p1, Point2f p2);
	double Distance(Point p1, Point p2);
	double Distance(Point2f p1);
	double Distance(Mat p1, Mat p2);
	void calcNormal(Point3f v0, Point3f v1, Point3f v2, Point3f& out);
	void calcNormal(double v0[3], double v1[3], double v2[3], double out[3]);
	double getRotationAngle(const Mat& R1, const Mat& R2);
	double getRotationAngle(vector<float> dat1, vector<float> dat2);
	Point3f MatToPoint3f(Mat m);
	RECT CvRectToRECT(const Rect& cvRect);
	Rect ConvRect(Rect2f norm_rect, int img_w, int img_h);
	Rect2f ConvRect2f(Rect pixel_rect, int img_w, int img_h);
	CRect ConvCRect(Rect rect);
	Rect ConvRect(CRect rect);
	vector<UINT32> removeDuplicates(vector<UINT32> vec);
	vector<size_t> removeExtraGroups(const vector<UINT32>& data, size_t N);
	size_t argmax_index_nan_safe(const vector<double>& v);
	vector<Point3f> DevideByN(vector<Point3f> pt, double N);
	bool IsExistIndex(vector<int> indexs, int query_index);
	int IsExist(int index, vector<int>data);
	int IsExist(int index, vector<UINT32>data);
	bool IsExist(size_t kk, const vector<size_t>& remove_index);
	int GetNearIndex(vector<int> data, int query);
	int GetInsertIndex(vector<int> data, int query);
	static char* StringToChar(CString str);
	static TCHAR* StringToTCHAR(CString str);
	static string StringToStdString(CString str);
	static CString CharToString(char* str);
	static TCHAR* CharToTCHAR(char* str);
	static CString TCHARToString(TCHAR* str);
	static char* TCHARToChar(TCHAR* str);
	DWORD convert_unicode_to_utf8_string(
		__out string& utf8,
		__in const wchar_t* unicode,
		__in const size_t unicode_size
	);
	DWORD convert_unicode_to_ansi_string(
		__out string& ansi,
		__in const wchar_t* unicode,
		__in const size_t unicode_size
	);
	DWORD CVideoUtil::convert_ansi_to_unicode_string(
		__out wstring& unicode,
		__in const char* ansi,
		__in const size_t ansi_size
	);
	private:
	BITMAPINFO* MakeBMPHeader(int width, int height);
	BITMAPINFO* MakeBMPHeader(int width, int height, int channel);
	int Mat2CImage(Mat* mat, CImage& img);
	int GetKoreaCharNum(CString str);
};
```
