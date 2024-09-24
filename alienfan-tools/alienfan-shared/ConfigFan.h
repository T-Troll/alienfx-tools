#pragma once

#include <vector>
#include <map>
#include "alienfan-sdk.h"

using namespace std;

DWORD WINAPI DPTFInit(LPVOID lparam);

struct fan_point {
	byte temp, boost;
};

struct sen_block {
	bool active = true;
	vector<fan_point> points;
};

struct fan_profile {
	union {
		struct {
			WORD powerStage;
			WORD gmodeStage;
		};
		DWORD powerSet = 0;
	};
	map<byte,map<WORD, sen_block>> fanControls;
	// OC block
	union {
		struct {
			byte currentTCC;
			byte memoryXMP;
		};
		DWORD ocSettings = 100;
	};
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
	WORD lastSelectedSensor;
	DWORD startWithWindows;
	DWORD startMinimized;
	DWORD updateCheck;
	DWORD awcc_disable;
	DWORD keyShortcuts;
	DWORD keepSystem;
	DWORD needDPTF;
	DWORD pollingRate;
	DWORD ocEnable;
	bool wasAWCC;

	fan_profile prof;
	fan_profile* lastProf = &prof;

	map<byte, fan_overboost> boosts;
	map<byte, string> powers;
	map<WORD, string> sensors;

	ConfigFan();
	~ConfigFan();

	void AddSensorCurve(fan_profile* prof, byte fid, WORD sid, byte* data, DWORD lend);
	void SaveSensorBlocks(HKEY key, string pname, fan_profile* data);
	//DWORD GetRegData(HKEY key, int vindex, char* name, byte** data);
	string* GetPowerName(int index);
	string GetSensorName(AlienFan_SDK::ALIENFAN_SEN_INFO* acpi);
	void UpdateBoost(byte fanID, byte boost, WORD rpm);
	int GetFanScale(byte fanID);
	void Load();
	void Save();
};

