#include "DFT_gosu.h"
#include <limits.h>
#include <math.h>

#ifndef PI
#define PI 3.141592653589793238462643383279502884197169399375
#endif

// default constructor
DFT_gosu::DFT_gosu(int m,int xscale)
{
	NUMPTS = m;
	//y_scale = yscale;
	RECTSNUM = xscale;
	spectrum= new int[xscale]{0};;

	x2 = new double[NUMPTS];// (double*) malloc(NUMPTS * sizeof(double));
	blackman = new double[NUMPTS];// (double*) malloc(NUMPTS * sizeof(double));
	//hanning = (double*)malloc(NUMPTS * sizeof(double));
	kiss_cfg = kiss_fftr_alloc(NUMPTS, 0, 0, 0);
	padded_in = (kiss_fft_scalar*) malloc(NUMPTS * sizeof(kiss_fft_scalar));
	padded_out = (kiss_fft_cpx*)malloc(NUMPTS * sizeof(kiss_fft_cpx));
	// Preparing data...
	for (int i = 0; i < NUMPTS; i++) {
		double p = (double)i / double(NUMPTS - 1);
		blackman[i] = (0.42 - 0.5 * cos(2 * PI * p) + 0.8 * cos(4 * PI * p));
		//double inv = 1 / (double)NUMPTS;
		//hanning[i] = sqrt(cos((PI * inv) * (i - (double)(NUMPTS - 1) / 2)));
	}
}

DFT_gosu::~DFT_gosu()
{
	delete[] x2;
	delete[] blackman;
	delete[] spectrum;
	//free(hanning);
	free(padded_in);
	free(padded_out);
	kiss_fft_free(kiss_cfg);
}

// calculate DFT of the signal x1, and calculate relevant parameters
int* DFT_gosu::calc(double *x1)
{

	for (int n = 0; n < NUMPTS; n++) {
		padded_in[n] = (kiss_fft_scalar) (x1[n] * blackman[n]);
	}

	kiss_fftr(kiss_cfg, padded_in, padded_out);

	double minP = INT_MAX, maxP = 0;
	double mult = 1.3394/* sqrt3(2)*/, f = 1.0;
	int idx = 2;
	for (int n = 0; n < RECTSNUM; n++) {
		double v = 0;
		for (int m = 0; m < f; m++) {
			if (idx < NUMPTS/2)
				v+= sqrt(padded_out[idx+1].r * padded_out[idx+1].r + padded_out[idx+1].i * padded_out[idx+1].i);
			idx++;
		}

		x2[n] = v / (int)f * 6 * (n+1);
		f *= mult;

		if (x2[n] < minP)
			minP = x2[n];
		if (x2[n] > maxP)
			maxP = x2[n];

	}

	if (peak > maxP)
		peak = (maxP < peak - peak/16) ? peak - peak/256 : peak;
	else
		peak = maxP;
	peak = (peak < minP) ? minP : peak;

	double coeff = peak > 0.00001 ? 255.0 / peak : 0.0;

//#ifdef _DEBUG
	//char buff[2048];
	//sprintf_s(buff, 2047, "Peak:%f, Coeff:%f, Min:%f, Max:%f, P(1):%f, P(2):%f\n", peak, coeff, minP, maxP, x2[0], x2[1]);
	//OutputDebugString(buff);
//#endif

	// Normalize
	for (int n = 0; n < RECTSNUM; n++) {
		spectrum[n] = (int)(x2[n] * coeff);
	}

	return spectrum;
} // end function calc
