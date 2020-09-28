#pragma once
#include "ConfigHandler.h"

struct UpdateData {
	Colorcode color;
	unsigned devid, lightid;
	ULONGLONG lastUpdate;
};

#define MODE_AC		0
#define MODE_BAT	1
#define MODE_CHARGE	2
#define MODE_LOW	3

class FXHelper
{
private:
	int pid;
	ConfigHandler* config;
	int activeMode = 0;
	int lastTest = -1;
	std::vector<int> devList;
public:
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	void StartFX();
	void StopFX();
	void UpdateLight(lightset* map, bool update = true);
	int Refresh();
	int SetMode(int mode);
	std::vector<int> GetDevList();
	void TestLight(int id);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD);
};

