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
	if ((acpi = new AlienFan_SDK::Control())->Probe(fan_conf->diskSensors)) {
		fan_conf->lastSelectedSensor = acpi->sensors.front().sid;
		fansize = (WORD)acpi->fans.size();
		powerSize = (WORD)acpi->powers.size();
		sensorSize = (WORD)acpi->sensors.size();
		systemID = acpi->GetSystemID();
		Start();
	}
}

MonHelper::~MonHelper() {
	Stop();
	delete acpi;
}

void MonHelper::SetOC()
{
	if (fan_conf->ocEnable) {
		acpi->SetTCC(fan_conf->lastProf->currentTCC);
		acpi->SetXMP(fan_conf->lastProf->memoryXMP);
	}
}

void MonHelper::ResetBoost() {
	//boostRaw.clear();
	lastBoost.clear();
	if (!powerMode) {
		DebugPrint("Mon: Boost reset\n");
		for (int i = 0; i < fansize; i++) {
			acpi->SetFanBoost(i, 0);
		}
	}
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
		oldPower = powerMode = GetPowerMode();
		SetOC();
		SetCurrentMode();
		monThread = new ThreadHelper(CMonProc, this, fan_conf->pollingRate, THREAD_PRIORITY_NORMAL/*THREAD_PRIORITY_BELOW_NORMAL*/);
		DebugPrint("Mon thread start.\n");
	}
	else {
		Stop();
		Start();
	}
}

void MonHelper::Stop() {
	if (monThread) {
		delete monThread;
		monThread = NULL;
		ResetBoost();
		if (fan_conf->keepSystem)
			SetCurrentMode(oldPower);
		DebugPrint("Mon thread stop.\n");
	}
}

void MonHelper::SetCurrentMode(int newMode) {
	if (newMode < 0)
		newMode = fan_conf->lastProf->gmodeStage ? powerSize : 
			fan_conf->acPower ? fan_conf->lastProf->powerStage : fan_conf->lastProf->PowerStageDC;
	if (newMode != powerMode) {
		if (newMode < powerSize) {
			if (powerMode == powerSize) {
				acpi->SetGMode(0);
				acpi->SetPower(0xa0);
			}
			acpi->SetPower(acpi->powers[newMode]);
			ResetBoost();
			DebugPrint("Mon: Power mode switch from " + (powerMode == powerSize ? "G-mode" : to_string(powerMode)) + " to " + to_string(newMode) + "\n");
		}
		else {
			acpi->SetGMode(1);
			DebugPrint("Mon: Power mode switch from " + to_string(powerMode) + " to G-mode\n");
		}
		powerMode = newMode;
	}
}

byte MonHelper::GetFanPercent(byte fanID)
{
	auto bst = &fan_conf->boosts[fanID].maxRPM;
	if (!(*bst))
		*bst = acpi->GetMaxRPM(fanID);
	return (fanRpm[fanID] * 100) / (*bst);
}

int MonHelper::GetPowerMode() {
	//if (acpi->GetGMode()) {
	//	if (systemID != 4800) { // buggy G25 BIOS fix
	//		return powerSize;
	//	} else {
	//		int cmode = acpi->GetPower(true);
	//		if (cmode == 0xab || cmode < 0)
	//			return powerSize;
	//	}
	//}
	return acpi->GetGMode() ? powerSize : acpi->GetPower();
}

void MonHelper::SetPowerMode(byte newMode) {
	if (newMode < powerSize)
		if (fan_conf->acPower)
			fan_conf->lastProf->powerStage = newMode;
		else
			fan_conf->lastProf->PowerStageDC = newMode;
	fan_conf->lastProf->gmodeStage = newMode == powerSize;
	SetCurrentMode(newMode);
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*) param;
	AlienFan_SDK::Control* acpi = src->acpi;
	src->modified = false;
	fan_profile* active = NULL;

	// update values:
	// temps..
	for (int i = 0; i < src->sensorSize; i++) {
		int temp = acpi->GetTempValue(i);
		WORD sid = acpi->sensors[i].sid;
		if (temp != src->senValues[sid]) {
			src->modified = true;
			src->senValues[sid] = temp;
			src->maxTemps[sid] = max(src->maxTemps[sid], temp);
		}
	}
	// fans...
	for (byte i = 0; i < src->fansize; i++) {
		src->fanRpm[i] = acpi->GetFanRPM(i);
	}

	if (active != fan_conf->lastProf) {
		active = fan_conf->lastProf; // protection from change profile
		src->SetOC();
	}

#ifdef _DEBUG
	if (!active)
		DebugPrint("Zero fan profile!");
#endif

	if (src->inControl && active) {
		// check power mode
		src->powerMode = src->GetPowerMode();
		src->SetCurrentMode();

		if (!src->powerMode && src->modified) {
			int cBoost;
			for (auto cIter = active->fanControls.begin(); cIter != active->fanControls.end(); cIter++) {
				// Check boost
				byte i = cIter->first;
				int curBoost = 0;
				for (auto fIter = cIter->second.begin(); fIter != cIter->second.end(); fIter++) {
					if (fIter->second.active) {
						sen_block* cur = &fIter->second;
						WORD senID = fIter->first;
						auto k = cur->points.begin() + 1;
						for (; k != cur->points.end() && src->senValues[senID] > k->temp; k++);
						if (k != cur->points.end())
							cBoost = (k - 1)->boost + ((k->boost - (k - 1)->boost) * (src->senValues[senID] - (k - 1)->temp))
							/ (k->temp - (k - 1)->temp);
						else
							cBoost = cur->points.back().boost;
						src->senBoosts[i][senID] = cBoost;
						if (cBoost > curBoost) {
							curBoost = cBoost;
							src->lastBoost[i] = senID;
						}
					}
				}
				// Set boost
				int curBoostRaw = (int)round((fan_conf->GetFanScale(i) * curBoost) / 100.0);
				if (curBoostRaw < 101 || !src->fanSleep[i]) {
					byte boostOld = src->acpi->GetFanBoost(i);
					// Check overboost tricks...
					if (boostOld < 90 && curBoostRaw > 100) {
						curBoostRaw = 100;
						src->fanSleep[i] = ((100 - boostOld) >> 3) + 2;
						DebugPrint("Overboost started, fan " + to_string(i) + " locked for " + to_string(src->fanSleep[i]) + " tacts (old "
							+ to_string(boostOld) + ", new " + to_string(curBoostRaw) + ")!\n");
					}
					else
						src->fanSleep[i] = 0;
					if (curBoostRaw != boostOld) {
						int res = acpi->SetFanBoost(i, curBoostRaw);
						//src->boostRaw[i] = curBoostRaw;
						//DebugPrint("Boost for fan#" + to_string(i) + " changed from " + to_string(boostOld)
						//	+ " to " + to_string(curBoostRaw) + ", result " + to_string(res) + "\n");
					}
				}
				else
					--src->fanSleep[i];
			}
		}
	}
}
