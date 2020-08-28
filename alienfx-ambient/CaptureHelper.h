#pragma once
#include <wtypes.h>
#include <DaramCam.h>
#include "ConfigHandler.h"
#include "FXHelper.h"

class CaptureHelper
{
public:
	CaptureHelper(HWND dlg, ConfigHandler* conf, FXHelper* fhh);
	~CaptureHelper();
	void Start();
	void Stop();
	int GetColor(int pos);
	int GetPID();
private:
	DCScreenCapturer* screenCapturer = NULL;
	DWORD dwThreadID = 0;
	HANDLE dwHandle = NULL;
};

