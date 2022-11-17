#pragma once
#include <queue>
#include <mutex>
#include "ConfigHandler.h"

struct EventData {
	byte CPU = 0, RAM = 0, HDD = 0, GPU = 0, Temp = 0, Batt = 0, KBD = 0, NET = 0, PWR = 1, ACP = 255, BST = 255;
	short Fan = 0;
};

struct LightQueryElement {
	AlienFX_SDK::afx_device* dev = NULL;
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

	void SetZoneLight(DWORD id, int x, int max, WORD flags, vector<AlienFX_SDK::afx_act> actions, double power = 0, bool force = false);
	void SetZone(groupset* grp, vector<AlienFX_SDK::afx_act> actions, double power = -1.0, bool force = false);
	//void SetGridLight(zonemap* zone, int x, int y, AlienFX_SDK::Colorcode fin);
	void SetGaugeGrid(groupset* grp, zonemap* zone, int phase, AlienFX_SDK::afx_act fin);
	void SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force = false);
	void QueryUpdate(bool force = false);

public:
	HANDLE updateThread = NULL;
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	queue<LightQueryElement> lightQuery;
	mutex modifyQuery;
	EventData eData, maxData;
	// Power button state...
	AlienFX_SDK::afx_act pbstate[2]{ 0 };

	FXHelper();

	~FXHelper();
	AlienFX_SDK::afx_act BlendPower(double power, AlienFX_SDK::afx_act* from, AlienFX_SDK::afx_act* to);
	int FillAllDevs(AlienFan_SDK::Control* acc);
	void Start();
	void Stop();
	void Refresh(int force = 0);
	void RefreshOne(groupset* map, bool update = true, int force = 0);
	void TestLight(AlienFX_SDK::afx_device* dev, int id, bool force = false, bool wp=false);
	void ResetPower(AlienFX_SDK::afx_device* dev);
	void SetCounterColor(EventData *data, bool force = false);
	void SetGridEffect(groupset* grp);
	void RefreshMon();
	void RefreshAmbient(UCHAR *img);
	void RefreshHaptics(int *freq);
	void RefreshGrid(int tact);
	void SetState(bool force = false);
	void UpdateGlobalEffect(AlienFX_SDK::Functions* dev = NULL, bool reset = false);
};
