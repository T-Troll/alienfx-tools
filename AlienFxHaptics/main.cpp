#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
//#include "resource.h"
//#include "AudioIn.h"
#include "Graphics.h"
#include "DFT_gosu.h"
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "WSAudioIn.h"
#pragma comment(lib, "winmm.lib")

double** cosarg;
double** sinarg;

Graphics* Graphika;
DFT_gosu* dftG;
FXHelper* FXproc;

const int NUMPTS = 2048;// 44100 / 15;
//void resample(double* waveDouble);
DWORD WINAPI resample(LPVOID lpParam);

bool tdone = false;

//HANDLE cEvent;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	//MSG Msg;
	int* freq;
	
	freq=(int*)malloc(21*sizeof(int));
	memset(freq, 0, 21 * sizeof(int));

	//cEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	ConfigHandler conf;
	conf.Load();

	FXproc = new FXHelper(freq, &conf);
	Graphika = new Graphics(hInstance ,nCmdShow, freq, &conf);
	dftG = new DFT_gosu(NUMPTS, Graphika->getBarsNum() , Graphika->getYScale() , freq);

	//void (*callbackfunc)(double*) = resample;
	DWORD (*callbackfunc)(LPVOID) = resample;

	int rate;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	WSAudioIn wsa(rate, NUMPTS, conf.inpType, Graphika, callbackfunc);
	dftG->setSampleRate(rate);
	wsa.startSampling();

	//AudioIn audio(44100, NUMPTS, callbackfunc);
	//audio.startSampling();

	Graphika->start();
	tdone = 1;
	Sleep(200);
	//audio.stopSampling();
	wsa.stopSampling();

	//killing dft:
	dftG->kill();

	free(freq);

	return 1;
}


DWORD WINAPI resample(LPVOID lpParam)
//void resample(double* waveDouble)
{
	if (tdone) return 0;
	double* waveDouble = (double*)lpParam;

	//dftG->setXscale(Graphika->getBarsNum());
	dftG->setYscale(Graphika->getYScale());

	dftG->calc(waveDouble);

	//Graphika->setCurrentPower(dftG->getCurrentPower());
	//Graphika->setShortPower(dftG->getShortPower());
	//Graphika->setLongPower(dftG->getLongPower());
	//Graphika->setCurrentAvgFreq(dftG->getCurrentAvgFreq());
	//Graphika->setShortAvgFreq(dftG->getShortAvgFreq());
	//Graphika->setLongAvgFreq(dftG->getLongAvgFreq());

	//Graphika->setYScale(dftG->getYscale());

	Graphika->refresh();
	FXproc->Refresh(Graphika->getBarsNum());
	//FXproc->UpdateLights();
	return 0;
}