#pragma once
#include <queue>
#include "ConfigHandler.h"
//#include "toolkit.h"
#include <mutex>

// Power modes: AC = 0, Battery = 1, Charge = 2, Low Battery = 4
#define MODE_AC		0
#define MODE_BAT	1
#define MODE_CHARGE	2
#define MODE_LOW	4

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

class FXHelper
{
private:
	int activeMode = -1;
	long lCPU = 0, lRAM = 0, lHDD = 0, lGPU = 0, lNET = 0, lTemp = 0, lBatt = 100, lFan = 0;
	bool blinkStage = false;
	HANDLE updateThread = NULL;
	bool updateLock = false;
	ConfigHandler* config;

	size_t FillDevs(bool state, bool power);
	void SetGroupLight(int groupID, vector<AlienFX_SDK::afx_act> actions, bool force = false,
					   AlienFX_SDK::afx_act* from_c = NULL, AlienFX_SDK::afx_act* to_c = NULL, double power = 0);
	bool SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force = false);
	void QueryUpdate(int did = -1, bool force = false);

public:

	AlienFX_SDK::Mappings afx_dev;
	std::vector<AlienFX_SDK::Functions * > devs;
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	HANDLE queryEmpty = NULL;
	deque<LightQueryElement> lightQuery;
	mutex modifyQuery;
	bool unblockUpdates = true;


	FXHelper(ConfigHandler *conf);
	~FXHelper();
	AlienFX_SDK::Functions *LocateDev(int pid);
	size_t FillAllDevs(bool state, bool power, HANDLE acc);
	void Start();
	void Stop();
	int Refresh(bool force = false);
	bool RefreshOne(lightset* map, bool force = false, bool update = false);
	bool SetMode(int mode);
	void TestLight(int did, int id);
	void ResetPower(int did);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, long cFan, bool force = false);
	void RefreshState(bool force = false);
	void RefreshMon();
	void RefreshAmbient(UCHAR *img);
	void RefreshHaptics(int *freq);
	void Flush();
	void ChangeState();
	void UpdateGlobalEffect(AlienFX_SDK::Functions* dev = NULL);
	void UnblockUpdates(bool newState, bool lock = false);

	ConfigHandler* GetConfig() { return config; };
};
