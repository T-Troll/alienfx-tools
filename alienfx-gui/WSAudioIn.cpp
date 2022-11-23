#include "WSAudioIn.h"
#include "ConfigHandler.h"
#include "FXHelper.h"
#include <math.h>

#ifndef PI
#define PI 3.141592653589793238462643383279502884197169399375
#endif
#include <tools/kiss_fftr.h>

extern ConfigHandler* conf;
extern FXHelper* fxhl;

DWORD WINAPI WSwaveInProc(LPVOID);
DWORD WINAPI FFTProc(LPVOID);
void resample(LPVOID);

const static CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const static IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const static IID IID_IAudioClient = __uuidof(IAudioClient);
const static IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const static IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
const static IID IID_IAudioClockAdjustment = __uuidof(IAudioClockAdjustment);

WSAudioIn::WSAudioIn()
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// Preparing data...
	for (int i = 0; i < NUMPTS; i++) {
		double p = (double)i / double(NUMPTS - 1);
		blackman[i] = (0.42 - 0.5 * cos(2 * PI * p) + 0.8 * cos(4 * PI * p));
		//double inv = 1 / (double)NUMPTS;
		//hanning[i] = sqrt(cos((PI * inv) * (i - (double)(NUMPTS - 1) / 2)));
	}

	stopEvent = CreateEvent(NULL, true, false, NULL);
	hEvent = CreateEvent(NULL, false, false, NULL);
	cEvent = CreateEvent(NULL, false, false, NULL);

	init(conf->hap_inpType);
}

WSAudioIn::~WSAudioIn()
{
	release();
	CloseHandle(stopEvent);
	CloseHandle(hEvent);
	CloseHandle(cEvent);
	CoUninitialize();
}

void WSAudioIn::startSampling()
{
	// creating listener thread...
	if (pAudioClient && !dwHandle) {
		lightUpdate = new ThreadHelper(resample, this, 100);
		fftHandle = CreateThread(NULL, 0, FFTProc, this, 0, NULL);
		dwHandle = CreateThread( NULL, 0, WSwaveInProc, this, 0, NULL);
		pAudioClient->Start();
	}
}

void WSAudioIn::stopSampling()
{
	if (dwHandle) {
		delete lightUpdate;
		SetEvent(stopEvent);
		WaitForSingleObject(dwHandle, 6000);
		WaitForSingleObject(fftHandle, 6000);
		pAudioClient->Stop();
		ResetEvent(stopEvent);
		CloseHandle(dwHandle);
		CloseHandle(fftHandle);
		dwHandle = 0;
	}
}

void WSAudioIn::RestartDevice(int type)
{
	release();
	init(type);
}

void WSAudioIn::init(int type)
{
	static const REFERENCE_TIME hnsRequestedDuration = 500000; // 50 ms buffer
	DWORD ret = 0;
	// get device
	if ((inpDev = GetDefaultMultimediaDevice(type ? eCapture : eRender)) &&
		inpDev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient) == S_OK) {

		pAudioClient->GetMixFormat(&pwfx);

		if (pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			type ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM :
			AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
			hnsRequestedDuration, 0, pwfx, NULL) == S_OK) {

			pAudioClient->GetMixFormat(&pwfx);

			pAudioClient->SetEventHandle(hEvent);
			pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);

			//sampleRate = pwfx->nSamplesPerSec;
			startSampling();
		}
	}
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
	IMMDeviceEnumerator* pEnumerator;
	IMMDevice* pDevice = NULL;

	CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	if (pEnumerator) {
		pEnumerator->GetDefaultAudioEndpoint(DevType, eConsole, &pDevice);
		pEnumerator->Release();
	}

	return pDevice;
}

