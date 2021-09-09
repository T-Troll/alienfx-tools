#include "ConfigHelper.h"
#include "resource.h"
#include <string>

ConfigHelper::ConfigHelper() {
	DWORD  dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER,
				   TEXT("SOFTWARE\\Alienfan"),
				   0,
				   NULL,
				   REG_OPTION_NON_VOLATILE,
				   KEY_ALL_ACCESS,
				   NULL,
				   &hKey1,
				   &dwDisposition);
	RegCreateKeyEx(HKEY_CURRENT_USER,
				   TEXT("SOFTWARE\\Alienfan\\Sensors"),
				   0,
				   NULL,
				   REG_OPTION_NON_VOLATILE,
				   KEY_ALL_ACCESS,
				   NULL,
				   &hKey2,
				   &dwDisposition);

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = IDI_ALIENFANGUI;
	niData.uFlags = NIF_ICON | NIF_MESSAGE;
	niData.uCallbackMessage = WM_APP + 1;
	niData.hIcon =
		(HICON) LoadImage(GetModuleHandle(NULL),
						  MAKEINTRESOURCE(IDI_ALIENFANGUI),
						  IMAGE_ICON,
						  GetSystemMetrics(SM_CXSMICON),
						  GetSystemMetrics(SM_CYSMICON),
						  LR_DEFAULTCOLOR);
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

void ConfigHelper::Load() {
	int size = 4, size_c = 4 * 16;

	RegGetValue(hKey1,
				NULL,
				TEXT("StartAtBoot"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&startWithWindows,
				(LPDWORD)&size);
	RegGetValue(hKey1,
				NULL,
				TEXT("StartMinimized"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&startMinimized,
				(LPDWORD)&size);
	RegGetValue(hKey1,
				NULL,
				TEXT("LastPowerStage"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&prof.powerStage,
				(LPDWORD)&size);
	RegGetValue(hKey1,
				NULL,
				TEXT("LastSensor"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&lastSelectedSensor,
				(LPDWORD)&size);
	RegGetValue(hKey1,
				NULL,
				TEXT("LastFan"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&lastSelectedFan,
				(LPDWORD)&size);
	RegGetValue(hKey1,
				NULL,
				TEXT("LastGPU"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&prof.GPUPower,
				(LPDWORD)&size);
	// Now load sensor mappings...
	unsigned vindex = 0;
	int ret = 0;
	char name[256];
	lastProf = &prof;
	do {
		DWORD len = 255, lend = 0;
		ret = RegEnumValueA(
			hKey2,
			vindex,
			name,
			&len,
			NULL,
			NULL,
			NULL,
			&lend
		);
		if (ret == ERROR_SUCCESS) {
			short sid, fid;
			unsigned ret2 = sscanf_s(name, "Sensor-%hd-%hd", &sid, &fid);
			if (ret2 == 2) { // Sensor-fan block
				temp_block* cSensor = FindSensor(sid);
				if (!cSensor) { // Need to add new sensor block
					cSensor = new temp_block{sid};
					prof.fanControls.push_back(*cSensor);
					delete cSensor;
					cSensor = &prof.fanControls.back();
				}
				// Now load and add fan data..
				byte* inarray = new byte[lend];
				RegEnumValueA(
					hKey2,
					vindex,
					name,
					&len,
					NULL,
					NULL,
					inarray,
					&lend
				);
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
}

void ConfigHelper::Save() {
	string name;
	DWORD dwDisposition;
	
	RegSetValueEx(
		hKey1,
		TEXT("StartAtBoot"),
		0,
		REG_DWORD,
		(BYTE*)&startWithWindows,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("StartMinimized"),
		0,
		REG_DWORD,
		(BYTE*)&startMinimized,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("LastPowerStage"),
		0,
		REG_DWORD,
		(BYTE*)&prof.powerStage,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("LastSensor"),
		0,
		REG_DWORD,
		(BYTE*)&lastSelectedSensor,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("LastFan"),
		0,
		REG_DWORD,
		(BYTE*)&lastSelectedFan,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("LastGPU"),
		0,
		REG_DWORD,
		(BYTE*)&prof.GPUPower,
		sizeof(DWORD)
	);
	if (prof.fanControls.size() > 0) {
		RegCloseKey(hKey2);
		RegDeleteTreeA(hKey1, "Sensors");
		RegCreateKeyEx(HKEY_CURRENT_USER,
					   TEXT("SOFTWARE\\Alienfan\\Sensors"),
					   0,
					   NULL,
					   REG_OPTION_NON_VOLATILE,
					   KEY_ALL_ACCESS,
					   NULL,
					   &hKey2,
					   &dwDisposition);
	}
	for (int i = 0; i < prof.fanControls.size(); i++) {
		for (int j = 0; j < prof.fanControls[i].fans.size(); j++) {
			name = "Sensor-" + to_string(prof.fanControls[i].sensorIndex) + "-" + to_string(prof.fanControls[i].fans[j].fanIndex);
			byte* outdata = new byte[prof.fanControls[i].fans[j].points.size() * 2];
			for (int k = 0; k < prof.fanControls[i].fans[j].points.size(); k++) {
				outdata[2 * k] = (byte)prof.fanControls[i].fans[j].points[k].temp;
				outdata[(2 * k)+1] = (byte)prof.fanControls[i].fans[j].points[k].boost;
			}

			RegSetValueExA(
				hKey2,
				name.c_str(),
				0,
				REG_BINARY,
				(BYTE*) outdata,
				(DWORD) prof.fanControls[i].fans[j].points.size() * 2
			);
			delete[] outdata;
		}
	}
}

