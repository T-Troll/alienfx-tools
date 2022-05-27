#pragma once
#ifndef WSAUDIO_H
#define WSAUDIO_H
#include <windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include "DFT_gosu.h"

class WSAudioIn
{
public:

	WSAudioIn();
	~WSAudioIn();
	void startSampling();
	void stopSampling();
	void RestartDevice(int type);
	int  init(int type);
	void release();

	// variables...
	DFT_gosu* dftGG = NULL;
	double* waveD;
	int *freqs = NULL;

	//HWND dlg = NULL;

	IAudioCaptureClient* pCaptureClient = NULL;

	WAVEFORMATEX* pwfx;

	HANDLE stopEvent, updateEvent, hEvent;

private:
	IMMDevice* inpDev;
	IAudioClient* pAudioClient = NULL;

	IAudioClockAdjustment* pRateClient = NULL;
	ISimpleAudioVolume* pAudioVolume = NULL;

	HANDLE dwHandle = 0;

	int rate;

	IMMDevice* GetDefaultMultimediaDevice(EDataFlow DevType);
};

#endif
