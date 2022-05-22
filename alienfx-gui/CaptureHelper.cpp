#include "CaptureHelper.h"

DWORD WINAPI CInProc(LPVOID);
DWORD WINAPI CFXProc(LPVOID);

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

extern ConfigHandler *conf;
extern FXHelper *fxhl;

HANDLE clrStopEvent, lhEvent;

CaptureHelper::CaptureHelper()
{
	imgz = new byte[conf->amb_conf->grid.x * conf->amb_conf->grid.y * 3];
	if (CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) == S_OK) {
		dxgi_manager = new DXGIManager();
		dxgi_manager->set_timeout(100);

		SetCaptureScreen(conf->amb_conf->mode);
	} else
		isDirty = true;
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
	SetCaptureScreen(conf->amb_conf->mode);
}

void CaptureHelper::SetGridSize(int x, int y)
{
	Stop();
	conf->amb_conf->grid.x = x;
	conf->amb_conf->grid.y = y;
	delete[] imgz;
	imgz = new byte[x * y * 3];
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

void SetDimensions(CaptureHelper* src) {
	RECT dimensions = src->dxgi_manager->get_output_rect();
	w = dimensions.right - dimensions.left;
	h = dimensions.bottom - dimensions.top;
	ww = w / conf->amb_conf->grid.x; hh = h / conf->amb_conf->grid.y;
	stride = w * 4;
}

DWORD WINAPI CInProc(LPVOID param)
{
	CaptureHelper* src = (CaptureHelper*)param;

	DWORD gridSize = conf->amb_conf->grid.x * conf->amb_conf->grid.y, gridDataSize = gridSize * 3;

	byte* imgo = new byte[gridDataSize];

	size_t buf_size;

	DWORD ret = 0;

	lhEvent = CreateEvent(NULL, false, false, NULL);

	HANDLE lightHandle = CreateThread(NULL, 0, CFXProc, src->imgz, 0, NULL);

	SetDimensions(src);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	procData callData[16];

	HANDLE pThread[16];
	HANDLE pfEvent[16];

	// prepare threads and data...
	for (UINT i = 0; i < 16; i++) {
		callData[i] = { 0, 0, NULL, CreateEvent(NULL, false, false, NULL), CreateEvent(NULL, false, false, NULL) };
		pThread[i] = CreateThread(NULL, 0, ColorCalc, &callData[i], 0, NULL);
		pfEvent[i] = callData[i].pfEvent;
	}

	while (WaitForSingleObject(clrStopEvent, 50) == WAIT_TIMEOUT) {
		// Resize & calc
		if (!(conf->monDelay > 200)) {
			if (w && h && (ret = src->dxgi_manager->get_output_data(&scrImg, &buf_size)) == CR_OK && scrImg) {
				UINT tInd = 0;
				for (int dy = 0; dy < conf->amb_conf->grid.y; dy++)
					for (int dx = 0; dx < conf->amb_conf->grid.x; dx++) {
						UINT ptr = (dy * conf->amb_conf->grid.x + dx);
						tInd = ptr % 16;
						if (ptr > 0 && !tInd) {
#ifndef _DEBUG
							WaitForMultipleObjects(16, pfEvent, true, 1000);
#else
							if (WaitForMultipleObjects(16, pfEvent, true, 1000) != WAIT_OBJECT_0)
								DebugPrint(("Ambient thread execution fails at " + to_string(ptr) + "\n").c_str());
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
				if (WaitForMultipleObjects(tInd + 1, pfEvent, true, 1000) != WAIT_OBJECT_0)
					DebugPrint("Ambient thread execution fails at last set\n");
#endif

				if (memcmp(src->imgz, imgo, gridDataSize)) {
					memcpy(src->imgz, imgo, gridDataSize);
					SetEvent(lhEvent);
					//src->needUpdate = true;
				}
			}
			else {
				if (ret != CR_TIMEOUT) {
					DebugPrint("Ambient feed restart!\n");
					src->dxgi_manager->refresh_output();
					Sleep(150);
					SetDimensions(src);
				}
			}
		}
		else {
			DebugPrint("Ambient update skipped!\n");
		}
	}

	WaitForMultipleObjects(16, pThread, true, 1000);
	// close handles here!
	for (DWORD i = 0; i < 16; i++) {
		CloseHandle(pThread[i]);
	}
	WaitForSingleObject(lightHandle, 1000);
	CloseHandle(lightHandle);

	delete[] imgo;

	return 0;
}

DWORD WINAPI CFXProc(LPVOID param) {
	HANDLE waitArray[2]{lhEvent, clrStopEvent};
	DWORD res = 0;

	while ((res = WaitForMultipleObjects(2, waitArray, false, 500)) != WAIT_OBJECT_0 + 1) {
		if (res == WAIT_OBJECT_0) {
			//DebugPrint("Ambient light update...\n");
			fxhl->RefreshAmbient((UCHAR*)param);
		}
	}
	return 0;
}