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
using std::tuple;



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
	//if (frame.empty()) return;

	//int src_width = frame.cols * height / frame.rows;

	//int white_width = (width - src_width) / 2 + 1;
	//Mat white_img = Mat::zeros(height, white_width, frame.type()); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)

	//DrawImageBMP(wnd, white_img, x, y, white_width, height);
	//DrawImageBMP(wnd, white_img, x + width - white_width, y, white_width, height);
	//DrawImageBMP(wnd, frame, x + white_width, y, src_width, height);

	if (frame.empty()) return;	

	Mat white_img(height, width, CV_8UC3, Scalar(255, 255, 255)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
	Mat src_resize_img;
	int src_width = frame.cols* height / frame.rows;
	resize(frame, src_resize_img, Size(src_width, height));
	
	src_resize_img.copyTo(white_img(Range(0, height), Range(width/2-src_width /2, width/2 + src_width / 2)));

	DrawImageBMP(wnd, white_img, x, y, width, height);
}
//
//void CVideoUtil::DrawImageBMPwKeepRatio(CWnd* wnd, Mat frame, int x, int y, int width, int height)
//{
//	if (frame.empty()) return;
//
//	double src_ratio = (double)frame.rows / (double)frame.cols;
//	double out_ratio = (double)height / (double)width;
//
//	int blank_width;
//	if (src_ratio > out_ratio) blank_width = width - (double)height / (double)src_ratio;
//	else blank_width = 0;
//
//	if (blank_width != 0)
//	{
//		Mat white_img(height, width, CV_8UC3, Scalar(255, 255, 255)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
//		Mat re_img;
//		resize(white_img, re_img, Size(width, height));
//		CImage m_img;
//		Mat2CImage(&re_img, m_img);
//		CClientDC dc(wnd);
//		m_img.Draw(dc, x, y);
//	}
//
//	Mat re_img;
//	resize(frame, re_img, Size(width - blank_width, height));
//	CImage m_img;
//	Mat2CImage(&re_img, m_img);
//	CClientDC dc(wnd);
//	m_img.Draw(dc, x + blank_width / 2, y);
//}


void CVideoUtil::DrawImageBMP(CDC& dc, Mat frame, int x, int y, int width, int height)
{
	if (frame.empty()) return;

	// 항상 32bpp BGRA로 통일(실패/깜빡임 방지)
	Mat img;
	switch (frame.type())
	{
	case CV_8UC4: img = frame; break;
	case CV_8UC3: cvtColor(frame, img, COLOR_BGR2BGRA); break;
	case CV_8UC1: cvtColor(frame, img, COLOR_GRAY2BGRA); break;
	default: return;
	}

	// 힙 할당 대신 스택 BITMAPINFO 사용 + 상하반전 방지(음수 높이)
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = img.cols;
	bmi.bmiHeader.biHeight = -img.rows;       // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	dc.RealizePalette();
	::SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);

	::StretchDIBits(dc.GetSafeHdc(),
		x, y, width, height,
		0, 0, img.cols, img.rows,
		img.data, &bmi, DIB_RGB_COLORS, SRCCOPY);

	::GdiFlush(); // 즉시 플러시(선택)
}


void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	if (!wnd) return;
	CClientDC dc(wnd);
	DrawImageBMP(dc, frame, x, y, width, height); // 내부 공용 구현 호출

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
		str.Format(_T("%d"), frame_index);	
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
		//if (frame.channels() == 3) 
		//	cvtColor(frame, frame, COLOR_BGR2BGRA);
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

void CVideoUtil::CopyImg(Mat src, Mat& dst, int x, int  y) // partial src -> all area of dst (src>dst)
{
	Mat roi = src(Rect(x, y, dst.cols, dst.rows));
	dst.copyTo(roi);
}

void CVideoUtil::CopyImg2(Mat src, Mat& dst, int x, int  y) // all area of src -> partial dst (src<dst)
{
	if ((y + src.rows) <= dst.rows && (x + src.cols) <= dst.cols)
	{
		Mat roi = dst(Rect(x, y, src.cols, src.rows));
		src.copyTo(roi);
	}
}

