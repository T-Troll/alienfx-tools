#pragma once
#ifndef WSAUDIO_H
#define WSAUDIO_H
#include <windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include "ConfigHaptics.h"
#include "FXHelper.h"
#include "DFT_gosu.h"

class WSAudioIn
{
public:

	WSAudioIn(ConfigHaptics* cf, FXHelper* fx);
	~WSAudioIn();
	void startSampling();
	void stopSampling();
	void RestartDevice(int type);
	int  init(int type);
	void release();

	// variables...
	FXHelper* fxha = NULL;
	DFT_gosu* dftGG = NULL;
	ConfigHaptics *conf = NULL;
	double* waveD;
	int *freqs = NULL;

	HWND dlg = NULL;

	IAudioCaptureClient* pCaptureClient = NULL;

	WAVEFORMATEX* pwfx;

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
