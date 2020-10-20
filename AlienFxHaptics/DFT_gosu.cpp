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
	//infinity=0;
	short_term_avg_freq=0;
	long_term_avg_freq=0;
	short_term_power=0;
	long_term_power=0;
	//--- New settings
	// Arrays...
	x2 = (double*)malloc(NUMPTS * sizeof(double));
	blackman = (double*)malloc(NUMPTS * sizeof(double));
	//hanning = (double*)malloc(NUMPTS * sizeof(double));
	padded_in = (kiss_fft_scalar*)malloc(NUMPTS * sizeof(kiss_fft_scalar));
	padded_out = (kiss_fft_cpx*)malloc(NUMPTS * sizeof(kiss_fft_cpx));
	// Preparing data...
	//int i, k;
	for (int i = 0; i < NUMPTS; i++) {
		double p = (double)i / double(NUMPTS - 1);
		blackman[i] = (0.42 - 0.5 * cos(2 * PI * p) + 0.8 * cos(4 * PI * p));
		double inv = 1 / (double)NUMPTS;
		//hanning[i] = sqrt(cos((PI * inv) * (i - (double)(NUMPTS - 1) / 2)));
	}
	kiss_cfg = kiss_fftr_alloc(NUMPTS, 0, 0, 0);

	//s_indexes = (int*)malloc((NUMPTS/2) * sizeof(int));
	s_numbers = (int*)malloc((RECTSNUM+2) * sizeof(int));
	memset(s_numbers, 0, RECTSNUM * sizeof(int));
	// exp(RECTNUM) = NUMPTS log(NUMPTS) = RECTNUM
	/*double correction = ((double)RECTSNUM+1) / log((NUMPTS/2) - 2);
	for (int i = 2; i < (NUMPTS/2) - 2; i++) {
		//s_indexes[i] = (NUMPTS / 2) * (i+1) / (RECTSNUM+1);
		s_indexes[i] = (int) round(correction * log(i)) - 2;
		s_numbers[s_indexes[i]]++;
	}*/
	double correction = (log(NUMPTS/2 - 1)/log(2))/(RECTSNUM);
	//double sum = 0;
	for (int i = 1 ; i < RECTSNUM+1; i++) {
		double res = pow(2, i * correction);
		//sum = round(res);
		//double delta = sum - s_numbers[i - 1];
		s_numbers[i-1] = (int) round(res);
	}


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

   // ----------------- new --------------------
	for (int n = 0; n < NUMPTS; n++) {
		padded_in[n] = (kiss_fft_scalar) (x1[n] * blackman[n]);
		//x1 += sizeof(double);
	}
	kiss_fftr(kiss_cfg, padded_in, padded_out);

	/*unsigned f = RECTSNUM * 2 / NUMPTS;
	for (unsigned n = 0; n < NUMPTS; n++) {
		unsigned idx = n / f;
		spectrum[n] = (int) sqrt(padded_out[idx].r * padded_out[idx].r + padded_out[idx].i * padded_out[idx].i);
	}*/
	//unsigned f = ((NUMPTS-2)/2) / RECTSNUM;
	//unsigned m, prev = 0;
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

	//double d_scale = y_scale / 10000.0;

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

	for (int n = 0; n < RECTSNUM; n++) {
		spectrum[n] = (int) ((x2[n] - minP) * coeff);
	}
	// Normalize

	return;

	/*  memset(x2, 0, RECTSNUM * sizeof(double));

	  //for(i=0; i<RECTSNUM; i++){
		//  x2[i]=0;
	  //}

	  //int j;
	  //double correction = ((double)RECTSNUM+2) / log(mm-2);
	  //accumulate ajusent frequencies into bars:
	  for (i=2; i < mm-2; i++){
	  //for (i = 4; i < mm - 4; i++) {
		  //j = (int)( ((double)i*(double)RECTSNUM)/(double)mm );
		  // f(0) = 0; f(mm) = RECTSNUM; log. log(mm+1) * x = RECTSNUM; x = RECTSNUM / log(mm+1);
		  //j = round(correction * log(i)) - 2;
		  x2[s_indexes[i]] += x3[i];
		  //x2[j] += x3[i];
	  }

	  //limit to y_scale and normalize the result to range [0,100]:
	  double cl_scale = y_scale, max_scale = 0;;
	  for (i=0; i<RECTSNUM; i++){
		  //x2[i] = x2[i] * log10((i + 1) * 10.0);// pow(i + 1, 2);
		  x2[i] /= s_numbers[i];
		  //x2[i] = x2[i] / 10;
		  if (x2[i] > max_scale)
			  max_scale = x2[i];
		  if (x2[i] > y_scale) {
			  // clipping, let's modify scale
			  if (x2[i] > cl_scale)
				  cl_scale = x2[i];
			  x2[i] = y_scale;
		  }
		  spectrum[i] = (int) ((255.0/y_scale)*x2[i]);
	  } 
	  // upscale if needed
//	  if (max_scale > y_scale)
//		y_scale=y_scale*2;
	  // do we need downscale?
//	  if (max_scale > 10 && y_scale > max_scale * max_scale)
//		  y_scale = y_scale / 2;
	*/

} // end function calc

double DFT_gosu::getCurrentPower(){
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
}

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
	  free(x2);
	  //free(s_indexes);
	  free(s_numbers);
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