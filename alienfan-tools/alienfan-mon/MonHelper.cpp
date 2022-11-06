#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

extern AlienFan_SDK::Control* acpi;
extern ConfigFan* fan_conf;

MonHelper::MonHelper(ConfigFan* config) {
	maxTemps.resize(acpi->sensors.size());
	senValues.resize(acpi->sensors.size());
	senBoosts.resize(acpi->sensors.size());
	for (auto i = senBoosts.begin(); i < senBoosts.end(); i++)
		i->resize(acpi->fans.size());
	fanRpm.resize(acpi->fans.size());
	boostRaw.resize(acpi->fans.size());
	boostSets.resize(acpi->fans.size());
	fanSleep.resize(acpi->fans.size());

	Start();
}

MonHelper::~MonHelper() {
	Stop();
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
		oldGmode = acpi->GetDeviceFlags() & DEV_FLAG_GMODE ? acpi->GetGMode() : 0;
		if (!oldGmode && oldPower < 0)
			oldPower = acpi->GetPower();
		if (oldGmode != fan_conf->lastProf->gmode) {
			SetCurrentGmode(fan_conf->lastProf->gmode);
		}
		monThread = new ThreadHelper(CMonProc, this, 750, THREAD_PRIORITY_BELOW_NORMAL);
#ifdef _DEBUG
		OutputDebugString("Mon thread start.\n");
#endif
	}
}

void MonHelper::Stop() {
	if (monThread) {
		delete monThread;
		monThread = NULL;
		if (acpi->GetDeviceFlags() & DEV_FLAG_GMODE && oldGmode != fan_conf->lastProf->gmode)
			SetCurrentGmode(oldGmode);
		if (!oldGmode && oldPower >= 0) {
			acpi->SetPower(acpi->powers[oldPower]);
			if (!oldPower)
				// reset boost
				for (int i = 0; i < acpi->fans.size(); i++)
					acpi->SetFanBoost(i, 0);
		}
#ifdef _DEBUG
		OutputDebugString("Mon thread stop.\n");
#endif
	}
}

void MonHelper::SetCurrentGmode(WORD newMode) {
	if (acpi->GetDeviceFlags() & DEV_FLAG_GMODE && acpi->GetGMode() != newMode) {
		if (acpi->GetSystemID() == 2933 && newMode) // G5 5510 fix
			acpi->SetPower(acpi->powers[1]);
		acpi->SetGMode(newMode);
		if (!newMode)
			acpi->SetPower(acpi->powers[fan_conf->lastProf->powerStage]);
	}
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;
	bool modified = false;
	// let's check power...
	if (!fan_conf->lastProf->gmode && acpi->GetPower() != fan_conf->lastProf->powerStage)
		acpi->SetPower(acpi->powers[fan_conf->lastProf->powerStage]);
	// update values:
	// temps..
	for (int i = 0; i < acpi->sensors.size(); i++) {
		int temp = acpi->GetTempValue(i);
		if (temp != src->senValues[i]) {
			modified = true;
			src->senValues[i] = temp;
			src->maxTemps[i] = max(src->maxTemps[i], temp);
		}
	}

	// fans...
	for (int i = 0; i < acpi->fans.size(); i++) {
		src->boostSets[i] = 0;
		src->boostRaw[i] = acpi->GetFanBoost(i, true);
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	// boosts..
	if (modified && !fan_conf->lastProf->powerStage && !fan_conf->lastProf->gmode) {
		// in manual mode only
		for (auto cIter = fan_conf->lastProf->fanControls.begin(); cIter < fan_conf->lastProf->fanControls.end(); cIter++) {
			if (cIter->sensorIndex < src->senValues.size())
				for (auto fIter = cIter->fans.begin(); fIter < cIter->fans.end(); fIter++) {
					// Look for boost point for temp...
					int cBoost = fIter->points.back().boost;
					for (int k = 1; k < fIter->points.size(); k++)
						if (src->senValues[cIter->sensorIndex] <= fIter->points[k].temp) {
							cBoost = (fIter->points[k - 1].boost +
								((fIter->points[k].boost - fIter->points[k - 1].boost) *
									(src->senValues[cIter->sensorIndex] - fIter->points[k - 1].temp)) /
								(fIter->points[k].temp - fIter->points[k - 1].temp));
							break;
						}
					if (cBoost > src->boostSets[fIter->fanIndex])
						src->boostSets[fIter->fanIndex] = cBoost;
					src->senBoosts[cIter->sensorIndex][fIter->fanIndex] = cBoost;
				}
		}
		// Now set if needed...
		for (int i = 0; i < acpi->fans.size(); i++)
			if (!src->fanSleep[i]) {
				int rawBoost = src->boostSets[i] * acpi->boosts[i] / 100;
				// Check overboost tricks...
				if (src->boostRaw[i] < 90 && rawBoost > 100) {
					acpi->SetFanBoost(i, 100, true);
					src->fanSleep[i] = ((100 - src->boostRaw[i]) >> 3) + 2;
					DebugPrint(("Overboost started, fan " + to_string(i) + " locked for " + to_string(src->fanSleep[i]) + " tacts(old "
						+ to_string(src->boostRaw[i]) + ", new " + to_string(rawBoost) +")!\n").c_str());
				} else
					if (rawBoost != src->boostRaw[i] /*|| src->boostSets[i] > 100*/) {
						if (src->boostRaw[i] > rawBoost)
							rawBoost += 15 * ((src->boostRaw[i] - rawBoost) >> 4);
						// fan RPM stuck patch v2
						//if (acpi->GetSystemID() == 3200 && src->boostRaw[i] > 50) {
						//	int pct = acpi->GetFanPercent(i) << 3;
						//	if (pct > 105 || pct < src->boostRaw[i]) {
						//		acpi->SetGMode(true);
						//		Sleep(300);
						//		acpi->SetGMode(false);
						//		acpi->SetPower((byte)fan_conf->lastProf->powerStage);
						//		DebugPrint("RPM fix engaged!\n");
						//	}
						//}

						acpi->SetFanBoost(i, rawBoost, true);

						//DebugPrint(("Boost for fan#" + to_string(i) + " changed from " + to_string(src->boostRaw[i])
						//	+ " to " + to_string(src->boostSets[i]) + "\n").c_str());
					}
			}
			else
				src->fanSleep[i]--;
	}

}
