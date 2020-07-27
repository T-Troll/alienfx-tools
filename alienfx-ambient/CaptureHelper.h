#pragma once
#include <wtypes.h>
#include <DaramCam.h>
class CaptureHelper
{
public:
	CaptureHelper(HWND dlg, int mode);
	~CaptureHelper();
	void Start();
	void Stop();
	int GetColor(int pos);
private:
	DCScreenCapturer* screenCapturer;
	DWORD dwThreadID;
};

