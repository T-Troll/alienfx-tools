#include "WSAudioIn.h"
#include "Graphics.h"
#include "FXHelper.h"
#include "DFT_gosu.h"

DWORD WINAPI WSwaveInProc(LPVOID);

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
const IID IID_IAudioClockAdjustment = __uuidof(IAudioClockAdjustment);

REFERENCE_TIME hnsRequestedDuration = 0;

int NUMSAM;
double* waveD;
int nChannel;
int bytePerSample;
int blockAlign;

HANDLE hEvent = 0, stopEvent = 0, updateEvent = 0, dwHandle = 0;

DWORD WINAPI resample(LPVOID lpParam);

HWND hDlg;
FXHelper* fxh;
DFT_gosu* dftGG;

WSAudioIn::WSAudioIn(int N, int type, void* gr, void* fx, void* dft)
{
	NUMSAM = N;
	fxh = (FXHelper*) fx;
	dftGG = (DFT_gosu*) dft;
	gHandle = (Graphics*) gr;
	waveD = new double[NUMSAM];
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hDlg = ((Graphics*)gHandle)->GetDlg();
	((Graphics*)gHandle)->SetAudioObject(this);
	rate = init(type);
	dftGG->setSampleRate(rate);
}

WSAudioIn::~WSAudioIn()
{
	release();
	delete[] waveD;
}

void WSAudioIn::startSampling()
{
	DWORD dwThreadID;
	// creating listener thread...
	if (pAudioClient && !dwHandle) {
		stopEvent = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread( NULL, 0, WSwaveInProc, pCaptureClient, 0, &dwThreadID);
		pAudioClient->Start();
	}
}

void WSAudioIn::stopSampling()
{
	if (dwHandle) {
		SetEvent(stopEvent);
		//SetEvent(hEvent);
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
	// get deveice
	if (!type)
		inpDev = GetDefaultMultimediaDevice(eRender);
	else
		inpDev = GetDefaultMultimediaDevice(eCapture);
	if (!inpDev)
		((Graphics*)gHandle)->ShowError("Input Device not found!");
	//open it for render...
	inpDev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);

	if (!pAudioClient) {
		((Graphics*)gHandle)->ShowError("Can't access input device!");
	}

	pAudioClient->GetMixFormat(&pwfx);

	/*switch (pwfx->wFormatTag)
	{
	case WAVE_FORMAT_IEEE_FLOAT:
		pwfx->wFormatTag = WAVE_FORMAT_PCM;
		//pwfx->cbSize = 0;
		//pwfx->wBitsPerSample = 16;
		//!!!
		//pwfx->nChannels = 2;
		//pwfx->nSamplesPerSec = 44100;// rate;
		//!!!
		//pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		//pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
	{
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
		{
			pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			//pEx->Format.cbSize = 0;
			//pEx->Samples.wValidBitsPerSample = 16;
			//pwfx->wBitsPerSample = 16;
			//!!!
			//pEx->Format.nChannels = 2;
			//pEx->Format.nSamplesPerSec = 44100;
			//pwfx->nChannels = 1;
			//pwfx->nSamplesPerSec = rate;
			//!!!
			//pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			//pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
			//pEx->Format.nBlockAlign = 2 * pwfx->wBitsPerSample / 8;
			//pEx->Format.nAvgBytesPerSec = pEx->Format.nBlockAlign * pwfx->nSamplesPerSec;
		}
	}
	break;
	}*/

	hnsRequestedDuration = 10000000 / 5;// (rate / (2 * N));
	//ret = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pwfx, &suggest);
	if (!type)
		ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
			hnsRequestedDuration, 0, pwfx, NULL);
	else
		ret = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
			hnsRequestedDuration, 0, pwfx, NULL);
	if (ret) {
		((Graphics*)gHandle)->ShowError("Input device doesn't support any suitable format!");
		return 0;
	}
	//ret = pAudioClient->GetService(IID_IAudioClockAdjustment,
	//	(void**)&pRateClient);
	//ret = pRateClient->SetSampleRate(44100.0);
	pAudioClient->GetMixFormat(&pwfx);
	//PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
	//rate = pwfx->nSamplesPerSec;
	//if (pwfx->nChannels > 6)
	//	nChannel = 2; // pwfx->nChannels; - Realtek bugfix
	//else
	nChannel = pwfx->nChannels;
	blockAlign = pwfx->nBlockAlign;
	bytePerSample = pwfx->wBitsPerSample / 8;
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

