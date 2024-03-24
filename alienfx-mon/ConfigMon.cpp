#include "ConfigMon.h"
#include "resource.h"
#include "RegHelperLib.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

ConfigMon::ConfigMon() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxmon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySensors, NULL);

	Load();
}

ConfigMon::~ConfigMon() {
	Save();
	RegCloseKey(hKeySensors);
	RegCloseKey(hKeyMain);
}

void ConfigMon::GetReg(const char* name, DWORD* value, DWORD defValue) {
	DWORD size = sizeof(DWORD);
	if (RegGetValue(hKeyMain, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigMon::SetReg(const char* text, DWORD value) {
	RegSetValueEx(hKeyMain, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
}

void ConfigMon::Load() {
	GetReg("AutoStart", &startWindows);
	GetReg("Minimized", &startMinimized);
	GetReg("UpdateCheck", &updateCheck, 1);
	GetReg("wSensors", &wSensors, 1);
	GetReg("eSensors", &eSensors);
	GetReg("bSensors", &bSensors);
	GetReg("Refresh", &refreshDelay, 500);

	char name[256];
	int src;
	DWORD id, lend = 0;
	byte* data = NULL;

	for (int vindex = 0; lend = GetRegData(hKeySensors, vindex, name, &data); vindex++) {
		if (sscanf_s(name, "%d-%ud", &src, &id) == 2) {
			switch (src) {
			case 0: active_sensors[id].name = GetRegString(data,lend); break;
			case 1: active_sensors[id].flags = *(DWORD*)data; break;
			case 2: active_sensors[id].traycolor = *(DWORD*)data; break;
			case 3: active_sensors[id].alarmPoint = *(DWORD*)data;
			}
		}
	}
	if (data)
		delete[] data;
}

void ConfigMon::Save() {

	SetReg("AutoStart", startWindows);
	SetReg("Minimized", startMinimized);
	SetReg("UpdateCheck", updateCheck);
	SetReg("wSensors", wSensors);
	SetReg("eSensors", eSensors);
	SetReg("bSensors", bSensors);
	SetReg("Refresh", refreshDelay);

	RegDeleteTree(hKeyMain, "Sensors");
	RegCreateKeyEx(hKeyMain, "Sensors", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySensors, NULL);

	string tName;

	for (auto j = active_sensors.begin(); j != active_sensors.end(); j++) {
		if (j->second.name.length()) {
			tName = "0-" + to_string(j->first);
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_SZ, (BYTE*)j->second.name.c_str(), (DWORD)j->second.name.size());
		}
		if (j->second.flags) {
			tName = "1-" + to_string(j->first);
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->second.flags, sizeof(DWORD));
		}
		if (j->second.traycolor != 0xffffff) {
			tName = "2-" + to_string(j->first);
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->second.traycolor, sizeof(DWORD));
		}
		if (j->second.alarmPoint) {
			tName = "3-" + to_string(j->first);
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->second.alarmPoint, sizeof(DWORD));
		}
	}
}