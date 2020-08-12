#pragma once
#include "ConfigHandler.h"
#include "../alienfx-cli/LFXUtil.h"

struct UpdateData {
	Colorcode color;
	unsigned devid, lightid;
	ULONGLONG lastUpdate;
};

class FXHelper
{
private:
	//int done, lastLights;
	int pid;
	LFXUtil::LFXUtilC* lfx;
	ConfigHandler* config;
	ULONGLONG lastUpdate;
	std::vector<UpdateData> updates;
public:
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	void StartFX();
	void StopFX();
	int Refresh(UCHAR* img);
	int UpdateLights();
};

