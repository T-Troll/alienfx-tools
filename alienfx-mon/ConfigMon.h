#pragma once
#include <vector>
#include <string>
#include <wtypes.h>
#include "resource.h"

using namespace std;

#define NO_SEN_VALUE -300

struct SENSOR {
	int source;
	byte type;
	DWORD id;
	string name;
	int min, max, cur, oldCur;
	union {
		struct {
			byte disabled;
			byte intray;
			byte inverse;
			byte alarm;
		};
		DWORD flags = 0;
	};
	union {
		struct {
			WORD ap;
			WORD direction;
		};
		DWORD alarmPoint = 0;
	};
	DWORD traycolor = 0xffffff;
	NOTIFYICONDATA* niData;
};

class ConfigMon
{
private:
	HKEY hKeyMain = NULL,
		hKeySensors = NULL;
	void GetReg(const char*, DWORD*, DWORD def = 0);
	void SetReg(const char* text, DWORD value);

	SENSOR* CheckSensor(int src, byte type, DWORD id);

public:
	DWORD startWindows = 0;
	DWORD startMinimized = 0;
	DWORD updateCheck = 1;
	DWORD wSensors = 1;
	DWORD eSensors = 0;
	DWORD bSensors = 0;
	DWORD refreshDelay = 500;

	bool needFullUpdate = false;
	bool paused = false;
	bool showHidden = false;

	std::vector<SENSOR> active_sensors;

	NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA), 0, 0, NIF_ICON | NIF_MESSAGE, WM_APP + 1, (HICON)LoadImage(GetModuleHandle(NULL),
					MAKEINTRESOURCE(IDI_ALIENFXMON),
					IMAGE_ICON,
					GetSystemMetrics(SM_CXSMICON),
					GetSystemMetrics(SM_CYSMICON),
					LR_DEFAULTCOLOR) };

	ConfigMon();
	~ConfigMon();
	void Load();
	void Save();

	SENSOR* FindSensor(int, byte, DWORD);
};