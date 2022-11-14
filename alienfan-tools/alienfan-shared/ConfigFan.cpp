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

vector<fan_point>* ConfigFan::FindSensor() {
	fan_block* fan = FindFanBlock(lastSelectedFan);
	if (fan) {
		auto sen = fan->sensors.find(lastSelectedSensor);
		if (sen != fan->sensors.end())
			return &sen->second;
	}
	return NULL;
}

fan_block* ConfigFan::FindFanBlock(short id) {
	for (auto tb = lastProf->fanControls.begin(); tb != lastProf->fanControls.end(); tb++)
		if (tb->fanIndex == id)
			return &(*tb);
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
	DWORD len = 255, lend = 0; short fid;
	for (int vindex = 0; RegEnumValue(keySensors, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		short sid; //len = 255;
		if (sscanf_s(name, "Fan-%hd-%hd", &fid, &sid) == 2) {
			fan_block* fan = FindFanBlock(fid);
			if (!fan) {
				prof.fanControls.push_back({ (byte)fid });
				fan = &prof.fanControls.back();
			}
			// Now load and add sensor data..
			byte* inarray = new byte[lend];
			RegEnumValue(keySensors, vindex, name, &(len = 255), NULL, NULL, inarray, &lend);
			//len = 255;
			vector<fan_point> curve;
			for (UINT i = 0; i < lend; i += 2) {
				curve.push_back({ inarray[i], inarray[i + 1] });
			}
			// Find sensor...
			fan->sensors[sid] = curve;
			delete[] inarray;
			continue;
		}
		if (sscanf_s(name, "Sensor-%hd-%hd", &sid, &fid) == 2) {
			// find fan block...
			fan_block* fan = FindFanBlock(fid);
			if (!fan) {
				prof.fanControls.push_back({ fid });
				fan = &prof.fanControls.back();
			}
			// Load sensor data...
			byte* inarray = new byte[lend];
			RegEnumValue(keySensors, vindex, name, &(len = 255), NULL, NULL, inarray, &lend);
			//len = 255;
			// ToDo: Find correct SID here!
			//sen_block cSen{ MAKEWORD(255,sid) };
			vector<fan_point> curve;
			for (UINT i = 0; i < lend; i += 2) {
				curve.push_back({ inarray[i], inarray[i + 1] });
			}
			// Find sensor...
			fan->sensors[sid + 0xff00] = curve;
			delete[] inarray;
			continue;
		}
		if (sscanf_s(name, "SensorName-%hd-%hd", &sid, &fid) == 2) {
			byte* inarray = new byte[lend];
			RegEnumValue(keySensors, vindex, name, &(len = 255), NULL, NULL, inarray, &lend);
			//len = 255;
			sensors.emplace(MAKEWORD(fid, sid), (char*)inarray);
			delete[] inarray;
			continue;
		}
		// old format, deprecated...
		if (sscanf_s(name, "SensorName-%hd", &sid) == 1) {
			byte* inarray = new byte[lend];
			RegEnumValue(keySensors, vindex, name, &(len = 255), NULL, NULL, inarray, &lend);
			//len = 255;
			sensors.emplace(MAKEWORD(sid, 0xff), (char*)inarray);
			delete[] inarray;
		}
	}
	for (int vindex = 0; RegEnumValue(keyMain, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		if (sscanf_s(name, "Boost-%hd", &fid) == 1) { // Boost block
			byte* inarray = new byte[lend];
			//len++;
			RegEnumValue(keyMain, vindex, name, &(len = 255), NULL, NULL, inarray, &lend);
			boosts.push_back({ (byte)fid, inarray[0],*(USHORT*)(inarray + 1) });
			delete[] inarray;
		}
		//len = 255;
	}
	for (int vindex = 0; RegEnumValue(keyPowers, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		if (sscanf_s(name, "Power-%hd", &fid) == 1) { // Power names
			char* inarray = new char[lend + 1];
			//len++;
			RegEnumValue(keyPowers, vindex, name, &(len = 255), NULL, NULL, (byte*)inarray, &lend);
			powers.emplace((byte)fid, inarray);
			delete[] inarray;
		}
		//len = 255;
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
	for (auto i = prof.fanControls.begin(); i != prof.fanControls.end(); i++) {
		for (auto j = i->sensors.begin(); j != i->sensors.end(); j++) {
			name = "Fan-" + to_string(i->fanIndex) + "-" + to_string(j->first);
			byte* outdata = new byte[j->second.size() * 2];
			for (int k = 0; k < j->second.size(); k++) {
				outdata[2 * k] = (byte)j->second[k].temp;
				outdata[(2 * k)+1] = (byte)j->second[k].boost;
			}

			RegSetValueEx( keySensors, name.c_str(), 0, REG_BINARY, (BYTE*) outdata, (DWORD) j->second.size() * 2 );
			delete[] outdata;
		}
	}
	// save boosts...
	for (int i = 0; i < boosts.size(); i++) {
		byte outarray[sizeof(byte) + sizeof(USHORT)] = {0};
		name = "Boost-" + to_string(boosts[i].fanID);
		outarray[0] = boosts[i].maxBoost;
		*(USHORT *) (outarray + sizeof(byte)) = boosts[i].maxRPM;
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

void ConfigFan::SetBoostsAndNames(AlienFan_SDK::Control* acpi) {
	vector<fan_overboost>::iterator maxB;
	for (byte fID = 0; fID < acpi->boosts.size(); fID++)
		for (auto maxB = boosts.begin(); maxB != boosts.end(); maxB++)
			if (maxB->fanID == fID) {
				acpi->boosts[fID] = maxB->maxBoost;
				acpi->maxrpm[fID] = maxB->maxRPM;
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
	for (auto fn = lastProf->fanControls.begin(); fn != lastProf->fanControls.end(); fn++) {
		map<WORD, vector<fan_point>> newSenData;
		for (auto sen = fn->sensors.begin(); sen != fn->sensors.end(); sen++)
			if (HIBYTE(sen->first) == 0xff) {
				newSenData[acpi->sensors[LOBYTE(sen->first)].sid] = sen->second;
			}
			else
				newSenData[sen->first] = sen->second;
		fn->sensors = newSenData;
	}
}