void CVideoUtil::CopyImg3(Mat src, Mat& dst, int dst_x, int  dst_y, int width, int height) // all area of src -> partial dst (src<dst)
{
	if ((dst_y + height) <= dst.rows && (dst_x + width) <= dst.cols)
	{
		Mat resize_img;
		resize(src, resize_img, Size(width, height), INTER_CUBIC);

		CopyImg2(resize_img, dst, dst_x, dst_y);	
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

	Mat white_img(frame_rect.Height(), frame_rect.Width(), CV_8UC3, Scalar(240, 240, 240)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
	

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

	Mat white_img(frame_rect.Height(), frame_rect.Width(), CV_8UC3, Scalar(240, 240, 240)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)


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

	hMemDC = CreateCompatibleDC(hdc); // 메모리 DC를 만든다

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
	DeleteObject(hImage); // 선택 해제된 비트맵을 제거한다
	DeleteDC(hMemDC); // 메모리 DC를 제거한다

	delete[] pBmp;
}

void CVideoUtil::DrawText_Hangul(CWnd* wnd,
	int x, int y,
	int ptSize,            // 포인트 단위 글꼴 크기
	const CString& txt,
	COLORREF textColor,
	int type,              // 0: 투명, 1: 불투명+테두리
	int mode,              // 0: 자동 줄바꿈, 1: 단일 줄
	const CRect& win_rec)  // 클리핑/최대 영역
{
	if (!wnd || !::IsWindow(wnd->GetSafeHwnd()) || txt.IsEmpty())
		return;

	CClientDC dc(wnd);
	const int saved = dc.SaveDC(); // 상태 복구를 위해 저장

	// ----- 클리핑 영역 -----
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);
	dc.SelectClipRgn(&rgn);

	// ----- 글꼴 생성 (pt → 논리픽셀 변환) -----
	LOGFONT lf = {};
	lf.lfWeight = FW_BOLD;
	lf.lfCharSet = DEFAULT_CHARSET;
	// 음수 높이는 문자 높이를 의미 (포인트 → 픽셀 변환)
	int dpiY = dc.GetDeviceCaps(LOGPIXELSY);
	lf.lfHeight = -MulDiv(ptSize, dpiY, 72);

	CFont font;
	font.CreateFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&font);

	// ----- 색상/배경 -----
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

	// ----- 서식 플래그 -----
	UINT fmt = DT_LEFT | DT_TOP | DT_NOPREFIX;
	if (mode == 0) {
		fmt |= DT_WORDBREAK;               // 자동 줄바꿈
	}
	else {
		fmt |= DT_SINGLELINE | DT_END_ELLIPSIS; // 단일줄, 길면 말줄임표
	}

	// ----- 그릴 영역 계산 -----
	// 시작점 (x,y)에서 win_rec의 우/하 경계까지 허용
	CRect rcDraw(x, y, win_rec.right, win_rec.bottom);

	// 먼저 크기 계산
	CRect rcCalc = rcDraw;
	dc.DrawText(txt, rcCalc, fmt | DT_CALCRECT);

	// 단일줄 모드일 경우, 너무 긴 텍스트라면 우측 경계를 win_rec로 제한
	if (mode == 1) {
		if (rcCalc.right > win_rec.right)
			rcCalc.right = win_rec.right;
		if (rcCalc.bottom > win_rec.bottom)
			rcCalc.bottom = y + (rcCalc.Height()); // 높이는 한 줄 높이 유지
	}
	else {
		// 워드브레이크 모드에서는 계산된 너비가 클 수 있으니, win_rec로 제한
		if (rcCalc.right > win_rec.right) rcCalc.right = win_rec.right;
		if (rcCalc.bottom > win_rec.bottom) rcCalc.bottom = win_rec.bottom;
	}

	// ----- 실제 출력 -----
	dc.DrawText(txt, rcCalc, fmt);

	// ----- 테두리(옵션) -----
	if (type == 1) {
		dc.Draw3dRect(rcCalc, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	}

	// ----- 상태 복구 -----
	dc.SetBkMode(oldBkMode);
	if (type == 1) dc.SetBkColor(oldBkColor);
	dc.SelectObject(pOldFont);
	dc.SelectClipRgn(nullptr);
	dc.RestoreDC(saved);
}


void CVideoUtil::DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, int mode, CRect win_rec) // type : 0 배경 투명, mode : 0 자동 줄바꿈
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

	// --- 0) 클리핑 영역 ---
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);
	dc.SelectClipRgn(&rgn);

	// --- 1) 폰트 생성: DPI 반영 (size = pt) ---
	const int dpi = dc.GetDeviceCaps(LOGPIXELSY);
	LOGFONT lf = {};
	lf.lfHeight = -MulDiv(size, dpi, 72); // 포인트 → 논리픽셀
	lf.lfWeight = FW_BOLD;                // DWRITE_* 대신 GDI 값 사용
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI")); // 원하시는 폰트로 변경 가능
	CFont font; font.CreateFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&font);

	// --- 2) 텍스트 크기 계산 ---
	CRect calc = CRect(0, 0, 0, 0);
	// 사전 계산용: DT_CALCRECT는 x,y 무시 → (0,0) 기준으로 크기 산출
	dc.DrawText(txt, &calc, DT_CALCRECT | DT_SINGLELINE);

	// 패딩(여백)
	const int padX = MulDiv(size, dpi, 72) / 2;   // 글자 크기 절반 정도 가로 여백
	const int padY = MulDiv(size, dpi, 72) / 4;   // 글자 크기 1/4 세로 여백

	CRect rec;
	rec.left = x;
	rec.top = y;
	rec.right = x + calc.Width() + padX * 2;
	rec.bottom = y + calc.Height() + padY * 2;

	// --- 3) 배경/텍스트 색 세팅 ---
	dc.SetBkMode(TRANSPARENT);

	if (type == 1) {
		// color = 배경(사각형) 색
		dc.FillSolidRect(&rec, color);

		// 배경 대비 텍스트 색 자동 결정(간단한 명도 기준)
		auto luminance = [](COLORREF c) {
			double r = GetRValue(c), g = GetGValue(c), b = GetBValue(c);
			return 0.2126 * r + 0.7152 * g + 0.0722 * b; // sRGB 가중치
			};
		COLORREF textCol = (luminance(color) < 128.0) ? RGB(255, 255, 255) : RGB(0, 0, 0);
		dc.SetTextColor(textCol);

		// 테두리(옵션) 원하시면 주석 해제
		// CPen pen(PS_SOLID, 1, RGB(0,0,0));
		// CPen* oldPen = dc.SelectObject(&pen);
		// dc.SelectStockObject(NULL_BRUSH);
		// dc.Rectangle(rec);
		// dc.SelectObject(oldPen);

	}
	else {
		// type == 0 : 텍스트만, color = 텍스트 색
		dc.SetTextColor(color);
	}

	// --- 4) 텍스트 그리기 (패딩 고려해서 정렬) ---
	CRect textRc = rec;
	textRc.DeflateRect(padX, padY);
	dc.DrawText(txt, &textRc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

	// 정중앙 정렬 원하면 위 한 줄을 아래로 바꾸세요:
	// dc.DrawText(txt, &rec, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	// --- 5) 정리 ---
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


//
//vector<CString> CVideoUtil::GetAllFile(CString srcDir2)
//{
//	vector<CString> ret;
//
//	string path;
//	CString srcDir;
//	CString end_srcDir2 = srcDir2.Right(1);
//	if (end_srcDir2.Compare(_T("\\")) == 0) srcDir = srcDir2.Left(srcDir2.GetLength() - 1);
//	else srcDir = srcDir2;
//	srcDir.Append(_T("\\."));
//	wstring wstr = srcDir.operator LPCWSTR();
//	convert_unicode_to_utf8_string(path, wstr.c_str(), wstr.size());
//	
//	for (auto& p : fs::recursive_directory_iterator(path))
//	{
//		cout << p << endl;							//결과: root/dir1/text.txt
//		cout << p.path().filename() << endl;			//결과: "test.txt"
//		cout << p.path().filename().string() << endl;	//결과: test.txt
//		wstring unicode_2;
//		convert_utf8_to_unicode_string(unicode_2, p.path().filename().string().c_str(), p.path().filename().string().size());
//		ret.push_back(unicode_2.c_str());
//	}
//
//	return ret;
//}

vector<CString> CVideoUtil::GetAllDir(CString srcDir2) // / is attached to the end of file
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
	// 벡터 길이
	double d1 = sqrt(p1.x * p1.x + p1.y * p1.y);
	double d2 = sqrt(p2.x * p2.x + p2.y * p2.y);

	// 제로벡터 예외 처리
	if (d1 < 1e-8 || d2 < 1e-8) {
		return 0.0;
	}

	// 내적 (dot)
	double dot = p1.x * p2.x + p1.y * p2.y;
	// 외적 크기 (cross product in 2D)
	double cross = p1.x * p2.y - p1.y * p2.x;

	// 각도 계산
	double angle = atan2(fabs(cross), dot) * 180.0 / CV_PI;
	// atan2(y, x) → 0~180° 보장됨

	return angle;
}

