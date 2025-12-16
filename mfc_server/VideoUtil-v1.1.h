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

	//화면출력 관련툴
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
	void DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, CRect win_rec); // type : 0 배경 투명
	void DrawText_Hangul(CWnd* wnd,
		int x, int y,
		int ptSize,            // 포인트 단위 글꼴 크기
		const CString& txt,
		COLORREF textColor,
		int type,              // 0: 투명, 1: 불투명+테두리
		int mode,              // 0: 자동 줄바꿈, 1: 단일 줄
		const CRect& win_rec); // 클리핑/최대 영역	
	void DrawXYChart(CWnd* wnd, vector<vector<float>>dat);
	vector<int>  DrawXYChart(CWnd* wnd, vector<vector<float>>dat, vector<vector<int>> index, Mat& bg_img_, int cur_index);	
	int PlayVideoFile(CWnd* wnd, CString file);
	int PlayVideoFile(CWnd* wnd, CString file, CString txt);
	int PlayImgs(CWnd* wnd, CString txt, vector<Mat> imgs);

	//영상처리툴 모음
	void CopyImg(Mat src, Mat& dst, int x, int y);
	void CopyImg2(Mat src, Mat& dst, int x, int y);
	void CopyImg3(Mat src, Mat& dst, int x, int  y, int width, int height); // all area of src -> partial dst (src<dst)
	Mat GetImageFromVideowithframeIndex(CString video_file, int index);
	tuple<int, int, double, int> GetVideoInfo(VideoCapture cap);
	tuple<int, int, double, int> GetVideoInfo(const string& filename);
	Rect getRectWidth(Rect rect, int width, Mat img);
	Point getRectCenter(const Rect& rect);
	Rect GetBox(vector <Point2i> pts, double scale, Mat img); // margin은 rect의 스케일	

	//CString 관련툴
	CString Vector2CString(vector<float> dat);
	CString Vector2CString(vector<Point3f> dat);
	CString Vector2CString(vector<UINT32> dat);
	vector<CString> SplitCString(CString str);
	vector<CString> SplitCString(CString str, CString space);
	vector<string> SplitString(string str);
	vector<CString> SPlitCString(CString str);
	vector<CString> SPlitBracket(CString str); //() 사이로 분리된 단어 리턴

	//File처리 관련툴
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
	
	CString FindMatchingFileName(vector<CString> file_nms, vector<CString> conditions); //conditions의 단어가 포함되는 파일 리스트 얻기

	//파일선택 다이얼로그 이전값 기억을 위한 함수
	CString LoadLastDir(LPCTSTR dialogKey, LPCTSTR defDir);
	void SaveLastDir(LPCTSTR dialogKey, const CString& dir);
	CString DirFromPath(const CString& path);
	// 존재 확인 후 대체 기본값으로 폴백
	CString EnsureDirExists(const CString& dir, LPCTSTR fallback);

	//Math tools
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

	//Type conversion
	Point3f MatToPoint3f(Mat m);	
	RECT CvRectToRECT(const Rect& cvRect);
	Rect ConvRect(Rect2f norm_rect, int img_w, int img_h);
	Rect2f ConvRect2f(Rect pixel_rect, int img_w, int img_h);
	CRect ConvCRect(Rect rect);
	Rect ConvRect(CRect rect);

	//기타 툴 모음
	vector<UINT32> removeDuplicates(vector<UINT32> vec); //벡터 정렬 및 중복된 원소 제거
	vector<size_t> removeExtraGroups(const vector<UINT32>& data, size_t N); //그룹네에 N을 초과하는 그룹에서 초과 원소를 균등 제거
	size_t argmax_index_nan_safe(const vector<double>& v); //벡터가 비어있거나 값이 무한대일때 예외처리
	vector<Point3f> DevideByN(vector<Point3f> pt, double N);
	bool IsExistIndex(vector<int> indexs, int query_index);
	int IsExist(int index, vector<int>data);
	int IsExist(int index, vector<UINT32>data);
	bool IsExist(size_t kk, const vector<size_t>& remove_index);
	int GetNearIndex(vector<int> data, int query);
	int GetInsertIndex(vector<int> data, int query);	

	//한글코드관련 툴
	static char* StringToChar(CString str);              // CString → Char
	static TCHAR* StringToTCHAR(CString str);            // CString → TCHAR
	static string StringToStdString(CString str);   // CString → string
	static CString CharToString(char* str);              // Char → CString
	static TCHAR* CharToTCHAR(char* str);                // Char → TCHAR
	static CString TCHARToString(TCHAR* str);            // TCHAR → CString
	static char* TCHARToChar(TCHAR* str);                // TCHAR → Char

/************************************************************************/
/* 유니코드 ↔ 멀티바이트 문자열 변환함수 정의부                             */
/************************************************************************/
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
