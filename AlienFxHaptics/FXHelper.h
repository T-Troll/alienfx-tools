#pragma once
#include "ConfigHandler.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

class FXHelper
{
private:
	int* freq;
	int done, stopped;// , pid;
	ConfigHandler *config;
	std::vector<int> devList;
public:
	AlienFX_SDK::Functions afx_dev;
	std::vector<AlienFX_SDK::Functions*> devs;
	AlienFX_SDK::Functions* LocateDev(int pid);
	void FillDevs();
	void UpdateColors();
	FXHelper(int* freqp, ConfigHandler* conf);
	~FXHelper();

	int Refresh(int numbars);
	void FadeToBlack();

};

