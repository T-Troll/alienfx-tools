#pragma once

#include <vector>
#include <map>
#include "alienfan-sdk.h"

using namespace std;

struct fan_point {
	short temp;
	short boost;
};

struct sen_block {
	bool active = true;
	vector<fan_point> points;
};

/*struct fan_block {
	short fanIndex;
	map<WORD, sen_block> sensors;
}*/;

struct fan_profile {
	WORD powerStage = 0;
	WORD gmode = 0;
	//DWORD GPUPower = 0;
	vector<map<WORD, sen_block>> fanControls;
};

struct fan_overboost {
	byte maxBoost;
	USHORT maxRPM;
};

class ConfigFan {
private:
	HKEY keyMain, keySensors, keyPowers;
	void GetReg(const char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(const char *text, DWORD value);
public:
	byte lastSelectedFan = 0;
	WORD lastSelectedSensor = 0;
	DWORD startWithWindows = 0;
	DWORD startMinimized = 0;
	DWORD updateCheck = 1;
	DWORD obCheck = 0;
	DWORD awcc_disable = 0;
	DWORD keyShortcuts = 1;

	bool wasAWCC = false;

	fan_profile prof;
	fan_profile* lastProf = &prof;

	map<byte, fan_overboost> boosts;
	map<byte, string> powers;
	map<WORD, string> sensors;

	ConfigFan();
	~ConfigFan();

	sen_block* FindSensor();
	map<WORD, sen_block>* FindFanBlock(short, fan_profile* prof = NULL);
	void AddSensorCurve(fan_profile* prof, WORD fid, WORD sid, byte* data, DWORD lend);
	void ConvertSenMappings(fan_profile* prof, AlienFan_SDK::Control* acpi);
	void SetBoostsAndNames(AlienFan_SDK::Control*);

	void Load();
	void Save();
};

