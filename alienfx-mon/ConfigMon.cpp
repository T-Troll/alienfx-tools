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

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxmon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySensors, NULL);

	Load();
}

ConfigMon::~ConfigMon() {
	Save();
	RegCloseKey(hKeySensors);
	RegCloseKey(hKeyMain);
}

//SENSOR* ConfigMon::FindSensor(byte src, byte type, WORD id)
//{
//	SENID sid = { id, type, src };
//	for (auto i = active_sensors.begin(); i != active_sensors.end(); i++)
//		if (i->first == sid.sid)
//			return &(i->second);
//	return NULL;
//}

SENSOR* ConfigMon::FindSensor(DWORD sid)
{
	for (auto i = active_sensors.begin(); i != active_sensors.end(); i++)
		if (i->first == sid)
			return &(i->second);
	return NULL;
}


//SENSOR* ConfigMon::CheckSensor(byte src, byte type, WORD id)
//{
//	SENSOR* sen = FindSensor(src, type, id);
//	if (!sen) {
//		active_sensors.push_back({ src, type, id, "???", NO_SEN_VALUE });
//		sen = &active_sensors.back();
//	}
//	return sen;
//}

void ConfigMon::GetReg(const char* name, DWORD* value, DWORD defValue) {
	DWORD size = sizeof(DWORD);
	if (RegGetValue(hKeyMain, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigMon::SetReg(const char* text, DWORD value) {
	RegSetValueEx(hKeyMain, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
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
	DWORD len, lend = 0;
	byte* data = NULL;
	SENID sid;
	// Profiles...
	for (int vindex = 0; RegEnumValue(hKeySensors, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		if (data)
			delete[] data;
		data = new BYTE[lend];
		RegEnumValue(hKeySensors, vindex, name, &(len = 255), NULL, NULL, data, &lend);
		if (sscanf_s(name, "Name-%d-%d-%d", &src,&type,&id) == 3) {
			sid = { (WORD)id, (byte)type, (byte)src };
			active_sensors[sid.sid].name = string((char*)data);
			//CheckSensor(src, type, id)->name = string((char*)data);
			continue;
		}
		//if (sscanf_s(name, "Tray-%d-%d-%d", &src, &type, &id) == 3) {
		//	active_sensors[{src, type, id}].intray = (byte)*(DWORD*)data;
		//	continue;
		//}
		//if (sscanf_s(name, "State-%d-%d-%d", &src, &type, &id) == 3) {
		//	active_sensors[{src, type, id}].disabled = (byte)*(DWORD*)data;
		//	continue;
		//}
		if (sscanf_s(name, "Flags-%d-%d-%d", &src, &type, &id) == 3) {
			sid = { (WORD)id, (byte)type, (byte)src };
			active_sensors[sid.sid].flags = *(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "Color-%d-%d-%d", &src, &type, &id) == 3) {
			sid = { (WORD)id, (byte)type, (byte)src };
			active_sensors[sid.sid].traycolor = *(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "Alarm-%d-%d-%d", &src, &type, &id) == 3) {
			sid = { (WORD)id, (byte)type, (byte)src };
			active_sensors[sid.sid].alarmPoint = *(DWORD*)data;
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

	RegDeleteTree(hKeyMain, "Sensors");
	RegCreateKeyEx(hKeyMain, "Sensors", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeySensors, NULL);

	string name, tName;
	SENID sid;

	for (auto j = active_sensors.begin(); j != active_sensors.end(); j++) {
		sid.sid = j->first;
		name = to_string(sid.source) + "-" + to_string(sid.type) + "-" + to_string(sid.id);
		if (j->second.name.length()) {
			tName = "Name-" + name;
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_SZ, (BYTE*)j->second.name.c_str(), (DWORD)j->second.name.length());
		}
		if (j->second.flags) {
			tName = "Flags-" + name;
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->second.flags, sizeof(DWORD));
		}
		if (j->second.traycolor != 0xffffff) {
			tName = "Color-" + name;
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->second.traycolor, sizeof(DWORD));
		}
		if (j->second.alarm) {
			tName = "Alarm-" + name;
			RegSetValueEx(hKeySensors, tName.c_str(), 0, REG_DWORD, (BYTE*)&j->second.alarmPoint, sizeof(DWORD));
		}
	}
}