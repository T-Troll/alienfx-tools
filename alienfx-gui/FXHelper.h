#pragma once
#include <queue>
#include "ConfigHandler.h"
#include "toolkit.h"

// Power modes: AC = 0, Battery = 1, Charge = 2, Low Battery = 4
#define MODE_AC		0
#define MODE_BAT	1
#define MODE_CHARGE	2
#define MODE_LOW	4

// Operation flags
#define LIGHTS_SET       0
#define LIGHTS_SET_POWER 1
#define LIGHTS_UPDATE    2

struct LightQuewryElement {
	int did = 0;
	int lid = 0;
	bool force = false;
	bool power = false;
	bool update = false;
	vector<AlienFX_SDK::afx_act> actions;
	lightset* map;
};

class FXHelper: public FXH<ConfigHandler>
{
private:
	int activeMode = -1;
	int lastTest = -1;
	long lCPU = 0, lRAM = 0, lHDD = 0, lGPU = 0, lNET = 0, lTemp = 0, lBatt = 100;
	bool bStage = false;
	HANDLE updateThread = NULL;
public:
	using FXH::FXH;
	void Start();
	void Stop();
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	queue<LightQuewryElement> lightQuery;
	int Refresh(bool force = false);
	bool RefreshOne(lightset* map, bool force = false, bool update = false);
	bool SetMode(int mode);
	void TestLight(int did, int id);
	void ResetPower(int did);
	void UpdateColors(int did = -1);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force = false);
	bool SetLight(int did, int id, bool power, std::vector<AlienFX_SDK::afx_act> actions, lightset* map, bool force = false);
	void RefreshState(bool force = false);
	void RefreshMon();
	void ChangeState(bool newState);
	ConfigHandler* GetConfig() { return config; };
};