double CVideoUtil::GetAngle(Point2f p1)
{
	if (p1.x == 0 && p1.y == 0) return 0;
	else return atan2(p1.x, p1.y) * 180 / CV_PI; // -180~180
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

//int CVideoUtil::Mat2CImage(Mat* mat, CImage& img)
//{
//	if (!mat || mat->empty())
//		return -1;
//	int nBPP = mat->channels() * 8;
//	img.Create(mat->cols, mat->rows, nBPP);
//	if (nBPP == 8)
//	{
//		static RGBQUAD pRGB[256];
//		for (int i = 0; i < 256; i++)
//			pRGB[i].rgbBlue = pRGB[i].rgbGreen = pRGB[i].rgbRed = i;
//		img.SetColorTable(0, 256, pRGB);
//	}
//	uchar* psrc = mat->data;
//	uchar* pdst = (uchar*)img.GetBits();
//	int imgPitch = img.GetPitch();
//	for (int y = 0; y < mat->rows; y++)
//	{
//		memcpy(pdst, psrc, mat->cols * mat->channels());//mat->step is incorrect for those images created by roi (sub-images!)
//		psrc += mat->step;
//		pdst += imgPitch;
//	}
//
//	return 0;
//}


void CVideoUtil::DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness)
{
	CClientDC dc(wnd);
	CPen pen;
	pen.CreatePen(PS_SOLID, thickness, color);


	CPen* oldPen = dc.SelectObject(&pen);

	dc.MoveTo(p1.x, p1.y);
	dc.LineTo(p2.x, p2.y);

	dc.SelectObject(oldPen);

	// 만약 빨간색으로 그린 후 파란색으로 그려야 한다면, 다시 새로운 펜을 만들고 그려줘야 한다.
	// 펜 굵기가 10인 파란색 실선을 그림다.
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

	// 만약 빨간색으로 그린 후 파란색으로 그려야 한다면, 다시 새로운 펜을 만들고 그려줘야 한다.
	// 펜 굵기가 10인 파란색 실선을 그림다.
	pen.DeleteObject();

}