DWORD WINAPI WSwaveInProc(LPVOID lpParam)
{
	DWORD dwThreadID;
	HANDLE updHandle = 0;
	UINT32 packetLength = 0;
	UINT32 numFramesAvailable = 0;
	int arrayPos = 0, shift = 0;
	UINT bytesPerChannel = bytePerSample;// / nChannel;
	BYTE* pData;
	DWORD flags;
	double* waveT = new double[NUMSAM];
	UINT32 maxLevel = (UINT32) pow(256, bytesPerChannel) - 1;
	IAudioCaptureClient* pCapCli = (IAudioCaptureClient * ) lpParam;
	int ret;

	updateEvent = CreateEvent(NULL, true, false, NULL);
	updHandle = CreateThread(NULL, 0, resample, waveD, 0, &dwThreadID);

	HANDLE hArray[2] = {stopEvent, hEvent};

	while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT) {
		switch (ret = WaitForMultipleObjects(2, hArray, false, 500))
		{
		case WAIT_OBJECT_0+1:
			// got new buffer....
			// = 0;
			pCapCli->GetNextPacketSize(&packetLength);
			while (packetLength != 0) {
				int ret = pCapCli->GetBuffer(
					(BYTE**) &pData,
					&numFramesAvailable,
					&flags, NULL, NULL);
				shift = 0;
				if (flags == AUDCLNT_BUFFERFLAGS_SILENT) {
					FillMemory(waveD, NUMSAM * sizeof(double), 0);
					//buffer full, send to process.
					SetEvent(updateEvent);
					//reset arrayPos
					arrayPos = 0;
					shift = 0;
				} else {
					for (UINT i = 0; i < numFramesAvailable; i++) {
						INT64 finVal = 0;
						for (int k = 0; k < nChannel; k++) {
							INT32 val = 0;
							for (int j = bytesPerChannel - 1; j >= 0; j--) {
								val = (val << 8) + pData[i * blockAlign + k * bytesPerChannel + j];
							}
							finVal += val;
						}
						waveT[arrayPos + i - shift] = (double) (finVal / nChannel) / maxLevel / NUMSAM;
						if (arrayPos + i - shift == NUMSAM - 1) {
							//buffer full, send to process.
							memcpy(waveD, waveT, NUMSAM * sizeof(double));
							SetEvent(updateEvent);
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
			FillMemory(waveD, NUMSAM * sizeof(double), 0);
			//buffer full, send to process.
			SetEvent(updateEvent);
			//reset arrayPos
			arrayPos = 0;
			shift = 0;
			break;
		}
	}
	WaitForSingleObject(updHandle, 6000);
	CloseHandle(updHandle);
	delete[] waveT;
	return 0;
}

DWORD WINAPI resample(LPVOID lpParam)
{
	double* waveDouble = (double*)lpParam;

	HANDLE waitArray[2] = {updateEvent, stopEvent};

	ULONGLONG lastTick = 0, currentTick  = 0;

	int* freqs = NULL;

	while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT) {
		if (WaitForMultipleObjects(2, waitArray, false, 100) == WAIT_OBJECT_0) {
			freqs = dftGG->calc(waveDouble);
			fxh->Refresh(freqs);
			currentTick = GetTickCount64();
			if (!IsIconic(hDlg) && currentTick - lastTick > 20) {
				DrawFreq(hDlg, freqs);
				lastTick = currentTick;
			}
		}
	}

	return 0;
}