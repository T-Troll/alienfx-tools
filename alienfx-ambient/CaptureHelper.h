#pragma once
#include <wtypes.h>
#include "DaramCam/DaramCam.h"
#include "ConfigHandler.h"
#include "FXHelper.h"

class CaptureHelper
{
public:
	CaptureHelper(HWND dlg, ConfigHandler* conf, FXHelper* fhh);
	~CaptureHelper();
	void SetCaptureScreen(int mode);
	void Start();
	void Stop();
	void Restart();
	bool isDirty = false;
private:
	DCScreenCapturer* screenCapturer = NULL;
	DWORD dwThreadID = 0;
	HANDLE dwHandle = NULL;
};

