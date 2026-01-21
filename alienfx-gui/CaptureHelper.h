#pragma once
#include "ThreadHelper.h"
#include "DXGIManager.hpp"
#include <set>

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
	void FillAmbientMap();
	bool needLightsUpdate = false;
	byte *imgz = NULL, *imgo;
	byte gridX, gridY;
	DWORD gridDataSize;
	HANDLE pfEvent[16], sEvent;
	procData callData[16];
	UINT ww, hh, div;
	std::set<byte> ambient_map;
	ThreadHelper* dwHandle = NULL;
private:
	HANDLE pThread[16];
};

