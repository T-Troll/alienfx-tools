#include "WSAudioIn.h"
#include "HapticsDialog.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)  
#endif

DWORD WINAPI WSwaveInProc(LPVOID);
DWORD WINAPI resample(LPVOID);
DWORD WINAPI UpdateUI(LPVOID);

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
const IID IID_IAudioClockAdjustment = __uuidof(IAudioClockAdjustment);

HANDLE hEvent = 0, astopEvent = 0, updateEvent = 0, auiEvent = 0;

WSAudioIn::WSAudioIn(ConfigHaptics* cf, FXHelper *fx)
{
	fxha = fx;
	conf = cf;
	waveD = new double[cf->numpts];
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	dftGG = new DFT_gosu(cf->numpts, cf->numbars);

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if (rate = init(cf->inpType)) {
		dftGG->setSampleRate(rate);
		startSampling();
	}
}

WSAudioIn::~WSAudioIn()
{
	release();
	delete dftGG;
	delete[] waveD;
	CoUninitialize();
}

void WSAudioIn::startSampling()
{
	// creating listener thread...
	if (pAudioClient && !dwHandle) {
		astopEvent = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread( NULL, 0, WSwaveInProc, this, 0, NULL);
		pAudioClient->Start();
	}
}

void WSAudioIn::stopSampling()
{
	if (dwHandle) {
		SetEvent(astopEvent);
		WaitForSingleObject(dwHandle, 6000);
		pAudioClient->Stop();
		ResetEvent(astopEvent);
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
		return 0;
	
	//open it for render...
	inpDev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);

	if (!pAudioClient) return 0;

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

	REFERENCE_TIME hnsRequestedDuration = 10000000 / 5;// (rate / (2 * N));
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
	//nChannel = pwfx->nChannels;
	//blockAlign = pwfx->nBlockAlign;
	//bytePerSample = pwfx->wBitsPerSample / 8;
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

	UINT bytesPerChannel = src->pwfx->wBitsPerSample / 8;// bytePerSample;// / nChannel;
	UINT nChannel = src->pwfx->nChannels;
	UINT blockAlign = src->pwfx->nBlockAlign;
	UINT NUMSAM = src->conf->numpts;

	BYTE* pData;
	DWORD flags, res = 0;
	double* waveT = new double[NUMSAM];
	UINT32 maxLevel = (UINT32) pow(256, bytesPerChannel) - 1;
	IAudioCaptureClient *pCapCli = src->pCaptureClient;

	updateEvent = CreateEvent(NULL, false, false, NULL);
	HANDLE updHandle = CreateThread(NULL, 0, resample, src, 0, NULL);
	HANDLE hArray[2]{astopEvent, hEvent};

	while ((res = WaitForMultipleObjects(2, hArray, false, 500)) != WAIT_OBJECT_0) {
		switch (res) {
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
					ZeroMemory(src->waveD, NUMSAM * sizeof(double));
					//buffer full, send to process.
					SetEvent(updateEvent);
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
						waveT[arrayPos + i - shift] = (double) (finVal / nChannel) / maxLevel / NUMSAM;
						if (arrayPos + i - shift == NUMSAM - 1) {
							//buffer full, send to process.
							memcpy(src->waveD, waveT, NUMSAM * sizeof(double));
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
			ZeroMemory(src->waveD, NUMSAM * sizeof(double));
			//buffer full, send to process.
			SetEvent(updateEvent);
			//reset arrayPos
			arrayPos = 0;
			shift = 0;
			break;
		}
	}
	WaitForSingleObject(updHandle, 3000);
	CloseHandle(updHandle);
	CloseHandle(updateEvent);
	delete[] waveT;
	return 0;
}

DWORD WINAPI resample(LPVOID lpParam)
{
	WSAudioIn *src = (WSAudioIn *) lpParam;
	double* waveDouble = src->waveD;

	HANDLE waitArray[2]{updateEvent, astopEvent};
	DWORD res = 0;

	auiEvent = CreateEvent(NULL, false, false, NULL);
	HANDLE uiHandle = CreateThread(NULL, 0, UpdateUI, src, 0, NULL);

	while ((res = WaitForMultipleObjects(2, waitArray, false, 200)) != WAIT_OBJECT_0 + 1) {
		if (res == WAIT_OBJECT_0) {
			src->freqs = src->dftGG->calc(waveDouble);
			SetEvent(auiEvent);
			//DebugPrint("Haptics light update...\n");
			src->fxha->RefreshHaptics(src->freqs);
		}
	}

	WaitForSingleObject(uiHandle, 3000);
	CloseHandle(uiHandle);
	CloseHandle(auiEvent);
	return 0;
}

DWORD WINAPI UpdateUI(LPVOID lpParam) {
	WSAudioIn *src = (WSAudioIn *) lpParam;
	//HANDLE waitArray[2] = {auiEvent, astopEvent};
	DWORD res = 0;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	while (WaitForSingleObject(astopEvent, 40) == WAIT_TIMEOUT) {
		if (src->conf->dlg && !IsIconic(GetParent(GetParent(src->conf->dlg))) && WaitForSingleObject(auiEvent, 0) == WAIT_OBJECT_0) {
			//DebugPrint("Haptics UI update...\n");
			DrawFreq(src->conf->dlg, src->freqs);
		}
	}
	//while ((res = WaitForMultipleObjects(2, waitArray, false, 200)) != WAIT_OBJECT_0 + 1) {
	//	if (res == WAIT_OBJECT_0 && src->conf->dlg && !IsIconic(GetParent(GetParent(src->conf->dlg)))) {
	//		DebugPrint("Haptics UI update...\n");
	//		DrawFreq(src->conf->dlg, src->freqs);
	//	}
	//}
	return 0;
}