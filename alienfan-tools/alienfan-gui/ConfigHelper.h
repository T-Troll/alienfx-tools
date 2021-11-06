#pragma once

#include <windows.h>
#include <vector>

using namespace std;

struct fan_point {
	short temp = 0;
	short boost = 0;
};

struct fan_block {
	short fanIndex = 0;
	vector<fan_point> points;
};

struct temp_block {
	short sensorIndex = 0;
	vector<fan_block> fans;
};

struct fan_profile {
	DWORD powerStage = 0;
	DWORD GPUPower = 0;
	vector<temp_block> fanControls;
};

class ConfigHelper {
private:
	HKEY   hKey1, hKey2;
	void GetReg(const char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(const char *text, DWORD value);
public:
	DWORD lastSelectedFan = -1;
	DWORD lastSelectedSensor = -1;
	DWORD startWithWindows = 0;
	DWORD startMinimized = 0;
	DWORD maxRPM = 4000;

	fan_profile* lastProf;
	fan_profile prof;

	NOTIFYICONDATA niData = {0};

	ConfigHelper();
	~ConfigHelper();
	temp_block* FindSensor(int);
	fan_block* FindFanBlock(temp_block*, int);

	void Load();
	void Save();

};

