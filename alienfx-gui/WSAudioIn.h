#pragma once
#ifndef WSAUDIO_H
#define WSAUDIO_H
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include "ThreadHelper.h"

#define NUMBARS 20
#define NUMPTS 2048


class WSAudioIn
{
public:

	WSAudioIn();
	~WSAudioIn();
	void startSampling();
	void stopSampling();
	void RestartDevice();
	void init();
	void release();
	void SetSilence();

	double waveD[NUMPTS];
	int freqs[NUMBARS];

	IAudioCaptureClient* pCaptureClient = NULL;

	WAVEFORMATEX* pwfx;

	HANDLE stopEvent, hEvent, cEvent;

	bool needUpdate = false, clearBuffer = false;

	// FFT variables
	//int sampleRate = 44100;
	double blackman[NUMPTS];// , * hanning;

private:
	IMMDevice* inpDev;
	IAudioClient* pAudioClient = NULL;

	IAudioClockAdjustment* pRateClient = NULL;
	ISimpleAudioVolume* pAudioVolume = NULL;

	HANDLE dwHandle = NULL, fftHandle;

	IMMDevice* GetDefaultMultimediaDevice(EDataFlow DevType);

	ThreadHelper* lightUpdate;
};

#endif
