#pragma once
#include <queue>
#include <map>
#include "ConfigHandler.h"

struct LightEventData {
	byte CPU = 0, RAM = 0, HDD = 0, GPU = 0, Temp = 0, Batt = 0, KBD = 0, NET = 0, PWR = 1, ACP = 255, BST = 255,
		PWM = 0;
	short Fan = 0;
};

struct LightQueryElement {
	AlienFX_SDK::Afx_device* dev;
	byte light;
	byte command; // 0 - color, 1 - update, 2 - set brightness
	byte actsize;
	AlienFX_SDK::Afx_action actions[9];
};

struct deviceQuery {
	WORD pid;
	vector<AlienFX_SDK::Afx_lightblock> dev_query;
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
	void QueryUpdate(bool force = false);

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
		finalPBState = true;
	bool lightsNoDelay = true;
	int finalBrightness = -1;

	FXHelper();

	~FXHelper();
	AlienFX_SDK::Afx_action BlendPower(double power, AlienFX_SDK::Afx_action* from, AlienFX_SDK::Afx_action* to);
	void Start();
	void Stop();
	void Refresh(bool force = false);
	void RefreshOne(groupset* map, bool update = true);
	void TestLight(AlienFX_SDK::Afx_device* dev, int id, bool force = false, bool wp=false);
	void ResetPower(AlienFX_SDK::Afx_device* dev);
	bool CheckEvent(LightEventData* eData, event* e);
	void RefreshCounters(LightEventData *data = NULL);
	void RefreshAmbient();
	void RefreshHaptics();
	void RefreshGrid();
	void SetZone(groupset* grp, vector<AlienFX_SDK::Afx_action>* actions, double power = 1.0);
	void SetState(bool force = false);
	void UpdateGlobalEffect(AlienFX_SDK::Afx_device* dev = NULL, bool reset = false);
};
