#pragma once
#include "ConfigHandler.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

class FXHelper
{
private:
	int* freq;
	int done, stopped, pid;
	ConfigHandler *config;

public:
	AlienFX_SDK::Functions* afx_dev;
	FXHelper(int* freqp, ConfigHandler* conf);
	~FXHelper();

	int Refresh(int numbars);
	void FadeToBlack();
	int GetPID();

};

