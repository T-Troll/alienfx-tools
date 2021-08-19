#include "AudioIn.h"
#include <iostream>
#include <iomanip>
using namespace std;


void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam);
double* waveDouble; // a pointer to a double array to store the results in
WAVEHDR WaveInHdr;
short int ** waveIn;
int waveIndex; //we maintaine 2 buffers to fill the samples in. this int inicates the current one.
int NUMPTS;
void (*pt2Function)(double*); // a pointer to a function to call to every time the buffer 'waveDouble' is ready



// default constructor
AudioIn::AudioIn( int rate, int N, void(*func)(double*) )
{

	pt2Function=func;
	sampleRate = rate;
	NUMPTS = N;

	waveIn = (short int **)malloc(2*sizeof(short int *));
	waveIn[0] = (short int *)malloc(NUMPTS*sizeof(short int));
	waveIn[1] = (short int *)malloc(NUMPTS*sizeof(short int));

	//prepearing an output buffer:
	waveDouble = (double*)malloc(NUMPTS*sizeof(double));

	// Specify recording parameters
	pFormat.wFormatTag=WAVE_FORMAT_PCM; // simple, uncompressed format
	pFormat.nChannels=1; // 1=mono, 2=stereo
	pFormat.nSamplesPerSec=sampleRate; // 44100
	pFormat.nAvgBytesPerSec=sampleRate*2; // = nSamplesPerSec * n.Channels * wBitsPerSample/8
	pFormat.nBlockAlign=2; // = n.Channels * wBitsPerSample/8
	pFormat.wBitsPerSample=16; // 16 for high quality, 8 for telephone-grade
	pFormat.cbSize=0;

	waveInOpen(&hWaveIn, WAVE_MAPPER,&pFormat, (DWORD_PTR) &waveInProc, 0L, CALLBACK_FUNCTION);

	// Set up and prepare header for input
	waveIndex=0;
	WaveInHdr.lpData = (LPSTR)waveIn[waveIndex];
	WaveInHdr.dwBufferLength = NUMPTS*2;
	WaveInHdr.dwBytesRecorded=0;
	WaveInHdr.dwUser = 0L;
	WaveInHdr.dwFlags = 0L;
	WaveInHdr.dwLoops = 0L;
	waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

	waveIndex = 1 - waveIndex; //changing the NEXT buffer.

	// Insert a wave input buffer
	waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

}


void AudioIn::startSampling()
{
	waveInStart(hWaveIn);
}

void AudioIn::stopSampling()
{
	waveInClose(hWaveIn);
}


void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
{
	if (message == WIM_DATA){
		WaveInHdr.lpData = (LPSTR)waveIn[waveIndex];
		waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
		waveInStart(hWaveIn);

		waveIndex = 1 - waveIndex; //changing the NEXT buffer. this is also the index of the buffer that was just filled.


		for(int i=0; i<NUMPTS; i++){
			//waveDouble[i]=waveIn[waveIndex][i];
			//waveDouble[i]=((double)waveIn[waveIndex][i])/500000;
			waveDouble[i] = ((double)waveIn[waveIndex][i]) / 65535;// 2040000;
		}

		(pt2Function)(waveDouble);
	}

}