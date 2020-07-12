#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
//#include "resource.h"
#include "AudioIn.h"
#include "Graphics.h"
#include "DFT_gosu.h"
#include "LFXUtil.h"
#include "ConfigHandler.h"
#include "FXHelper.h"
#pragma comment(lib, "winmm.lib")

double** cosarg;
double** sinarg;

Graphics* Graphika;
DFT_gosu* dftG;
FXHelper* FXproc;

const int NUMPTS = 2048;// 44100 / 20;
void resample(double* waveDouble);

namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{

	//MSG Msg;
	int* freq;
	
	freq=(int*)malloc(201*sizeof(int));
	for (int i=0; i<201; i++)
		freq[i]=0;

	if (lfxUtil.InitLFX()) {
		lfxUtil.FillInfo();
	}

	ConfigHandler conf;
	conf.Load();

	Graphika = new Graphics(hInstance ,nCmdShow, freq, &lfxUtil, &conf);
	dftG = new DFT_gosu(NUMPTS, Graphika->getBarsNum() , Graphika->getYScale() , freq);
	FXproc = new FXHelper(freq, &lfxUtil, &conf);

	void (*callbackfunc)(double*) = NULL;
	callbackfunc = resample;

	AudioIn audio(44100, NUMPTS, callbackfunc);
	audio.startSampling();

	Graphika->start();

	//killing audio:
	audio.stopSampling();
	FXproc->StopFX();

	//killing dft:
	dftG->kill();

	free(freq);

	return 1;
}



void resample(double* waveDouble)
{
		dftG->setXscale(  Graphika->getBarsNum()  );
		dftG->setYscale(  Graphika->getYScale()  );

		dftG->calc(waveDouble);

		Graphika->setCurrentPower(  dftG->getCurrentPower()  );
		Graphika->setShortPower(  dftG->getShortPower()  );
		Graphika->setLongPower(  dftG->getLongPower()  );
		Graphika->setCurrentAvgFreq(  dftG->getCurrentAvgFreq()  );
		Graphika->setShortAvgFreq(  dftG->getShortAvgFreq()  );
		Graphika->setLongAvgFreq(  dftG->getLongAvgFreq()  );

		//Graphika->setYScale(dftG->getYscale());

		Graphika->refresh();
		FXproc->Refresh(Graphika->getBarsNum());
		FXproc->UpdateLights();
}