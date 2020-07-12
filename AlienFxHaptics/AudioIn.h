#ifndef AudioIn_H
#define AudioIn_H
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include<conio.h>
#include <math.h>
#pragma comment(lib, "winmm.lib")


class AudioIn {

public:
	AudioIn( int rate, int N, void(*func)(double*) );
	void startSampling();
	void stopSampling();

private:
	HWAVEIN hWaveIn;
	WAVEFORMATEX pFormat;
	int sampleRate;
};

#endif