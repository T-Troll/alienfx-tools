#include "CaptureHelper.h"
//#include "opencv2/imgproc.hpp"
#include "DXGIManager.hpp"

//using namespace cv;

DWORD WINAPI CInProc(LPVOID);
DWORD WINAPI CDlgProc(LPVOID);
DWORD WINAPI CFXProc(LPVOID);

HWND hDlg;

ConfigHandler* config;
FXHelper* fxh;

UCHAR  imgz[12 * 3] = { 0 }, imgui[12 * 3] = { 0 };

DXGIManager* dxgi_manager = NULL;

HANDLE stopEvent, uiEvent, lhEvent;

CaptureHelper::CaptureHelper(HWND dlg, ConfigHandler* conf, FXHelper* fhh)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    dxgi_manager = new DXGIManager();
	dxgi_manager->set_timeout(100);

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
	Start();
}

void CaptureHelper::Start()
{
	DWORD dwThreadID;
	if (dwHandle == 0) {
		stopEvent = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread( NULL, 0, CInProc, NULL, 0, &dwThreadID);
	}
}

void CaptureHelper::Stop()
{
	if (dwHandle != 0) {
		SetEvent(stopEvent);
		WaitForSingleObject(dwHandle, 5000);
		CloseHandle(dwHandle);
		CloseHandle(stopEvent);
		dwHandle = 0;
		memset(imgz, 0xff, sizeof(imgz));
	}
}

void CaptureHelper::Restart() {
	SetCaptureScreen(config->mode);
}

//Mat extractHPts(const Mat& inImage)
//{
//	// container for storing Hue Points
//	Mat listOfHPts(inImage.cols * inImage.rows, 1, CV_32FC1);
//	Mat res[4];
//	split(inImage, res);
//	res[0].reshape(1, inImage.cols * inImage.rows).convertTo(listOfHPts, CV_32FC1);
//	return listOfHPts;
//}
//
//// function for extracting dominant color from foreground pixels
//Mat getDominantColor(const Mat& inImage, const Mat& ptsLabel)
//{
//	// first we determine which cluster is foreground
//	// assuming the our object of interest is the biggest object in the image region
//
//	Mat fPtsLabel, sumLabel;
//	ptsLabel.convertTo(fPtsLabel, CV_32FC1);
//
//	reduce(fPtsLabel, sumLabel, 0, 0/*CV_REDUCE_SUM*/, CV_32FC1);
//
//	int numFGPts = 0;
//
//	if (sumLabel.at<float>(0, 0) < ptsLabel.rows / 2)
//	{
//		// invert the 0's and 1's where 1s represent foreground
//		fPtsLabel = (fPtsLabel - 1) * (-1);
//		numFGPts = fPtsLabel.rows - (int) sumLabel.at<float>(0, 0);
//	}
//	else
//		numFGPts = (int) sumLabel.at<float>(0, 0);
//
//	// to find dominant color, I just average all points belonging to foreground
//	Mat dominantColor(1,3, CV_32FC1);// = cv::Mat::zeros(1, 3, CV_32FC1);
//
//	int idx = 0;
//	dominantColor.at<float>(0, 0) = 0;
//	dominantColor.at<float>(0, 1) = 0;
//	dominantColor.at<float>(0, 2) = 0;
//	for (int j = 0; j < inImage.rows; j++)
//	{
//		for (int i = 0; i < inImage.cols; i++)
//		{
//			if (fPtsLabel.at<float>(idx++, 0) == 1)
//			{
//				cv::Vec4b tempVec; //!!
//				tempVec = inImage.at<cv::Vec4b>(j, i);
//				dominantColor.at<float>(0, 0) += (tempVec[0]);
//				dominantColor.at<float>(0, 1) += (tempVec[1]);
//				dominantColor.at<float>(0, 2) += (tempVec[2]);
//
//			}
//		}
//	}
//
//	dominantColor /= numFGPts;
//
//	Mat cColor;
//
//	dominantColor.convertTo(cColor, CV_8UC1);
//
//	return cColor;
//}

struct procData {
	int dx, dy;
	UCHAR* dst;
	HANDLE pEvent;
	HANDLE pfEvent;
};

static procData callData[3][4];
UINT w = 2, h = 2, ww = 1, hh = 1, stride = w * 4, divider = 1;
HANDLE pThread[12] = { 0 };
HANDLE pfEvent[12] = { 0 };
UCHAR* scrImg = NULL;