void CVideoUtil::DrawRect(CWnd* wnd, CRect rect, COLORREF color)
{
	if (!wnd || !::IsWindow(wnd->GetSafeHwnd())) return;

	const int thickness = 3;

	CClientDC dc(wnd);

	// 클라이언트 영역으로 클리핑
	CRect clientRect;
	wnd->GetClientRect(&clientRect);
	dc.IntersectClipRect(&clientRect);

	// 펜 준비
	CPen pen;
	pen.CreatePen(PS_SOLID, thickness, color);
	CPen* pOldPen = dc.SelectObject(&pen);

	// 외곽선만 그릴 것이므로 속이 비는 스톡 브러시 선택 (올바른 방법)
	CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(HOLLOW_BRUSH); // == NULL_BRUSH

	// 사각형 그리기
	dc.Rectangle(rect);

	// 원복
	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBrush);

	pen.DeleteObject(); // 스톡 브러시는 Delete 금지 (우린 선택만 했음)
}


// 내부 사각형만 채우기(외곽선 없음) + 클라이언트 밖으로 못 나가게
void CVideoUtil::DrawFillRect(CWnd* wnd, const CRect& rect, COLORREF color)
{
	if (!wnd) return;

	CClientDC dc(wnd);

	// 클라이언트 영역으로 클리핑
	CRect client;
	wnd->GetClientRect(&client);
	dc.IntersectClipRect(&client);

	// 클라이언트와의 교집합만 칠하고 싶으면 주석 해제
	// CRect r = rect; r.IntersectRect(rect, client);
	// dc.FillSolidRect(&r, color);

	// 클리핑이 걸렸으니 그냥 rect로 칠해도 바깥은 안 칠해짐
	dc.FillSolidRect(&rect, color);
}

