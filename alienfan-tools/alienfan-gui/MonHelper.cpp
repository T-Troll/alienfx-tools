#include "MonHelper.h"
#include <CommCtrl.h>
#include "resource.h"

DWORD WINAPI CMonProc(LPVOID);

MonHelper::MonHelper(HWND cDlg, HWND fanDlg, ConfigHelper* config, AlienFan_SDK::Control* acp) {
	dlg = cDlg;
	fDlg = fanDlg;
	conf = config;
	acpi = acp;
	oldPower = acpi->GetPower();
	if (oldPower != conf->lastProf->powerStage)
		acpi->SetPower(conf->lastProf->powerStage);
	stopEvent = CreateEvent(NULL, false, false, NULL);
	Start();
}

MonHelper::~MonHelper() {
	Stop();
	CloseHandle(stopEvent);
	if (oldPower != conf->lastProf->powerStage)
		acpi->SetPower(oldPower);
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
	//vector<int> senValues, fanValues, boostValues, boostSets;
	src->senValues.resize(src->acpi->HowManySensors());
	src->fanValues.resize(src->acpi->HowManyFans());
	src->boostValues.resize(src->acpi->HowManyFans());
	src->boostSets.resize(src->acpi->HowManyFans());

	HWND tempList = GetDlgItem(src->dlg, IDC_TEMP_LIST),
		fanList = GetDlgItem(src->dlg, IDC_FAN_LIST);

	while (WaitForSingleObject(src->stopEvent, 500) == WAIT_TIMEOUT) {
		// update values.....
		bool visible = IsWindowVisible(src->dlg);// IsIconic(src->dlg);
		bool needUpdate = false;

		// temps..
		for (int i = 0; i < src->acpi->HowManySensors(); i++) {
			int sValue = src->acpi->GetTempValue(i);
			if (sValue != src->senValues[i]) {
				src->senValues[i] = sValue;
				needUpdate = true;
				if (visible && tempList) {
					string name = to_string(sValue);
					ListView_SetItemText(tempList, i, 0, (LPSTR) name.c_str());
				}
			}
		}

		// fans...
		for (int i = 0; i < src->acpi->HowManyFans(); i++) {
			src->boostSets[i] = 0;
			src->boostValues[i] = src->acpi->GetFanValue(i);
			int rpValue = src->acpi->GetFanRPM(i);
			// Set max. rpm
			//if (rpValue > src->conf->maxRPM)
			//	src->conf->maxRPM = rpValue;
			if (visible && fanList && rpValue != src->fanValues[i]) {
				// Update RPM block...
				src->fanValues[i] = rpValue;
				needUpdate = true;
				string name = "Fan " + to_string(i + 1) + " (" + to_string(rpValue) + ")";
				ListView_SetItemText(fanList, i, 0, (LPSTR) name.c_str());
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
						if (src->senValues[sen->sensorIndex] <= fan->points[k].temp) {
							int tBoost = fan->points[k - 1].boost +
								(fan->points[k].boost - fan->points[k - 1].boost) *
								(src->senValues[sen->sensorIndex] - fan->points[k - 1].temp) /
								(fan->points[k].temp - fan->points[k - 1].temp);
							tBoost = tBoost < 0 ? 0 : tBoost > 100 ? 100 : tBoost;
							if (fan->fanIndex < src->boostSets.size() && tBoost > src->boostSets[fan->fanIndex])
								src->boostSets[fan->fanIndex] = tBoost;
							break;
						}
					}
				}
			}
			// Now set if needed...
			for (int i = 0; i < src->acpi->HowManyFans(); i++)
				if (src->boostSets[i] != src->boostValues[i] || src->boostSets[i] == 100) {
					src->acpi->SetFanValue(i, src->boostSets[i]);
//#ifdef _DEBUG
//					string msg = "Boost for fan#" + to_string(i) + " changed to " + to_string(boostSets[i]) + "\n";
//					OutputDebugString(msg.c_str());
//#endif
				}
			if (visible && needUpdate && src->fDlg) {
				SendMessage(src->fDlg, WM_PAINT, 0, 0);
			}
		}

	}
	return 0;
}
