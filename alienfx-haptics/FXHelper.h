#pragma once
#include "ConfigHandler.h"
#include "toolkit.h"

class FXHelper: public FXH<ConfigHandler>
{
private:
	int* freq;
	//ConfigHandler *config;
	//std::vector<int> devList;
	//void UpdateColors();
	//std::vector<AlienFX_SDK::Functions*> devs;

public:
	using FXH::FXH;
	//AlienFX_SDK::Mappings afx_dev;
	//AlienFX_SDK::Functions* LocateDev(int pid);
	//void FillDevs();

	//FXHelper(ConfigHandler* conf);
	//~FXHelper();

	void Refresh(int* freq);
	void FadeToBlack();

};

