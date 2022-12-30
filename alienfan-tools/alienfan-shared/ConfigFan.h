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

struct fan_profile {
	WORD powerStage = 0;
	WORD gmode = 0;
	//DWORD GPUPower = 0;
	map<short,map<WORD, sen_block>> fanControls;
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
	DWORD startWithWindows;
	DWORD startMinimized;
	DWORD updateCheck;
	DWORD awcc_disable;
	DWORD keyShortcuts;

	bool wasAWCC;

	fan_profile prof;
	fan_profile* lastProf = &prof;

	map<byte, fan_overboost> boosts;
	map<byte, string> powers;
	map<WORD, string> sensors;

	ConfigFan();
	~ConfigFan();

	sen_block* FindSensor();
	void AddSensorCurve(fan_profile* prof, WORD fid, WORD sid, byte* data, DWORD lend);
	void SaveSensorBlocks(HKEY key, string pname, fan_profile* data);
	DWORD GetRegData(HKEY key, int vindex, char* name, byte** data);
	void SetSensorNames(AlienFan_SDK::Control*);
	int GetFanScale(byte fanID);

	void Load();
	void Save();
};

