#pragma once
#include "ThreadHelper.h"
#include "DXGIManager.hpp"

//#define GRIDSIZE 36 // 4x3 x 3

struct procData {
	int dx, dy;
	UCHAR* dst;
	HANDLE pEvent;
	HANDLE pfEvent;
};

class CaptureHelper
{
public:
	CaptureHelper();
	~CaptureHelper();
	void SetCaptureScreen(int mode);
	void Start();
	void Stop();
	void Restart();
	void SetLightGridSize(int, int);
	void SetDimensions();
	bool needUpdate = false;
	bool needUIUpdate = false;
	byte *imgz;
	DXGIManager* dxgi_manager = NULL;
private:
	HANDLE dwHandle = NULL;
	ThreadHelper* lThread;
};

