#include "ConfigMon.h"
#include "resource.h"
#include <Shlwapi.h>

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

ConfigMon::ConfigMon() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxmon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, NULL);
	RegCreateKeyEx(hKey1, TEXT("Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey2, NULL);

	Load();
}

ConfigMon::~ConfigMon() {
	Save();
	RegCloseKey(hKey1);
	RegCloseKey(hKey2);
}

SENSOR* ConfigMon::FindSensor(int src, byte type, DWORD id)
{
	for (int i = 0; i < active_sensors.size(); i++)
		if (active_sensors[i].source == src && active_sensors[i].type == type && active_sensors[i].id == id)
			return &active_sensors[i];
	return NULL;
}

void ConfigMon::GetReg(const char* name, DWORD* value, DWORD defValue) {
	DWORD size = sizeof(DWORD);
	if (RegGetValue(hKey1, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigMon::SetReg(const char* text, DWORD value) {
	RegSetValueEx(hKey1, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
}

SENSOR* ConfigMon::CheckSensor(int src, byte type, DWORD id)
{
	SENSOR* sen = FindSensor(src, type, id);
	if (!sen) {
		active_sensors.push_back({ src, (byte)type, (DWORD)id, "", NO_SEN_VALUE });
		sen = &active_sensors.back();
	}
	return sen;
}

void ConfigMon::Load() {
	DWORD size_c = 16 * sizeof(DWORD);// 4 * 16
	DWORD activeProfileID = 0;

	GetReg("AutoStart", &startWindows);
	GetReg("Minimized", &startMinimized);
	GetReg("UpdateCheck", &updateCheck, 1);
	GetReg("wSensors", &wSensors, 1);
	GetReg("eSensors", &eSensors);
	GetReg("bSensors", &bSensors);
	GetReg("Refresh", &refreshDelay, 500);

	char name[256];
	int src, type, id;
	DWORD len = 255, lend = 0;
	byte* data = NULL;
	// Profiles...
	for (int vindex = 0; RegEnumValue(hKey2, vindex, name, &len, NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		len++;
		if (data)
			delete[] data;
		data = new BYTE[lend];
		RegEnumValue(hKey2, vindex, name, &len, NULL, NULL, data, &lend);
		len = 255;
		if (sscanf_s(name, "Name-%d-%d-%d", &src,&type,&id) == 3) {
			CheckSensor(src, type, id)->name = string((char*)data);
			continue;
		}
		if (sscanf_s(name, "Tray-%d-%d-%d", &src, &type, &id) == 3) {
			CheckSensor(src, type, id)->intray = (byte)*(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "State-%d-%d-%d", &src, &type, &id) == 3) {
			CheckSensor(src, type, id)->disabled = (byte)*(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "Flags-%d-%d-%d", &src, &type, &id) == 3) {
			CheckSensor(src, type, id)->flags = *(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "Color-%d-%d-%d", &src, &type, &id) == 3) {
			CheckSensor(src, type, id)->traycolor = *(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "Alarm-%d-%d-%d", &src, &type, &id) == 3) {
			CheckSensor(src, type, id)->alarmPoint = *(DWORD*)data;
			//continue;
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

	RegDeleteTree(hKey1, "Sensors");
	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxmon\\Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey2, NULL);// &dwDisposition);

	for (auto j = active_sensors.begin(); j < active_sensors.end(); j++) {
		string name = to_string(j->source) + "-" + to_string(j->type) + "-" + to_string(j->id);
		string tName = "Name-" + name;
		RegSetValueEx(hKey2, tName.c_str(), 0, REG_SZ, (BYTE*)j->name.c_str(), (DWORD)j->name.length());
		if (j->flags) {
			tName = "Flags-" + name;
			RegSetValueEx(hKey2, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->flags, sizeof(DWORD));
		}
		if (j->traycolor != 0xffffff) {
			tName = "Color-" + name;
			RegSetValueEx(hKey2, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->traycolor, sizeof(DWORD));
		}
		if (j->alarm) {
			tName = "Alarm-" + name;
			RegSetValueEx(hKey2, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->alarmPoint, sizeof(DWORD));
		}
	}
}