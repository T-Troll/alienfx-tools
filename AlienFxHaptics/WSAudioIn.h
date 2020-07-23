#pragma once
#ifndef WSAUDIO_H
#define WSAUDIO_H
#include <windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
//#include "Graphics.h"

class WSAudioIn
{
public:

	WSAudioIn(int& rate, int N, int type, void* gr, DWORD(*func)(LPVOID));
	~WSAudioIn();
	void startSampling();
	void stopSampling();
	void RestartDevice(int type);
	int init(int type);
	void release();
private:
	IMMDevice* inpDev;
	IAudioClient* pAudioClient;
	IAudioCaptureClient* pCaptureClient;
	IAudioClockAdjustment* pRateClient;
	ISimpleAudioVolume* pAudioVolume;
	float originalVolume;

	//Graphics* gHandle;

	WAVEFORMATEX* pwfx;

	BYTE* byteArray;
	unsigned byteArrayLength;

	UINT32 bufferFrameCount;

	bool silentNullBuffer;

	HANDLE hWakeUp;
	//HANDLE hEvent;

	//double* waveDouble; // a pointer to a double array to store the results in
	//void(*pt2Function)(double*);

	IMMDevice* GetDefaultMultimediaDevice(EDataFlow DevType);
};

#endif
