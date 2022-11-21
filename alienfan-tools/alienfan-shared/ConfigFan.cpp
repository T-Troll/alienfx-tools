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

map<WORD, sen_block>*/*fan_block**/ ConfigFan::FindFanBlock(short id, fan_profile* prof) {
	if (!prof) prof = lastProf;
	if (id >= prof->fanControls.size())
		prof->fanControls.resize(id + 1);
	return &prof->fanControls[id];
	//for (auto tb = prof->fanControls.begin(); tb != prof->fanControls.end(); tb++)
	//	if (tb->fanIndex == id)
	//		return &(*tb);
	//return NULL;
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
	auto fan = FindFanBlock(fid, prof);
	//if (!fan) {
	//	prof->fanControls.push_back({ (byte)fid });
	//	fan = &prof->fanControls.back();
	//}
	// Now load and add sensor data..
	sen_block curve = { true };
	for (UINT i = 0; i < lend; i += 2) {
		curve.points.push_back({ data[i], data[i + 1] });
	}
	(*fan)[sid] = curve;
}

void ConfigFan::Load() {

	DWORD power;

	GetReg("StartAtBoot", &startWithWindows);
	GetReg("StartMinimized", &startMinimized);
	GetReg("UpdateCheck", &updateCheck, 1);
	GetReg("LastPowerStage", &power);
	//GetReg("LastSensor", &lastSelectedSensor);
	//GetReg("LastFan", &lastSelectedFan);
	GetReg("ObCheck", &obCheck);
	GetReg("DisableAWCC", &awcc_disable);
	GetReg("KeyboardShortcut", &keyShortcuts, 1);
	// set power values
	prof.powerStage = LOWORD(power);
	prof.gmode = HIWORD(power);

	// Now load sensor mappings...
	char name[256];
	byte* inarray = NULL;
	DWORD len = 255, lend = 0; short fid, sid;
	for (int vindex = 0; RegEnumValue(keySensors, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		if (inarray)
			delete[] inarray;
		inarray = new byte[lend];
		RegEnumValue(keySensors, vindex, name, &(len = 255), NULL, NULL, inarray, &lend);
		if (sscanf_s(name, "Fan-%hd-%hd", &fid, &sid) == 2) {
			AddSensorCurve(&prof, fid, sid, inarray, lend);
			continue;
		}
		if (sscanf_s(name, "Sensor-%hd-%hd", &sid, &fid) == 2) {
			AddSensorCurve(&prof, fid, sid | 0xff00, inarray, lend);
			continue;
		}
		sid = 0xff;
		if (sscanf_s(name, "SensorName-%hd-%hd", &sid, &fid) == 2 || sscanf_s(name, "SensorName-%hd", &fid) == 1) {
			sensors[MAKEWORD(fid, sid)] = (char*)inarray;
			continue;
		}
		// old format, deprecated...
		//if (sscanf_s(name, "SensorName-%hd", &sid) == 1) {
		//	sensors.emplace(MAKEWORD(sid, 0xff), (char*)inarray);
		//}
	}
	if (inarray) delete[] inarray;
	for (int vindex = 0; RegEnumValue(keyMain, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		if (sscanf_s(name, "Boost-%hd", &fid) == 1) { // Boost block
			inarray = new byte[lend];
			RegEnumValue(keyMain, vindex, name, &(len = 255), NULL, NULL, inarray, &lend);
			boosts[(byte)fid] = { inarray[0],*(WORD*)(inarray + 1) };
			delete[] inarray;
		}
	}
	for (int vindex = 0; RegEnumValue(keyPowers, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		if (sscanf_s(name, "Power-%hd", &fid) == 1) { // Power names
			char* inarray = new char[lend + 1];
			RegEnumValue(keyPowers, vindex, name, &(len = 255), NULL, NULL, (byte*)inarray, &lend);
			powers.emplace((byte)fid, inarray);
			delete[] inarray;
		}
	}
}

void ConfigFan::Save() {
	string name;

	SetReg("StartAtBoot", startWithWindows);
	SetReg("StartMinimized", startMinimized);
	SetReg("LastPowerStage", MAKELPARAM(prof.powerStage, prof.gmode));
	SetReg("UpdateCheck", updateCheck);
	//SetReg("LastSensor", lastSelectedSensor);
	//SetReg("LastFan", lastSelectedFan);
	SetReg("ObCheck", obCheck);
	SetReg("DisableAWCC", awcc_disable);
	SetReg("KeyboardShortcut", keyShortcuts);
	// clean old data
	RegDeleteTree(keyMain, "Sensors");
	RegCreateKeyEx(keyMain, "Sensors", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keySensors, NULL);
	RegDeleteTree(keyMain, "Powers");
	RegCreateKeyEx(keyMain, "Powers", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keyPowers, NULL);

	// save profile..
	for (auto i = 0; i < prof.fanControls.size(); i++) {
		for (auto j = prof.fanControls[i].begin(); j != prof.fanControls[i].end(); j++) {
			if (j->second.active) {
				name = "Fan-" + to_string(i) + "-" + to_string(j->first);
				byte* outdata = new byte[j->second.points.size() * 2];
				for (int k = 0; k < j->second.points.size(); k++) {
					outdata[2 * k] = (byte)j->second.points[k].temp;
					outdata[(2 * k) + 1] = (byte)j->second.points[k].boost;
				}

				RegSetValueEx(keySensors, name.c_str(), 0, REG_BINARY, (BYTE*)outdata, (DWORD)j->second.points.size() * 2);
				delete[] outdata;
			}
		}
	}
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
		RegSetValueEx(keyPowers, name.c_str(), 0, REG_SZ, (BYTE*)i->second.c_str(), (DWORD) i->second.length());
	}
	// save sensors...
	for (auto i = sensors.begin(); i != sensors.end(); i++) {
		name = "SensorName-" + to_string(HIBYTE(i->first)) + "-" + to_string(LOBYTE(i->first));
		RegSetValueEx(keySensors, name.c_str(), 0, REG_SZ, (BYTE*)i->second.c_str(), (DWORD)i->second.length());
	}
}

void ConfigFan::ConvertSenMappings(fan_profile* prof, AlienFan_SDK::Control* acpi) {
	prof->fanControls.resize(acpi->fans.size());
	for (auto fn = prof->fanControls.begin(); fn != prof->fanControls.end(); fn++) {
		map<WORD, sen_block> newSenData;
		for (auto sen = fn->begin(); sen != fn->end(); sen++)
			if (HIBYTE(sen->first) == 0xff) {
				newSenData[acpi->sensors[LOBYTE(sen->first)].sid] = sen->second;
			}
			else
				newSenData[sen->first] = sen->second;
		(*fn) = newSenData;
	}
}

void ConfigFan::SetBoostsAndNames(AlienFan_SDK::Control* acpi) {
	// Resize fan blocks...
	for (byte fID = 0; fID < acpi->boosts.size(); fID++)
		for (auto maxB = boosts.begin(); maxB != boosts.end(); maxB++)
			if (maxB->first == fID) {
				acpi->boosts[fID] = maxB->second.maxBoost;
				acpi->maxrpm[fID] = maxB->second.maxRPM;
				break;
			}
	for (auto i = acpi->powers.begin(); i < acpi->powers.end(); i++)
		if (powers.find(*i) == powers.end())
			powers.emplace(*i, *i ? "Level " + to_string(i - acpi->powers.begin()) : "Manual");

	// old formats convert
	// sensor names...
	map<WORD, string> newSenNames;
	for (auto i = sensors.begin(); i != sensors.end(); i++)
		if (HIBYTE(i->first) == 0xff) {
			newSenNames[acpi->sensors[LOBYTE(i->first)].sid] = i->second;
		}
		else
			newSenNames[i->first] = i->second;
	sensors = newSenNames;
	// fan mappings
	ConvertSenMappings(lastProf, acpi);
}


