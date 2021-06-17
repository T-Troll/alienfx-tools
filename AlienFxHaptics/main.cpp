#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "Graphics.h"
#include "DFT_gosu.h"
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "WSAudioIn.h"
#pragma comment(lib, "winmm.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

double** cosarg;
double** sinarg;

Graphics* Graphika;
DFT_gosu* dftG;
FXHelper* FXproc;

const int NUMPTS = 2048;// 44100 / 15;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	int* freq;
	
	freq=(int*)malloc(21*sizeof(int));
	memset(freq, 0, 21 * sizeof(int));

	ConfigHandler conf;
	conf.Load();

	FXproc = new FXHelper(freq, &conf);
	Graphika = new Graphics(hInstance ,nCmdShow, freq, &conf, FXproc);
	dftG = new DFT_gosu(NUMPTS, Graphika->getBarsNum() , Graphika->getYScale() , freq);

	int rate;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	WSAudioIn wsa(rate, NUMPTS, conf.inpType, Graphika, FXproc, dftG);
	dftG->setSampleRate(rate);
	wsa.startSampling();

	Graphika->start();
	wsa.stopSampling();

	dftG->kill();
	FXproc->FadeToBlack();

	free(freq);
	delete FXproc;
	delete dftG;
	delete Graphika;

	return 1;
}
