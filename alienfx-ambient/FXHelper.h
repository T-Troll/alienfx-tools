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
	int* freq, done, stopped, lastLights;
	LFXUtil::LFXUtilC* lfx;
	ConfigHandler* config;
	ULONGLONG lastUpdate;
	UpdateData updates[50];
public:
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	void StartFX();
	void StopFX();
	int Refresh(UCHAR* img);
	int UpdateLights();
};

