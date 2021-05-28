#include "CaptureHelper.h"
#include <WinNT.h>
//#include <map>
//#include <list>
//#include <algorithm>
#include "resource.h"
#include "opencv2/imgproc.hpp"
#include <opencv2\imgproc\types_c.h>
#include <windowsx.h>

#include "DXGIManager.hpp"

// C ABI
extern "C" {
	//__declspec(dllexport)
	void init() {
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	}

	//__declspec(dllexport)
	void uninit() {
		CoUninitialize();
	}

	//__declspec(dllexport)
	void* create_dxgi_manager() {
		DXGIManager* dxgi_manager = new DXGIManager();
		dxgi_manager->setup();
		return (void*)dxgi_manager;
	}
	//__declspec(dllexport)
	void delete_dxgi_manager(void* dxgi_manager) {
		DXGIManager* m = (DXGIManager*)(dxgi_manager);
		delete m;
	}

	//__declspec(dllexport)
	void set_timeout(void* dxgi_manager, uint32_t timeout) {
		((DXGIManager*)dxgi_manager)->set_timeout(timeout);
	}

	//__declspec(dllexport)
	void set_capture_source(void* dxgi_manager, uint16_t cs) {
		((DXGIManager*)dxgi_manager)->set_capture_source(cs);
	}

	//__declspec(dllexport)
	uint16_t get_capture_source(void* dxgi_manager) {
		return ((DXGIManager*)dxgi_manager)->get_capture_source();
	}

	//__declspec(dllexport)
	bool refresh_output(void* dxgi_manager) {
		return ((DXGIManager*)dxgi_manager)->refresh_output();
	}

	//__declspec(dllexport)
	void get_output_dimensions(void* const dxgi_manager, uint32_t* width, uint32_t* height) {
		RECT dimensions = ((DXGIManager*)dxgi_manager)->get_output_rect();
		*width = dimensions.right - dimensions.left;
		*height = dimensions.bottom - dimensions.top;
	}

	// Return the CaptureResult of acquiring frame and its data
	//__declspec(dllexport)
	uint8_t get_frame_bytes(void* dxgi_manager, size_t* o_size, uint8_t** o_bytes) {
		return ((DXGIManager*)dxgi_manager)->get_output_data(o_bytes, o_size);
	}
}

using namespace cv;
using namespace std;

DWORD WINAPI CInProc(LPVOID);
DWORD WINAPI CDlgProc(LPVOID);
DWORD WINAPI CFXProc(LPVOID);
DWORD WINAPI ColorProc(LPVOID);

bool inWork = true;

HWND hDlg;

ConfigHandler* config;
FXHelper* fxh;

UCHAR  imgz[12 * 3];

DWORD uiThread, cuThread;
HANDLE uiHandle = 0, cuHandle = 0;

void* dxgi_manager;

CaptureHelper::CaptureHelper(HWND dlg, ConfigHandler* conf, FXHelper* fhh)
{
	init();
    dxgi_manager = create_dxgi_manager();
	SetCaptureScreen(conf->mode);
	set_timeout(dxgi_manager, 100);

	hDlg = dlg;
	config = conf;
	fxh = fhh;
}

CaptureHelper::~CaptureHelper()
{
	delete_dxgi_manager(dxgi_manager);
	uninit();
}

void CaptureHelper::SetCaptureScreen(int mode) {
	set_capture_source(dxgi_manager, mode);
}

void CaptureHelper::Start()
{
	if (dwHandle == 0) {
		inWork = true;
		dwHandle = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			CInProc,        // name of the thread function
			dxgi_manager,//screenCapturer,
			0,                 // default startup flags
			&dwThreadID);
	}
}

void CaptureHelper::Stop()
{
	DWORD exitCode;
	if (dwHandle != 0) {
		inWork = false;
		GetExitCodeThread(dwHandle, &exitCode);
		while (exitCode == STILL_ACTIVE) {
			Sleep(100);
			GetExitCodeThread(dwHandle, &exitCode);
		}
		CloseHandle(dwHandle);
		GetExitCodeThread(uiHandle, &exitCode);
		while (exitCode == STILL_ACTIVE) {
			Sleep(100);
			GetExitCodeThread(uiHandle, &exitCode);
		}
		CloseHandle(uiHandle);
		GetExitCodeThread(cuHandle, &exitCode);
		if (exitCode == STILL_ACTIVE)
			CloseHandle(cuHandle);
		dwHandle = uiHandle = cuHandle = 0;
	}
}

