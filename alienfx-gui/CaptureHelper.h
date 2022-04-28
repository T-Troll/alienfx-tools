#pragma once
#include "ConfigAmbient.h"
#include "FXHelper.h"

class CaptureHelper
{
public:
	CaptureHelper();
	~CaptureHelper();
	void SetCaptureScreen(int mode);
	void Start();
	void Stop();
	void Restart();
	void SetGridSize(int, int);
	bool isDirty = false;
	bool needUpdate = false;
	byte *imgz;
private:
	HANDLE dwHandle = NULL;
};