DWORD WINAPI ColorCalc(LPVOID inp) {
	procData* src = (procData*) inp;
	while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT) {
		if (WaitForSingleObject(src->pEvent, 200) == WAIT_OBJECT_0) {
			UINT idx = src->dy * hh * stride + src->dx * ww * 4;//src->dy * 4 + src->dx;
			ULONG64 r = 0, g = 0, b = 0, div = (ULONG64)hh*ww / (divider*divider);
			for (UINT y = 0; y < hh; y+=divider) { 
				UINT pos = idx + y * stride;
				for (UINT x = 0; x < ww; x+=divider) {
					r += scrImg[pos];
					g += scrImg[pos + 1];
					b += scrImg[pos + 2];
					pos += 4;
				}
			}
			r /= div;
			g /= div;
			b /= div;
			src->dst[0] = (UCHAR) r;
			src->dst[1] = (UCHAR) g;
			src->dst[2] = (UCHAR) b;
			SetEvent(pfEvent[src->dy * 4 + src->dx]);
		}
	}
	CloseHandle(pfEvent[src->dy * 4 + src->dx]);
	pfEvent[src->dy * 4 + src->dx] = 0;
	return 0;
}

void FindColors(UCHAR* src, UCHAR* imgz) {
	scrImg = src;
	for (UINT dy = 0; dy < 3; dy++)
		for (UINT dx = 0; dx < 4; dx++) {
			//ColorCalc(&callData[dy][dx]);
			if (callData[dy][dx].pEvent) {
				SetEvent(callData[dy][dx].pEvent);
			} else {
				UINT ptr = (dy * 4 + dx);// *3;
				callData[dy][dx].dy = dy; callData[dy][dx].dx = dx;
				callData[dy][dx].dst = imgz + ptr * 3;
				callData[dy][dx].pEvent = CreateEvent(NULL, false, true, NULL);
				pfEvent[ptr] = CreateEvent(NULL, false, false, NULL);
				pThread[ptr] = CreateThread(NULL, 6 * w * h, ColorCalc, &callData[dy][dx], 0, NULL);
			}
		}
	WaitForMultipleObjects(12, pfEvent, true, 3000);
}

#define GRIDSIZE 36 // 4x3 x 3

DWORD WINAPI CInProc(LPVOID param)
{
	UCHAR* img = NULL;
	UCHAR* imgo[GRIDSIZE] = { 0 };

	size_t buf_size;
	ULONGLONG lastTick = 0;

	DWORD uiThread, cuThread, wait_time = 200;
	HANDLE uiHandle = CreateThread(NULL, 0, CDlgProc, imgz, 0, &uiThread),
		lightHandle = CreateThread(NULL, 0, CFXProc, imgz, 0, &cuThread);

	uiEvent = CreateEvent(NULL, false, false, NULL);
	lhEvent = CreateEvent(NULL, false, false, NULL);

	RECT dimensions = dxgi_manager->get_output_rect();
	w = dimensions.right - dimensions.left;
	h = dimensions.bottom - dimensions.top;

	ww = w / 4; hh = h / 3;
	stride = w * 4;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	while (WaitForSingleObject(stopEvent, wait_time) == WAIT_TIMEOUT) {
		//divider = 9 - (config->divider >> 2);
		// Resize & calc
		if (dxgi_manager->get_output_data(&img, &buf_size) == CR_OK && img != NULL) {
			if (w && h) {
				FindColors(img, imgz);

				if (memcmp(imgz, imgo, GRIDSIZE) != 0) {
					SetEvent(lhEvent);
					SetEvent(uiEvent);
					memcpy(imgo, imgz, GRIDSIZE);
				}
			}
			else {
				dxgi_manager->refresh_output();
				Sleep(200);
				dimensions = dxgi_manager->get_output_rect();
				w = dimensions.right - dimensions.left;
				h = dimensions.bottom - dimensions.top;
				ww = w / 4; hh = h / 3;
				stride = w * 4;
			}
		}

		wait_time = 50;
	}

	WaitForSingleObject(uiHandle, 1000);
	WaitForSingleObject(lightHandle, 1000);
	CloseHandle(uiHandle);
	CloseHandle(lightHandle);

	return 0;
}

DWORD WINAPI CDlgProc(LPVOID param)
{
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	while (WaitForSingleObject(stopEvent, 50) == WAIT_TIMEOUT) {
		if (!IsIconic(hDlg) && WaitForSingleObject(uiEvent, 0) == WAIT_OBJECT_0) {
//#ifdef _DEBUG
//	OutputDebugString("UI update...\n");
//#endif
			memcpy(imgui, (UCHAR*)param, sizeof(imgz));
			SendMessage(hDlg, WM_PAINT, 0, (LPARAM)imgui);
		}
	}
	return 0;
}

DWORD WINAPI CFXProc(LPVOID param) {
	UCHAR  imgz[12 * 3];
	HANDLE waitArray[2] = {lhEvent, stopEvent};
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT)
		if (WaitForMultipleObjects(2, waitArray, false, 200) == WAIT_OBJECT_0) {
//#ifdef _DEBUG
//			OutputDebugString("Light update...\n");
//#endif
			memcpy(imgz, (UCHAR*)param, sizeof(imgz));
			fxh->Refresh(imgz);
		}
	return 0;
}