#pragma once
#include <queue>
#include "ConfigHandler.h"
#include "Alienfan-sdk.h"
#include <mutex>

// Power modes: AC = 0, Battery = 1, Charge = 2, Low Battery = 4
#define MODE_AC		0
#define MODE_BAT	1
#define MODE_CHARGE	2
#define MODE_LOW	4

struct EventData {
	byte CPU = 0, RAM = 0, HDD = 0, GPU = 0, Temp = 0, Batt = 0, KBD = 0, NET = 0, PWR = 1;
	short Fan = 0;
};

struct LightQueryElement {
	int did = 0;
	int lid = 0;
	DWORD flags = 0;
	bool update = false;
	byte actsize;
	AlienFX_SDK::afx_act actions[10];
};

struct deviceQuery {
	AlienFX_SDK::afx_device* dev;
	vector<AlienFX_SDK::act_block> dev_query;
};

class FXHelper
{
private:

	bool blinkStage = false;
	int oldtest = -1;

	void SetGaugeLight(DWORD id, int x, int max, WORD flags, vector<AlienFX_SDK::afx_act> actions, double power = 0, bool force = false);
	void SetGroupLight(groupset* grp, vector<AlienFX_SDK::afx_act> actions, double power = -1.0, bool force = false);
	void SetGridLight(groupset* grp, zonemap* zone, AlienFX_SDK::lightgrid* grid, int x, int y, AlienFX_SDK::Colorcode fin, vector<DWORD>* setLights);
	void SetGaugeGrid(groupset* grp, zonemap* zone, AlienFX_SDK::lightgrid* grid, int phase, int dist, AlienFX_SDK::Colorcode fin, vector<DWORD>* setLights);
	void SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force = false);
	void QueryUpdate(int did = -1, bool force = false);

public:
	HANDLE updateThread = NULL;
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	//HANDLE queryEmpty = NULL;
	deque<LightQueryElement> lightQuery;
	mutex modifyQuery;
	//bool unblockUpdates = true;
	//bool updateLock = false;
	int activePowerMode = -1;
	EventData eData, maxData;
	int numActiveDevs = 0;

	//FXHelper();
	~FXHelper();
	//AlienFX_SDK::afx_device *LocateDev(int pid);
	size_t FillAllDevs(AlienFan_SDK::Control* acc);
	void Start();
	void Stop();
	void Refresh(int force = 0);
	bool RefreshOne(groupset* map, int force = 0, bool update = true);
	bool SetPowerMode(int mode);
	void TestLight(AlienFX_SDK::afx_device* dev, int id, bool force = false, bool wp=false);
	void ResetPower(AlienFX_SDK::afx_device* dev);
	void SetCounterColor(EventData *data, bool force = false);
	void SetGridEffect(groupset* grp);
	void RefreshMon();
	void RefreshAmbient(UCHAR *img);
	void RefreshHaptics(int *freq);
	//void Flush();
	void ChangeState();
	void UpdateGlobalEffect(AlienFX_SDK::Functions* dev = NULL);
	//void UnblockUpdates(bool newState);

	//ConfigHandler* GetConfig() { return config; };
};
