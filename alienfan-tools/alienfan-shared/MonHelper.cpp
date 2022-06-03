#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

extern AlienFan_SDK::Control* acpi;

MonHelper::MonHelper(ConfigFan* config) {
	conf = config;

	maxTemps.resize(acpi->HowManySensors());
	senValues.resize(acpi->HowManySensors());
	fanRpm.resize(acpi->HowManyFans());
	boostRaw.resize(acpi->HowManyFans());
	boostSets.resize(acpi->HowManyFans());
	fanSleep.resize(acpi->HowManyFans());

	Start();
}

MonHelper::~MonHelper() {
	Stop();
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
		if ((oldPower = acpi->GetPower()) != conf->lastProf->powerStage)
			acpi->SetPower(conf->lastProf->powerStage);
		acpi->SetGPU(conf->lastProf->GPUPower);
#ifdef _DEBUG
		OutputDebugString("Mon thread start.\n");
#endif
		monThread = new ThreadHelper(CMonProc, this, 500);
	}
}

void MonHelper::Stop() {
	if (monThread) {
#ifdef _DEBUG
		OutputDebugString("Mon thread stop.\n");
#endif
		delete monThread;
		monThread = NULL;
		if (oldPower != conf->lastProf->powerStage)
			acpi->SetPower(oldPower);
		if (!oldPower)
			// reset boost
			for (int i = 0; i < acpi->fans.size(); i++)
				acpi->SetFanValue(i, 0);
	}
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;

	// update values.....

	// temps..
	for (int i = 0; i < acpi->HowManySensors(); i++) {
		src->senValues[i] = acpi->GetTempValue(i);
		if (src->senValues[i] > src->maxTemps[i])
			src->maxTemps[i] = src->senValues[i];
	}

	// fans...
	for (int i = 0; i < acpi->HowManyFans(); i++) {
		src->boostSets[i] = -273;
		src->boostRaw[i] = acpi->GetFanValue(i, true);
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	// boosts..
	if (!src->conf->lastProf->powerStage) {
		// in manual mode only
		for (auto cIter = src->conf->lastProf->fanControls.begin(); cIter < src->conf->lastProf->fanControls.end(); cIter++) {
			for (auto fIter = cIter->fans.begin(); fIter < cIter->fans.end(); fIter++) {
				// Look for boost point for temp...
				for (int k = 1; k < fIter->points.size(); k++)
					if (src->senValues[cIter->sensorIndex] <= fIter->points[k].temp) {
						int tBoost = (fIter->points[k - 1].boost +
								((fIter->points[k].boost - fIter->points[k - 1].boost) *
								(src->senValues[cIter->sensorIndex] - fIter->points[k - 1].temp)) /
								(fIter->points[k].temp - fIter->points[k - 1].temp)) * acpi->boosts[fIter->fanIndex] / 100;
						if (tBoost > src->boostSets[fIter->fanIndex])
							src->boostSets[fIter->fanIndex] = tBoost;
						break;
					}
			}
		}
		// Now set if needed...
		for (int i = 0; i < acpi->HowManyFans(); i++)
			if (src->boostSets[i] >= 0 && !src->fanSleep[i]) {
				// Check overboost tricks...
				if (src->boostRaw[i] < 100 && src->boostSets[i] > 100) {
					acpi->SetFanValue(i, 100, true);
					src->fanSleep[i] = 6;
					DebugPrint("Overboost started, locked for 3 sec!\n");
				} else
					if (src->boostSets[i] != src->boostRaw[i] || src->boostSets[i] > 100) {
						if (src->boostRaw[i] > src->boostSets[i])
							src->boostSets[i] += 31 * (src->boostRaw[i] - src->boostSets[i]) / 32;
						acpi->SetFanValue(i, src->boostSets[i], true);
						//DebugPrint(("Boost for fan#" + to_string(i) + " changed from " + to_string(src->boostRaw[i])
						//	+ " to " + to_string(src->boostSets[i]) + "\n").c_str());
					}
			}
			else
				if (src->fanSleep[i])
					src->fanSleep[i]--;
	}

}
