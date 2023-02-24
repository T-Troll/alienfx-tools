#include "CaptureHelper.h"
#include "EventHandler.h"
#include "FXHelper.h"

void CInProc(LPVOID);
DWORD WINAPI ColorCalc(LPVOID inp);

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

extern ConfigHandler *conf;
extern FXHelper *fxhl;
extern EventHandler* eve;

DXGIManager* dxgi_manager = NULL;
int dxgi_counter = 0;
ThreadHelper* dxgi_thread;

byte* scrImg = NULL;
UINT w, h, stride, divider;
int capRes = CR_TIMEOUT;

void dxgi_loop(LPVOID param);

void dxgi_SetDimensions() {
	RECT dimensions = dxgi_manager->get_output_rect();
	w = dimensions.right - dimensions.left;
	h = dimensions.bottom - dimensions.top;
	stride = w * 4;
	divider = w > 1920 ? 2 : 1;
}

void dxgi_Restart() {
	if (dxgi_manager) {
		delete dxgi_thread;
		capRes = CR_TIMEOUT;
		dxgi_manager->set_capture_source((WORD)conf->amb_mode);
		Sleep(150);
		dxgi_SetDimensions();
		dxgi_thread = new ThreadHelper(dxgi_loop, NULL, 100, THREAD_PRIORITY_LOWEST);
	}
}

void dxgi_loop(LPVOID param) {
	size_t buf_size;
	if ((capRes = dxgi_manager->get_output_data(&scrImg, &buf_size)) != CR_OK && capRes != CR_TIMEOUT) {
		// restart capture
		DebugPrint("Ambient feed restart!\n");
		dxgi_manager->refresh_output();
		Sleep(150);
		dxgi_SetDimensions();
	}
}

CaptureHelper::CaptureHelper(bool needLights)
{
	if (!dxgi_manager) {
		DebugPrint("Startinging screen capture\n");
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		dxgi_manager = new DXGIManager();
		dxgi_manager->set_timeout(100);
		dxgi_manager->set_capture_source((WORD)conf->amb_mode);
		dxgi_thread = new ThreadHelper(dxgi_loop, NULL, 100, THREAD_PRIORITY_NORMAL);
		dxgi_SetDimensions();
	}
	dxgi_counter++;
	// prepare threads and data...
	sEvent = CreateEvent(NULL, true, false, NULL);
	for (UINT i = 0; i < 16; i++) {
		callData[i] = { CreateEvent(NULL, false, false, NULL), CreateEvent(NULL, false, false, NULL), this };
		pThread[i] = CreateThread(NULL, 0, ColorCalc, &callData[i], 0, NULL);
		SetThreadPriority(pThread[i], THREAD_PRIORITY_ABOVE_NORMAL);
		pfEvent[i] = callData[i].pfEvent;
	}
	needLightsUpdate = needLights;
	SetLightGridSize(conf->amb_grid.x, conf->amb_grid.y);
}

CaptureHelper::~CaptureHelper()
{
	Stop();
	// Stop and remove processing threads
	SetEvent(sEvent);
	WaitForMultipleObjects(16, pThread, true, 1000);
	CloseHandle(sEvent);
	for (DWORD i = 0; i < 16; i++) {
		CloseHandle(callData[i].pEvent);
		CloseHandle(pfEvent[i]);
	}
	delete[] imgz;
	if (!(--dxgi_counter)) {
		DebugPrint("Deleting screen capture\n");
		delete dxgi_thread;
		delete dxgi_manager;
		dxgi_manager = NULL;
		scrImg = NULL;
		CoUninitialize();
	}
}

void CaptureHelper::Start()
{
	if (!dwHandle) {
		dwHandle = new ThreadHelper(CInProc, this, 100, THREAD_PRIORITY_BELOW_NORMAL);
	}
}

void CaptureHelper::Stop()
{
	if (dwHandle) {
		delete dwHandle;
		dwHandle = NULL;
	}
}

void CaptureHelper::SetLightGridSize(int x, int y)
{
	Stop();
	if (imgz)
		delete[] imgz;
	gridX = x; gridY = y;
	imgz = new byte[gridX * gridY * 6];
	gridDataSize = gridX * gridY * 3;
	imgo = imgz + gridDataSize;
	needUpdate = true;
	Start();
}

DWORD WINAPI ColorCalc(LPVOID inp) {
	procData* src = (procData*)inp;
	CaptureHelper* cap = (CaptureHelper*)src->cap;
	HANDLE waitArray[2]{ cap->sEvent, src->pEvent };
	DWORD res;
	while ((res = WaitForMultipleObjects(2, waitArray, false, 150)) != WAIT_OBJECT_0)
		if (res != WAIT_TIMEOUT) {
			UINT idx = src->idx;
			ULONG64 r = 0, g = 0, b = 0;
			for (UINT y = 0; y < cap->hh; y += divider) {
				UINT pos = idx;
				for (UINT x = 0; x < cap->ww; x += divider) {
					r += scrImg[pos++];
					g += scrImg[pos++];
					b += scrImg[pos++];
					pos++;
				}
				idx += stride;
			}
			src->dst[0] = (UCHAR) (r / cap->div);
			src->dst[1] = (UCHAR) (g / cap->div);
			src->dst[2] = (UCHAR) (b / cap->div);
			SetEvent(src->pfEvent);
		}
	return 0;
}

void CInProc(LPVOID param)
{
	CaptureHelper* src = (CaptureHelper*)param;

	// Resize & calc
	if (!capRes && scrImg) {
		src->ww = w / src->gridX; src->hh = h / src->gridY;
		src->div = src->hh * src->ww / (divider * divider);
		UINT ptr = 0;
		UINT tInd = 0;
		for (int ind = 0; ind < src->gridY * src->gridX; ind++) {
			tInd = ptr % 16;
			if (ptr > 0 && !tInd) {
#ifndef _DEBUG
				WaitForMultipleObjects(16, src->pfEvent, true, 1000);
#else
				if (WaitForMultipleObjects(16, src->pfEvent, true, 1000) != WAIT_OBJECT_0)
					DebugPrint("Ambient thread execution fails at " + to_string(ptr) + "\n");
#endif
			}
			src->callData[tInd].idx = (ptr / src->gridX) * src->hh * stride + (ptr % src->gridX) * src->ww * 4;
			//src->callData[tInd].dx = dx;
			//src->callData[tInd].dy = dy;
			src->callData[tInd].dst = src->imgo + ptr * 3;
			SetEvent(src->callData[tInd].pEvent);
			ptr++;
		}
#ifndef _DEBUG
		WaitForMultipleObjects(tInd + 1, src->pfEvent, true, 1000);
#else
		if (WaitForMultipleObjects(tInd+1, src->pfEvent, true, 1000) != WAIT_OBJECT_0)
			DebugPrint("Ambient thread execution fails at last set\n");
#endif

		if (memcmp(src->imgz, src->imgo, src->gridDataSize)) {
			memcpy(src->imgz, src->imgo, src->gridDataSize);
			src->needUpdate = true;
			if (src->needLightsUpdate && conf->lightsNoDelay)
				fxhl->RefreshAmbient();
		}
	}
}
