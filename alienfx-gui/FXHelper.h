#pragma once
#include <queue>
#include <map>
#include "ConfigHandler.h"

struct LightEventData {
	byte CPU = 0, RAM = 0, HDD = 0, GPU = 0, Temp = 0, Batt = 0, KBD = 0, NET = 0, PWR = 1, ACP = 255, BST = 255,
		PWM = 0, Fan = 0;
};

struct LightQueryElement {
	WORD pid;
	byte light;
	byte command; // 0 - color, 1 - update, 2 - set brightness, 3 - set power
	byte actsize;
	AlienFX_SDK::Afx_action actions[9];
};

class FXHelper
{
private:

	bool blinkStage = false, wasLFX = false;
	int oldtest = -1;

	void SetZoneLight(DWORD id, int x, int max, int scale, WORD flags, vector<AlienFX_SDK::Afx_action>* actions, double power = 0);
	void SetGaugeGrid(groupset* grp, zonemap* zone, int phase, AlienFX_SDK::Afx_action* fin);
	void QueryCommand(LightQueryElement &lqe);
	void SetLight(DWORD lgh, vector<AlienFX_SDK::Afx_action>* actions);

public:
	HANDLE updateThread = NULL;
	HANDLE stopQuery;
	HANDLE haveNewElement;
	CustomMutex modifyQuery;
	queue<LightQueryElement> lightQuery;
	LightEventData eData, maxData;
	// Power button state...
	map<WORD, AlienFX_SDK::Afx_action[2]> pbstate;

	// light states
	bool stateScreen = true,
		stateDim = false,
		stateAction = true,
		finalPBState = true;
	bool lightsNoDelay = true;
	int finalBrightness = -1;

	FXHelper();
	~FXHelper();
	void FXHelper::FillAllDevs();
	AlienFX_SDK::Afx_action BlendPower(double power, AlienFX_SDK::Afx_action* from, AlienFX_SDK::Afx_action* to);
	void Start();
	void Stop();
	void Refresh(bool force = false);
	void RefreshZone(groupset* map, bool update = true);
	void TestLight(AlienFX_SDK::Afx_device* dev, int id, bool force = false, bool wp=false);
	bool CheckEvent(LightEventData* eData, event* e);
	void RefreshCounters(LightEventData *data = NULL, bool fromRefresh = false);
	void RefreshAmbient(bool fromRefresh = false);
	void RefreshHaptics(bool fromRefresh = false);
	void RefreshGrid(bool fromRefresh = false);
	void SetZone(groupset* grp, vector<AlienFX_SDK::Afx_action>* actions, double power = 1.0);
	void SetState(bool force = false);
	void UpdateGlobalEffect(AlienFX_SDK::Afx_device* dev = NULL, bool reset = false);
	void QueryUpdate(bool force = false);
};
