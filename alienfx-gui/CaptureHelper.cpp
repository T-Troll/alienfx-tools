#include "CaptureHelper.h"
#include "EventHandler.h"
#include "FXHelper.h"

void CInProc(LPVOID);
void ColorCalc(LPVOID inp);

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
bool dxgi_inuse = false;
ThreadHelper* dxgi_thread;

byte* scrImg = NULL;
UINT w = 0, h;

void dxgi_SetDimensions() {
	RECT dimensions = dxgi_manager->get_output_rect();
	w = dimensions.right - dimensions.left;
	h = dimensions.bottom - dimensions.top;
}

void dxgi_loop(LPVOID param) {
	int res;
	size_t buf_size;
	if ((res = dxgi_manager->get_output_data(&scrImg, &buf_size)) != CR_OK && res != CR_TIMEOUT) {
		// restart capture
		DebugPrint("Ambient feed restart!\n");
		w = 0;
		dxgi_manager->refresh_output();
		Sleep(150);
		dxgi_SetDimensions();
	}
}

CaptureHelper::CaptureHelper(bool needLights)
{
	if (!dxgi_manager) {
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		dxgi_manager = new DXGIManager();
		dxgi_manager->set_timeout(100);
		dxgi_manager->set_capture_source((WORD)conf->amb_mode);
		dxgi_thread = new ThreadHelper(dxgi_loop, NULL, 100, THREAD_PRIORITY_LOWEST);
		dxgi_SetDimensions();
	}
	else
		dxgi_inuse = true;
	// prepare threads and data...
	for (UINT i = 0; i < 16; i++) {
		callData[i] = { 0, 0, NULL, CreateEvent(NULL, false, false, NULL), CreateEvent(NULL, false, false, NULL), this };
		pThread[i] = new ThreadHelper(ColorCalc, &callData[i], 0, THREAD_PRIORITY_ABOVE_NORMAL);
		pfEvent[i] = callData[i].pfEvent;
	}
	needLightsUpdate = needLights;
	SetLightGridSize(conf->amb_grid.x, conf->amb_grid.y);
}

CaptureHelper::~CaptureHelper()
{
	Stop();
	// Stop and remove processing threads
	for (DWORD i = 0; i < 16; i++) {
		if (callData[i].dst)
			SetEvent(callData[i].pEvent);
		delete pThread[i];
		CloseHandle(callData[i].pEvent);
		CloseHandle(pfEvent[i]);
	}
	if (dxgi_inuse)
		dxgi_inuse = false;
	else {
		delete dxgi_thread;
		delete dxgi_manager;
		dxgi_manager = NULL;
		scrImg = NULL;
		CoUninitialize();
	}
	delete[] imgz;
}

void CaptureHelper::Start()
{
	if (!dwHandle) {
		dwHandle = new ThreadHelper(CInProc, this, 100, THREAD_PRIORITY_LOWEST);
	}
}

void CaptureHelper::Stop()
{
	if (dwHandle) {
		delete dwHandle;
		dwHandle = NULL;
	}
}

void dxgi_Restart() {
	if (dxgi_manager) {
		w = 0;
		delete dxgi_thread;
		dxgi_manager->set_capture_source((WORD)conf->amb_mode);
		dxgi_thread = new ThreadHelper(dxgi_loop, NULL, 100, THREAD_PRIORITY_LOWEST);
		Sleep(150);
		dxgi_SetDimensions();
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

void ColorCalc(LPVOID inp) {
	procData* src = (procData*) inp;

	if (WaitForSingleObject(src->pEvent, 150) == WAIT_OBJECT_0) {
		CaptureHelper* cap = (CaptureHelper*)src->cap;
		UINT idx = src->dy * cap->hh * cap->stride + src->dx * cap->ww * 4;
		ULONG64 r = 0, g = 0, b = 0;
		for (UINT y = 0; y < cap->hh; y += cap->divider) {
			UINT pos = idx + y * cap->stride;
			for (UINT x = 0; x < cap->ww; x += cap->divider) {
				r += scrImg[pos];
				g += scrImg[pos + 1];
				b += scrImg[pos + 2];
				pos += 4;
			}
		}
		src->dst[0] = (UCHAR) (r / cap->div);
		src->dst[1] = (UCHAR) (g / cap->div);
		src->dst[2] = (UCHAR) (b / cap->div);
		SetEvent(src->pfEvent);
	}
}

//void CaptureHelper::SetDimensions() {
//	RECT dimensions = dxgi_manager->get_output_rect();
//	w = dimensions.right - dimensions.left;
//	h = dimensions.bottom - dimensions.top;
//	ww = w / gridX; hh = h / gridY;
//	stride = w * 4;
//	divider = w > 1920 ? 2 : 1;
//	div = hh * ww / (divider * divider);
//	DebugPrint("Screen resolution set to " + to_string(w) + "x" + to_string(h) + ", div " + to_string(divider) + ".\n");
//}

void CInProc(LPVOID param)
{
	CaptureHelper* src = (CaptureHelper*)param;

	//size_t buf_size;
	//int res;

	// Resize & calc
	UINT tInd = 0;
	if (w && /*h &&*/ /*(res = dxgi_manager->get_output_data(&src->scrImg, &buf_size)) == CR_OK*/ /*&&*/ scrImg) {
		src->ww = w / src->gridX; src->hh = h / src->gridY;
		src->stride = w * 4;
		src->divider = w > 1920 ? 2 : 1;
		src->div = src->hh * src->ww / (src->divider * src->divider);
		for (int dy = 0; dy < src->gridY; dy++)
			for (int dx = 0; dx < src->gridX; dx++) {
				UINT ptr = (dy * src->gridX + dx);
				tInd = ptr % 16;
				if (ptr > 0 && !tInd) {
#ifndef _DEBUG
					WaitForMultipleObjects(16, src->pfEvent, true, 1000);
#else
					if (WaitForMultipleObjects(16, src->pfEvent, true, 1000) != WAIT_OBJECT_0)
						DebugPrint("Ambient thread execution fails at " + to_string(ptr) + "\n");
#endif
				}
				src->callData[tInd].dx = dx;
				src->callData[tInd].dy = dy;
				src->callData[tInd].dst = src->imgo + ptr * 3;
				SetEvent(src->callData[tInd].pEvent);
			}
#ifndef _DEBUG
		WaitForMultipleObjects(tInd + 1, src->pfEvent, true, 1000);
#else
		if (WaitForMultipleObjects(tInd+1, src->pfEvent, true, 1000) != WAIT_OBJECT_0)
			DebugPrint("Ambient thread execution fails at last set\n");
#endif

		if (src->needUpdate = memcmp(src->imgz, src->imgo, src->gridDataSize)) {
			memcpy(src->imgz, src->imgo, src->gridDataSize);
			if (src->needLightsUpdate && conf->lightsNoDelay)
				fxhl->RefreshAmbient();
		}
	}
	//else {
	//	if (res != CR_TIMEOUT) {
	//		DebugPrint("Ambient feed restart!\n");
	//		dxgi_manager->refresh_output();
	//		Sleep(150);
	//		src->SetDimensions();
	//	}
	//}
}
