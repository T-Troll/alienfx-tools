#include "WSAudioIn.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

DWORD WINAPI WSwaveInProc(LPVOID);
DWORD WINAPI resample(LPVOID);

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
const IID IID_IAudioClockAdjustment = __uuidof(IAudioClockAdjustment);

//HANDLE hEvent = 0;// , astopEvent = 0, updateEvent = 0;

extern FXHelper* fxhl;
extern ConfigHandler* conf;

WSAudioIn::WSAudioIn(/*ConfigHaptics* cf, FXHelper* fx*/)
{
	//conf = cf;
	//fxha = fx;
	waveD = new double[NUMPTS];

	stopEvent = CreateEvent(NULL, true, false, NULL);
	updateEvent = CreateEvent(NULL, false, false, NULL);
	hEvent = CreateEvent(NULL, false, false, NULL);

	dftGG = new DFT_gosu();

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if (rate = init(conf->hap_conf->inpType)) {
		dftGG->setSampleRate(rate);
		startSampling();
	}
}

WSAudioIn::~WSAudioIn()
{
	release();
	CloseHandle(stopEvent);
	CloseHandle(updateEvent);
	CloseHandle(hEvent);
	delete dftGG;
	delete[] waveD;
	CoUninitialize();
}

void WSAudioIn::startSampling()
{
	// creating listener thread...
	if (pAudioClient && !dwHandle) {
		//astopEvent = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread( NULL, 0, WSwaveInProc, this, 0, NULL);
		pAudioClient->Start();
	}
}

void WSAudioIn::stopSampling()
{
	if (dwHandle) {
		SetEvent(stopEvent);
		WaitForSingleObject(dwHandle, 6000);
		pAudioClient->Stop();
		ResetEvent(stopEvent);
		CloseHandle(dwHandle);
		dwHandle = 0;
	}
}

void WSAudioIn::RestartDevice(int type)
{
	release();

	if (rate = init(type)) {
		dftGG->setSampleRate(rate);
		startSampling();
	}
}

int WSAudioIn::init(int type)
{
	DWORD ret = 0;
	// get device
	if (!type)
		inpDev = GetDefaultMultimediaDevice(eRender);
	else
		inpDev = GetDefaultMultimediaDevice(eCapture);
	if (!inpDev) return 0;

	//open it for render...
	inpDev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);

	if (!pAudioClient) return 0;

	pAudioClient->GetMixFormat(&pwfx);

	REFERENCE_TIME hnsRequestedDuration = 10000000 / 5;
	if (!type)
		ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
			hnsRequestedDuration, 0, pwfx, NULL);
	else
		ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
			hnsRequestedDuration, 0, pwfx, NULL);
	if (ret) {
		return 0;
	}

	pAudioClient->GetMixFormat(&pwfx);

	pAudioClient->SetEventHandle(hEvent);
	pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&pCaptureClient);
	return pwfx->nSamplesPerSec;
}

void WSAudioIn::release()
{
	stopSampling();
	if (pCaptureClient)
		pCaptureClient->Release();
	if (pAudioClient)
		pAudioClient->Release();
	if (inpDev)
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

DWORD WINAPI WSwaveInProc(LPVOID lpParam)
{
	WSAudioIn *src = (WSAudioIn *) lpParam;

	UINT32 packetLength = 0;
	UINT32 numFramesAvailable = 0;
	int arrayPos = 0, shift = 0;

	UINT bytesPerChannel = src->pwfx->wBitsPerSample >> 3;
	UINT nChannel = src->pwfx->nChannels;
	UINT blockAlign = src->pwfx->nBlockAlign;

	BYTE* pData;
	DWORD flags, res = 0;
	double* waveT = new double[NUMPTS];

	IAudioCaptureClient *pCapCli = src->pCaptureClient;

	HANDLE updHandle = CreateThread(NULL, 0, resample, src, 0, NULL);
	HANDLE hArray[2]{src->stopEvent, src->hEvent};

	while ((res = WaitForMultipleObjects(2, hArray, false, 500)) != WAIT_OBJECT_0) {
		switch (res) {
		case WAIT_OBJECT_0+1:
			// got new buffer....
			pCapCli->GetNextPacketSize(&packetLength);
			while (packetLength != 0) {
				int ret = pCapCli->GetBuffer(
					(BYTE**) &pData,
					&numFramesAvailable,
					&flags, NULL, NULL);
				shift = 0;
				if (flags == AUDCLNT_BUFFERFLAGS_SILENT) {
					ZeroMemory(src->waveD, NUMPTS * sizeof(double));
					//buffer full, send to process.
					SetEvent(src->updateEvent);
					//reset arrayPos
					arrayPos = 0;
					shift = 0;
				} else {
					for (UINT i = 0; i < numFramesAvailable; i++) {
						INT64 finVal = 0;
						for (UINT k = 0; k < nChannel; k++) {
							INT32 val = 0;
							for (int j = bytesPerChannel - 1; j >= 0; j--) {
								val = (val << 8) + pData[i * blockAlign + k * bytesPerChannel + j];
							}
							finVal += val;
						}
						waveT[arrayPos + i - shift] = (double) (finVal);
						if (arrayPos + i - shift == NUMPTS - 1) {
							//buffer full, send to process.
							memcpy(src->waveD, waveT, NUMPTS * sizeof(double));
							SetEvent(src->updateEvent);
							//reset arrayPos
							arrayPos = 0;
							shift = i - shift;
						}
					}
					arrayPos += numFramesAvailable - shift;
				}
				pCapCli->ReleaseBuffer(numFramesAvailable);
				pCapCli->GetNextPacketSize(&packetLength);
			}
			break;
		case WAIT_TIMEOUT: // no buffer data for 1 sec...
			ZeroMemory(src->waveD, NUMPTS * sizeof(double));
			//buffer full, send to process.
			SetEvent(src->updateEvent);
			//reset arrayPos
			arrayPos = 0;
			shift = 0;
			break;
		}
	}
	WaitForSingleObject(updHandle, 3000);
	CloseHandle(updHandle);
	delete[] waveT;
	return 0;
}

DWORD WINAPI resample(LPVOID lpParam)
{
	WSAudioIn *src = (WSAudioIn *) lpParam;

	HANDLE waitArray[2]{src->stopEvent, src->updateEvent};
	DWORD res = 0;

	while ((res = WaitForMultipleObjects(2, waitArray, false, 500)) != WAIT_OBJECT_0) {
		if (res == WAIT_OBJECT_0 + 1) {
			src->freqs = src->dftGG->calc(src->waveD);
			//DebugPrint("Haptics light update...\n");
			fxhl->RefreshHaptics(src->freqs);
		}
	}

	return 0;
}
