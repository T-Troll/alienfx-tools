#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

void CMonProc(LPVOID);

AlienFan_SDK::Control* acpi = NULL;
extern ConfigFan* fan_conf;

MonHelper::MonHelper() {
	if ((acpi = new AlienFan_SDK::Control())->Probe()) {
		fan_conf->SetSensorNames(acpi);
		size_t fansize = acpi->fans.size();
		senBoosts.resize(fansize);
		fanRpm.resize(fansize);
		boostRaw.resize(fansize);
		boostSets.resize(fansize);
		fanSleep.resize(fansize);
		Start();
	}
}

MonHelper::~MonHelper() {
	Stop();
	delete acpi;
	acpi = NULL;
}

void MonHelper::Start() {
	// start thread...
	if (!monThread) {
		oldGmode = acpi->GetGMode() > 0;
		if (!oldGmode && oldPower < 0)
			oldPower = acpi->GetPower();
		if (oldGmode != fan_conf->lastProf->gmode) {
			SetCurrentGmode(fan_conf->lastProf->gmode);
		}
		// Patch for R4
		if (!acpi->GetSystemID())
			acpi->SetPower(0xa0);
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
		if ((acpi->GetDeviceFlags() & DEV_FLAG_GMODE) && oldGmode != fan_conf->lastProf->gmode)
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
	if (acpi->GetDeviceFlags() & DEV_FLAG_GMODE) {
		if (acpi->GetGMode() != newMode) {
			fan_conf->lastProf->gmode = newMode;
			if (newMode && (acpi->GetSystemID() == 2933 || acpi->GetSystemID() == 3200)) // m15R5 && G5 5510 fix
				acpi->SetPower(0xa0);
			acpi->SetGMode(newMode);
			if (!newMode)
				acpi->SetPower(acpi->powers[fan_conf->lastProf->powerStage]);
		}
	}
	else
		fan_conf->lastProf->gmode = 0;
}

byte MonHelper::GetFanPercent(byte fanID)
{
	auto maxboost = fan_conf->boosts.find(fanID);
	return maxboost == fan_conf->boosts.end() ? acpi->GetFanPercent(fanID) : (fanRpm[fanID] * 100) / maxboost->second.maxRPM;
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
		src->boostSets[i] = 0;
		src->boostRaw[i] = acpi->GetFanBoost(i);
		src->fanRpm[i] = acpi->GetFanRPM(i);
		if (modified && !fan_conf->lastProf->powerStage && !fan_conf->lastProf->gmode) {
			auto cIter = fan_conf->lastProf->fanControls.find(i);
			if (cIter != fan_conf->lastProf->fanControls.end()) {
				// Set boost
				short fnum = cIter->first;
				for (auto fIter = cIter->second.begin(); fIter != cIter->second.end(); fIter++) {
					sen_block* cur = &fIter->second;
					if (cur->active && cur->points.size()) {
						int cBoost = cur->points.back().boost;
						for (auto k = cur->points.begin() + 1; k != cur->points.end(); k++)
							if (src->senValues[fIter->first] <= k->temp) {
								if (k->temp != (k - 1)->temp)
									cBoost = ((k - 1)->boost + ((k->boost - (k - 1)->boost) * (src->senValues[fIter->first] - (k - 1)->temp))
										/ (k->temp - (k - 1)->temp));
								else
									cBoost = k->boost;
								break;
							}
						if (cBoost > src->boostSets[fnum])
							src->boostSets[fnum] = cBoost;
						src->senBoosts[fnum][fIter->first] = cBoost;
					}
				}
				if (!src->fanSleep[fnum]) {
					int rawBoost = (int)round((fan_conf->GetFanScale((byte)fnum) * src->boostSets[fnum]) / 100.0);
					// Check overboost tricks...
					if (src->boostRaw[fnum] < 90 && rawBoost > 100) {
						acpi->SetFanBoost(fnum, 100/*, true*/);
						src->fanSleep[fnum] = ((100 - src->boostRaw[fnum]) >> 3) + 2;
						DebugPrint("Overboost started, fan " + to_string(fnum) + " locked for " + to_string(src->fanSleep[fnum]) + " tacts(old "
							+ to_string(src->boostRaw[fnum]) + ", new " + to_string(rawBoost) + ")!\n");
					}
					else
						if (rawBoost != src->boostRaw[fnum] /*|| src->boostSets[i] > 100*/) {
							if (src->boostRaw[fnum] > rawBoost)
								rawBoost += 15 * ((src->boostRaw[fnum] - rawBoost) >> 4);
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

							acpi->SetFanBoost(fnum, rawBoost/*, true*/);

							//DebugPrint(("Boost for fan#" + to_string(i) + " changed from " + to_string(src->boostRaw[i])
							//	+ " to " + to_string(src->boostSets[i]) + "\n").c_str());
						}
				}
				else
					src->fanSleep[fnum]--;
			}
		}
	}
}
