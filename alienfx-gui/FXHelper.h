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
#define MODE_LOW	4

class FXHelper
{
private:
	int pid;
	ConfigHandler* config;
	int activeMode = 0;
	int lastTest = -1;
	bool devbusy = false;
	std::vector<int> devList;
	long lCPU=0, lRAM=0, lHDD=0, lGPU=0, lNET=0, lTemp=0;
public:
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	void StartFX();
	void StopFX();
	//void UpdateLight(lightset* map, bool update = true);
	int Refresh();
	int SetMode(int mode);
	std::vector<int> GetDevList();
	void TestLight(int id);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp);
	void SetLight(int id, int mode1, int length1, int speed1, BYTE r, BYTE g, BYTE b,
		int mode2=0, int length2 =0, int speed2 =0, BYTE r2=0, BYTE g2=0, BYTE b2=0);
};

