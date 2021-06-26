#pragma once
#include "ConfigHandler.h"
#include "FXHelper.h"

class CaptureHelper
{
public:
	CaptureHelper(HWND dlg, ConfigHandler* conf, FXHelper* fhh);
	~CaptureHelper();
	void SetCaptureScreen(int mode);
	void Start(DWORD delay = 0);
	void Stop();
	void Restart();
	bool isDirty = false;
private:
	HANDLE dwHandle = NULL;
};

