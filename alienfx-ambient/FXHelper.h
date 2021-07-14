#pragma once
#include "ConfigHandler.h"
#include "toolkit.h"

class FXHelper: public FXH<ConfigHandler>
{
private:
	//ConfigHandler* config;
	//std::vector<int> devList;
public:
	using FXH::FXH;
	//AlienFX_SDK::Mappings afx_dev;
	//std::vector<AlienFX_SDK::Functions*> devs;
	//AlienFX_SDK::Functions* LocateDev(int pid);
	//void FillDevs();
	//void UpdateColors();
	//FXHelper(ConfigHandler* conf);
	//~FXHelper();
	int Refresh(UCHAR* img);
	void FadeToBlack();
};

