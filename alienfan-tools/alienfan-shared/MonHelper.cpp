#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

MonHelper::MonHelper(ConfigFan* config, AlienFan_SDK::Control* acp) {
	conf = config;
	acpi = acp;
	if ((oldPower = acpi->GetPower()) != conf->lastProf->powerStage)
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

	Start();
}

MonHelper::~MonHelper() {
	Stop();

	if (oldPower != conf->lastProf->powerStage)
		acpi->SetPower(oldPower);
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
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
	}
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;

	// update values.....

	// temps..
	for (int i = 0; i < src->acpi->HowManySensors(); i++) {
		src->senValues[i] = src->acpi->GetTempValue(i);
		if (src->senValues[i] > src->maxTemps[i])
			src->maxTemps[i] = src->senValues[i];
	}

	// fans...
	for (int i = 0; i < src->acpi->HowManyFans(); i++) {
		src->boostSets[i] = -273;
		src->boostValues[i] = src->acpi->GetFanValue(i);
		src->fanValues[i] = src->acpi->GetFanRPM(i);
	}

	// boosts..
	if (!src->conf->lastProf->powerStage) {
		// in manual mode only
		for (auto cIter = src->conf->lastProf->fanControls.begin(); cIter < src->conf->lastProf->fanControls.end(); cIter++) {
			for (auto fIter = cIter->fans.begin(); fIter < cIter->fans.end(); fIter++) {
				// Look for boost point for temp...
				for (int k = 1; k < fIter->points.size(); k++)
					if (src->senValues[cIter->sensorIndex] <= fIter->points[k].temp) {
						int tBoost = fIter->points[k - 1].boost +
							((fIter->points[k].boost - fIter->points[k - 1].boost) *
								(src->senValues[cIter->sensorIndex] - fIter->points[k - 1].temp)) /
								(fIter->points[k].temp - fIter->points[k - 1].temp);
						if (tBoost > src->boostSets[fIter->fanIndex])
							src->boostSets[fIter->fanIndex] = tBoost;
						break;
					}
			}
		}
		// Now set if needed...
		for (int i = 0; i < src->acpi->HowManyFans(); i++)
			if (src->boostSets[i] >= 0 && (src->boostSets[i] != src->boostValues[i]) || src->boostSets[i] == 100) {
				src->acpi->SetFanValue(i, src->boostSets[i]);
//#ifdef _DEBUG
//					string msg = "Boost for fan#" + to_string(i) + " changed to " + to_string(boostSets[i]) + "\n";
//					OutputDebugString(msg.c_str());
//#endif
			}
	}

}
