#ifndef Graphics_H
#define Graphics_H
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "ConfigHandler.h"
#include "WSAudioIn.h"
#include "FXHelper.h"
#pragma comment(lib, "winmm.lib")

class Graphics {

public:
	Graphics(HINSTANCE hInstance, int mainCmdShow, int* freqp, ConfigHandler *conf, FXHelper *fxproc);
	void start();
	int getBarsNum();
	double getYScale();
	void refresh();
	void ShowError(char* T);
	void SetAudioObject(WSAudioIn* wsa);
	HWND dlg = NULL;

private:
	MSG Msg;
};

#endif