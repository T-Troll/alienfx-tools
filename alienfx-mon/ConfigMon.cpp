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

	//niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = IDI_ALIENFXMON;
	niData.uFlags = NIF_ICON | NIF_MESSAGE;
	niData.uCallbackMessage = WM_APP + 1;
	Load();
}

ConfigMon::~ConfigMon() {
	Save();
	RegCloseKey(hKey1);
	RegCloseKey(hKey2);
}

void ConfigMon::SetIconState() {
	// change tray icon...
			niData.hIcon =
				(HICON)LoadImage(GetModuleHandle(NULL),
					MAKEINTRESOURCE(IDI_ALIENFXMON),
					IMAGE_ICON,
					GetSystemMetrics(SM_CXSMICON),
					GetSystemMetrics(SM_CYSMICON),
					LR_DEFAULTCOLOR);
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
	SENSOR* sen;
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
				if (sen = FindSensor(src, type, id))
					sen->name = string((char*)data);
				else {
					sen = new SENSOR({ src, (byte)type, (DWORD)id, string((char*)data), 30000 });
					active_sensors.push_back(*sen);
				}
			}
			if (sscanf_s(name, "Tray-%d-%d-%d", &src, &type, &id) == 3) {
				if (sen = FindSensor(src, type, id))
					sen->intray = *(DWORD*)data;
				else {
					sen = new SENSOR({ src, (byte)type, (DWORD)id, "", 30000, 0, 0, false, (bool)*(DWORD*)data });
					active_sensors.push_back(*sen);
				}
			}
			if (sscanf_s(name, "State-%d-%d-%d", &src, &type, &id) == 3) {
				if (sen = FindSensor(src, type, id))
					sen->disabled = *(DWORD*)data;
				else {
					sen = new SENSOR({ src, (byte)type, (DWORD)id, "", 30000, 0, 0, (bool)*(DWORD*)data });
					active_sensors.push_back(*sen);
				}
			}
			if (sscanf_s(name, "Color-%d-%d-%d", &src, &type, &id) == 3) {
				if (sen = FindSensor(src, type, id))
					sen->traycolor = *(DWORD*)data;
				else {
					sen = new SENSOR({ src, (byte)type, (DWORD)id, "", 30000, 0, 0, false, false, *(DWORD*)data });
					active_sensors.push_back(*sen);
				}
			}
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
		DWORD val = active_sensors[j].intray;
		name = "Tray-" + to_string(active_sensors[j].source) + "-" + to_string(active_sensors[j].type) + "-" + to_string(active_sensors[j].id);
		RegSetValueEx(hKey2, name.c_str(), 0, REG_DWORD, (BYTE*)&val, sizeof(DWORD));
		val = active_sensors[j].disabled;
		name = "State-" + to_string(active_sensors[j].source) + "-" + to_string(active_sensors[j].type) + "-" + to_string(active_sensors[j].id);
		RegSetValueEx(hKey2, name.c_str(), 0, REG_DWORD, (BYTE*)&val, sizeof(DWORD));
		name = "Color-" + to_string(active_sensors[j].source) + "-" + to_string(active_sensors[j].type) + "-" + to_string(active_sensors[j].id);
		RegSetValueEx(hKey2, name.c_str(), 0, REG_DWORD, (BYTE*)&active_sensors[j].traycolor, sizeof(DWORD));
	}
}