void CaptureHelper::Restart() {
	Stop();
	SetCaptureScreen(config->mode);
	Start();
}

cv::Mat extractHPts(const cv::Mat& inImage)
{
	// container for storing Hue Points
	cv::Mat listOfHPts(inImage.cols * inImage.rows, 1, CV_32FC1);
	//listOfHPts = cv::Mat::zeros(inImage.cols * inImage.rows, 1, CV_32FC1);
	Mat res[3];
	split(inImage, res);
	res[0].reshape(1, inImage.cols * inImage.rows).convertTo(listOfHPts, CV_32FC1);
	return listOfHPts;
}

// function for extracting dominant color from foreground pixels
cv::Mat getDominantColor(const cv::Mat& inImage, const cv::Mat& ptsLabel)
{
	// first we determine which cluster is foreground
	// assuming the our object of interest is the biggest object in the image region

	cv::Mat fPtsLabel, sumLabel;
	ptsLabel.convertTo(fPtsLabel, CV_32FC1);

	cv::reduce(fPtsLabel, sumLabel, 0, CV_REDUCE_SUM, CV_32FC1);

	int numFGPts = 0;

	if (sumLabel.at<float>(0, 0) < ptsLabel.rows / 2)
	{
		// invert the 0's and 1's where 1s represent foreground
		fPtsLabel = (fPtsLabel - 1) * (-1);
		numFGPts = fPtsLabel.rows - (int) sumLabel.at<float>(0, 0);
	}
	else
		numFGPts = (int) sumLabel.at<float>(0, 0);

	// to find dominant color, I just average all points belonging to foreground
	cv::Mat dominantColor;
	dominantColor = cv::Mat::zeros(1, 3, CV_32FC1);

	int idx = 0; //int fgIdx = 0;
	for (int j = 0; j < inImage.rows; j++)
	{
		for (int i = 0; i < inImage.cols; i++)
		{
			if (fPtsLabel.at<float>(idx++, 0) == 1)
			{
				cv::Vec3b tempVec;
				tempVec = inImage.at<cv::Vec3b>(j, i);
				dominantColor.at<float>(0, 0) += (tempVec[0]);
				dominantColor.at<float>(0, 1) += (tempVec[1]);
				dominantColor.at<float>(0, 2) += (tempVec[2]);

				//fgIdx++;
			}
		}
	}

	dominantColor /= numFGPts;

	// convert to uchar so that it can be used directly for visualization
	cv::Mat dColor(1, 3, CV_8UC1);

	dominantColor.convertTo(dColor, CV_8UC1);

	return dColor;
}

// Commented part is for multi-thread processing, but single-thread working better.
/*struct procData {
	Mat src;
	UCHAR* dst;
	bool done;
};

uint fcount = 0;

DWORD WINAPI ColorProc(LPVOID inp) {
	procData* src = (procData*) inp;
	Mat hPts;
	hPts = extractHPts(src->src);
	Mat ptsLabel, kCenters;
	cv::kmeans(hPts, 2, ptsLabel, cv::TermCriteria(cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 1000, 0.00001)), 5, cv::KMEANS_PP_CENTERS, kCenters);

	Mat dColor;
	dColor = getDominantColor(src->src, ptsLabel);
	src->dst[0] = dColor.ptr<UCHAR>()[0];
	src->dst[1] = dColor.ptr<UCHAR>()[1];
	src->dst[2] = dColor.ptr<UCHAR>()[2];
	//delete src;
	//fcount++;
	src->done = true;
	//ExitThread(1);
	return 0;
}*/

