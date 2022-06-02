#pragma once

#include <vector>
#include <map>
#include "alienfan-sdk.h"

using namespace std;

struct fan_point {
	short temp;
	short boost;
};

struct fan_block {
	short fanIndex;
	vector<fan_point> points;
};

struct temp_block {
	short sensorIndex;
	vector<fan_block> fans;
};

struct fan_profile {
	DWORD powerStage = 0;
	DWORD GPUPower = 0;
	vector<temp_block> fanControls;
};

struct fan_overboost {
	byte fanID;
	byte maxBoost;
	USHORT maxRPM;
};

class ConfigFan {
private:
	HKEY keyMain, keySensors, keyPowers;
	void GetReg(const char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(const char *text, DWORD value);
public:
	DWORD lastSelectedFan = -1;
	DWORD lastSelectedSensor = -1;
	DWORD startWithWindows = 0;
	DWORD startMinimized = 0;
	DWORD updateCheck = 1;
	DWORD obCheck = 0;

	fan_profile prof;
	fan_profile* lastProf = &prof;

	vector<fan_overboost> boosts;
	map<byte, string> powers;

	ConfigFan();
	~ConfigFan();
	temp_block* FindSensor(int);
	fan_block* FindFanBlock(temp_block*, int);

	void Load();
	void Save();

	void SetBoosts(AlienFan_SDK::Control *);
};

