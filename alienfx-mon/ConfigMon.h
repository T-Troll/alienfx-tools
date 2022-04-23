#pragma once
#include <vector>
#include <string>
#include <wtypes.h>

using namespace std;

struct SENSOR {
	int source;
	byte type;
	DWORD id;
	string name;
	int min, max, cur;
	bool disabled;
	bool intray;
	DWORD traycolor = 0xffffff;
	NOTIFYICONDATA* niData;
	int oldCur;
};

class ConfigMon
{
private:
	HKEY hKey1 = NULL,
		hKey2 = NULL;
	void GetReg(const char*, DWORD*, DWORD def = 0);
	void SetReg(const char* text, DWORD value);

public:
	DWORD startWindows = 0;
	DWORD startMinimized = 0;
	DWORD updateCheck = 1;
	DWORD wSensors = 1;
	DWORD eSensors = 0;
	DWORD bSensors = 0;
	DWORD refreshDelay = 500;

	bool needFullUpdate = false;

	std::vector<SENSOR> active_sensors;

	NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA) };

	ConfigMon();
	~ConfigMon();
	void Load();
	void Save();
	void SetIconState();
	SENSOR* FindSensor(int, byte, DWORD);
};