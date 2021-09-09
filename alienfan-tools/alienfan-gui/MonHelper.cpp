#include "MonHelper.h"
#include <CommCtrl.h>
#include "resource.h"

DWORD WINAPI CMonProc(LPVOID);

MonHelper::MonHelper(HWND cDlg, HWND fanDlg, ConfigHelper* config, AlienFan_SDK::Control* acp) {
	dlg = cDlg;
	fDlg = fanDlg;
	conf = config;
	acpi = acp;
	stopEvent = CreateEvent(NULL, false, false, NULL);
	Start();
}

MonHelper::~MonHelper() {
	Stop();
	CloseHandle(stopEvent);
}

void MonHelper::Start() {
		// start thread...
	if (!dwHandle) {
#ifdef _DEBUG
		OutputDebugString("Mon thread start.\n");
#endif
		ResetEvent(stopEvent);
		dwHandle = CreateThread(NULL, 0, CMonProc, this, 0, NULL);
	}
}

void MonHelper::Stop() {
	if (dwHandle) {
#ifdef _DEBUG
		OutputDebugString("Mon thread stop.\n");
#endif
		SetEvent(stopEvent);
		WaitForSingleObject(dwHandle, 1500);
		CloseHandle(dwHandle);
		dwHandle = 0;
	}
}

DWORD WINAPI CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;
	vector<int> senValues, fanValues, boostValues, boostSets;
	senValues.resize(src->acpi->HowManySensors());
	fanValues.resize(src->acpi->HowManyFans());
	boostValues.resize(src->acpi->HowManyFans());
	boostSets.resize(src->acpi->HowManyFans());

	HWND tempList = GetDlgItem(src->dlg, IDC_TEMP_LIST),
		fanList = GetDlgItem(src->dlg, IDC_FAN_LIST);

	while (WaitForSingleObject(src->stopEvent, 500) == WAIT_TIMEOUT) {
		// update values.....
		bool visible = IsWindowVisible(src->dlg);// IsIconic(src->dlg);

		if (src->acpi->GetPower() != src->conf->lastProf->powerStage)
			src->acpi->SetPower(src->conf->lastProf->powerStage);

		// temps..
		for (int i = 0; i < src->acpi->HowManySensors(); i++) {
			int sValue = src->acpi->GetTempValue(i);
			if (sValue != senValues[i]) {
				senValues[i] = sValue;
				if (visible && tempList) {
					string name = to_string(sValue);
					ListView_SetItemText(tempList, i, 0, (LPSTR) name.c_str());
				}
			}
		}

		// fans...
		for (int i = 0; i < src->acpi->HowManyFans(); i++) {
			boostSets[i] = 0;
			boostValues[i] = src->acpi->GetFanValue(i);
			if (visible && fanList) {
				int rpValue = src->acpi->GetFanRPM(i);
				if (rpValue != fanValues[i]) {
					// Update RPM block...
					fanValues[i] = rpValue;
					string name = "Fan " + to_string(i + 1) + " (" + to_string(rpValue) + ")";
					ListView_SetItemText(fanList, i, 0, (LPSTR) name.c_str());
				}
			}
		}

		// boosts..
		if (!src->conf->lastProf->powerStage) {
			// in manual mode, can set...
			for (int i = 0; i < src->conf->lastProf->fanControls.size(); i++) {
				temp_block* sen = &src->conf->lastProf->fanControls[i];
				for (int j = 0; j < sen->fans.size(); j++) {
					fan_block* fan = &sen->fans[j];
					// Look for boost point for temp...
					for (int k = 1; k < fan->points.size(); k++) {
						if (senValues[sen->sensorIndex] <= fan->points[k].temp) {
							int tBoost = fan->points[k - 1].boost +
								(fan->points[k].boost - fan->points[k - 1].boost) *
								(senValues[sen->sensorIndex] - fan->points[k - 1].temp) /
								(fan->points[k].temp - fan->points[k - 1].temp);
							tBoost = tBoost < 0 ? 0 : tBoost > 100 ? 100 : tBoost;
							if (fan->fanIndex < boostSets.size() && tBoost > boostSets[fan->fanIndex])
								boostSets[fan->fanIndex] = tBoost;
							break;
						}
					}
				}
			}
			// Now set if needed...
			for (int i = 0; i < src->acpi->HowManyFans(); i++)
				if (boostSets[i] != boostValues[i]) {
					src->acpi->SetFanValue(i, boostSets[i]);
//#ifdef _DEBUG
//					string msg = "Boost for fan#" + to_string(i) + " changed to " + to_string(boostSets[i]) + "\n";
//					OutputDebugString(msg.c_str());
//#endif
					if (visible && src->fDlg && i == src->conf->lastSelectedFan) {
						SendMessage(src->fDlg, WM_PAINT, 0, 0);
					}
				}
		}

	}
	return 0;
}
