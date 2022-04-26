#pragma once
#include "ConfigAmbient.h"
#include "FXHelper.h"

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
	bool isDirty = false;
	byte imgz[GRIDSIZE]{ 0 };
private:
	HANDLE dwHandle = NULL;
};

