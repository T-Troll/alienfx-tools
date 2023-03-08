#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

extern ConfigFan* fan_conf;

MonHelper::MonHelper() {
	if ((acpi = new AlienFan_SDK::Control())->Probe()) {
		fan_conf->lastSelectedSensor = acpi->sensors.front().sid;
		size_t fansize = acpi->fans.size();
		senBoosts.resize(fansize);
		fanRpm.resize(fansize);
		boostRaw.resize(fansize);
		lastBoost.resize(fansize);
		fanSleep.resize(fansize);
		oldPower = GetPowerMode();
		Start();
	}
}

MonHelper::~MonHelper() {
	Stop();
	SetCurrentMode(oldPower);
	delete acpi;
}

void MonHelper::ResetBoost() {
	if (!acpi->GetPower())
		for (int i = 0; i < acpi->fans.size(); i++) {
			acpi->SetFanBoost(i, 0);
			boostRaw[i] = 0;
		}
}

bool MonHelper::IsGMode() {
	return fan_conf->lastProf->powerStage == acpi->powers.size();
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
		ResetBoost();
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
		ResetBoost();
#ifdef _DEBUG
		OutputDebugString("Mon thread stop.\n");
#endif
	}
}

void MonHelper::SetCurrentMode(size_t newMode) {
	if (newMode < acpi->powers.size()) {
		if (acpi->GetGMode()) {
			acpi->SetGMode(false);
		}
		acpi->SetPower(acpi->powers[newMode]);
	}
	else {
		if (!acpi->GetGMode()) {
			if (acpi->GetSystemID() == 2933 || acpi->GetSystemID() == 3200) // m15R5 && G5 5510 fix
				acpi->SetPower(0xa0);
			acpi->SetGMode(true);
		}
	}
}

byte MonHelper::GetFanPercent(byte fanID)
{
	if (!fan_conf->boosts[fanID].maxRPM)
		fan_conf->boosts[fanID].maxRPM = acpi->GetMaxRPM(fanID);
	return (fanRpm[fanID] * 100) / fan_conf->boosts[fanID].maxRPM;
}

int MonHelper::GetPowerMode() {
	return acpi->GetGMode() ? (int)acpi->powers.size() : acpi->GetPower();
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;
	AlienFan_SDK::Control* acpi = src->acpi;
	bool modified = false;
	auto prof = fan_conf->lastProf;
	// Stop if no active profile - just for sure
	if (!prof)
		return;

	// update values:
	// temps..
	for (int i = 0; i < acpi->sensors.size(); i++) {
		int temp = acpi->GetTempValue(i);
		WORD sid = acpi->sensors[i].sid;
		// find or create maps...
		if (temp != src->senValues[sid]) {
			modified = true;
			src->senValues[sid] = temp;
			src->maxTemps[sid] = max(src->maxTemps[sid], temp);
		}
	}
	// fans...
	for (byte i = 0; i < acpi->fans.size(); i++) {
		//src->boostRaw[i] = acpi->GetFanBoost(i);
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	if (src->inControl) {
		// check power mode
		if (prof->powerStage != src->GetPowerMode())
			src->SetCurrentMode(prof->powerStage);
		if (!prof->powerStage && modified) {
			int cBoost;
			for (auto cIter = prof->fanControls.begin(); cIter != prof->fanControls.end(); cIter++) {
				// Check boost
				byte i = cIter->first;
				int curBoost = 0, boostCooked = (int)round(src->boostRaw[i] * 100.0 / fan_conf->GetFanScale(i));
				for (auto fIter = cIter->second.begin(); fIter != cIter->second.end(); fIter++) {
					sen_block* cur = &fIter->second;
					if (cur->active) {
						auto k = cur->points.begin() + 1;
						for (; k != cur->points.end() && src->senValues[fIter->first] >= k->temp; k++);
						if (k != cur->points.end())
							cBoost = (k - 1)->boost + ((k->boost - (k - 1)->boost) * (src->senValues[fIter->first] - (k - 1)->temp))
							/ (k->temp - (k - 1)->temp);
						else
							cBoost = cur->points.back().boost;
						if (cBoost > curBoost) {
							if (cBoost < boostCooked)
								cBoost += 7 * ((boostCooked - cBoost) >> 3);
							curBoost = cBoost;
							src->lastBoost[i] = fIter->first;
						}
						src->senBoosts[i][fIter->first] = cBoost;
					}
				}
				curBoost = (int)round((fan_conf->GetFanScale(i) * curBoost) / 100.0);
				if (!src->fanSleep[i] || curBoost <= 100) {
					byte boostOld = src->boostRaw[i];
					// Check overboost tricks...
					if (boostOld < 90 && curBoost > 100) {
						acpi->SetFanBoost(i, 100);
						src->boostRaw[i] = 100;
						src->fanSleep[i] = ((100 - boostOld) >> 3) + 2;
						DebugPrint("Overboost started, fan " + to_string(i) + " locked for " + to_string(src->fanSleep[i]) + " tacts (old "
							+ to_string(boostOld) + ", new " + to_string(curBoost) + ")!\n");
					}
					else
						if (curBoost != boostOld) {
							src->fanSleep[i] = 0;
							acpi->SetFanBoost(i, curBoost);
							src->boostRaw[i] = curBoost;
							//DebugPrint(("Boost for fan#" + to_string(i) + " changed from " + to_string(src->boostRaw[i])
							//	+ " to " + to_string(src->boostSets[i]) + "\n").c_str());
						}
				}
				else
					src->fanSleep[i]--;
			}
		}
	}
}
