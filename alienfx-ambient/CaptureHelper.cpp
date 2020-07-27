#include "CaptureHelper.h"
#include <DaramCam.MediaFoundationGenerator.h>
#include <map>
#include <list>
#include <algorithm>

DWORD WINAPI CInProc(LPVOID);

bool inWork = true;

CaptureHelper::CaptureHelper(HWND dlg, int mode)
{
	DCStartup();
	DCMFStartup();
	switch (mode) {
	case 0: screenCapturer = DCCreateDXGIScreenCapturer(DCDXGIScreenCapturerRange_MainMonitor);
		break;
	case 1: screenCapturer = DCCreateDXGIScreenCapturer(DCDXGIScreenCapturerRange_SubMonitors); break; // other monitors.
	}
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
	inWork = true;
}

int CaptureHelper::GetColor(int pos)
{
	return 0;
}

bool compare(std::pair<UINT32, int> a, std::pair<UINT32, int> b)
{
	return a.second > b.second;
}

UCHAR* Resize(UCHAR* src, UINT w1, UINT h1)
{
	typedef std::vector<std::pair<UINT32, int> > colordist;
	typedef std::map<UINT32, int> colormap;
	UCHAR* retval = new UCHAR[3 * 3 * 3];
	int wPix = w1 / 3, hPix = h1 / 3;
	colormap cMap;
	colordist pCount;
	int ptr = 0;
	for (int y = 0; y < 3; y++)
		for (int x = 0; x < 3; x++) {
			//count colors across zone....
			ptr = (y * w1 * hPix + x * wPix) * 3;
			for (int dy = 0; dy < hPix; dy++) {
				for (int dx = 0; dx < wPix; dx++) {
					cMap[(src[ptr] << 8 + src[ptr + 1]) << 8 + src[ptr + 2]] ++;
					ptr += 3;
				}
				ptr += 3 * (w1 - wPix);
			}
			// Now reduce colors.....
			pCount.clear();
			pCount.resize(cMap.size());
			copy(cMap.begin(), cMap.end(), pCount.begin());
			// Sort by popularity
			sort(pCount.begin(), pCount.end(), compare);
			cMap.clear();
		}
	
	return retval;
}

DWORD WINAPI CInProc(LPVOID param)
{
	IStream* stream;
	DCScreenCapturer* screenCapturer = (DCScreenCapturer*)param;
	UINT w, h;
	UCHAR* img;
	UCHAR* imgz;
	while (inWork) {
		screenCapturer->Capture();
		img = screenCapturer->GetCapturedBitmap()->GetByteArray();
		w = screenCapturer->GetCapturedBitmap()->GetWidth();
		h = screenCapturer->GetCapturedBitmap()->GetHeight();
		// Resize
		imgz = Resize(img, w, h);
		// DEBUG!
		SHCreateStreamOnFileEx(TEXT("C:\\TEMP\\Test.png"), STGM_READWRITE | STGM_CREATE, 0, false, 0, &stream);
		DCBitmap out(0,0);
		out.Resize(3, 3);
		out.SetDirectBuffer(imgz);
		DCImageGenerator* imgGen = DCCreateWICImageGenerator(DCWICImageType_PNG);
		SIZE size = { 3, 3 }; //(LONG)screenCapturer->GetCapturedBitmap()->GetWidth(), (LONG)screenCapturer->GetCapturedBitmap()->GetHeight()
		
		DCSetSizeToWICImageGenerator(imgGen, &size);
		imgGen->Generate(stream, &out);

		stream->Release();

		delete imgGen;
		// Update lights
		// Update UI
		free(imgz);
		Sleep(100);
	}
	return 0;
}
