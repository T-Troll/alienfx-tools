#pragma once
#include "ConfigHandler.h"

/*struct UpdateData {
	Colorcode color;
	unsigned devid, lightid;
	ULONGLONG lastUpdate;
};*/

class FXHelper
{
private:
	int* freq;
	int done, stopped, pid;
	ConfigHandler *config;
	//ULONGLONG lastUpdate;
	//UpdateData updates[50];
public:
	FXHelper(int* freqp, ConfigHandler* conf);
	~FXHelper();
	//void Reset();
	int Refresh(int numbars);
	void FadeToBlack();
	int GetPID();
	//int UpdateLights();
};