void FillColors(Mat* src) {
	uint w = src->cols / 4, h = src->rows / 3;
	Mat cPos( w, h, CV_8UC3), hPts;
	Mat ptsLabel, kCenters, dColor;
	//DWORD pThread;
	//procData callData[3][4];
	for (uint dy = 0; dy < 3; dy++)
		for (uint dx = 0; dx < 4; dx++) {
			uint ptr = (dy * 4 + dx) * 3;
			cPos = src->rowRange(dy * h + 1, (dy + 1) * h)
				.colRange(dx * w + 1, (dx + 1) * w);
			/*callData[dy][dx].src = cPos;
			callData[dy][dx].dst = imgz + ptr;
			callData[dy][dx].done = false;
			CreateThread(
				NULL,              // default security
				4*1024*1024,                 // default stack size
				ColorProc,        // name of the thread function
				&callData[dy][dx],
				0,                 // default startup flags
				&pThread);*/
			hPts = extractHPts(cPos);
			cv::kmeans(hPts, 2, ptsLabel, cv::TermCriteria(cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 1000, 0.00001)), 5, cv::KMEANS_PP_CENTERS, kCenters);
			dColor = getDominantColor(cPos, ptsLabel);
			imgz[ptr] = dColor.ptr<UCHAR>()[0];
			imgz[ptr + 1] = dColor.ptr<UCHAR>()[1];
			imgz[ptr + 2] = dColor.ptr<UCHAR>()[2];
		}
	/*bool finished = false;
	do {
		Sleep(5);
		finished = true;
		for (int y=0; y<3; y++)
			for (int x=0; x<4; x++)
				if (!callData[y][x].done) {
					finished = false;
					break;
				}
	} while (!finished);*/
}

DWORD WINAPI CInProc(LPVOID param)
{
	UINT w, h, cdp = 4;
	UCHAR* img = NULL;
	DWORD exitCode = 0;
	size_t buf_size;
	ULONGLONG lastTick = GetTickCount64();

	while (inWork) {
		UINT div = config->divider;
		get_output_dimensions(dxgi_manager, &w, &h);
		if (!w && !h) {
			// Restart capture!
			set_capture_source(dxgi_manager, config->mode);
			get_output_dimensions(dxgi_manager, &w, &h);
		} else
		// Resize & calc
			if (get_frame_bytes(dxgi_manager, &buf_size, &img) == CR_OK && img != NULL) {
				Mat* src = new Mat(h, w, CV_8UC4, img);// , st);
				cv::cvtColor(*src, *src, CV_RGBA2RGB);

				cv::resize(*src, *src, Size(w / div, h / div), 0, 0, INTER_AREA);
				FillColors(src);
				delete src;

				// Update lights
				if (uiHandle)
					GetExitCodeThread(uiHandle, &exitCode);
				if (exitCode != STILL_ACTIVE)
					uiHandle = CreateThread(
						NULL,              // default security
						0,                 // default stack size
						CFXProc,        // name of the thread function
						imgz,
						0,                 // default startup flags
						&uiThread);
				// Update UI
				if (cuHandle)
					GetExitCodeThread(cuHandle, &exitCode);
				if (exitCode != STILL_ACTIVE)
					cuHandle = CreateThread(
						NULL,              // default security
						0,                 // default stack size
						CDlgProc,        // name of the thread function
						imgz,
						0,                 // default startup flags
						&cuThread);
			}
		ULONGLONG nextTick = GetTickCount64();
		if (nextTick - lastTick < 100) {
			Sleep(100 - (DWORD)(nextTick - lastTick));
		}
		lastTick = GetTickCount64();
	}
	return 0;
}

DWORD WINAPI CDlgProc(LPVOID param)
{
	UCHAR* img = (UCHAR *)param;
	RECT rect;
	HBRUSH Brush = NULL;
	for (int i = 0; i < 12; i++) {
		HWND tl = GetDlgItem(hDlg, IDC_BUTTON1 + i);
		HWND cBid = GetDlgItem(hDlg, IDC_CHECK1 + i);
		GetWindowRect(tl, &rect);
		HDC cnt = GetWindowDC(tl);
		rect.bottom -= rect.top;
		rect.right -= rect.left;
		rect.top = rect.left = 0;
		// BGR!
		Brush = CreateSolidBrush(RGB(img[i*3+2], img[i*3+1], img[i*3]));
		FillRect(cnt, &rect, Brush);
		DeleteObject(Brush);
		UINT state = IsDlgButtonChecked(hDlg, IDC_CHECK1 + i); 
		if ((state & BST_CHECKED))            
			DrawEdge(cnt, &rect, EDGE_SUNKEN, BF_RECT);   
		else
			DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);    
		RedrawWindow(cBid, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	return 0;
}

DWORD WINAPI CFXProc(LPVOID param) {
	fxh->Refresh((UCHAR*)param);
	return 0;
}