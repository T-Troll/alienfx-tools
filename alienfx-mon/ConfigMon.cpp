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
	if (RegGetValueA(hKey1, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigMon::SetReg(const char* text, DWORD value) {
	RegSetValueEx(hKey1, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
}

SENSOR* ConfigMon::CheckSensor(int src, byte type, DWORD id)
{
	SENSOR* sen = FindSensor(src, type, id);
	if (!sen) {
		active_sensors.push_back({ src, (byte)type, (DWORD)id, "", -1});
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

	int vindex = 0;
	char name[256];
	int src, type, id;
	//SENSOR* sen;
	LSTATUS ret = 0;
	// Profiles...
	do {
		DWORD len = 255, lend = 0;
		if ((ret = RegEnumValueA(hKey2, vindex, name, &len, NULL, NULL, NULL, &lend)) == ERROR_SUCCESS) {
			lend++; len++;
			BYTE* data = new BYTE[lend];
			RegEnumValueA(hKey2, vindex, name, &len, NULL, NULL, data, &lend);
			vindex++;
			if (sscanf_s(name, "Name-%d-%d-%d", &src,&type,&id) == 3) {
				CheckSensor(src, type, id)->name = string((char*)data);
				goto nextEntry;
			}
			if (sscanf_s(name, "Tray-%d-%d-%d", &src, &type, &id) == 3) {
				CheckSensor(src, type, id)->intray = (byte)*(DWORD*)data;
				goto nextEntry;
			}
			if (sscanf_s(name, "State-%d-%d-%d", &src, &type, &id) == 3) {
				CheckSensor(src, type, id)->disabled = (byte)*(DWORD*)data;
				goto nextEntry;
			}
			if (sscanf_s(name, "Flags-%d-%d-%d", &src, &type, &id) == 3) {
				CheckSensor(src, type, id)->flags = *(DWORD*)data;
				goto nextEntry;
			}
			if (sscanf_s(name, "Color-%d-%d-%d", &src, &type, &id) == 3) {
				CheckSensor(src, type, id)->traycolor = *(DWORD*)data;
				goto nextEntry;
			}
			nextEntry:
			delete[] data;
		}
	} while (ret == ERROR_SUCCESS);

}

void ConfigMon::Save() {

	SetReg("AutoStart", startWindows);
	SetReg("Minimized", startMinimized);
	SetReg("UpdateCheck", updateCheck);
	SetReg("wSensors", wSensors);
	SetReg("eSensors", eSensors);
	SetReg("bSensors", bSensors);
	SetReg("Refresh", refreshDelay);

	RegDeleteTreeA(hKey1, "Sensors");
	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxmon\\Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey2, NULL);// &dwDisposition);

	for (int j = 0; j < active_sensors.size(); j++) {
		string name = "Name-" + to_string(active_sensors[j].source) + "-" + to_string(active_sensors[j].type) + "-" + to_string(active_sensors[j].id);
		RegSetValueExA(hKey2, name.c_str(), 0, REG_SZ, (BYTE*)active_sensors[j].name.c_str(), (DWORD)active_sensors[j].name.length());
		//DWORD val = active_sensors[j].intray;
		name = "Flags-" + to_string(active_sensors[j].source) + "-" + to_string(active_sensors[j].type) + "-" + to_string(active_sensors[j].id);
		RegSetValueEx(hKey2, name.c_str(), 0, REG_DWORD, (BYTE*)&active_sensors[j].flags, sizeof(DWORD));
		//val = active_sensors[j].disabled;
		//name = "State-" + to_string(active_sensors[j].source) + "-" + to_string(active_sensors[j].type) + "-" + to_string(active_sensors[j].id);
		//RegSetValueEx(hKey2, name.c_str(), 0, REG_DWORD, (BYTE*)&val, sizeof(DWORD));
		name = "Color-" + to_string(active_sensors[j].source) + "-" + to_string(active_sensors[j].type) + "-" + to_string(active_sensors[j].id);
		RegSetValueEx(hKey2, name.c_str(), 0, REG_DWORD, (BYTE*)&active_sensors[j].traycolor, sizeof(DWORD));
	}
}