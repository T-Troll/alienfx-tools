#include "ConfigHelper.h"
#include <string>

ConfigHelper::ConfigHelper() {
	
	RegCreateKeyEx(HKEY_CURRENT_USER,
				   TEXT("SOFTWARE\\Alienfan"),
				   0,
				   NULL,
				   REG_OPTION_NON_VOLATILE,
				   KEY_ALL_ACCESS,
				   NULL,
				   &hKey1,
				   NULL);
	RegCreateKeyEx(HKEY_CURRENT_USER,
				   TEXT("SOFTWARE\\Alienfan\\Sensors"),
				   0,
				   NULL,
				   REG_OPTION_NON_VOLATILE,
				   KEY_ALL_ACCESS,
				   NULL,
				   &hKey2,
				   NULL);

	Load();
}

ConfigHelper::~ConfigHelper() {
	Save();
	RegCloseKey(hKey1);
	RegCloseKey(hKey2);
}

temp_block* ConfigHelper::FindSensor(int id) {
	temp_block* res = NULL;
	if (id >= 0) {
		for (int i = 0; i < lastProf->fanControls.size(); i++)
			if (lastProf->fanControls[i].sensorIndex == id) {
				res = &lastProf->fanControls[i];
				break;
			}
	}
	return res;
}

fan_block* ConfigHelper::FindFanBlock(temp_block* sen, int id) {
	fan_block* res = 0;
	if (sen && id >= 0)
		for (int i = 0; i < sen->fans.size(); i++)
			if (sen->fans[i].fanIndex == id) {
				res = &sen->fans[i];
				break;
			}
	return res;
}

void ConfigHelper::GetReg(const char *name, DWORD *value, DWORD defValue) {
	DWORD size = sizeof(DWORD);
	if (RegGetValueA(hKey1, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigHelper::SetReg(const char *text, DWORD value) {
	RegSetValueEx( hKey1, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

void ConfigHelper::Load() {

	GetReg("StartAtBoot", &startWithWindows);
	GetReg("StartMinimized", &startMinimized);
	GetReg("LastPowerStage", &prof.powerStage);
	GetReg("LastSensor", &lastSelectedSensor);
	GetReg("LastFan", &lastSelectedFan);
	GetReg("LastGPU", &prof.GPUPower);
	//GetReg("MaxRPM", &maxRPM, 4000);

	// Now load sensor mappings...
	unsigned vindex = 0;
	int ret = 0;
	char name[256];
	lastProf = &prof;
	do {
		DWORD len = 255, lend = 0;
		if ((ret = RegEnumValueA( hKey2, vindex, name, &len, NULL, NULL, NULL, &lend )) == ERROR_SUCCESS) {
			short sid, fid; len++;
			if (sscanf_s(name, "Sensor-%hd-%hd", &sid, &fid) == 2) { // Sensor-fan block
				temp_block* cSensor = FindSensor(sid);
				if (!cSensor) { // Need to add new sensor block
					cSensor = new temp_block{sid};
					prof.fanControls.push_back(*cSensor);
					delete cSensor;
					cSensor = &prof.fanControls.back();
				}
				// Now load and add fan data..
				byte* inarray = new byte[lend];
				RegEnumValueA( hKey2, vindex, name, &len, NULL, NULL, inarray, &lend );
				fan_block cFan;
				cFan.fanIndex = fid;
				for (UINT i = 0; i < lend; i += 2) {
					fan_point cPos;
					cPos.temp = inarray[i];
					cPos.boost = inarray[i + 1];
					cFan.points.push_back(cPos);
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
		if ((ret = RegEnumValueA( hKey1, vindex, name, &len, NULL, NULL, NULL, &lend )) == ERROR_SUCCESS) {
			int fid; len++;
			if (sscanf_s(name, "Boost-%d", &fid) == 1) { // Boost block
				byte* inarray = new byte[lend];
				if (fid >= boosts.size())
					boosts.resize(fid + 1);
				RegEnumValueA( hKey1, vindex, name, &len, NULL, NULL, inarray, &lend );
				boosts[fid] = {inarray[0],*(USHORT*)(inarray+sizeof(byte))};
				delete[] inarray;
			}
		}
		vindex++;
	} while (ret == ERROR_SUCCESS);
}

void ConfigHelper::Save() {
	string name;
	
	SetReg("StartAtBoot", startWithWindows);
	SetReg("StartMinimized", startMinimized);
	SetReg("LastPowerStage", prof.powerStage);
	SetReg("LastSensor", lastSelectedSensor);
	SetReg("LastFan", lastSelectedFan);
	SetReg("LastGPU", prof.GPUPower);
	//SetReg("MaxRPM", maxRPM);

	if (prof.fanControls.size() > 0) {
		// clean old data
		RegCloseKey(hKey2);
		RegDeleteTreeA(hKey1, "Sensors");
		RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfan\\Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey2, NULL);
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

			RegSetValueExA( hKey2, name.c_str(), 0, REG_BINARY, (BYTE*) outdata, (DWORD) prof.fanControls[i].fans[j].points.size() * 2 );
			delete[] outdata;
		}
	}
	// save boosts..
	for (int i = 0; i < boosts.size(); i++) {
		byte outarray[sizeof(byte) + sizeof(USHORT)] = {0};
		name = "Boost-" + to_string(i);
		outarray[0] = boosts[i].maxBoost;
		*(USHORT *) (outarray + sizeof(byte)) = boosts[i].maxRPM;
		RegSetValueExA( hKey1, name.c_str(), 0, REG_BINARY, outarray, (DWORD) sizeof(byte) + sizeof(USHORT));
	}
}

void ConfigHelper::SetBoosts(AlienFan_SDK::Control *acpi) {
	for (int i = 0; i < acpi->HowManyFans(); i++)
		if (i < boosts.size())
			if (boosts[i].maxBoost)
				acpi->boosts[i] = boosts[i].maxBoost;
			else
				boosts[i] = {acpi->boosts[i], 4000};
		else
			boosts.push_back({acpi->boosts[i], 4000});
}

