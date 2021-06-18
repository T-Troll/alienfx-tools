#include "CaptureHelper.h"
#include <WinNT.h>
#include "resource.h"
#include <opencv2/imgproc.hpp>
//#include <opencv2/imgproc/types_c.h>
#include <windowsx.h>

#include "DXGIManager.hpp"

using namespace cv;
using namespace std;

DWORD WINAPI CInProc(LPVOID);
DWORD WINAPI CDlgProc(LPVOID);
DWORD WINAPI CFXProc(LPVOID);
DWORD WINAPI ColorProc(LPVOID);

//bool inWork = true;

HWND hDlg;

ConfigHandler* config;
FXHelper* fxh;

UCHAR  imgz[12 * 3];

DXGIManager* dxgi_manager = NULL;

CaptureHelper::CaptureHelper(HWND dlg, ConfigHandler* conf, FXHelper* fhh)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    dxgi_manager = new DXGIManager();
	//dxgi_manager->setup();
	dxgi_manager->set_timeout(50);
	//SetCaptureScreen(conf->mode);

	hDlg = dlg;
	config = conf;
	fxh = fhh;
}

CaptureHelper::~CaptureHelper()
{
	delete dxgi_manager;
	CoUninitialize();
}

void CaptureHelper::SetCaptureScreen(int mode) {
	Stop();
	dxgi_manager->set_capture_source(mode);
	Start(200);
}

HANDLE stopEvent;

void CaptureHelper::Start(DWORD delay)
{
	if (dwHandle == 0) {
		stopEvent = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread( NULL, 0, CInProc, (LPVOID)&delay, 0, &dwThreadID);
	}
}

void CaptureHelper::Stop()
{
	if (dwHandle != 0) {
		SetEvent(stopEvent);
		WaitForSingleObject(dwHandle, 10000);
		CloseHandle(dwHandle);
		CloseHandle(stopEvent);
		dwHandle = 0;
		memset(imgz, 0xff, sizeof(imgz));
	}
}

void CaptureHelper::Restart() {
	//Stop();	
	SetCaptureScreen(config->mode);
	//Start();
}

cv::Mat extractHPts(const cv::Mat* inImage)
{
	// container for storing Hue Points
	cv::Mat listOfHPts(inImage->cols * inImage->rows, 1, CV_32FC1);
	//listOfHPts = cv::Mat::zeros(inImage.cols * inImage.rows, 1, CV_32FC1);
	Mat res[4];
	split(*inImage, res);
	res[0].reshape(1, inImage->cols * inImage->rows).convertTo(listOfHPts, CV_32FC1);
	return listOfHPts;
}

// function for extracting dominant color from foreground pixels
cv::Mat getDominantColor(const cv::Mat& inImage, const cv::Mat& ptsLabel)
{
	// first we determine which cluster is foreground
	// assuming the our object of interest is the biggest object in the image region

	Mat fPtsLabel, sumLabel;
	ptsLabel.convertTo(fPtsLabel, CV_32FC1);

	reduce(fPtsLabel, sumLabel, 0, 0/*CV_REDUCE_SUM*/, CV_32FC1);

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
	Mat dominantColor = cv::Mat::zeros(1, 3, CV_32FC1);
	//dominantColor = cv::Mat::zeros(1, 3, CV_32FC1);

	int idx = 0; //int fgIdx = 0;
	for (int j = 0; j < inImage.rows; j++)
	{
		for (int i = 0; i < inImage.cols; i++)
		{
			if (fPtsLabel.at<float>(idx++, 0) == 1)
			{
				cv::Vec4b tempVec; //!!
				tempVec = inImage.at<cv::Vec4b>(j, i);
				dominantColor.at<float>(0, 0) += (tempVec[0]);
				dominantColor.at<float>(0, 1) += (tempVec[1]);
				dominantColor.at<float>(0, 2) += (tempVec[2]);

				//fgIdx++;
			}
		}
	}

	dominantColor /= numFGPts;

	// convert to uchar so that it can be used directly for visualization
	//cv::Mat dColor;// (1, 3, CV_8UC1);

	dominantColor.convertTo(dominantColor/*dColor*/, CV_8UC1);

	return dominantColor;// dColor;
}

// Commented part is for multi-thread processing, but single-thread working faster!
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
	return 0;
}*/

void FillColors(Mat* src, UCHAR* imgz) {
	uint w = src->cols / 4, h = src->rows / 3;
	Mat cPos, hPts;
	Mat ptsLabel, kCenters, dColor;
	//HANDLE pThread[12];
	//DWORD tId;
	//procData callData[3][4];
	for (uint dy = 0; dy < 3; dy++)
		for (uint dx = 0; dx < 4; dx++) {
			uint ptr = (dy * 4 + dx) * 3;
			cPos = src->rowRange(dy * h, (dy + 1) * h)
				.colRange(dx * w, (dx + 1) * w);
			/*callData[dy][dx].src = cPos;
			callData[dy][dx].dst = imgz + ptr;
			callData[dy][dx].done = false;
			pThread[dy * 4 + dx] = CreateThread( NULL, 4*1024*1024, ColorProc, &callData[dy][dx], 0, &tId);*/
			hPts = extractHPts(&cPos);
			cv::kmeans(hPts, 2, ptsLabel, cv::TermCriteria(cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 1000, 0.00001)), 5, cv::KMEANS_PP_CENTERS, kCenters);
			dColor = getDominantColor(cPos, ptsLabel);
			imgz[ptr] = dColor.ptr<UCHAR>()[0];
			imgz[ptr + 1] = dColor.ptr<UCHAR>()[1];
			imgz[ptr + 2] = dColor.ptr<UCHAR>()[2];
		}
	//WaitForMultipleObjects(12, pThread, true, 3000);
}

