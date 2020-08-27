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
	int* freq, done, stopped;// , lastLights;
	ConfigHandler* config;
	//ULONGLONG lastUpdate;
	//UpdateData updates[50];
public:
	FXHelper(int* freqp, ConfigHandler* conf);
	~FXHelper();
	void StartFX();
	void StopFX();
	int Refresh(int numbars);
	//int UpdateLights();
};

