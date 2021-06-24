#pragma once
#ifndef WSAUDIO_H
#define WSAUDIO_H
#include <windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>

class WSAudioIn
{
public:

	WSAudioIn(int& rate, int N, int type, void* gr, void* fx, void* dft);
	~WSAudioIn();
	void startSampling();
	void stopSampling();
	void RestartDevice(int type);
	int init(int type);
	void release();
private:
	IMMDevice* inpDev;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pCaptureClient = NULL;
	IAudioClockAdjustment* pRateClient = NULL;
	ISimpleAudioVolume* pAudioVolume = NULL;
	float originalVolume;

	//Graphics* gHandle = NULL;

	WAVEFORMATEX* pwfx, *suggest;

	BYTE* byteArray;
	unsigned byteArrayLength;
	int rate;

	UINT32 bufferFrameCount;

	bool silentNullBuffer;

	HANDLE hWakeUp;
	//HANDLE hEvent;

	//double* waveDouble; // a pointer to a double array to store the results in
	//void(*pt2Function)(double*);

	IMMDevice* GetDefaultMultimediaDevice(EDataFlow DevType);
};

#endif
