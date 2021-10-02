#pragma once
#ifndef WSAUDIO_H
#define WSAUDIO_H
#include <windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include "ConfigHaptics.h"
#include "FXHelper.h"

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
	void SetDlg(HWND hDlg);

private:
	IMMDevice* inpDev;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pCaptureClient = NULL;
	IAudioClockAdjustment* pRateClient = NULL;
	ISimpleAudioVolume* pAudioVolume = NULL;

	WAVEFORMATEX* pwfx;

	HANDLE dwHandle = 0;

	int rate;

	//HANDLE hWakeUp;

	IMMDevice* GetDefaultMultimediaDevice(EDataFlow DevType);
};

#endif
