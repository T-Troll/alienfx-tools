#include "MonHelper.h"

DWORD WINAPI CMonProc(LPVOID);

MonHelper::MonHelper(ConfigHelper* config, AlienFan_SDK::Control* acp) {
	conf = config;
	acpi = acp;
	oldPower = acpi->GetPower();
	if (oldPower != conf->lastProf->powerStage)
		acpi->SetPower(conf->lastProf->powerStage);
	acpi->SetGPU(conf->lastProf->GPUPower);

	maxTemps.resize(acpi->HowManySensors());
	senValues.resize(acpi->HowManySensors());
	fanValues.resize(acpi->HowManyFans());
	boostValues.resize(acpi->HowManyFans());
	boostSets.resize(acpi->HowManyFans());
	for (int i = 0; i < acpi->HowManySensors(); i++) {
		maxTemps[i] = acpi->GetTempValue(i);
	}

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

	while (WaitForSingleObject(src->stopEvent, 500) == WAIT_TIMEOUT) {
		// update values.....

		// temps..
		for (int i = 0; i < src->acpi->HowManySensors(); i++) {
			src->senValues[i] = src->acpi->GetTempValue(i);
			if (src->senValues[i] > src->maxTemps[i])
				src->maxTemps[i] = src->senValues[i];
		}

		// fans...
		for (int i = 0; i < src->acpi->HowManyFans(); i++) {
			src->boostSets[i] = 0;
			src->boostValues[i] = src->acpi->GetFanValue(i);
			src->fanValues[i] = src->acpi->GetFanRPM(i);
		}

		// boosts..
		if (!src->conf->lastProf->powerStage) {
			// in manual mode only
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
		}

	}
	return 0;
}
