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
	int lastTest = -1;
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
	void ResetPower();
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force = false);
	void SetLight(int id, bool power, std::vector<AlienFX_SDK::afx_act> actions, bool force); //BYTE mode1, BYTE length1, BYTE speed1, BYTE r, BYTE g, BYTE b,
		//BYTE mode2=0, BYTE length2 =0, BYTE speed2 =0, BYTE r2=0, BYTE g2=0, BYTE b2=0, bool force = false);
	void RefreshState();
};

