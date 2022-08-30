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
		if (acpi->GetDeviceFlags() & DEV_FLAG_GMODE && (oldGmode = acpi->GetGMode()) != fan_conf->lastProf->gmode)
			acpi->SetGMode(fan_conf->lastProf->gmode);
		if (!fan_conf->lastProf->gmode && (oldPower = acpi->GetPower()) != fan_conf->lastProf->powerStage)
			acpi->SetPower(acpi->powers[fan_conf->lastProf->powerStage]);
		//acpi->SetGPU(fan_conf->lastProf->GPUPower);
#ifdef _DEBUG
		OutputDebugString("Mon thread start.\n");
#endif
		monThread = new ThreadHelper(CMonProc, this, 1000, THREAD_PRIORITY_BELOW_NORMAL);
	}
}

void MonHelper::Stop() {
	if (monThread) {
#ifdef _DEBUG
		OutputDebugString("Mon thread stop.\n");
#endif
		delete monThread;
		monThread = NULL;
		if (acpi->GetDeviceFlags() & DEV_FLAG_GMODE && oldGmode != fan_conf->lastProf->gmode)
			acpi->SetGMode(oldGmode);
		if (!oldGmode && oldPower != fan_conf->lastProf->powerStage)
			acpi->SetPower(acpi->powers[oldPower]);
		if (!oldGmode && !oldPower)
			// reset boost
			for (int i = 0; i < acpi->fans.size(); i++)
				acpi->SetFanBoost(i, 0);
	}
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;

	// update values.....

	// temps..
	for (int i = 0; i < acpi->sensors.size(); i++) {
		src->senValues[i] = acpi->GetTempValue(i);
		if (src->senValues[i] > src->maxTemps[i])
			src->maxTemps[i] = src->senValues[i];
	}

	// fans...
	for (int i = 0; i < acpi->fans.size(); i++) {
		src->boostSets[i] = 0;
		src->boostRaw[i] = acpi->GetFanBoost(i, true);
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	// boosts..
	if (!fan_conf->lastProf->powerStage && !fan_conf->lastProf->gmode) {
		// in manual mode only
		for (auto cIter = fan_conf->lastProf->fanControls.begin(); cIter < fan_conf->lastProf->fanControls.end(); cIter++) {
			if (cIter->sensorIndex < src->senValues.size())
				for (auto fIter = cIter->fans.begin(); fIter < cIter->fans.end(); fIter++) {
					// Look for boost point for temp...
					for (int k = 1; k < fIter->points.size(); k++)
						if (src->senValues[cIter->sensorIndex] <= fIter->points[k].temp) {
							int cBoost = (fIter->points[k - 1].boost +
									((fIter->points[k].boost - fIter->points[k - 1].boost) *
									(src->senValues[cIter->sensorIndex] - fIter->points[k - 1].temp)) /
									(fIter->points[k].temp - fIter->points[k - 1].temp)) * acpi->boosts[fIter->fanIndex] / 100;
							if (cBoost > src->boostSets[fIter->fanIndex])
								src->boostSets[fIter->fanIndex] = cBoost;
							src->senBoosts[cIter->sensorIndex][fIter->fanIndex] = cBoost * 100 / acpi->boosts[fIter->fanIndex];
							break;
						}
				}
		}
		// Now set if needed...
		for (int i = 0; i < acpi->fans.size(); i++)
			if (!src->fanSleep[i]) {
				// Check overboost tricks...
				if (src->boostRaw[i] < 90 && src->boostSets[i] > 100) {
					acpi->SetFanBoost(i, 100, true);
					src->fanSleep[i] = ((100 - src->boostRaw[i]) >> 3) + 2;
					DebugPrint(("Overboost started, fan " + to_string(i) + " locked for " + to_string(src->fanSleep[i]) + " tacts(old "
						+ to_string(src->boostRaw[i]) + ", new " + to_string(src->boostSets[i]) +")!\n").c_str());
				} else
					if (src->boostSets[i] != src->boostRaw[i] /*|| src->boostSets[i] > 100*/) {
						if (src->boostRaw[i] > src->boostSets[i])
							src->boostSets[i] += 15 * ((src->boostRaw[i] - src->boostSets[i]) >> 4);

						if (acpi->GetSystemID() == 3200 && src->boostSets[i] > 20 && src->boostRaw[i] <= 20) { // RPM stuck override
							acpi->SetGMode(true);
							acpi->SetGMode(false);
						}

						acpi->SetFanBoost(i, src->boostSets[i], true);

						//DebugPrint(("Boost for fan#" + to_string(i) + " changed from " + to_string(src->boostRaw[i])
						//	+ " to " + to_string(src->boostSets[i]) + "\n").c_str());
					}
			}
			else
				src->fanSleep[i]--;
	}

}
