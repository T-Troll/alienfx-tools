#pragma once
#ifndef Graphics_H
#define Graphics_H
//#include <windows.h>
#include "ConfigHaptics.h"
#include "WSAudioIn.h"
#include "FXHelper.h"
//#pragma comment(lib, "winmm.lib")

void DrawFreq(HWND hDlg, int* freq);

class Graphics {

public:
	Graphics(HINSTANCE hInstance, int mainCmdShow, int* freqp, ConfigHaptics *conf, FXHelper *fxproc);
	void start();
	void ShowError(char* T);
	void SetAudioObject(WSAudioIn* wsa);
	HWND GetDlg();

private:
	MSG Msg;
	HWND dlg = NULL;
};

#endif