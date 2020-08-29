#include "DaramCam.h"

#include <Audioclient.h>

class DCWASAPIAudioCapturer : public DCAudioCapturer
{
public:
	DCWASAPIAudioCapturer ( IMMDevice * pDevice, bool deviceRelease, DWORD streamFlags );
	virtual ~DCWASAPIAudioCapturer ();

public:
	virtual void Begin ();
	virtual void End ();

public:
	virtual unsigned GetChannels () noexcept;
	virtual unsigned GetBitsPerSample () noexcept;
	virtual unsigned GetSamplerate () noexcept;

public:
	virtual void* GetAudioData ( unsigned * bufferLength ) noexcept;

public:
	float GetVolume () noexcept;
	void SetVolume ( float volume ) noexcept;

	bool IsSilentNullBuffer () noexcept { return silentNullBuffer; }
	void SetSilentNullBuffer ( bool silentNullBuffer ) noexcept { this->silentNullBuffer = silentNullBuffer; }

private:
	IAudioClient *pAudioClient;
	IAudioCaptureClient * pCaptureClient;

	ISimpleAudioVolume * pAudioVolume;
	float originalVolume;

	WAVEFORMATEX *pwfx;

	BYTE * byteArray;
	unsigned byteArrayLength;

	UINT32 bufferFrameCount;

	bool silentNullBuffer;

	HANDLE hWakeUp;
};

