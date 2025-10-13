#pragma once
#include <queue>
#include <map>
#include "ConfigHandler.h"

struct LightEventData {
	byte CPU = 0, RAM = 0, HDD = 0, GPU = 0, Temp = 0, Batt = 0, KBD = 0, NET = 0, PWR = 1, ACP = 255, BST = 255,
		PWM = 0, Fan = 0;
};

struct LightQueryElement {
	//WORD pid;
	byte light;	  // lightID or forced state
	//byte force;	  // forced state: 0 - no force, 1 - save light state, 2 - force update
	byte command; // 0 - color, 1 - update, 2 - set brightness, 3 - set power
	byte actsize;
	AlienFX_SDK::Afx_action actions[9];
};

struct DeviceUpdateQuery {
	HANDLE haveNewElement = NULL,
		   updateThread = NULL;
	bool inUpdate = false;
	queue<LightQueryElement> lightQuery;
	map<byte, LightQueryElement> lstate;
};

struct LightQueryData {
	WORD pid;
	void* src;
};

class FXHelper
{
private:

	bool blinkStage = false, wasLFX = false;
	int oldtest = -1;

	void SetZoneLight(DWORD id, int x, int max, int scale, WORD flags, vector<AlienFX_SDK::Afx_action>* actions, double power = 0);
	void SetGaugeGrid(groupset* grp, zonemap* zone, int phase, AlienFX_SDK::Afx_action* fin);
	//void QueryCommand(WORD pid, LightQueryElement &lqe);
	void QueryClearState();
	void QueryAllDevs(LightQueryElement& lqe);
	void SetLight(DWORD lgh, vector<AlienFX_SDK::Afx_action>* actions);
	void QueryUpdate(byte force = 0);
	void SetZone(groupset* grp, vector<AlienFX_SDK::Afx_action>* actions, double power = 1.0);

public:
	HANDLE stopQuery;
	CustomMutex modifyQuery;
	map<WORD, DeviceUpdateQuery> devLightQuery;
	LightEventData eData, maxData;
	// Power button state...
	//map<WORD, AlienFX_SDK::Afx_action[2]> pbstate;

	// light states
	bool stateScreen = true,
		stateDim = false,
		stateAction = true,
		finalPBState = true, 
		updateAllowed = false;
	int finalBrightness = -1;

	FXHelper();
	~FXHelper();
	AlienFX_SDK::Afx_action BlendPower(double power, AlienFX_SDK::Afx_action* from, AlienFX_SDK::Afx_action* to);
	void Start();
	void Stop();
	void Refresh(byte force = 0); // 0 - not forced, 1 - forced and save, 2 - forced
	void TestLight(AlienFX_SDK::Afx_device* dev, int id, bool force = false, bool wp=false);
	bool CheckEvent(LightEventData* eData, event* e);
	void RefreshZone(groupset* map, bool update = true);
	void RefreshCounters(bool fromRefresh = false);
	void RefreshAmbient(bool fromRefresh = false);
	void RefreshHaptics(bool fromRefresh = false);
	void RefreshGrid(bool fromRefresh = false);
	void SetState(bool force = false);
	void UpdateGlobalEffect(AlienFX_SDK::Afx_device* dev = NULL, bool reset = false);
	void QueryCommand(WORD pid, LightQueryElement& lqe);
};
