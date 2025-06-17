#pragma once
#include <map>
#include <string>
#include <wtypes.h>
#include "resource.h"
#include "ConfigFan.h"

using namespace std;

extern HINSTANCE hInst;

#define NO_SEN_VALUE -1

struct SENID {
	union {
		struct {
			WORD id;
			byte type;
			byte source;
		};
		DWORD sid;
	};
};

struct SENSOR {
	string sname;
	int min = NO_SEN_VALUE, max = NO_SEN_VALUE, cur = NO_SEN_VALUE;
	bool changed = true;
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
			byte direction;
			byte highlight;
		};
		DWORD alarmPoint = 0;
	};
	string name;
	bool alarming = false;
	DWORD traycolor = 0xffffff;
	NOTIFYICONDATA* niData = NULL;
};

class ConfigMon
{
private:
	HKEY hKeyMain = NULL,
		hKeySensors = NULL;
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
	bool paused = false;
	bool showHidden = false;

	DWORD selection[2]{ 0xffffffff, 0xffffffff };

	ConfigFan fan_conf;

	map<DWORD,SENSOR> active_sensors;

	NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA), 0, 0xffffffff, NIF_ICON | NIF_MESSAGE, WM_APP + 1, (HICON)LoadImage(hInst,
					MAKEINTRESOURCE(IDI_ALIENFXMON),
					IMAGE_ICON,
					GetSystemMetrics(SM_CXSMICON),
					GetSystemMetrics(SM_CYSMICON),
					LR_DEFAULTCOLOR) };

	ConfigMon();
	~ConfigMon();
	void Load();
	void Save();
};