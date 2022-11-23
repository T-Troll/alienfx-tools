#pragma once
#include <map>
//#include <vector>
#include <string>
#include <wtypes.h>
#include "resource.h"

using namespace std;

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
	int min = NO_SEN_VALUE, max = 0, cur = 0;
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
			WORD direction;
		};
		DWORD alarmPoint = 0;
	};
	string name;
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

	map<DWORD,SENSOR> active_sensors;

	NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA), 0, 0xffffffff, NIF_ICON | NIF_MESSAGE, WM_APP + 1, (HICON)LoadImage(GetModuleHandle(NULL),
					MAKEINTRESOURCE(IDI_ALIENFXMON),
					IMAGE_ICON,
					GetSystemMetrics(SM_CXSMICON),
					GetSystemMetrics(SM_CYSMICON),
					LR_DEFAULTCOLOR) };

	ConfigMon();
	~ConfigMon();
	void Load();
	void Save();

	//SENSOR* FindSensor(byte, byte, WORD);
	SENSOR* FindSensor(DWORD sid);
};