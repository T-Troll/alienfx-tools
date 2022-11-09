#include "CaptureHelper.h"
#include "DXGIManager.hpp"

DWORD WINAPI CInProc(LPVOID);
DWORD WINAPI CFXProc(LPVOID);

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

extern ConfigHandler *conf;
extern FXHelper *fxhl;

DXGIManager* dxgi_manager = NULL;

HANDLE clrStopEvent, lhEvent;

CaptureHelper::CaptureHelper()
{
	imgz = new byte[LOWORD(conf->amb_grid) * HIWORD(conf->amb_grid) * 6];
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	dxgi_manager = new DXGIManager();
	dxgi_manager->set_timeout(100);

	SetCaptureScreen(conf->amb_mode);
}

CaptureHelper::~CaptureHelper()
{
	Stop();
	delete dxgi_manager;
	CoUninitialize();
	delete[] imgz;
}

void CaptureHelper::SetCaptureScreen(int mode) {
	Stop();
	dxgi_manager->set_capture_source(mode);
	Start();
}

void CaptureHelper::Start()
{
	if (!dwHandle) {
		clrStopEvent = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread( NULL, 0, CInProc, this, 0, NULL);
	}
}

void CaptureHelper::Stop()
{
	if (dwHandle) {
		SetEvent(clrStopEvent);
		WaitForSingleObject(dwHandle, 5000);
		CloseHandle(dwHandle);
		CloseHandle(clrStopEvent);
		dwHandle = NULL;
		memset(imgz, 0xff, sizeof(imgz));
	}
}

void CaptureHelper::Restart() {
	SetCaptureScreen(conf->amb_mode);
}

void CaptureHelper::SetLightGridSize(int x, int y)
{
	Stop();
	conf->amb_grid = MAKELPARAM(x, y);
	delete[] imgz;
	imgz = new byte[x * y * 6];
	Start();
}

struct procData {
	int dx, dy;
	UCHAR* dst;
	HANDLE pEvent;
	HANDLE pfEvent;
};

UINT w = 0, h = 0, ww = 0, hh = 0, stride = 0 , divider = 1;
byte* scrImg = NULL;

DWORD WINAPI ColorCalc(LPVOID inp) {
	procData* src = (procData*) inp;
	HANDLE waitArray[2]{src->pEvent, clrStopEvent};
	DWORD res = 0;

	while ((res = WaitForMultipleObjects(2, waitArray, false, 200)) != WAIT_OBJECT_0 + 1) {
		if (res == WAIT_OBJECT_0) {
			UINT idx = src->dy * hh * stride + src->dx * ww * 4;
			ULONG64 r = 0, g = 0, b = 0;
			UINT div = hh * ww / (divider * divider);
			for (UINT y = 0; y < hh; y += divider) {
				UINT pos = idx + y * stride;
				for (UINT x = 0; x < ww; x += divider) {
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
			SetEvent(src->pfEvent);
		}
	}
	CloseHandle(src->pfEvent);
	CloseHandle(src->pEvent);
	src->pfEvent = 0;
	return 0;
}

void SetDimensions() {
	RECT dimensions = dxgi_manager->get_output_rect();
	w = dimensions.right - dimensions.left;
	h = dimensions.bottom - dimensions.top;
	ww = w / LOWORD(conf->amb_grid); hh = h / HIWORD(conf->amb_grid);
	stride = w * 4;
	divider = w > 1920 ? 2 : 1;
	DebugPrint("Screen resolution set to " + to_string(w) + "x" + to_string(h) + ", div " + to_string(divider) + ".\n");
}

DWORD WINAPI CInProc(LPVOID param)
{
	CaptureHelper* src = (CaptureHelper*)param;

	DWORD gridSize = LOWORD(conf->amb_grid) * HIWORD(conf->amb_grid), gridDataSize = gridSize * 3, ret = 0;
	byte* imgo = src->imgz + gridDataSize;
	size_t buf_size;

	lhEvent = CreateEvent(NULL, false, false, NULL);

	HANDLE lightHandle = CreateThread(NULL, 0, CFXProc, src->imgz, 0, NULL), pThread[16], pfEvent[16];;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	procData callData[16];

	SetDimensions();

	// prepare threads and data...
	for (UINT i = 0; i < 16; i++) {
		callData[i] = { 0, 0, NULL, CreateEvent(NULL, false, false, NULL), CreateEvent(NULL, false, false, NULL) };
		pThread[i] = CreateThread(NULL, 0, ColorCalc, &callData[i], 0, NULL);
		pfEvent[i] = callData[i].pfEvent;
	}

	while (WaitForSingleObject(clrStopEvent, 50) == WAIT_TIMEOUT) {
		// Resize & calc
		UINT tInd = 0;
		if (w && h && (ret = dxgi_manager->get_output_data(&scrImg, &buf_size)) == CR_OK && scrImg) {
			if (!(conf->monDelay > DEFAULT_MON_DELAY)) {
				for (int dy = 0; dy < HIWORD(conf->amb_grid); dy++)
					for (int dx = 0; dx < LOWORD(conf->amb_grid); dx++) {
						UINT ptr = (dy * LOWORD(conf->amb_grid) + dx);
						tInd = ptr % 16;
						if (ptr > 0 && !tInd) {
#ifndef _DEBUG
							WaitForMultipleObjects(16, pfEvent, true, 1000);
#else
							if (WaitForMultipleObjects(16, pfEvent, true, 1000) != WAIT_OBJECT_0)
								DebugPrint("Ambient thread execution fails at " + to_string(ptr) + "\n");
#endif
						}
						callData[tInd].dx = dx;
						callData[tInd].dy = dy;
						callData[tInd].dst = imgo + ptr * 3;
						SetEvent(callData[tInd].pEvent);
					}
#ifndef _DEBUG
				WaitForMultipleObjects(tInd + 1, pfEvent, true, 1000);
#else
				if (WaitForMultipleObjects(tInd+1, pfEvent, true, 1000) != WAIT_OBJECT_0)
					DebugPrint("Ambient thread execution fails at last set\n");
#endif

				if (memcmp(src->imgz, imgo, gridDataSize)) {
					memcpy(src->imgz, imgo, gridDataSize);
					SetEvent(lhEvent);
					src->needUpdate = true;
				}
			} else {
				DebugPrint("Ambient update skipped!\n");
			}
		}
		else {
			if (ret != CR_TIMEOUT) {
				DebugPrint("Ambient feed restart!\n");
				dxgi_manager->refresh_output();
				Sleep(150);
				SetDimensions();
			}
		}
	}

	WaitForMultipleObjects(16, pThread, true, 1000);
	// close handles here!
	for (DWORD i = 0; i < 16; i++) {
		CloseHandle(pThread[i]);
	}
	WaitForSingleObject(lightHandle, 1000);
	CloseHandle(lightHandle);

	//delete[] imgo;

	return 0;
}

DWORD WINAPI CFXProc(LPVOID param) {
	HANDLE waitArray[2]{lhEvent, clrStopEvent};
	DWORD res = 0;

	while ((res = WaitForMultipleObjects(2, waitArray, false, 200)) != WAIT_OBJECT_0 + 1) {
		if (res == WAIT_OBJECT_0) {
			//DebugPrint("Ambient light update...\n");
			fxhl->RefreshAmbient((UCHAR*)param);
		}
	}
	return 0;
}