#include "ConfigFan.h"
#include <string>

ConfigFan::ConfigFan() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfan"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keyMain, NULL);
	RegCreateKeyEx(keyMain, TEXT("Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keySensors, NULL);
	RegCreateKeyEx(keyMain, TEXT("Powers"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keyPowers, NULL);
	Load();
}

ConfigFan::~ConfigFan() {
	Save();
	RegCloseKey(keyPowers);
	RegCloseKey(keySensors);
	RegCloseKey(keyMain);
}

sen_block* ConfigFan::FindSensor() {
	auto sen = lastProf->fanControls[lastSelectedFan].find(lastSelectedSensor);
	if (sen != lastProf->fanControls[lastSelectedFan].end())
		return &sen->second;
	return NULL;
}

void ConfigFan::GetReg(const char *name, DWORD *value, DWORD defValue) {
	DWORD size = sizeof(DWORD);
	if (RegGetValue(keyMain, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigFan::SetReg(const char *text, DWORD value) {
	RegSetValueEx( keyMain, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

void ConfigFan::AddSensorCurve(fan_profile *prof, WORD fid, WORD sid, byte* data, DWORD lend) {
	if (!prof) prof = lastProf;
	// Now load and add sensor data..
	sen_block curve = { true };
	for (UINT i = 0; i < lend; i += 2) {
		curve.points.push_back({ data[i], data[i + 1] });
	}
	prof->fanControls[fid][sid] = curve;
}

DWORD ConfigFan::GetRegData(HKEY key, int vindex, char* name, byte** data) {
	DWORD len, lend;
	if (*data)
		delete[] *data;
	if (RegEnumValue(key, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS) {
		*data = new byte[lend];
		RegEnumValue(key, vindex, name, &(len = 255), NULL, NULL, *data, &lend);
		return lend;
	}
	return 0;
}

void ConfigFan::Load() {

	DWORD power;

	GetReg("StartAtBoot", &startWithWindows);
	GetReg("StartMinimized", &startMinimized);
	GetReg("UpdateCheck", &updateCheck, 1);
	GetReg("LastPowerStage", &power);
	//GetReg("ObCheck", &obCheck);
	GetReg("DisableAWCC", &awcc_disable);
	GetReg("KeyboardShortcut", &keyShortcuts, 1);
	// set power values
	prof.powerStage = LOWORD(power);
	prof.gmode = HIWORD(power);

	// Now load sensor mappings...
	char name[256];
	byte* inarray = NULL;
	DWORD lend; short fid, sid;
	for (int vindex = 0; lend = GetRegData(keySensors, vindex, name, &inarray); vindex++) {
		if (sscanf_s(name, "Fan-%hd-%hd", &fid, &sid) == 2) {
			AddSensorCurve(&prof, fid, sid, inarray, lend);
			continue;
		}
		if (sscanf_s(name, "SensorName-%hd-%hd", &sid, &fid) == 2) {
			sensors[MAKEWORD(fid, sid)] = (char*)inarray;
			continue;
		}
	}
	inarray = NULL;
	for (int vindex = 0; lend = GetRegData(keyMain, vindex, name, &inarray); vindex++) {
		if (sscanf_s(name, "Boost-%hd", &fid) == 1) { // Boost block
			boosts[(byte)fid] = { inarray[0],*(WORD*)(inarray + 1) };
		}
	}
	inarray = NULL;
	for (int vindex = 0; lend = GetRegData(keyPowers, vindex, name, &inarray); vindex++) {
		if (sscanf_s(name, "Power-%hd", &fid) == 1) { // Power names
			powers[(byte)fid] = (char*)inarray;
		}
	}
}

void ConfigFan::SaveSensorBlocks(HKEY key, string pname, fan_profile* data) {
	for (auto i = data->fanControls.begin(); i != data->fanControls.end(); i++) {
		for (auto j = i->second.begin(); j != i->second.end(); j++) {
			if (j->second.active) {
				string name = pname + "-" + to_string(i->first) + "-" + to_string(j->first);
				byte* outdata = new byte[j->second.points.size() * 2];
				for (int k = 0; k < j->second.points.size(); k++) {
					outdata[2 * k] = (byte)j->second.points[k].temp;
					outdata[(2 * k) + 1] = (byte)j->second.points[k].boost;
				}

				RegSetValueEx(key, name.c_str(), 0, REG_BINARY, (BYTE*)outdata, (DWORD)j->second.points.size() * 2);
				delete[] outdata;
			}
		}
	}
}

void ConfigFan::Save() {
	string name;

	SetReg("StartAtBoot", startWithWindows);
	SetReg("StartMinimized", startMinimized);
	SetReg("LastPowerStage", MAKELPARAM(prof.powerStage, prof.gmode));
	SetReg("UpdateCheck", updateCheck);
	//SetReg("ObCheck", obCheck);
	SetReg("DisableAWCC", awcc_disable);
	SetReg("KeyboardShortcut", keyShortcuts);
	// clean old data
	RegDeleteTree(keyMain, "Sensors");
	RegCreateKeyEx(keyMain, "Sensors", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keySensors, NULL);
	RegDeleteTree(keyMain, "Powers");
	RegCreateKeyEx(keyMain, "Powers", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keyPowers, NULL);

	// save profile..
	SaveSensorBlocks(keySensors, "Fan", &prof);
	// save boosts...
	for (auto i = boosts.begin(); i != boosts.end(); i++) {
		byte outarray[sizeof(byte) + sizeof(USHORT)] = {0};
		name = "Boost-" + to_string(i->first);
		outarray[0] = i->second.maxBoost;
		*(USHORT *) (outarray + sizeof(byte)) = i->second.maxRPM;
		RegSetValueEx( keyMain, name.c_str(), 0, REG_BINARY, outarray, (DWORD) sizeof(byte) + sizeof(USHORT));
	}
	// save powers...
	for (auto i = powers.begin(); i != powers.end(); i++) {
		name = "Power-" + to_string(i->first);
		RegSetValueEx(keyPowers, name.c_str(), 0, REG_SZ, (BYTE*)i->second.c_str(), (DWORD) i->second.size());
	}
	// save sensors...
	for (auto i = sensors.begin(); i != sensors.end(); i++) {
		name = "SensorName-" + to_string(HIBYTE(i->first)) + "-" + to_string(LOBYTE(i->first));
		RegSetValueEx(keySensors, name.c_str(), 0, REG_SZ, (BYTE*)i->second.c_str(), (DWORD)i->second.size());
	}
}

string ConfigFan::GetPowerName(int index) {
	if (powers[index].empty())
		powers[index] = index ? "Level " + to_string(index) : "Manual";
	return powers[index];
}

void ConfigFan::UpdateBoost(byte fanID, byte boost, WORD rpm) {
	fan_overboost* fo = &boosts[fanID];
	fo->maxBoost = max(boost, 100);
	fo->maxRPM = max(rpm, fo->maxRPM);
}

//void ConfigFan::SetSensorNames(AlienFan_SDK::Control* acpi) {
//	// patch for incorrect fan block size
//	for (auto i = acpi->powers.begin(); i < acpi->powers.end(); i++)
//		if (powers[*i].empty())
//			powers[*i] = *i ? "Level " + to_string(i - acpi->powers.begin()) : "Manual";
//	for (int i = 0; i < acpi->fans.size(); i++) {
//		fan_overboost* fo = &boosts[i];
//		fo->maxBoost = max(100, fo->maxBoost);
//		fo->maxRPM = max(acpi->GetMaxRPM(i), fo->maxRPM);
//	}
//		//if (boosts.find(i) == boosts.end())
//		//	boosts[i] = { 100, (unsigned short)acpi->GetMaxRPM(i) };
//		//else
//		//	boosts[i].maxBoost = max(boosts[i].maxBoost, 100);
//}

int ConfigFan::GetFanScale(byte fanID) {
	if (!boosts[fanID].maxBoost)
		boosts[fanID].maxBoost = 100;
	return boosts[fanID].maxBoost;
}


