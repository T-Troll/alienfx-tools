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
	int pid;
	ConfigHandler* config;
	int activeMode = 0;
	//int lastTest = -1;
	//bool devbusy = false;
	std::vector<int> devList;
	long lCPU=0, lRAM=0, lHDD=0, lGPU=0, lNET=0, lTemp=0, lBatt = 100;
	int HDDTrigger = 0, NetTrigger = 0;
	bool bStage = false;
public:
	FXHelper(ConfigHandler* conf);
	~FXHelper();
	int Refresh(bool force=false);
	int SetMode(int mode);
	std::vector<int> GetDevList();
	void TestLight(int id);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force = false);
	void SetLight(int id, bool power, int mode1, int length1, int speed1, int r, int g, int b,
		int mode2=0, int length2 =0, int speed2 =0, int r2=0, int g2=0, int b2=0);
	void RefreshState();
};

