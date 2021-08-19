#pragma once
#ifndef WSAUDIO_H
#define WSAUDIO_H
#include <windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>

class WSAudioIn
{
public:

	WSAudioIn(int N, int type, void* gr, void* fx, void* dft);
	~WSAudioIn();
	void startSampling();
	void stopSampling();
	void RestartDevice(int type);
	int  init(int type);
	void release();
private:
	IMMDevice* inpDev;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pCaptureClient = NULL;
	IAudioClockAdjustment* pRateClient = NULL;
	ISimpleAudioVolume* pAudioVolume = NULL;

	WAVEFORMATEX* pwfx;

	void* gHandle;

	int rate;

	HANDLE hWakeUp;

	IMMDevice* GetDefaultMultimediaDevice(EDataFlow DevType);
};

#endif