// 내부 원(원형)만 채우기(외곽선 없음) + 클라이언트 밖으로 못 나가게
void CVideoUtil::DrawFillCircle(CWnd* wnd, CPoint center, int radius, COLORREF color)
{
	if (!wnd || radius <= 0) return;

	CClientDC dc(wnd);

	// 클라이언트 영역으로 클리핑
	CRect client;
	wnd->GetClientRect(&client);
	dc.IntersectClipRect(&client);

	// 외곽선(펜) 제거 → 내부만 채움
	HPEN hOldPen = (HPEN)dc.SelectObject(GetStockObject(NULL_PEN));

	// 채우기 브러시
	CBrush brush;
	brush.CreateSolidBrush(color);
	CBrush* pOldBrush = dc.SelectObject(&brush);

	// 중심/반지름 → 바운딩 사각형
	CRect r(center.x - radius, center.y - radius,
		center.x + radius, center.y + radius);

	// 원(타원) 채우기
	dc.Ellipse(&r);

	// 원복
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



/************************************************************************/
/* 유니코드 ↔ 멀티바이트 문자열 변환함수 구현부                             */
/************************************************************************/
//
// CString → Char
//
char* CVideoUtil::StringToChar(CString str)
{
	char* szStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = str.GetLength() + 1;
	TCHAR* tszTemp = NULL;
	tszTemp = new TCHAR[nLen];
	memset(tszTemp, 0x00, nLen * sizeof(TCHAR));
	_tcscpy_s(tszTemp, nLen, str);
	// Get size (실제사용되는바이트사이즈)
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

//
// CString → TCHAR
//
TCHAR* CVideoUtil::StringToTCHAR(CString str)
{
	TCHAR* tszStr = NULL;
	int nLen = str.GetLength() + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	_tcscpy_s(tszStr, nLen, str);

	return tszStr;
}

//
// CString → string
//
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

//
// Char → CString
//
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

//
// Char → TCHAR
//
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

//
// TCHAR → CString
//
CString CVideoUtil::TCHARToString(TCHAR* str)
{
	CString cStr;
	cStr.Format(_T("%s"), str);
	return cStr;
}

//
// TCHAR → Char
//
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

	// 원하는 프레임으로 이동
	cap.set(CAP_PROP_POS_FRAMES, index);

	// 프레임 읽기
	Mat frame;
	if (!cap.read(frame)) {
		cerr << "프레임 읽기 실패 (index=" << index << ")\n";
		return Mat();
	}

	return frame.clone(); // 안전하게 반환


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
	if (data.empty()) return -1; // 빈 벡터 예외 처리

	auto it = min_element(data.begin(), data.end(),
		[query](int a, int b) {
			return abs(a - query) < abs(b - query);
		});

	return distance(data.begin(), it);

}

int CVideoUtil::GetInsertIndex(vector<int> data, int query)
{
	if (data.empty()) return -1; // 빈 벡터 예외 처리

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
//	for (int m = 0; m < (dat[0].size() + (dat[0].size() % 20) ); m += (dat[0].size() / 20))
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
	// UpdateData(TRUE);
	// csPath = m_csTopFolderName + csPath;

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

// 존재 확인 후 대체 기본값으로 폴백
CString CVideoUtil::EnsureDirExists(const CString& dir, LPCTSTR fallback = _T("C:\\")) {
	DWORD attr = GetFileAttributes(dir);
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		return dir;
	return fallback;
}

void CVideoUtil::DrawRactangles(Mat& img, vector<Point> pts, Scalar s, int width) //scalar B G R order
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
			if (isnan(a)) return true;   // a가 NaN이면 b가 우선(= a<b 취급)
			if (isnan(b)) return false;  // b가 NaN이면 a가 우선
			return a < b;
		});
	return static_cast<size_t>(distance(v.begin(), it));
}

vector<UINT32> CVideoUtil::removeDuplicates(vector<UINT32> vec) {
	sort(vec.begin(), vec.end());
	vec.erase(unique(vec.begin(), vec.end()), vec.end());
	return vec; // 정렬 + 중복제거된 벡터 반환
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


// data : 원본 데이터
// N : 유지할 그룹 개수
// return : 제거되는 인덱스들
vector<size_t> CVideoUtil::removeExtraGroups(const vector<UINT32>& data, size_t N)
{
	vector<size_t> removedIndices;

	if (data.empty() || N == 0) {
		for (size_t i = 0; i < data.size(); ++i)
			removedIndices.push_back(i);
		return removedIndices;
	}

	// 1️⃣ 연속된 동일값 그룹 탐색
	struct Group { size_t start, end; uint32_t value; };
	vector<Group> groups;
	size_t start = 0;

	for (size_t i = 1; i <= data.size(); ++i) {
		if (i == data.size() || data[i] != data[i - 1]) {
			groups.push_back({ start, i - 1, data[start] });
			start = i;
		}
	}

	// 2️⃣ 각 그룹 내부에서 N개 이하만 균등 샘플링
	for (const auto& g : groups) {
		size_t len = g.end - g.start + 1;

		if (len <= N)
			continue; // N개 이하 → 제거 없음

		// 균등 간격 샘플 인덱스 계산
		vector<size_t> keepIdx;
		keepIdx.reserve(N);
		double step = static_cast<double>(len - 1) / static_cast<double>(N - 1);

		for (size_t i = 0; i < N; ++i) {
			size_t idx = g.start + static_cast<size_t>(llround(i * step));
			keepIdx.push_back(idx);
		}

		// 나머지 인덱스 제거 리스트에 추가
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