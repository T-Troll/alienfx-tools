#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

extern AlienFan_SDK::Control* acpi;
extern ConfigFan* fan_conf;

MonHelper::MonHelper(ConfigFan* config) {
	//for (auto i = acpi->sensors.begin(); i != acpi->sensors.end(); i++) {
	//	senValues.emplace(i->sid, -1);
	//	maxTemps.emplace(i->sid, -1);
	//}
	size_t fansize = acpi->fans.size();
	senBoosts.resize(fansize);
	fanRpm.resize(fansize);
	boostRaw.resize(fansize);
	boostSets.resize(fansize);
	fanSleep.resize(fansize);

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
		fan_conf->lastProf->gmode = newMode;
		if (newMode && (acpi->GetSystemID() == 2933 || acpi->GetSystemID() == 3200)) // m15R5 && G5 5510 fix
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
		// find or create maps...
		if (temp != src->senValues[acpi->sensors[i].sid]) {
			modified = true;
			src->senValues[acpi->sensors[i].sid] = temp;
			src->maxTemps[acpi->sensors[i].sid] = max(src->maxTemps[acpi->sensors[i].sid], temp);
		}
	}

	// fans...
	for (int i = 0; i < acpi->fans.size(); i++) {
		//src->boostSets[i] = 0;
		src->boostRaw[i] = acpi->GetFanBoost(i, true);
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	// boosts..
	if (modified && !fan_conf->lastProf->powerStage && !fan_conf->lastProf->gmode) {
		// in manual mode only
		for (auto cIter = fan_conf->lastProf->fanControls.begin(); cIter != fan_conf->lastProf->fanControls.end(); cIter++) {
			src->boostSets[cIter->fanIndex] = 0;
			for (auto fIter = cIter->sensors.begin(); fIter != cIter->sensors.end(); fIter++) {
				int cBoost = fIter->second.points.back().boost;
				for (auto k = fIter->second.points.begin() + 1; k != fIter->second.points.end(); k++)
					if (src->senValues[fIter->first] <= k->temp) {
						if (k->temp != (k - 1)->temp)
							cBoost = ((k - 1)->boost + ((k->boost - (k - 1)->boost) * (src->senValues[fIter->first] - (k - 1)->temp))
								/ (k->temp - (k - 1)->temp));
						else
							cBoost = k->boost;
						break;
					}
				if (cBoost > src->boostSets[cIter->fanIndex])
					src->boostSets[cIter->fanIndex] = cBoost;
				src->senBoosts[cIter->fanIndex][fIter->first] = cBoost;
			}
			if (!src->fanSleep[cIter->fanIndex]) {
				int rawBoost = src->boostSets[cIter->fanIndex] * acpi->boosts[cIter->fanIndex] / 100;
				// Check overboost tricks...
				if (src->boostRaw[cIter->fanIndex] < 90 && rawBoost > 100) {
					acpi->SetFanBoost(cIter->fanIndex, 100, true);
					src->fanSleep[cIter->fanIndex] = ((100 - src->boostRaw[cIter->fanIndex]) >> 3) + 2;
					DebugPrint("Overboost started, fan " + to_string(cIter->fanIndex) + " locked for " + to_string(src->fanSleep[cIter->fanIndex]) + " tacts(old "
						+ to_string(src->boostRaw[cIter->fanIndex]) + ", new " + to_string(rawBoost) + ")!\n");
				}
				else
					if (rawBoost != src->boostRaw[cIter->fanIndex] /*|| src->boostSets[i] > 100*/) {
						if (src->boostRaw[cIter->fanIndex] > rawBoost)
							rawBoost += 15 * ((src->boostRaw[cIter->fanIndex] - rawBoost) >> 4);
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

						acpi->SetFanBoost(cIter->fanIndex, rawBoost, true);

						//DebugPrint(("Boost for fan#" + to_string(i) + " changed from " + to_string(src->boostRaw[i])
						//	+ " to " + to_string(src->boostSets[i]) + "\n").c_str());
					}
			}
			else
				src->fanSleep[cIter->fanIndex]--;
		}
	}

}
