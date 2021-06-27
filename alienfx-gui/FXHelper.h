#pragma once
#include "ConfigHandler.h"

struct UpdateData {
	Colorcode color;
	unsigned devid, lightid;
	ULONGLONG lastUpdate;
};

// Power modes: AC = 0, Battery = 1, Charge = 2, Low Battery = 4
#define MODE_AC		0
#define MODE_BAT	1
#define MODE_CHARGE	2
#define MODE_LOW	4

class FXHelper
{
private:
	ConfigHandler* config;
	int activeMode = -1;
	int lastTest = -1;
	std::vector<int> devList;
	long lCPU = 0, lRAM = 0, lHDD = 0, lGPU = 0, lNET = 0, lTemp = 0, lBatt = 100;
	bool bStage = false;
public:
	AlienFX_SDK::Mappings afx_dev;
	std::vector<AlienFX_SDK::Functions*> devs;
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	AlienFX_SDK::Functions* LocateDev(int pid);
	void FillDevs();
	int Refresh(bool force = false);
	bool RefreshOne(lightset* map, bool force = false, bool update = false);
	bool SetMode(int mode);
	std::vector<int> GetDevList();
	void TestLight(int did, int id);
	void ResetPower(int did);
	void UpdateColors(int did = -1);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force = false);
	bool SetLight(int did, int id, bool power, std::vector<AlienFX_SDK::afx_act> actions, bool force = false);
	void RefreshState(bool force = false);
	void RefreshMon();
};
