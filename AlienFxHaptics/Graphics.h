#ifndef Graphics_H
#define Graphics_H
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "LFXUtil.h"
#include "ConfigHandler.h"
#include "WSAudioIn.h"
#pragma comment(lib, "winmm.lib")

class Graphics {

public:
	Graphics(HINSTANCE hInstance, int mainCmdShow, int* freqp, ConfigHandler *conf);
	void start();
	int getBarsNum();
	double getYScale();
	void setYScale(double newS);
	void refresh();
	void setCurrentPower(double po);
	void setShortPower(double po);
	void setLongPower(double po);
	void setCurrentAvgFreq(int af);
	void setShortAvgFreq(int af);
	void setLongAvgFreq(int af);
	void ShowError(char* T);
	void SetAudioObject(WSAudioIn* wsa);

private:
	WNDCLASSEX wc;
	MSG Msg;

	char g_szClassName[14];
	
	BOOL g_bOpaque;
	COLORREF g_rgbText;
	COLORREF g_rgbBackground;

	COLORREF g_rgbCustom[16];

	HWND hwnd;

};

#endif