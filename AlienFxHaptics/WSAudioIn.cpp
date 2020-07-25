#include "WSAudioIn.h"
#include <math.h>
#include "Graphics.h"

//void CALLBACK WSwaveInProc(HWAVEIN hWaveIn, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam);
DWORD WINAPI WSwaveInProc(LPVOID);

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
const IID IID_IAudioClockAdjustment = __uuidof(IAudioClockAdjustment);

REFERENCE_TIME hnsRequestedDuration = 0;

int NUMSAM;
bool done = false;
double* waveD;
int nChannel;
int bytePerSample;
int blockAlign;

HANDLE hEvent;

DWORD(*mFunction)(LPVOID);

Graphics* gHandle;

WSAudioIn::WSAudioIn(int &rate, int N, int type, void* gr, DWORD(*func)(LPVOID))
{
	NUMSAM = N;
	mFunction = func;
	waveD = (double*)malloc(NUMSAM * sizeof(double));
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	gHandle = (Graphics *) gr;
	gHandle->SetAudioObject(this);
	rate = init(type);

}

WSAudioIn::~WSAudioIn()
{
	release();
	free(waveD);
}

void WSAudioIn::startSampling()
{
	DWORD dwThreadID;
	done = false;
	// creating listener thread...
	if (pAudioClient) {
		CreateThread(
			NULL,              // default security
			0,                 // default stack size
			WSwaveInProc,        // name of the thread function
			pCaptureClient,
			0,                 // default startup flags
			&dwThreadID);
		pAudioClient->Start();
	}
}

void WSAudioIn::stopSampling()
{
	done = true;
	pAudioClient->Stop();
}

void WSAudioIn::RestartDevice(int type)
{
	release();
	
	init(type);
	startSampling();
}

int WSAudioIn::init(int type)
{
	DWORD ret = 0;
	// get deveice
	if (!type)
		inpDev = GetDefaultMultimediaDevice(eRender);
	else
		inpDev = GetDefaultMultimediaDevice(eCapture);
	if (!inpDev)
		gHandle->ShowError("Input Device not found!");
	//open it for render...
	inpDev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);

	if (!pAudioClient) {
		gHandle->ShowError("Can't access input device!");
	}

	pAudioClient->GetMixFormat(&pwfx);
	switch (pwfx->wFormatTag)
	{
	case WAVE_FORMAT_IEEE_FLOAT:
		pwfx->wFormatTag = WAVE_FORMAT_PCM;
		//pwfx->wBitsPerSample = 16;
		//!!!
		pwfx->nChannels = 2;
		//pwfx->nSamplesPerSec = rate;
		//!!!
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
	{
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
		{
			pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			//pEx->Samples.wValidBitsPerSample = 16;
			//pwfx->wBitsPerSample = 16;
			//!!!
			pEx->Format.nChannels = 2;
			//pEx->Format.nSamplesPerSec = 44100;
			//pwfx->nChannels = 1;
			//pwfx->nSamplesPerSec = rate;
			//!!!
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
		}
	}
	break;
	}

	hnsRequestedDuration = 10000000 / 5;// (rate / (2 * N));
	//ret = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pwfx, &suggest);
	if (!type)
		ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST /*| AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM*/,
			hnsRequestedDuration, 0, pwfx, NULL);
	else
		ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST /*| AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM*/,
			hnsRequestedDuration, 0, pwfx, NULL);
	if (ret)
		gHandle->ShowError("Input device doesn't support and suitable format!");
	//ret = pAudioClient->GetService(IID_IAudioClockAdjustment,
	//	(void**)&pRateClient);
	//ret = pRateClient->SetSampleRate(44100.0);
	pAudioClient->GetMixFormat(&pwfx);
	//rate = pwfx->nSamplesPerSec;
	nChannel = pwfx->nChannels;
	bytePerSample = pwfx->wBitsPerSample / 8;
	blockAlign = pwfx->nBlockAlign;
	pAudioClient->SetEventHandle(hEvent);
	pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&pCaptureClient);
	return pwfx->nSamplesPerSec;
}

void WSAudioIn::release()
{
	stopSampling();
	pCaptureClient->Release();
	pAudioClient->Release();
	inpDev->Release();
}

IMMDevice* WSAudioIn::GetDefaultMultimediaDevice(EDataFlow DevType)
{
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;

	UINT ret = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	if (pEnumerator) {
		pEnumerator->GetDefaultAudioEndpoint(DevType, eConsole, &pDevice);
		pEnumerator->Release();
	}

	return pDevice;
}

//void CALLBACK WSwaveInProc(HWAVEIN hWaveIn, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
DWORD WINAPI WSwaveInProc(LPVOID lpParam)
{
	DWORD dwThreadID;
	UINT32 packetLength = 0;
	UINT32 numFramesAvailable = 0;
	int arrayPos = 0, shift;
	UINT bytesPerChannel = bytePerSample / nChannel;
	BYTE* pData;
	DWORD flags;
	double* waveT = (double*)malloc(NUMSAM * sizeof(double));
	IAudioCaptureClient* pCapCli = (IAudioCaptureClient * ) lpParam;
	//pCaptureClient = lpParam;

	//dwWaitResult = WaitForSingleObject(
	//	hEvent, // event handle
	//	2000);
	//waveD = (double*)malloc(NUMSAM * sizeof(double));
	while (!done) {
		while (!done && WaitForSingleObject(
			hEvent, // event handle
			1000) == WAIT_OBJECT_0)
		{
			// got new buffer....
			pCapCli->GetNextPacketSize(&packetLength);
			while (!done && packetLength != 0) {
				pCapCli->GetBuffer(
					(BYTE**)&pData,
					&numFramesAvailable,
					&flags, NULL, NULL);
				shift = 0;
				for (UINT i = 0; i < numFramesAvailable; i++) {
					INT64 finVal = 0;
					for (int k = 0; k < nChannel; k++) {
						INT32 val = 0;
						//for (UINT j = 0; j < bytesPerChannel; j++) {
						for (int j = bytesPerChannel - 1; j >=0 ; j--) {
							val = (val << 8) + pData[i * blockAlign + k * bytesPerChannel + j];
						}
						finVal += val;
					}
					//double val = pData[4 * i] + pData[4 * i + 1] * 256;
					waveT[arrayPos + i - shift] = (double)(finVal / nChannel);// / (pow(256, bytesPerChannel) - 1);
					if (arrayPos + i == NUMSAM - 1) {
						//buffer full, send to process.
						memcpy(waveD, waveT, NUMSAM * sizeof(double));
						CreateThread(
							NULL,              // default security
							0,                 // default stack size
							mFunction,        // name of the thread function
							waveD,
							0,                 // default startup flags
							&dwThreadID);
						//reset arrayPos
						arrayPos = 0;
						shift = i;
					}
				}
				pCapCli->ReleaseBuffer(numFramesAvailable);
				arrayPos += numFramesAvailable - shift;
				pCapCli->GetNextPacketSize(&packetLength);
			}
		}
	}
	free(waveT);
	return 0;
}