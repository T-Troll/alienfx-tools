#include <windows.h>
#include "Graphics.h"
#include "DFT_gosu.h"
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "WSAudioIn.h"
#pragma comment(lib, "winmm.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

Graphics* Graphika;
DFT_gosu* dftG;
FXHelper* FXproc;

const int NUMPTS = 2048;// 44100 / 15;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	int* freq;
	
	ConfigHandler conf;

	FXproc = new FXHelper(&conf);
	freq = new int[conf.numbars];
	ZeroMemory(freq, conf.numbars * sizeof(int));
	Graphika = new Graphics(hInstance ,nCmdShow, freq, &conf, FXproc);

	dftG = new DFT_gosu(NUMPTS, conf.numbars, conf.res , freq);

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	WSAudioIn wsa(NUMPTS, conf.inpType, Graphika, FXproc, dftG);
	wsa.startSampling();

	Graphika->start();
	wsa.stopSampling();

	FXproc->FadeToBlack();

	delete FXproc;
	delete dftG;

	delete[] freq;
	delete Graphika;

	return 1;
}
