#ifndef DFT_gosu_H
#define DFT_gosu_H

#include "kiss_fftr.h"

class DFT_gosu {

public:
	DFT_gosu(int m,int xscale ,double yscale, int* output); // default constructor
	~DFT_gosu();
	void calc(double *x1); 
	void kill();
	void setXscale(int x);
	void setYscale(double y);
	void setSampleRate(int rate) { sampleRate = rate; };
	double getCurrentPower();
	double getShortPower();
	double getLongPower();
	int getCurrentAvgFreq();
	int getShortAvgFreq();
	int getLongAvgFreq();
	double getYscale();

protected:
	double *x2;
	//double *y2;
	//double* x3;
	//double** cosarg;
	//double** sinarg;
	int NUMPTS;
	double y_scale;
	int RECTSNUM;
	int sampleRate = 44100;
	int done, stopped;
	//int* s_indexes;
	//int* s_numbers;
	int* spectrum;
	//long long int infinity;
	double avg_freq;
	double long_term_avg_freq,short_term_avg_freq;//integral over the avg on sample

	double power;
	double long_term_power,short_term_power;
	//-------------------------------------------------
	double *blackman, *hanning;
	kiss_fft_scalar *padded_in;
	kiss_fft_cpx *padded_out;
	void* kiss_cfg;
	double peak = 0;

}; // end class DFT_gosu

#endif