#include <windows.h>
#include "Graphics.h"
//#include "DFT_gosu.h"
#include "FXHelper.h"
#include "WSAudioIn.h"

Graphics* Graphika;
//DFT_gosu* dftG;
FXHelper* FXproc;

//const int NUMPTS = 2048;// 44100 / 15;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	ConfigHaptics conf;

	FXproc = new FXHelper(&conf);
	//ZeroMemory(freq, conf.numbars * sizeof(int));
	WSAudioIn wsa(&conf, FXproc);
	Graphika = new Graphics(hInstance, &conf, FXproc, &wsa);

	//dftG = new DFT_gosu(NUMPTS, conf.numbars, conf.res , freq);

	wsa.startSampling();

	Graphika->start();
	wsa.stopSampling();

	FXproc->FadeToBlack();

	delete FXproc;
	//delete dftG;

	delete Graphika;

	return 1;
}
