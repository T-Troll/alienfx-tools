#pragma once
#include "ThreadHelper.h"
#include "DXGIManager.hpp"

struct procData {
	HANDLE pEvent;
	HANDLE pfEvent;
	void* cap;
	int idx;
	UCHAR* dst;
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
	byte *imgz = NULL, *imgo;
	byte gridX, gridY;
	DWORD gridDataSize;
	//DXGIManager* dxgi_manager = NULL;
	HANDLE pfEvent[16], sEvent;
	procData callData[16];
	UINT ww, hh, div;
private:
	HANDLE pThread[16];
	ThreadHelper* dwHandle = NULL;
};

