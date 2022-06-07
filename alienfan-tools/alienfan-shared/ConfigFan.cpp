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

temp_block* ConfigFan::FindSensor(int id) {
	if (id >= 0) {
		for (int i = 0; i < lastProf->fanControls.size(); i++)
			if (lastProf->fanControls[i].sensorIndex == id) {
				return &lastProf->fanControls[i];
			}
	}
	return NULL;
}

fan_block* ConfigFan::FindFanBlock(temp_block* sen, int id) {
	if (sen && id >= 0)
		for (int i = 0; i < sen->fans.size(); i++)
			if (sen->fans[i].fanIndex == id) {
				return &sen->fans[i];
			}
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
	GetReg("LastSensor", &lastSelectedSensor);
	GetReg("LastFan", &lastSelectedFan);
	GetReg("LastGPU", &prof.GPUPower);
	GetReg("ObCheck", &obCheck);

	// set power values
	prof.powerStage = LOWORD(power);
	prof.gmode = HIWORD(power);

	// Now load sensor mappings...
	unsigned vindex = 0;
	int ret = 0;
	char name[256];

	do {
		DWORD len = 255, lend = 0;
		if ((ret = RegEnumValue( keySensors, vindex, name, &len, NULL, NULL, NULL, &lend )) == ERROR_SUCCESS) {
			short sid, fid; len++;
			if (sscanf_s(name, "Sensor-%hd-%hd", &sid, &fid) == 2) { // Sensor-fan block
				temp_block* cSensor = FindSensor(sid);
				if (!cSensor) { // Need to add new sensor block
					prof.fanControls.push_back({ sid });
					cSensor = &prof.fanControls.back();
				}
				// Now load and add fan data..
				byte* inarray = new byte[lend];
				RegEnumValue( keySensors, vindex, name, &len, NULL, NULL, inarray, &lend );
				fan_block cFan;
				cFan.fanIndex = fid;
				for (UINT i = 0; i < lend; i += 2) {
					cFan.points.push_back({ inarray[i], inarray[i + 1 ]});
				}
				cSensor->fans.push_back(cFan);
				delete[] inarray;
			}
		}
		vindex++;
	} while (ret == ERROR_SUCCESS);
	vindex = 0;
	do {
		DWORD len = 255, lend = 0;
		if ((ret = RegEnumValue( keyMain, vindex, name, &len, NULL, NULL, NULL, &lend )) == ERROR_SUCCESS) {
			int fid; len++;
			if (sscanf_s(name, "Boost-%d", &fid) == 1) { // Boost block
				byte* inarray = new byte[lend];
				RegEnumValue( keyMain, vindex, name, &len, NULL, NULL, inarray, &lend );
				boosts.push_back({(byte)fid, inarray[0],*(USHORT*)(inarray+1)});
				delete[] inarray;
			}
		}
		vindex++;
	} while (ret == ERROR_SUCCESS);
	vindex = 0;
	do {
		DWORD len = 255, lend = 0;
		if ((ret = RegEnumValue(keyPowers, vindex, name, &len, NULL, NULL, NULL, &lend)) == ERROR_SUCCESS) {
			int fid; len++;
			if (sscanf_s(name, "Power-%d", &fid) == 1) { // Power names
				char* inarray = new char[lend+1];
				RegEnumValue(keyPowers, vindex, name, &len, NULL, NULL, (byte *) inarray, &lend);
				powers.emplace(fid, inarray);
				delete[] inarray;
			}
		}
		vindex++;
	} while (ret == ERROR_SUCCESS);

	// If not enough boosts recorded....

}

void ConfigFan::Save() {
	string name;

	SetReg("StartAtBoot", startWithWindows);
	SetReg("StartMinimized", startMinimized);
	SetReg("LastPowerStage", MAKELPARAM(prof.powerStage, prof.gmode));
	SetReg("UpdateCheck", updateCheck);
	SetReg("LastSensor", lastSelectedSensor);
	SetReg("LastFan", lastSelectedFan);
	SetReg("LastGPU", prof.GPUPower);
	SetReg("ObCheck", obCheck);

	if (prof.fanControls.size() > 0) {
		// clean old data
		RegCloseKey(keySensors);
		RegDeleteTree(keyMain, "Sensors");
		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfan\\Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keySensors, NULL);
	}
	// save profiles..
	for (int i = 0; i < prof.fanControls.size(); i++) {
		for (int j = 0; j < prof.fanControls[i].fans.size(); j++) {
			name = "Sensor-" + to_string(prof.fanControls[i].sensorIndex) + "-"
				+ to_string(prof.fanControls[i].fans[j].fanIndex);
			byte* outdata = new byte[prof.fanControls[i].fans[j].points.size() * 2];
			for (int k = 0; k < prof.fanControls[i].fans[j].points.size(); k++) {
				outdata[2 * k] = (byte)prof.fanControls[i].fans[j].points[k].temp;
				outdata[(2 * k)+1] = (byte)prof.fanControls[i].fans[j].points[k].boost;
			}

			RegSetValueEx( keySensors, name.c_str(), 0, REG_BINARY, (BYTE*) outdata, (DWORD) prof.fanControls[i].fans[j].points.size() * 2 );
			delete[] outdata;
		}
	}
	// save boosts..
	for (int i = 0; i < boosts.size(); i++) {
		byte outarray[sizeof(byte) + sizeof(USHORT)] = {0};
		name = "Boost-" + to_string(boosts[i].fanID);
		outarray[0] = boosts[i].maxBoost;
		*(USHORT *) (outarray + sizeof(byte)) = boosts[i].maxRPM;
		RegSetValueEx( keyMain, name.c_str(), 0, REG_BINARY, outarray, (DWORD) sizeof(byte) + sizeof(USHORT));
	}
	// save powers..
	for (auto i = powers.begin(); i != powers.end(); i++) {
		name = "Power-" + to_string(i->first);
		RegSetValueEx(keyPowers, name.c_str(), 0, REG_SZ, (BYTE*)i->second.c_str(), (DWORD) i->second.length());
	}
}

void ConfigFan::SetBoosts(AlienFan_SDK::Control* acpi) {
	auto maxB = boosts.begin();
	for (byte fID = 0; fID < acpi->boosts.size(); fID++) {
		if ((maxB = find_if(boosts.begin(), boosts.end(),
			[fID](auto t) {
				return t.fanID == fID;
			})) != boosts.end()) {
			acpi->boosts[fID] = maxB->maxBoost;
			acpi->maxrpm[fID] = maxB->maxRPM;
		}
		//else
		//	// No boost found for fan, need to use ACPI data
		//	boosts.push_back({ fID, acpi->boosts[fID], 5000 });
	}
}


