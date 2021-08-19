#ifndef DFT_gosu_H
#define DFT_gosu_H

#include "kiss_fftr.h"

class DFT_gosu {

public:
	DFT_gosu(int m,int xscale ,double yscale, int* output);
	~DFT_gosu();
	int* calc(double *x1); 
	void setSampleRate(int rate) { sampleRate = rate; };

protected:
	double *x2;
	int NUMPTS;
	int RECTSNUM;
	int sampleRate = 44100;
	int* spectrum;
	double* blackman;// , * hanning;
	kiss_fft_scalar *padded_in;
	kiss_fft_cpx *padded_out;
	void* kiss_cfg;
	double peak = 0;

};

#endif