class DCWASAPILoopbackAudioCapturer : public DCWASAPIAudioCapturer
{
public:
	DCWASAPILoopbackAudioCapturer ();
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DARAMCAM_EXPORTS DCAudioCapturer * DCCreateWASAPIAudioCapturer ( IMMDevice * pDevice )
{
	return new DCWASAPIAudioCapturer ( pDevice, false, 0 );
}
DARAMCAM_EXPORTS DCAudioCapturer * DCCreateWASAPILoopbackAudioCapturer ()
{
	return new DCWASAPILoopbackAudioCapturer ();
}

DARAMCAM_EXPORTS float DCGetVolumeFromWASAPIAudioCapturer ( DCAudioCapturer * capturer )
{
	return dynamic_cast< DCWASAPIAudioCapturer* >( capturer )->GetVolume ();
}
DARAMCAM_EXPORTS void DCSetVolumeToWASAPIAudioCapturer ( DCAudioCapturer * capturer, float volume )
{
	dynamic_cast< DCWASAPIAudioCapturer* >( capturer )->SetVolume ( volume );
}

DARAMCAM_EXPORTS bool DCIsSilentNullBufferFromWASAPIAudioCapturer ( DCAudioCapturer * capturer )
{
	return dynamic_cast< DCWASAPIAudioCapturer* >( capturer )->IsSilentNullBuffer ();
}

DARAMCAM_EXPORTS void DCSetSilentNullBufferToWASAPIAudioCapturer ( DCAudioCapturer * capturer, bool silentNullBuffer )
{
	dynamic_cast< DCWASAPIAudioCapturer* >( capturer )->SetSilentNullBuffer ( silentNullBuffer );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

const CLSID CLSID_MMDeviceEnumerator = __uuidof( MMDeviceEnumerator );
const IID IID_IMMDeviceEnumerator = __uuidof( IMMDeviceEnumerator );
const IID IID_IAudioClient = __uuidof( IAudioClient );
const IID IID_IAudioCaptureClient = __uuidof( IAudioCaptureClient );
const IID IID_ISimpleAudioVolume = __uuidof( ISimpleAudioVolume );

DCWASAPIAudioCapturer::DCWASAPIAudioCapturer ( IMMDevice * pDevice, bool deviceRelease, DWORD streamFlags )
	: byteArray ( nullptr ), silentNullBuffer ( false )
{
	pDevice->Activate ( IID_IAudioClient, CLSCTX_ALL, NULL, ( void** ) &pAudioClient );

	pAudioClient->GetMixFormat ( &pwfx );
	switch ( pwfx->wFormatTag )
	{
	case WAVE_FORMAT_IEEE_FLOAT:
		pwfx->wFormatTag = WAVE_FORMAT_PCM;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
		{
			PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>( pwfx );
			if ( IsEqualGUID ( KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat ) )
			{
				pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
				pEx->Samples.wValidBitsPerSample = 16;
				pwfx->wBitsPerSample = 16;
				pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
				pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
			}
		}
		break;
	}

	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	pAudioClient->Initialize ( AUDCLNT_SHAREMODE_SHARED, streamFlags, hnsRequestedDuration, 0, pwfx, NULL );

	REFERENCE_TIME hnsDefaultDevicePeriod;
	pAudioClient->GetDevicePeriod ( &hnsDefaultDevicePeriod, NULL );
	pAudioClient->GetBufferSize ( &bufferFrameCount );

	HRESULT hr = pAudioClient->GetService ( IID_IAudioCaptureClient, ( void** ) &pCaptureClient );
	pAudioClient->GetService ( IID_ISimpleAudioVolume, ( void** ) &pAudioVolume );
	originalVolume = GetVolume ();

	byteArray = new BYTE [ byteArrayLength = 46000 * 128 ];

	hWakeUp = CreateWaitableTimer ( NULL, FALSE, NULL );

	LARGE_INTEGER liFirstFire;
	liFirstFire.QuadPart = -hnsDefaultDevicePeriod / 2;
	LONG lTimeBetweenFires = ( LONG ) hnsDefaultDevicePeriod / 2 / ( 10 * 1000 );
	BOOL bOK = SetWaitableTimer (
		hWakeUp,
		&liFirstFire,
		lTimeBetweenFires,
		NULL, NULL, FALSE
	);

	if ( deviceRelease )
		pDevice->Release ();
}

DCWASAPIAudioCapturer::~DCWASAPIAudioCapturer ()
{
	CloseHandle ( hWakeUp );

	if ( byteArray )
		delete [] byteArray;

	CoTaskMemFree ( pwfx );

	SetVolume ( originalVolume );
	pAudioVolume->Release ();

	pCaptureClient->Release ();
	pAudioClient->Release ();
}

void DCWASAPIAudioCapturer::Begin ()
{
	pAudioClient->Start ();
}

void DCWASAPIAudioCapturer::End ()
{
	pAudioClient->Stop ();
}

unsigned DCWASAPIAudioCapturer::GetChannels () noexcept { return pwfx->nChannels; }
unsigned DCWASAPIAudioCapturer::GetBitsPerSample () noexcept { return pwfx->wBitsPerSample; }
unsigned DCWASAPIAudioCapturer::GetSamplerate () noexcept { return pwfx->nSamplesPerSec; }

void * DCWASAPIAudioCapturer::GetAudioData ( unsigned * bufferLength ) noexcept
{
	UINT32 packetLength = 0;
	DWORD totalLength = 0;

	HRESULT hr;
	hr = pCaptureClient->GetNextPacketSize ( &packetLength );

	void * retBuffer = byteArray;

	while ( SUCCEEDED ( hr ) && packetLength > 0 )
	{
		BYTE *pData;
		UINT32 nNumFramesToRead;
		DWORD dwFlags;

		if ( FAILED ( hr = pCaptureClient->GetBuffer ( &pData, &nNumFramesToRead, &dwFlags, NULL, NULL ) ) )
		{
			*bufferLength = -1;
			return nullptr;
		}

		if ( AUDCLNT_BUFFERFLAGS_SILENT == dwFlags )
		{
			fprintf ( stderr, "Silent(nNumFramesToRead: %d).\n", nNumFramesToRead );
			if ( silentNullBuffer )
			{
				pCaptureClient->ReleaseBuffer ( nNumFramesToRead );
				*bufferLength = -1;
				return nullptr;
			}
		}
		else if ( AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY == dwFlags )
		{
			fprintf ( stderr, "Discontinuity(nNumFramesToRead: %d).\n", nNumFramesToRead );
		}
		else if ( AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR == dwFlags )
		{
			fprintf ( stderr, "Timestamp Error(nNumFramesToRead: %d).\n", nNumFramesToRead );
		}
		else
		{
			fprintf ( stderr, "Maybe Discontinuity(nNumFramesToRead: %d).\n", nNumFramesToRead );
		}

		if ( 0 == nNumFramesToRead )
		{
			//*bufferLength = 0;
			//pCaptureClient->ReleaseBuffer ( nNumFramesToRead );
			//return nullptr;
		}

		LONG lBytesToWrite = nNumFramesToRead * pwfx->nBlockAlign;
		memcpy ( byteArray + totalLength, pData, lBytesToWrite );
		totalLength += lBytesToWrite;

		if ( FAILED ( hr = pCaptureClient->ReleaseBuffer ( nNumFramesToRead ) ) )
			return nullptr;

		hr = pCaptureClient->GetNextPacketSize ( &packetLength );
	}

	DWORD dwWaitResult = WaitForMultipleObjects ( 1, &hWakeUp, FALSE, INFINITE );

	*bufferLength = totalLength;
	return byteArray;
}

float DCWASAPIAudioCapturer::GetVolume () noexcept
{
	float temp;
	pAudioVolume->GetMasterVolume ( &temp );
	return temp;
}

void DCWASAPIAudioCapturer::SetVolume ( float volume ) noexcept
{
	pAudioVolume->SetMasterVolume ( volume, NULL );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

IMMDevice * _GetDefaultMultimediaDevice ()
{
	IMMDeviceEnumerator *pEnumerator;
	IMMDevice * pDevice;

	CoCreateInstance ( CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, ( void** ) &pEnumerator );
	pEnumerator->GetDefaultAudioEndpoint ( eRender, eConsole, &pDevice );
	pEnumerator->Release ();

	return pDevice;
}

DCWASAPILoopbackAudioCapturer::DCWASAPILoopbackAudioCapturer ()
	: DCWASAPIAudioCapturer ( _GetDefaultMultimediaDevice (), true, AUDCLNT_STREAMFLAGS_LOOPBACK )
{

}