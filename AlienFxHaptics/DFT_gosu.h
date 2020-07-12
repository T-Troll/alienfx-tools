#ifndef DFT_gosu_H
#define DFT_gosu_H

class DFT_gosu {

public:
	DFT_gosu(int m,int xscale ,double yscale, int* output); // default constructor
	void calc(double *x1); 
	void kill();
	void setXscale(int x);
	void setYscale(double y);
	double getCurrentPower();
	double getShortPower();
	double getLongPower();
	int getCurrentAvgFreq();
	int getShortAvgFreq();
	int getLongAvgFreq();
	double getYscale();

protected:
	double *x2;
	double *y2;
	double** cosarg;
	double** sinarg;
	int NUMPTS;
	double y_scale;
	int RECTSNUM;
	int done, stopped;
	int* s_indexes;
	int* spectrum;
	long long int infinity;
	double avg_freq;
	double long_term_avg_freq,short_term_avg_freq;//integral over the avg on sample

	double power;
	double long_term_power,short_term_power;

}; // end class DFT_gosu

#endif