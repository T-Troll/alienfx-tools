#pragma once
#include "ConfigHandler.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

class FXHelper
{
private:
	int* freq;
	ConfigHandler *config;
	std::vector<int> devList;
	void UpdateColors();
	std::vector<AlienFX_SDK::Functions*> devs;

public:
	AlienFX_SDK::Mappings afx_dev;
	AlienFX_SDK::Functions* LocateDev(int pid);
	void FillDevs();

	FXHelper(ConfigHandler* conf);
	~FXHelper();

	void Refresh(int* freq);
	void FadeToBlack();

};