HANDLE uiEvent = 0;

DWORD WINAPI CInProc(LPVOID param)
{
	UINT w, h;
	UCHAR* img = NULL;
	UCHAR  oldimg[12 * 3] = { 0 };

	size_t buf_size;
	ULONGLONG lastTick = GetTickCount64();

	DWORD uiThread, cuThread, wait_time = *((DWORD *) param);
	HANDLE uiHandle = 0, lightHandle = 0;

	uiEvent = CreateSemaphore(NULL, 0, 2, NULL);

	RECT dimensions = dxgi_manager->get_output_rect();
	w = dimensions.right - dimensions.left;
	h = dimensions.bottom - dimensions.top;

	uiHandle = CreateThread(NULL, 0, CDlgProc, imgz, 0, &uiThread);
	lightHandle = CreateThread(NULL, 0, CFXProc, imgz, 0, &cuThread);

	while(WaitForSingleObject(stopEvent, wait_time) == WAIT_TIMEOUT) {
		UINT div = 34 - config->divider;
		// Resize & calc
		if (dxgi_manager->get_output_data(&img, &buf_size) == CR_OK && img != NULL) {
			Mat* src = new Mat(h, w, CV_8UC4, img);
			cv::resize(*src, *src, //Size(w / div, h / div),
				Size(8 * div, 6 * div), 0, 0, INTER_NEAREST);// INTER_AREA);// 
			//cv::cvtColor(*src, *src, 1 /*CV_RGBA2RGB*/);

			FillColors(src, imgz);
			delete src;

			if (memcmp(imgz, oldimg, sizeof(oldimg)) != 0) {
				ReleaseSemaphore(uiEvent, 2, NULL);
				memcpy(oldimg, imgz, sizeof(oldimg));
			}
		}
		ULONGLONG nextTick = GetTickCount64();
#ifdef _DEBUG
		char buff[2048];
		sprintf_s(buff, 2047, "Update at %fs\n", (nextTick - lastTick) / 1000.0);
		OutputDebugString(buff);
#endif
		if (nextTick - lastTick < 50) {
			wait_time = 50 - (DWORD)(nextTick - lastTick);
		}
		else
			wait_time = 0;
		lastTick = nextTick;
	}
	ReleaseSemaphore(uiEvent, 2, NULL);
	WaitForSingleObject(uiHandle, 1000);
	WaitForSingleObject(lightHandle, 1000);
	CloseHandle(uiHandle);
	CloseHandle(lightHandle);

	return 0;
}

DWORD WINAPI CDlgProc(LPVOID param)
{
	UCHAR  imgz[12 * 3];
	RECT rect;
	HBRUSH Brush = NULL;
	ULONGLONG lastTick = 0, nextTick = 0;
	while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT) {
		if (WaitForSingleObject(uiEvent, 500) == WAIT_OBJECT_0 && !IsIconic(hDlg)) {
			//#ifdef _DEBUG
			//			OutputDebugString("UI update...\n");
			//#endif
			nextTick = GetTickCount64();
			if (nextTick - lastTick > 250) {
				memcpy(imgz, (UCHAR*)param, sizeof(imgz));
				for (int i = 0; i < 12; i++) {
					HWND tl = GetDlgItem(hDlg, IDC_BUTTON1 + i);
					HWND cBid = GetDlgItem(hDlg, IDC_CHECK1 + i);
					GetWindowRect(tl, &rect);
					HDC cnt = GetWindowDC(tl);
					rect.bottom -= rect.top;
					rect.right -= rect.left;
					rect.top = rect.left = 0;
					// BGR!
					Brush = CreateSolidBrush(RGB(imgz[i * 3 + 2], imgz[i * 3 + 1], imgz[i * 3]));
					FillRect(cnt, &rect, Brush);
					DeleteObject(Brush);
					UINT state = IsDlgButtonChecked(hDlg, IDC_CHECK1 + i);
					if ((state & BST_CHECKED))
						DrawEdge(cnt, &rect, EDGE_SUNKEN, BF_RECT);
					else
						DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
					RedrawWindow(cBid, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				}
				lastTick = nextTick;
			}
		}
	}
	return 0;
}

DWORD WINAPI CFXProc(LPVOID param) {
	UCHAR  imgz[12 * 3];
	while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT)
		if (WaitForSingleObject(uiEvent, 500) == WAIT_OBJECT_0) {
//#ifdef _DEBUG
//			OutputDebugString("Light update...\n");
//#endif
			memcpy(imgz, (UCHAR*)param, sizeof(imgz));
			fxh->Refresh(imgz);
		}
	return 0;
}