#include "CaptureHelper.h"
#include "DXGIManager.hpp"

DWORD WINAPI CInProc(LPVOID);
DWORD WINAPI CDlgProc(LPVOID);
DWORD WINAPI CFXProc(LPVOID);

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)  
#endif

#define GRIDSIZE 36 // 4x3 x 3

//ConfigAmbient* config;
//FXHelper* fxh;

extern ConfigHandler *conf;
extern FXHelper *fxhl;

byte imgz[GRIDSIZE]{0}, imgo[GRIDSIZE]{0};

DXGIManager* dxgi_manager = NULL;

HANDLE clrStopEvent, uiEvent, lhEvent;

CaptureHelper::CaptureHelper(/*ConfigAmbient* conf, FXHelper* fhh*/)
{
	//config = conf;
	//fxh = fhh;

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
		dwHandle = CreateThread( NULL, 0, CInProc, NULL, 0, NULL);
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

struct procData {
	int dx, dy;
	UCHAR* dst;
	HANDLE pEvent;
	HANDLE pfEvent;
};

static procData callData[3][4];
UINT w = 0, h = 0, ww = 0, hh = 0, stride = 0 , divider = 1;
HANDLE pThread[12]{ 0 };
HANDLE pfEvent[12]{ 0 };
byte* scrImg = NULL;

DWORD WINAPI ColorCalc(LPVOID inp) {
	procData* src = (procData*) inp;
	HANDLE waitArray[2]{src->pEvent, clrStopEvent};
	DWORD res = 0;

	UINT ptr = (src->dy * 4 + src->dx);// *3;

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
			SetEvent(pfEvent[ptr]);
		}
	}
	CloseHandle(pfEvent[ptr]);
	CloseHandle(src->pEvent);
	pfEvent[ptr] = 0;
	return 0;
}

void SetDimensions() {
	RECT dimensions = dxgi_manager->get_output_rect();
	w = dimensions.right - dimensions.left;
	h = dimensions.bottom - dimensions.top;
	ww = w / 4; hh = h / 3;
	stride = w * 4;
}

DWORD WINAPI CInProc(LPVOID param)
{
	//byte* img = NULL;
	byte  imgo[GRIDSIZE]{0};

	size_t buf_size;

	DWORD ret = 0;

	uiEvent = CreateEvent(NULL, false, false, NULL);
	lhEvent = CreateEvent(NULL, false, false, NULL);

	HANDLE uiHandle = CreateThread(NULL, 0, CDlgProc, imgz, 0, NULL),
		lightHandle = CreateThread(NULL, 0, CFXProc, imgz, 0, NULL);

	SetDimensions();

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	Sleep(150);

	while (WaitForSingleObject(clrStopEvent, 50) == WAIT_TIMEOUT) {
		// Resize & calc
		if (w && h && (ret = dxgi_manager->get_output_data(&scrImg, &buf_size)) == CR_OK && scrImg) {
			if (!(conf->monDelay > 200)) {
				for (UINT dy = 0; dy < 3; dy++)
					for (UINT dx = 0; dx < 4; dx++) {
						//ColorCalc(&callData[dy][dx]);
						UINT ptr = (dy * 4 + dx);// *3;
						if (pfEvent[ptr]) {
							//ResetEvent(pfEvent[dy * 4 + dx]);
							SetEvent(callData[dy][dx].pEvent);
						} else {
							callData[dy][dx].dy = dy; callData[dy][dx].dx = dx;
							callData[dy][dx].dst = imgo + ptr * 3;
							callData[dy][dx].pEvent = CreateEvent(NULL, false, true, NULL);
							pfEvent[ptr] = CreateEvent(NULL, false, false, NULL);
							pThread[ptr] = CreateThread(NULL, 6 * w * h, ColorCalc, &callData[dy][dx], 0, NULL);
						}
					}

				if (WaitForMultipleObjects(12, pfEvent, true, 500) == WAIT_OBJECT_0 && memcmp(imgz, imgo, GRIDSIZE)) {
					memcpy(imgz, imgo, GRIDSIZE);
					SetEvent(lhEvent);
					SetEvent(uiEvent);
				}
			} else {
				DebugPrint("Ambient update skipped!\n");
			}
		}
		else {
			if (ret != CR_TIMEOUT) {
				dxgi_manager->refresh_output();
				Sleep(200);
				SetDimensions();
			}
		}
	}

	WaitForMultipleObjects(12, pThread, true, 1000);
	// close handles here!
	for (int i = 0; i < 12; i++) {
		CloseHandle(pThread[i]);
		pThread[i] = 0;
	}
	WaitForSingleObject(uiHandle, 1000);
	WaitForSingleObject(lightHandle, 1000);
	CloseHandle(uiHandle);
	CloseHandle(lightHandle);

	return 0;
}

DWORD WINAPI CDlgProc(LPVOID param)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	while (WaitForSingleObject(clrStopEvent, 50) == WAIT_TIMEOUT) {
		if (conf->amb_conf->hDlg && !IsIconic(GetParent(conf->amb_conf->hDlg)) && WaitForSingleObject(uiEvent, 0) == WAIT_OBJECT_0) {
			//DebugPrint("Ambient UI update...\n");
			//memcpy(imgui, (UCHAR*)param, sizeof(imgz));
			SendMessage(conf->amb_conf->hDlg, WM_PAINT, 0, (LPARAM)imgz);
		}
	}
	return 0;
}

DWORD WINAPI CFXProc(LPVOID param) {
	//UCHAR  imgz[12 * 3];
	HANDLE waitArray[2]{lhEvent, clrStopEvent};
	DWORD res = 0;
	//SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	while ((res = WaitForMultipleObjects(2, waitArray, false, 200)) != WAIT_OBJECT_0 + 1) {
		if (res == WAIT_OBJECT_0) {
			//DebugPrint("Ambient light update...\n");
			//memcpy(imgz, (UCHAR *) param, sizeof(imgz));
			fxhl->RefreshAmbient(imgz);
		}
	}
	return 0;
}