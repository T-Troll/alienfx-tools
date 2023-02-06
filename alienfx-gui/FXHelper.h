#pragma once
#include <queue>
#include <mutex>
#include "ConfigHandler.h"

struct LightEventData {
	byte CPU = 0, RAM = 0, HDD = 0, GPU = 0, Temp = 0, Batt = 0, KBD = 0, NET = 0, PWR = 1, ACP = 255, BST = 255,
		PWM = 0;
	short Fan = 0;
};

struct LightQueryElement {
	DWORD light;
	bool flags, update = false;
	byte actsize;
	AlienFX_SDK::Afx_action actions[10];
};

struct deviceQuery {
	WORD pid;
	vector<AlienFX_SDK::Afx_lightblock> dev_query;
};

class FXHelper
{
private:

	bool blinkStage = false;
	int oldtest = -1;
	HANDLE lightFX;

	void SetZoneLight(DWORD id, int x, int max, WORD flags, vector<AlienFX_SDK::Afx_action>* actions, double power = 0, bool force = false);
	void SetGaugeGrid(groupset* grp, zonemap* zone, int phase, AlienFX_SDK::Afx_action* fin);
	void QueryCommand(LightQueryElement* lqe);
	void SetLight(DWORD lgh, vector<AlienFX_SDK::Afx_action>* actions, bool force = false);
	void QueryUpdate(bool force = false);

public:
	HANDLE updateThread = NULL;
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	queue<LightQueryElement> lightQuery;
	mutex modifyQuery;
	LightEventData eData, maxData;
	// Power button state...
	map<WORD, AlienFX_SDK::Afx_action[2]> pbstate;

	FXHelper();

	~FXHelper();
	AlienFX_SDK::Afx_action BlendPower(double power, AlienFX_SDK::Afx_action* from, AlienFX_SDK::Afx_action* to);
	void FillAllDevs(AlienFan_SDK::Control* acc);
	void Start();
	void Stop();
	// Force:
	// 0: update lights not involved to effect, then effect
	// 1: update all lights, ignoring on status
	// 2: save lights into device
	void Refresh(int force = 0);
	void RefreshOne(groupset* map, bool update = true, int force = 0);
	void TestLight(AlienFX_SDK::Afx_device* dev, int id, bool force = false, bool wp=false);
	void ResetPower(AlienFX_SDK::Afx_device* dev);
	bool CheckEvent(LightEventData* eData, event* e);
	void RefreshCounters(LightEventData *data = NULL);
	void RefreshAmbient();
	void RefreshHaptics();
	void RefreshGrid();
	void SetZone(groupset* grp, vector<AlienFX_SDK::Afx_action>* actions, double power = 1.0, bool force = false);
	void SetState(bool force = false);
	void UpdateGlobalEffect(AlienFX_SDK::Functions* dev = NULL, bool reset = false);
};
