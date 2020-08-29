#include "DaramCam.h"

class DCWaveAudioGenerator : public DCAudioGenerator
{
	friend DWORD WINAPI WAVAG_Progress ( LPVOID vg );
public:
	DCWaveAudioGenerator ();

public:
	virtual void Begin ( IStream * stream, DCAudioCapturer * capturer );
	virtual void End ();

private:
	IStream * stream;
	DCAudioCapturer * capturer;

	bool threadRunning;
	HANDLE threadHandle;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DARAMCAM_EXPORTS DCAudioGenerator * DCCreateWaveAudioGenerator ()
{
	return new DCWaveAudioGenerator ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DCWaveAudioGenerator::DCWaveAudioGenerator ()
	: stream ( nullptr ), capturer ( nullptr )
{
}

DWORD WINAPI WAVAG_Progress ( LPVOID vg )
{
	DCWaveAudioGenerator * audioGen = ( DCWaveAudioGenerator* ) vg;

	audioGen->capturer->Begin ();

	HRESULT hr;

	unsigned total = 0;
	while ( audioGen->threadRunning )
	{
		unsigned bufferLength;
		void * data = audioGen->capturer->GetAudioData ( &bufferLength );
		if ( data == nullptr || bufferLength == 0 )
			continue;
		audioGen->stream->Write ( data, bufferLength, nullptr );
		total += bufferLength;
	}

	audioGen->capturer->End ();

	ULARGE_INTEGER temp;

	LARGE_INTEGER riffSeeker = { 0, };
	riffSeeker.QuadPart = 4;
	hr = audioGen->stream->Seek ( riffSeeker, STREAM_SEEK_SET, &temp );
	int totalFileSize = total + 44;
	hr = audioGen->stream->Write ( &totalFileSize, 4, nullptr );

	LARGE_INTEGER dataSeeker = { 0, };
	dataSeeker.QuadPart = 40;
	hr = audioGen->stream->Seek ( dataSeeker, STREAM_SEEK_SET, &temp );
	hr = audioGen->stream->Write ( &total, 4, nullptr );

	LARGE_INTEGER endPointSeeker = { 0, };
	endPointSeeker.QuadPart = 0;
	hr = audioGen->stream->Seek ( endPointSeeker, STREAM_SEEK_END, nullptr );

	hr = audioGen->stream->Commit ( STGC_DEFAULT );

	return 0;
}

void DCWaveAudioGenerator::Begin ( IStream * _stream, DCAudioCapturer * _capturer )
{
	stream = _stream;
	capturer = _capturer;

	stream->Write ( "RIFF", 4, nullptr );
	LARGE_INTEGER riffSeeker = { 0, };
	riffSeeker.QuadPart = 4;
	stream->Seek ( riffSeeker, STREAM_SEEK_CUR, nullptr );
	stream->Write ( "WAVE", 4, nullptr );

	int fmtSize = 16;
	short audioType = WAVE_FORMAT_PCM;
	short audioChannels = capturer->GetChannels ();
	int sampleRate = capturer->GetSamplerate ();
	short blockAlign = capturer->GetBitsPerSample () / 8;
	short bitsPerSample = capturer->GetBitsPerSample ();
	int byteRate = audioChannels * bitsPerSample * sampleRate / 8;

	stream->Write ( "fmt ", 4, nullptr );
	stream->Write ( &fmtSize, 4, nullptr );
	stream->Write ( &audioType, 2, nullptr );
	stream->Write ( &audioChannels, 2, nullptr );
	stream->Write ( &sampleRate, 4, nullptr );
	stream->Write ( &byteRate, 4, nullptr );
	stream->Write ( &blockAlign, 2, nullptr );
	stream->Write ( &bitsPerSample, 2, nullptr );

	stream->Write ( "data", 4, nullptr );
	LARGE_INTEGER dataSeeker = { 0, };
	riffSeeker.QuadPart = 4;
	stream->Seek ( dataSeeker, STREAM_SEEK_CUR, nullptr );

	threadRunning = true;
	threadHandle = CreateThread ( nullptr, 0, WAVAG_Progress, this, 0, 0 );
}

void DCWaveAudioGenerator::End ()
{
	threadRunning = false;
	WaitForSingleObject ( threadHandle, INFINITE );
}
