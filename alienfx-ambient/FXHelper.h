#pragma once
#include "ConfigHandler.h"

struct UpdateData {
	Colorcode color;
	unsigned devid, lightid;
	//ULONGLONG lastUpdate;
};

class FXHelper
{
private:
	//int done, lastLights;
	int pid;
	ConfigHandler* config;
	//ULONGLONG lastUpdate;
	//std::vector<UpdateData> updates;
public:
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	//void StartFX();
	//void StopFX();
	int Refresh(UCHAR* img);
	void FadeToBlack();
	//int GetPID();
	//int UpdateLights();
};

