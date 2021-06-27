#pragma once
#include "ConfigHandler.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

class FXHelper
{
private:
	ConfigHandler* config;
	std::vector<int> devList;
public:
	AlienFX_SDK::Mappings afx_dev;
	std::vector<AlienFX_SDK::Functions*> devs;
	AlienFX_SDK::Functions* LocateDev(int pid);
	void FillDevs();
	void UpdateColors();
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	int Refresh(UCHAR* img);
	void FadeToBlack();
};

