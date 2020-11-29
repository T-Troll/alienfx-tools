#pragma once
#include "ConfigHandler.h"

class FXHelper
{
private:
	int* freq;
	int done, stopped, pid;
	ConfigHandler *config;

public:
	FXHelper(int* freqp, ConfigHandler* conf);
	~FXHelper();

	int Refresh(int numbars);
	void FadeToBlack();
	int GetPID();

};

