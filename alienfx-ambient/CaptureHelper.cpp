#include "CaptureHelper.h"
#include <DaramCam.MediaFoundationGenerator.h>
#include <map>
#include <list>
#include <algorithm>
#include "resource.h"
#include "FXHelper.h"

DWORD WINAPI CInProc(LPVOID);
DWORD WINAPI CDlgProc(LPVOID);
DWORD WINAPI CFXProc(LPVOID);

bool inWork = true;

HWND hDlg;

ConfigHandler* config;
FXHelper* fxh;

CaptureHelper::CaptureHelper(HWND dlg, ConfigHandler* conf)
{
	DCStartup();
	DCMFStartup();
	switch (conf->mode) {
	case 0: screenCapturer = DCCreateDXGIScreenCapturer(DCDXGIScreenCapturerRange_MainMonitor);
		break;
	case 1: screenCapturer = DCCreateDXGIScreenCapturer(DCDXGIScreenCapturerRange_SubMonitors); break; // other monitors.
	}
	hDlg = dlg;
	config = conf;
	fxh = new FXHelper(conf);
}

CaptureHelper::~CaptureHelper()
{
	DCMFShutdown();
	DCShutdown();
}

void CaptureHelper::Start()
{
	inWork = true;
	CreateThread(
		NULL,              // default security
		0,                 // default stack size
		CInProc,        // name of the thread function
		screenCapturer,
		0,                 // default startup flags
		&dwThreadID);
}

void CaptureHelper::Stop()
{
	inWork = false;
}

int CaptureHelper::GetColor(int pos)
{
	return 0;
}

bool compare(std::pair<UINT32, int> a, std::pair<UINT32, int> b)
{
	return a.second > b.second;
}

UINT GetPtr(int x, int y, int cd, int st) {
	return y * st + x * cd;
}

UCHAR* Resize(UCHAR* src, UINT w1, UINT h1, UINT cd, UINT st, UINT div)
{
	struct ColorComp
	{
		unsigned char b;
		unsigned char g;
		unsigned char r;
		unsigned char br;
	};
	union Colorcode
	{
		struct ColorComp cs;
		unsigned int ci;
	};
	typedef std::vector<std::pair<UINT32, int> > colordist;
	typedef std::map<UINT32, int> colormap;
	int wPix = w1 / 3, hPix = h1 / 3;
	UCHAR* retval = new UCHAR[9 * cd];
	colormap cMap;
	colordist pCount;
	Colorcode cColor;
	int ptr = 0;
	//int rad = 50; // hPix;
	int x = 0, y = 0;

	for (int y = 0; y < 3; y++)
		for (int x = 0; x < 3; x++) {
			//count colors across zone....
			//ptr = (y * w1 * hPix + x * wPix) * 3;
			for (int dy = 0; dy < hPix; dy+=div) {
				for (int dx = 0; dx < wPix; dx+=div) {
					int ptr = GetPtr(dx + x * wPix, dy + y * hPix, cd, st);
					cColor.cs.r = src[ptr];
					cColor.cs.g = src[ptr+1];
					cColor.cs.b = src[ptr+2];
					cColor.cs.br = src[ptr+3];
					cMap[cColor.ci]++;
				}
			}
			// Now reduce colors.....
			//pCount.clear();
			//pCount.resize(cMap.size());
			//copy(cMap.begin(), cMap.end(), pCount.begin());
			// Sort by popularity
			// sort(pCount.begin(), pCount.end(), compare);
			Colorcode tk; int mcount = 0;
			for (auto& it : cMap) {
				if (it.second > mcount) {
					tk.ci = it.first;
					mcount = it.second;
				}
			}
			cMap.clear();
			//tk.ci = pCount[0].first;
			int ptr = GetPtr(x, y, cd, 3 * cd);
				retval[ptr] = tk.cs.r;
				retval[ptr+1] = tk.cs.g;
				retval[ptr+2] = tk.cs.b;
				retval[ptr+3] = tk.cs.br;
		}
	return retval;
}

DWORD WINAPI CInProc(LPVOID param)
{
	//IStream* stream;
	DCScreenCapturer* screenCapturer = (DCScreenCapturer*)param;
	UINT w, h, st, cdp;
	UCHAR* img = 0;
	UCHAR* imgz = 0;
	DWORD uiThread;
	UINT div = config->divider;
	while (inWork) {
		screenCapturer->Capture();
		img = screenCapturer->GetCapturedBitmap()->GetByteArray();
		w = screenCapturer->GetCapturedBitmap()->GetWidth();
		h = screenCapturer->GetCapturedBitmap()->GetHeight();
		cdp = screenCapturer->GetCapturedBitmap()->GetColorDepth();
		st = screenCapturer->GetCapturedBitmap()->GetStride();
		// Resize
		delete imgz;
		imgz = Resize(img, w, h, cdp, st, div);
		// DEBUG!
		//SHCreateStreamOnFileEx(TEXT("C:\\TEMP\\Test.png"), STGM_READWRITE | STGM_CREATE, 0, false, 0, &stream);
		//DCBitmap out(3, 3, cdp);
		//out.Resize(w/3, h/3);
		//out.SetIsDirectMode(true);
		//out.SetDirectBuffer(imgz);
		//DCImageGenerator* imgGen = DCCreateWICImageGenerator(DCWICImageType_PNG);
		//SIZE size = { 3, 3 }; //(LONG)screenCapturer->GetCapturedBitmap()->GetWidth(), (LONG)screenCapturer->GetCapturedBitmap()->GetHeight()
		
		//DCSetSizeToWICImageGenerator(imgGen, &size);
		//imgGen->Generate(stream, &out);

		//stream->Release();

		//delete imgGen;
		// Update lights
		CreateThread(
			NULL,              // default security
			0,                 // default stack size
			CFXProc,        // name of the thread function
			imgz,
			0,                 // default startup flags
			&uiThread);
		// Update UI
		CreateThread(
			NULL,              // default security
			0,                 // default stack size
			CDlgProc,        // name of the thread function
			imgz,
			0,                 // default startup flags
			&uiThread);
		//free(imgz);
		//Sleep(100);
	}
	return 0;
}

DWORD WINAPI CDlgProc(LPVOID param)
{
	UCHAR* img = (UCHAR *)param;
	RECT rect;
	HBRUSH Brush = NULL;
	for (int i = 0; i < 9; i++) {
		HWND tl = GetDlgItem(hDlg, IDC_BUTTON1 + i);
		HWND cBid = GetDlgItem(hDlg, IDC_CHECK1 + i);
		GetWindowRect(tl, &rect);
		HDC cnt = GetWindowDC(tl);
		//SetBkColor(cnt, RGB(255, 0, 0));
		//SetBkMode(cnt, TRANSPARENT);
		rect.bottom -= rect.top;
		rect.right -= rect.left;
		rect.top = rect.left = 0;
		// BGR!
		Brush = CreateSolidBrush(RGB(img[i*4+2], img[i*4+1], img[i*4]));
		FillRect(cnt, &rect, Brush);
		DeleteObject(Brush);
		UINT state = IsDlgButtonChecked(hDlg, IDC_CHECK1 + i); //Get state of the button
		if ((state & BST_CHECKED))            // If it is pressed
		{
			DrawEdge(cnt, &rect, EDGE_SUNKEN, BF_RECT);    // Draw a sunken face
		}
		else
		{
			DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);    // Draw a raised face
		}
		RedrawWindow(cBid, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	return 0;
}

DWORD WINAPI CFXProc(LPVOID param) {
	fxh->Refresh((UCHAR*)param);
	fxh->UpdateLights();
	return 0;
}