#pragma once
#include <wtypes.h>
#include <DaramCam.h>
#include "ConfigHandler.h"

class CaptureHelper
{
public:
	CaptureHelper(HWND dlg, ConfigHandler* conf);
	~CaptureHelper();
	void Start();
	void Stop();
	int GetColor(int pos);
private:
	DCScreenCapturer* screenCapturer = NULL;
	DWORD dwThreadID = 0;
	HANDLE dwHandle = NULL;
};

