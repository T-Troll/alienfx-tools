#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

extern ConfigFan* fan_conf;

//fan_profile* active = NULL;

MonHelper::MonHelper() {
	acpi = new AlienFan_SDK::Control();
	if (acpi->Probe(fan_conf->diskSensors)) {
		fan_conf->lastSelectedSensor = acpi->sensors.front().sid;
		fansize = (WORD)acpi->fans.size();
		powerSize = (WORD)acpi->powers.size();
		sensorSize = (WORD)acpi->sensors.size();
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
	lastBoost.clear();
	if (!powerMode) {
		DebugPrint("Mon: Boost reset\n");
		for (int i = 0; i < fansize; i++) {
			acpi->SetFanBoost(i, 0);
		}
	}
}

void MonHelper::Run() {
	if (!powerMode && !monThread) {
		monThread = new ThreadHelper(CMonProc, this, fan_conf->pollingRate, THREAD_PRIORITY_NORMAL/*THREAD_PRIORITY_BELOW_NORMAL*/);
		DebugPrint("Mon thread start.\n");
	}
}

void MonHelper::Finish() {
	delete monThread;
	monThread = NULL;
	DebugPrint("Mon thread stop.\n");
	ResetBoost();
}

void MonHelper::ToggleMode() {
	if (powerMode && monThread) {
		Finish();
		return;
	}
	if (!stopped)
		Run();
}

void MonHelper::Start() {
	SetOC();
	oldPower = GetPowerMode();
	SetCurrentMode();
	// start thread...
	Run();
	stopped = false;
	//SetCurrentMode();
}

void MonHelper::Stop() {
	if (monThread) {
		Finish();
	}
	if (fan_conf->keepSystem)
		SetCurrentMode(oldPower);
	stopped = true;
}

void MonHelper::SetCurrentMode(int newMode) {
	if (newMode < 0) {
		//DebugPrint("Mon: Switching from profile\n");
		newMode = fan_conf->lastProf->gmodeStage ? powerSize :
			fan_conf->acPower ? fan_conf->lastProf->powerStage : fan_conf->lastProf->powerStageDC;
	}
	if (newMode != powerMode) {
		if (newMode < powerSize) {
			if (powerMode == powerSize) {
				acpi->SetGMode(0);
				acpi->SetPower(0xa0);
			}
			ResetBoost();
			acpi->SetPower(acpi->powers[newMode]);
			//DebugPrint("Mon: Power mode switch from " + (powerMode == powerSize ? "G-mode" : to_string(powerMode)) + " to " + to_string(newMode) + "\n");
		}
		else {
			acpi->SetGMode(1);
			//DebugPrint("Mon: Power mode switch from " + to_string(powerMode) + " to G-mode\n");
		}
		powerMode = newMode;
		ToggleMode();
	}
#ifdef _DEBUG
	//else {
	//	DebugPrint("Mon: Same power mode\n");
	//}
#endif
}

byte MonHelper::GetFanPercent(byte fanID)
{
	auto bst = &fan_conf->boosts[fanID];
	WORD rpm = fanRpm[fanID];
	if (!bst->maxRPM) // no MaxRPM yet
		*bst = { 100, (WORD)max(acpi->GetMaxRPM(fanID), 1000) };
	if (bst->maxRPM < rpm) // current RPM higher, then BIOS high
		bst->maxRPM = rpm;
	return (rpm * 100) / bst->maxRPM;
}

int MonHelper::GetPowerMode() {
#ifdef _DEBUG
	int res = acpi->GetGMode() ? powerSize : acpi->GetPower();
	//DebugPrint("Mon: BIOS mode " + to_string(res) + ", current " + to_string(powerMode) + "\n");
	return res;
#else
	return acpi->GetGMode() ? powerSize : acpi->GetPower();
#endif // _DEBUG
}

void MonHelper::SetPowerMode(byte newMode) {
	if (newMode < powerSize)
		if (fan_conf->acPower)
			fan_conf->lastProf->powerStage = newMode;
		else
			fan_conf->lastProf->powerStageDC = newMode;
	fan_conf->lastProf->gmodeStage = newMode == powerSize;
	SetCurrentMode(newMode);
}

// I need this wrapper for buggy G-series BIOS which return error from time to time
int MonHelper::GetFanRPM(int fanID) {
	int res;
	return (res = acpi->GetFanRPM(fanID)) < 0 ? fanRpm[fanID] : res;
}

WORD MonHelper::GetSensorData(bool withRPM) {
	// update values:
	// temps..
	for (int i = 0; i < sensorSize; i++) {
		int temp = acpi->GetTempValue(i);
		WORD sid = acpi->sensors[i].sid;
		if (temp != senValues[sid]) {
			modified = true;
			senValues[sid] = temp;
			maxTemps[sid] = max(maxTemps[sid], temp);
		}
	}
	if (withRPM) {
		// fans...
		for (byte i = 0; i < fansize; i++) {
			fanRpm[i] = GetFanRPM(i);
		}
	}
	return GetPowerMode();
}

void CMonProc(LPVOID param) {
	MonHelper* src = (MonHelper*)param;
	AlienFan_SDK::Control* acpi = src->acpi;
	fan_profile* active = fan_conf->lastProf;
	src->modified = false;

#ifdef _DEBUG
	if (!active)
		DebugPrint("Mon: Zero fan profile!");
#endif

	//DebugPrint("Mon: Poll started\n");
	if (src->inControl && active) {
		// check power mode
		src->powerMode = src->GetSensorData(false);
		src->SetCurrentMode();

		if (!src->powerMode && src->modified) {
			//DebugPrint("Mon: Boost calc started\n");
			int cBoost;
			for (auto& cIter : active->fanControls) {
				// Check boost
				byte fanID = cIter.first;
				int curBoost = 0;
				for (auto fIter = cIter.second.begin(); fIter != cIter.second.end(); fIter++) {
					if (fIter->second.active) {
						sen_block* cur = &fIter->second;
						WORD senID = fIter->first;
						auto k = cur->points.begin() + 1;
						for (; k != cur->points.end() && src->senValues[senID] > k->temp; k++);
						auto ko = k - 1;
						cBoost = k == cur->points.end() || k->temp == ko->temp ? ko->boost :
							ko->boost + ((k->boost - ko->boost) * (src->senValues[senID] - ko->temp))
							/ (k->temp - ko->temp);
						src->senBoosts[fanID][senID] = cBoost;
						if (cBoost > curBoost) {
							curBoost = cBoost;
							src->lastBoost[fanID] = senID;
						}
					}
				}
				// Set boost
				int curBoostRaw = (int)round((fan_conf->GetFanScale(fanID) * curBoost) / 100.0);
				if (curBoostRaw < 101 || !src->fanSleep[fanID]) {
					byte boostOld = src->acpi->GetFanBoost(fanID);
					// Check overboost tricks...
					if (boostOld < 90 && curBoostRaw > 100) {
						curBoostRaw = 100;
						src->fanSleep[fanID] = ((100 - boostOld) >> 3) + 2;
						DebugPrint("Overboost started, fan " + to_string(fanID) + " locked for " + to_string(src->fanSleep[fanID]) + " tacts (old "
							+ to_string(boostOld) + ", new " + to_string(curBoostRaw) + ")!\n");
					}
					else
						src->fanSleep[fanID] = 0;
					if (curBoostRaw != boostOld) {
						int res = acpi->SetFanBoost(fanID, curBoostRaw);
						//DebugPrint("Boost for fan#" + to_string(fanID) + " changed from " + to_string(boostOld)
						//	+ " to " + to_string(curBoostRaw) + ", result " + to_string(res) + "\n");
					}
				}
				else
					--src->fanSleep[fanID];
			}
		}
	}
	//DebugPrint("Mon: poll ended\n");
}