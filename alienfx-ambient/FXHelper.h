#pragma once
#include "ConfigHandler.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

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
	std::vector<int> devList;
	AlienFX_SDK::Functions* afx_dev;
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	//void StartFX();
	//void StopFX();
	int Refresh(UCHAR* img);
	void FadeToBlack();
	//int GetPID();
	//int UpdateLights();
};

