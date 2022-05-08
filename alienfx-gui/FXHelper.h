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
	byte CPU = 0, RAM = 0, HDD = 0, GPU = 0, Temp = 0, Batt = 0, KBD = 0;
	long NET = 1;
	short PWR = 100, Fan = 0;
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
	int devID = 0;
	vector<AlienFX_SDK::act_block> dev_query;
};

class FXHelper
{
private:

	bool blinkStage = false;
	HANDLE updateThread = NULL;

	void SetGroupLight(int groupID, vector<AlienFX_SDK::afx_act> actions, bool force = false,
					   AlienFX_SDK::afx_act* from_c = NULL, AlienFX_SDK::afx_act* to_c = NULL, double power = 0);
	bool SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force = false);
	void QueryUpdate(int did = -1, bool force = false);

public:
	ConfigHandler* config;
	AlienFX_SDK::Mappings afx_dev;
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	HANDLE queryEmpty = NULL;
	deque<LightQueryElement> lightQuery;
	mutex modifyQuery;
	bool unblockUpdates = true;
	bool updateLock = false;
	int activeMode = -1;
	EventData eData, maxData;

	FXHelper(ConfigHandler *conf);
	~FXHelper();
	//bool SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force = false);
	//void QueryUpdate(int did = -1, bool force = false);
	AlienFX_SDK::afx_device *LocateDev(int pid);
	size_t FillAllDevs(AlienFan_SDK::Control* acc);
	void Start();
	void Stop();
	void Refresh(int force = 0);
	bool RefreshOne(lightset* map, int force = 0, bool update = false);
	bool SetMode(int mode);
	void TestLight(int did, int id, bool wp=false);
	void ResetPower(int did);
	void SetCounterColor(EventData *data, bool force = false);
	void RefreshState(bool force = false);
	void RefreshMon();
	void RefreshAmbient(UCHAR *img);
	void RefreshHaptics(int *freq);
	//void Flush();
	void ChangeState();
	void UpdateGlobalEffect(AlienFX_SDK::Functions* dev = NULL);
	void UnblockUpdates(bool newState, bool lock = false);

	ConfigHandler* GetConfig() { return config; };
};