DWORD WINAPI WSwaveInProc(LPVOID lpParam)
{
	WSAudioIn *src = (WSAudioIn *) lpParam;

	UINT32 packetLength;
	UINT32 numFramesAvailable;
	int arrayPos = 0, shift;

	UINT bytesPerChannel = src->pwfx->wBitsPerSample >> 3;

	BYTE* pData;
	DWORD flags, res = 0;
	double* waveT = src->waveD;

	HANDLE hArray[2]{src->stopEvent, src->hEvent};

	while ((res = WaitForMultipleObjects(2, hArray, false, 200)) != WAIT_OBJECT_0) {
		switch (res) {
		case WAIT_OBJECT_0+1:
			// got new buffer....
			src->pCaptureClient->GetNextPacketSize(&packetLength);
			while (packetLength != 0) {
				src->pCaptureClient->GetBuffer((BYTE**) &pData, &numFramesAvailable, &flags, NULL, NULL);
				shift = 0;
				if (flags == AUDCLNT_BUFFERFLAGS_SILENT) {
					if (!src->clearBuffer) {
						ZeroMemory(src->freqs, NUMBARS * sizeof(int));
						src->clearBuffer = true;
						src->needUpdate = true;
					}
					arrayPos = 0;
				} else {
					src->clearBuffer = false;
					for (UINT i = 0; i < numFramesAvailable; i++) {
						INT64 finVal = 0;
						for (UINT k = 0; k < src->pwfx->nChannels; k++) {
							INT32 val = 0;
							for (int j = bytesPerChannel - 1; j >= 0; j--) {
								val = (val << 8) + pData[i * src->pwfx->nBlockAlign + k * bytesPerChannel + j];
							}
							finVal += val;
						}
						waveT[arrayPos + i - shift] = (double) (finVal);
						if (arrayPos + i - shift == NUMPTS - 1) {
							//buffer full, send to process.
							SetEvent(src->cEvent);
							arrayPos = 0;
							shift = i - shift;
						}
					}
					arrayPos += numFramesAvailable - shift;
				}
				src->pCaptureClient->ReleaseBuffer(numFramesAvailable);
				src->pCaptureClient->GetNextPacketSize(&packetLength);
			}
			break;
		case WAIT_TIMEOUT: // no buffer data for 200 ms...
			if (!src->clearBuffer) {
				ZeroMemory(src->freqs, NUMBARS * sizeof(int));
				src->clearBuffer = true;
				src->needUpdate = true;
			}
			arrayPos = 0;
			break;
		}
	}
	return 0;
}

void resample(LPVOID lpParam)
{
	WSAudioIn *src = (WSAudioIn *) lpParam;

	if (conf->lightsNoDelay && src->needUpdate) {
		src->needUpdate = false;
		fxhl->RefreshHaptics(src->freqs);
	}
}

DWORD WINAPI FFTProc(LPVOID lpParam)
{
	WSAudioIn* src = (WSAudioIn*)lpParam;
	HANDLE hArray[2]{ src->stopEvent, src->cEvent };

	// Preparing FFT...
	void* kiss_cfg = kiss_fftr_alloc(NUMPTS, 0, 0, 0);
	kiss_fft_scalar* padded_in = (kiss_fft_scalar*)malloc(NUMPTS * sizeof(kiss_fft_scalar));
	kiss_fft_cpx* padded_out = (kiss_fft_cpx*)malloc(NUMPTS * sizeof(kiss_fft_cpx));
	double x2[NUMPTS];
	double peak = 0;

	DWORD res;

	while ((res = WaitForMultipleObjects(2, hArray, false, 200)) != WAIT_OBJECT_0) {
		if (res != WAIT_TIMEOUT) {
			for (int n = 0; n < NUMPTS; n++) {
				padded_in[n] = (kiss_fft_scalar)(src->waveD[n] * src->blackman[n]);
			}

			kiss_fftr(kiss_cfg, padded_in, padded_out);

			double minP = INT_MAX, maxP = 0;
			double mult = 1.3394/* sqrt3(2)*/, f = 1.0;
			int idx = 2;
			for (int n = 0; n < NUMBARS; n++) {
				double v = 0;
				for (int m = 0; m < f; m++) {
					if (idx < NUMPTS / 2)
						v += sqrt(padded_out[idx + 1].r * padded_out[idx + 1].r + padded_out[idx + 1].i * padded_out[idx + 1].i);
					idx++;
				}

				x2[n] = v / (int)f * 6 * (n + 1);
				f *= mult;

				minP = min(x2[n], minP);
				maxP = max(x2[n], maxP);
			}

			if (peak > maxP)
				peak = (maxP < peak - peak / 16) ? peak - peak / 256 : peak;
			else
				peak = maxP;
			peak = max(peak, minP);

			double coeff = peak > 0.00001 ? 255.0 / peak : 0.0;

			//#ifdef _DEBUG
				//char buff[2048];
				//sprintf_s(buff, 2047, "Peak:%f, Coeff:%f, Min:%f, Max:%f, P(1):%f, P(2):%f\n", peak, coeff, minP, maxP, x2[0], x2[1]);
				//OutputDebugString(buff);
			//#endif

			// Normalize
			for (int n = 0; n < NUMBARS; n++) {
				src->freqs[n] = (int)(x2[n] * coeff);
			}
			src->needUpdate = true;
		}
	}
	free(padded_in);
	free(padded_out);
	kiss_fft_free(kiss_cfg);
	return 0;
}