#pragma once
#include "ThreadHelper.h"
#include "DXGIManager.hpp"

struct procData {
	int dx, dy;
	UCHAR* dst;
	HANDLE pEvent;
	HANDLE pfEvent;
	void* cap;
};

class CaptureHelper
{
public:
	CaptureHelper(bool needLights);
	~CaptureHelper();
	void Start();
	void Stop();
	//void Restart();
	void SetLightGridSize(int, int);
	//void SetDimensions();
	bool needUpdate = false, needLightsUpdate = false;
	byte *imgz = NULL, *imgo/*, *scrImg = NULL*/;
	byte gridX, gridY;
	DWORD gridDataSize;
	//DXGIManager* dxgi_manager = NULL;
	HANDLE pfEvent[16];
	procData callData[16];
	UINT /*w, h,*/ ww, hh, stride, divider, div;
private:
	ThreadHelper* dwHandle = NULL, *pThread[16];
};

