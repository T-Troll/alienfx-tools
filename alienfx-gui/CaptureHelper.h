#pragma once
#include "ThreadHelper.h"
#include "DXGIManager.hpp"

#define GRIDSIZE 36 // 4x3 x 3

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

