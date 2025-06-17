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
	CaptureHelper(bool needLights = false);
	~CaptureHelper();
	void Stop();
	void SetLightGridSize(int, int);
	bool needUpdate = false, needLightsUpdate = false;
	byte *imgz = NULL, *imgo;
	byte gridX, gridY;
	DWORD gridDataSize;
	HANDLE pfEvent[16], sEvent;
	procData callData[16];
	UINT ww, hh, div;
private:
	HANDLE pThread[16];
	ThreadHelper* dwHandle = NULL;
};

