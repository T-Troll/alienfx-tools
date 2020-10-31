#include <iostream>
#include <iomanip>
#include "DFT_gosu.h" // DFT_gosu class definition
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include<conio.h>
#include <math.h>
using namespace std;

#ifndef PI
#define PI 3.141592653589793238462643383279502884197169399375
#endif

// default constructor
DFT_gosu::DFT_gosu(int m,int xscale ,double yscale, int* output)
{
	NUMPTS = m;
	y_scale = yscale;
	RECTSNUM = xscale;
	done=0;
	stopped = 0;
	spectrum=output;

	x2 = (double*)malloc(NUMPTS * sizeof(double));
	blackman = (double*)malloc(NUMPTS * sizeof(double));
	//hanning = (double*)malloc(NUMPTS * sizeof(double));
	padded_in = (kiss_fft_scalar*)malloc(NUMPTS * sizeof(kiss_fft_scalar));
	padded_out = (kiss_fft_cpx*)malloc(NUMPTS * sizeof(kiss_fft_cpx));
	// Preparing data...
	for (int i = 0; i < NUMPTS; i++) {
		double p = (double)i / double(NUMPTS - 1);
		blackman[i] = (0.42 - 0.5 * cos(2 * PI * p) + 0.8 * cos(4 * PI * p));
		double inv = 1 / (double)NUMPTS;
		//hanning[i] = sqrt(cos((PI * inv) * (i - (double)(NUMPTS - 1) / 2)));
	}
	kiss_cfg = kiss_fftr_alloc(NUMPTS, 0, 0, 0);
} // end DFT_gosu constructor

DFT_gosu::~DFT_gosu()
{
	free(x2);
	free(blackman);
	//free(hanning);
	free(padded_in);
	free(padded_out);
	kiss_fft_free(kiss_cfg);
}

// calculate DFT of the signal x1, and calculate relevant parameters
void DFT_gosu::calc(double *x1)
{
	if (done == 1) {
	//	stopped = 1;
		return;
	}

	for (int n = 0; n < NUMPTS; n++) {
		padded_in[n] = (kiss_fft_scalar) (x1[n] * blackman[n]);
		//x1 += sizeof(double);
	}
	kiss_fftr(kiss_cfg, padded_in, padded_out);

	double minP = MAXINT, maxP = 0;
	double mult = 1.3394/* sqrt3(2)*/, f = 1.0;
	int idx = 2;
	for (int n = 0; n < RECTSNUM; n++) {
		double v = 0;
		for (int m = 0; m < f; m++) {
			//int idx = n * f + m;
			if (idx < NUMPTS/2)
				v = v + sqrt(padded_out[idx+1].r * padded_out[idx+1].r + padded_out[idx+1].i * padded_out[idx+1].i);
			//v += sqrt(padded_out[idx].r * padded_out[idx].r + padded_out[idx].i * padded_out[idx].i);
			idx++;
		}

		x2[n] = v / (int)f * 6 * (n+1);
		f *= mult;
		//mult = n * diver;
		//x2[n] = v / (s_numbers[n] - prev);
		//prev = s_numbers[n];
		if (x2[n] < minP)
			minP = x2[n];
		if (x2[n] > maxP)
			maxP = x2[n];

	}

	if (peak < maxP)
		peak = (peak < maxP - 2 * minP) ? maxP : peak + 2 * minP; // 2* y_scale
	else
		peak = (peak > 2 * minP) ? peak - minP : peak; // y_scale
	peak = (peak > minP) ? peak : minP;

	double coeff = 256.0 / peak;// (peak - minP > 0) ? 256.0 / (peak - minP) : 0.0;

#ifdef _DEBUG
	char buff[2048];
	sprintf_s(buff, 2047, "Peak:%f, Coeff:%f, Min:%f, Max:%f, P(1):%f, P(2):%f\n", peak, coeff, minP, maxP, x2[0], x2[1]);
	OutputDebugString(buff);
#endif

	// Normalize
	for (int n = 0; n < RECTSNUM; n++) {
		spectrum[n] = (int) ((x2[n] - minP) * coeff);
	}

	return;
} // end function calc

/*double DFT_gosu::getCurrentPower(){
	return power;
}
double DFT_gosu::getShortPower(){
	return short_term_power;
}
double DFT_gosu::getLongPower(){
	return long_term_power;
}
int DFT_gosu::getCurrentAvgFreq(){
	return (int)avg_freq;
}

int DFT_gosu::getShortAvgFreq(){
	return (int)short_term_avg_freq;
}

int DFT_gosu::getLongAvgFreq(){
	return (int)long_term_avg_freq;
}*/

void DFT_gosu::kill()
{
//int i;
	//killing dft:
	done=1;
	//while (!stopped)
	//	Sleep(100);
	/*for(i=0;i<NUMPTS;i++){
		free(cosarg[i]);
		free(sinarg[i]);
	}
	free(cosarg);
	free(sinarg);
	  free(y2);
	  free(x3);*/
	  //free(x2);
	  //free(s_indexes);
	  //free(s_numbers);
}

void DFT_gosu::setXscale(int x)
{
	RECTSNUM=x;
}

void DFT_gosu::setYscale(double y)
{
	y_scale=y;
}

double DFT_gosu::getYscale()
{
	return y_scale;
}