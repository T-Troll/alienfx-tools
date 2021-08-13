#pragma once
#include <queue>
#include "ConfigHandler.h"
#include "toolkit.h"
#include <mutex>

// Power modes: AC = 0, Battery = 1, Charge = 2, Low Battery = 4
#define MODE_AC		0
#define MODE_BAT	1
#define MODE_CHARGE	2
#define MODE_LOW	4

// Operation flags
//#define LIGHTS_SET       0
//#define LIGHTS_SET_POWER 1
//#define LIGHTS_UPDATE    2

struct LightQueryElement {
	int did = 0;
	int lid = 0;
	DWORD flags = 0;
	bool update = false;
	byte actsize;
	AlienFX_SDK::afx_act actions[10];
};

struct deviceQuery {
	int devID = 0;
	vector<pair<int, vector<AlienFX_SDK::afx_act>>> dev_query;
};

class FXHelper: public FXH<ConfigHandler>
{
private:
	int activeMode = -1;
	//int lastTest = -1;
	long lCPU = 0, lRAM = 0, lHDD = 0, lGPU = 0, lNET = 0, lTemp = 0, lBatt = 100;
	bool blinkStage = false;
	HANDLE updateThread = NULL;
public:
	using FXH::FXH;
	void Start();
	void Stop();
	bool unblockUpdates = true;
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	deque<LightQueryElement> lightQuery;
	mutex modifyQuery;
	int Refresh(bool force = false);
	bool RefreshOne(lightset* map, bool force = false, bool update = false);
	bool SetMode(int mode);
	void TestLight(int did, int id);
	void ResetPower(int did);
	void UpdateColors(int did = -1, bool force = false);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force = false);
	bool SetLight(int did, int id, std::vector<AlienFX_SDK::afx_act> actions, bool force = false);
	void RefreshState(bool force = false);
	void RefreshMon();
	void Flush();
	void ChangeState();
	void UpdateGlobalEffect(AlienFX_SDK::Functions* dev = NULL);
	void UnblockUpdates(bool newState);
	ConfigHandler* GetConfig() { return config; };
};
