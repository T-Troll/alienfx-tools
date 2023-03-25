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
		fansize = (WORD)acpi->fans.size();
		powerSize = (WORD)acpi->powers.size();
		senBoosts.resize(fansize);
		fanRpm.resize(fansize);
		boostRaw.resize(fansize);
		lastBoost.resize(fansize);
		fanSleep.resize(fansize);
		oldPower = powerMode = GetPowerMode();
		Start();
	}
}

MonHelper::~MonHelper() {
	Stop();
	delete acpi;
}

void MonHelper::ResetBoost() {
	boostRaw.assign(fansize, 0);
	lastBoost.assign(fansize, 0);
	if (!powerMode) {
		for (int i = 0; i < fansize; i++) {
			acpi->SetFanBoost(i, 0);
		}
	}
}

void MonHelper::SetProfilePower() {
	SetCurrentMode(fan_conf->lastProf->gmode_stage ? powerSize : fan_conf->lastProf->powerStage);
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
		SetProfilePower();
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
		if (fan_conf->keepSystem)
			SetCurrentMode(oldPower);
		ResetBoost();
#ifdef _DEBUG
		OutputDebugString("Mon thread stop.\n");
#endif
	}
}

void MonHelper::SetCurrentMode(WORD newMode) {
	int cmode = GetPowerMode();
	if (newMode != cmode) {
		switch (acpi->GetSystemID()) {
		case 2933: case 3200: case 0:
			acpi->SetPower(0xa0);
		}
		if (newMode < powerSize) {
			if (cmode == powerSize) {
				acpi->SetGMode(false);
			}
			acpi->SetPower(acpi->powers[newMode]);
		}
		else
			acpi->SetGMode(true);
		powerMode = newMode;
		//for (int i = 0; i < fansize; i++)
		//	boostRaw[i] = acpi->GetFanBoost(i);
		//boostRaw.assign(fansize, 0);
		ResetBoost();
	}
}

byte MonHelper::GetFanPercent(byte fanID)
{
	if (!fan_conf->boosts[fanID].maxRPM)
		fan_conf->boosts[fanID].maxRPM = acpi->GetMaxRPM(fanID);
	return (fanRpm[fanID] * 100) / fan_conf->boosts[fanID].maxRPM;
}

int MonHelper::GetPowerMode() {
	return acpi->GetGMode() ? powerSize : acpi->GetPower();
}

void MonHelper::SetPowerMode(WORD newMode) {
	if (!(fan_conf->lastProf->gmode_stage = (newMode == powerSize)))
		fan_conf->lastProf->powerStage = newMode;
	SetCurrentMode(newMode);
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;
	AlienFan_SDK::Control* acpi = src->acpi;
	bool modified = false;

	// update values:
	// temps..
	for (int i = 0; i < acpi->sensors.size(); i++) {
		int temp = acpi->GetTempValue(i);
		WORD sid = acpi->sensors[i].sid;
		if (temp != src->senValues[sid]) {
			modified = true;
			src->senValues[sid] = temp;
			src->maxTemps[sid] = max(src->maxTemps[sid], temp);
		}
	}
	// fans...
	for (byte i = 0; i < src->fansize; i++) {
		//src->boostRaw[i] = acpi->GetFanBoost(i);
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	fan_profile* active = fan_conf->lastProf; // protection from change profile

#ifdef _DEBUG
	if (!active)
		DebugPrint("Zero fan profile!");
#endif

	if (src->inControl && active) {
		// check power mode
		src->SetProfilePower();

		if (!src->powerMode && modified) {
			int cBoost;
			for (auto cIter = active->fanControls.begin(); cIter != active->fanControls.end(); cIter++) {
				// Check boost
				byte i = cIter->first;
				int curBoost = 0, boostCooked = (int)round(src->boostRaw[i] * 100.0 / fan_conf->GetFanScale(i));
				for (auto fIter = cIter->second.begin(); fIter != cIter->second.end(); fIter++) {
					sen_block* cur = &fIter->second;
					if (cur->active) {
						cBoost = cur->points.back().boost;
						auto k = cur->points.begin() + 1;
						for (; k != cur->points.end() && src->senValues[fIter->first] > k->temp; k++);
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
				// Set boost
				curBoost = (int)round((fan_conf->GetFanScale(i) * curBoost) / 100.0);
				if (curBoost < 100 || !src->fanSleep[i]) {
					byte boostOld = src->boostRaw[i];
					// Check overboost tricks...
					if (boostOld < 90 && curBoost > 100) {
						curBoost = 100;
						src->fanSleep[i] = ((100 - boostOld) >> 3) + 2;
						DebugPrint("Overboost started, fan " + to_string(i) + " locked for " + to_string(src->fanSleep[i]) + " tacts (old "
							+ to_string(boostOld) + ", new " + to_string(curBoost) + ")!\n");
					}
					if (curBoost != boostOld) {
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
