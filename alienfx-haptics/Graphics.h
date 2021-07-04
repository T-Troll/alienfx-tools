#ifndef Graphics_H
#define Graphics_H
#include <windows.h>
#include "ConfigHandler.h"
#include "WSAudioIn.h"
#include "FXHelper.h"
#pragma comment(lib, "winmm.lib")

class Graphics {

public:
	Graphics(HINSTANCE hInstance, int mainCmdShow, int* freqp, ConfigHandler *conf, FXHelper *fxproc);
	void start();
	void ShowError(char* T);
	void SetAudioObject(WSAudioIn* wsa);
	HWND GetDlg();

private:
	MSG Msg;
	HWND dlg = NULL;
};

#endif