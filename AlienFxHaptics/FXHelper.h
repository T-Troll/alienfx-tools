#pragma once
#include <LFXUtil.h>
#include "ConfigHandler.h"

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
	FXHelper(int* freqp, LFXUtil::LFXUtilC* lfxUtil, ConfigHandler* conf);
	~FXHelper();
	void StartFX();
	void StopFX();
	int Refresh(int numbars);
	int UpdateLights();
};

