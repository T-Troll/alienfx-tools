#include "FXHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

extern ConfigHandler* conf;

DWORD WINAPI CLightsProc(LPVOID param);

FXHelper::FXHelper() {
	Start();
};
FXHelper::~FXHelper() {
	Stop();
};

AlienFX_SDK::afx_device* FXHelper::LocateDev(int pid) {
	for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++)
		if (conf->afx_dev.fxdevs[i].dev->GetPID() == pid)
			return &conf->afx_dev.fxdevs[i];
	return nullptr;
};

void FXHelper::SetGaugeLight(pair<DWORD,DWORD> id, int x, int max, bool grad, vector<AlienFX_SDK::afx_act> actions, double power, bool force)
{
	vector<AlienFX_SDK::afx_act> fAct{ actions.front() };
	if (grad) {
		double newPower = (double)x / max;
		fAct[0].r = (byte)((1.0 - newPower) * actions.front().r + newPower * actions.back().r);
		fAct[0].g = (byte)((1.0 - newPower) * actions.front().g + newPower * actions.back().g);
		fAct[0].b = (byte)((1.0 - newPower) * actions.front().b + newPower * actions.back().b);
	}
	else {
		if (((double)x) / max < power) {
			if (((double)x + 1) / max < power)
				fAct[0] = actions.back();
			else {
				double newPower = (power - ((double)x) / max) * max;
				fAct[0].r = (byte)((1.0 - newPower) * actions.front().r + newPower * actions.back().r);
				fAct[0].g = (byte)((1.0 - newPower) * actions.front().g + newPower * actions.back().g);
				fAct[0].b = (byte)((1.0 - newPower) * actions.front().b + newPower * actions.back().b);
			}
		}
	}
	SetLight(id.first, id.second, fAct, force);
}

void FXHelper::SetGroupLight(groupset* grp, vector<AlienFX_SDK::afx_act> actions, double power, bool force) {
	AlienFX_SDK::group* cGrp = conf->afx_dev.GetGroupById(grp->group);
	if (cGrp->lights.size()) {
		zonemap* zone = conf->FindZoneMap(grp->group);
		if (!grp->gauge || actions.size() < 2 || !zone) {
			for (auto i = cGrp->lights.begin(); i < cGrp->lights.end(); i++)
				SetLight(i->first, i->second, actions, force);
		}
		else {
			for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
				switch (grp->gauge) {
				case 1: // horizontal
					SetGaugeLight(t->light, t->x, zone->xMax, grp->gradient, actions, power, force);
					break;
				case 2: // vertical
					SetGaugeLight(t->light, t->y, zone->yMax, grp->gradient, actions, power, force);
					break;
				case 3: // diagonal
					SetGaugeLight(t->light, t->x + t->y, zone->xMax + zone->yMax, grp->gradient, actions, power, force);
					break;
				}
		}
	}
}


void FXHelper::TestLight(int did, int id, bool wp)
{
	vector<byte> opLights;

	for (auto lIter = conf->afx_dev.fxdevs[did].lights.begin(); lIter != conf->afx_dev.fxdevs[did].lights.end(); lIter++)
		if ((*lIter)->lightid != id && !((*lIter)->flags & ALIENFX_FLAG_POWER))
			opLights.push_back((byte)(*lIter)->lightid);

	bool dev_ready = false;
	for (int c_count = 0; c_count < 200 && !(dev_ready = conf->afx_dev.fxdevs[did].dev->IsDeviceReady()); c_count++)
		Sleep(20);
	if (!dev_ready) return;

	AlienFX_SDK::Colorcode c = wp ? conf->afx_dev.fxdevs[did].desc->white : AlienFX_SDK::Colorcode({0});
	conf->afx_dev.fxdevs[did].dev->SetMultiLights(&opLights, c);
	conf->afx_dev.fxdevs[did].dev->UpdateColors();

	if (id != -1) {
		conf->afx_dev.fxdevs[did].dev->SetColor(id, conf->testColor);
		conf->afx_dev.fxdevs[did].dev->UpdateColors();
	}

}

void FXHelper::ResetPower(int did)
{
	AlienFX_SDK::afx_device* dev = LocateDev(did);
	if (dev != NULL) {
		vector<AlienFX_SDK::act_block> act{ { (byte)63, {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64}, {AlienFX_SDK::AlienFX_A_Power, 3, 0x64}} } };
		dev->dev->SetPowerAction(&act);
	}
}

void FXHelper::SetCounterColor(EventData *data, bool force)
{

	blinkStage = !blinkStage;
	bool wasChanged = false;
	if (force) {
		DebugPrint("Forced Counter update initiated...\n");
		//eData = {101,101,101,101,200,101,101,-1,-1,-1};
	}

	vector<groupset> active = conf->activeProfile->lightsets;

	for (auto Iter = active.begin(); Iter != active.end(); Iter++) {
		vector<AlienFX_SDK::afx_act> actions;
		bool noDiff = true;
		int lVal = 0, cVal = 0;
		//AlienFX_SDK::afx_act from{ 99 }, fin{ 99 }, * gFin = NULL;
		if (Iter->fromColor && Iter->color.size()) {
			actions.push_back(conf->afx_dev.GetGroupById(Iter->group)->have_power && activeMode != MODE_AC && activeMode != MODE_CHARGE ? Iter->color.back() : Iter->color.front());
			actions.back().type = 0;
		}
		if (Iter->events[1].state) {
			// counter
			if (actions.empty())
				actions.push_back(Iter->events[1].from);
			switch (Iter->events[1].source) {
			case 0: lVal = eData.CPU; cVal = data->CPU; break;
			case 1: lVal = eData.RAM; cVal = data->RAM; break;
			case 2: lVal = eData.HDD; cVal = data->HDD; break;
			case 3: lVal = eData.GPU; cVal = data->GPU; break;
			case 4: lVal = eData.NET; cVal = data->NET; break;
			case 5: lVal = eData.Temp; cVal = data->Temp; break;
			case 6: lVal = eData.Batt; cVal = data->Batt; break;
			case 7: lVal = eData.Fan; cVal = data->Fan; break;
			case 8: lVal = eData.PWR; cVal = data->PWR; break;
			}

			if (force || (lVal != cVal && (lVal > Iter->events[1].cut || cVal > Iter->events[1].cut))) {
				//if (from.type)
				//	from = Iter->perfs[0].from;
				noDiff = false;
				Iter->events[1].coeff = cVal > Iter->events[1].cut ? (cVal - Iter->events[1].cut) / (100.0 - Iter->events[1].cut) : 0.0;
				if (Iter->gauge && !Iter->gradient)
					actions.push_back(Iter->events[1].to);
				else
					actions.push_back({ 0,0,0,
						(BYTE)(actions.front().r * (1 - Iter->events[1].coeff) + Iter->events[1].to.r * Iter->events[1].coeff),
						(BYTE)(actions.front().g * (1 - Iter->events[1].coeff) + Iter->events[1].to.g * Iter->events[1].coeff),
						(BYTE)(actions.front().b * (1 - Iter->events[1].coeff) + Iter->events[1].to.b * Iter->events[1].coeff) });
			}
		}
		if (Iter->events[2].state) {
			// indicator
			if (actions.empty())
				actions.push_back(Iter->events[2].from);
			int ccut = Iter->events[2].cut;
			bool blink = Iter->events[2].mode;
			switch (Iter->events[2].source) {
			case 0: lVal = eData.HDD; cVal = data->HDD; break;
			case 1: lVal = eData.NET; cVal = data->NET; break;
			case 2: lVal = eData.Temp - ccut; cVal = data->Temp - ccut; break;
			case 3: lVal = eData.RAM - ccut; cVal = data->RAM - ccut; break;
			case 4: lVal = eData.Batt - ccut; cVal = data->Batt - ccut; break;
			case 5: lVal = eData.KBD; cVal = data->KBD; break;
			}

			if (force || ((byte)(cVal > 0) + (byte)(lVal > 0)) == 1) {
				noDiff = false;
				if (cVal > 0) {
					actions.erase(actions.begin());
					actions.push_back(Iter->events[2].to);
				}
			} else
				if (blink && cVal > 0 ) {
					noDiff = false;
					if (blinkStage) {
						actions.erase(actions.begin());
						actions.push_back(Iter->events[2].to);
					}
				}
		}

		// check for change.
		if (noDiff)	continue;
		wasChanged = true;

		if (conf->afx_dev.GetGroupById(Iter->group)->have_power)
			if (activeMode == MODE_AC || activeMode == MODE_CHARGE) {
				actions.push_back(Iter->color[1]);
			} else {
				actions.insert(actions.begin(), Iter->color[0]);
			}

		SetGroupLight(&(*Iter), actions, Iter->events[1].state ? Iter->events[1].coeff : 0);
	}
	if (wasChanged) {
		QueryUpdate();
	}
	memcpy(&eData, data, sizeof(EventData));
}

void FXHelper::QueryUpdate(int did, bool force)
{
	if (unblockUpdates && conf->stateOn) {
		LightQueryElement newBlock{did, 0, force, true};
		modifyQuery.lock();
		lightQuery.push_back(newBlock);
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

bool FXHelper::SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force)
{
	if (unblockUpdates && conf->stateOn && !actions.empty()) {
		if (did) {
			LightQueryElement newBlock{ did, id, force, false, (byte)actions.size() };
			for (int i = 0; i < newBlock.actsize; i++)
				newBlock.actions[i] = actions[i];
			if (did && newBlock.actsize) {
				modifyQuery.lock();
				lightQuery.push_back(newBlock);
				modifyQuery.unlock();
				SetEvent(haveNewElement);
			}
		}
	}
	return true;
}

inline void FXHelper::RefreshMon()
{
	if (conf->enableMon && conf->GetEffect() == 1)
		SetCounterColor(&eData, true);
	//eData = EventData();
}

void FXHelper::ChangeState() {
	if (conf->SetStates()) {
		UnblockUpdates(false);

		for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++) {
			conf->afx_dev.fxdevs[i].dev->ToggleState(conf->finalBrightness, conf->afx_dev.GetMappings(), conf->finalPBState);
			if (conf->stateOn)
				switch (conf->afx_dev.fxdevs[i].dev->GetVersion()) {
				case API_L_ACPI: case API_L_V1: case API_L_V2: case API_L_V3: case API_L_V6: case API_L_V7:
					UnblockUpdates(true);
					Refresh();
					UnblockUpdates(false);
				}
		}
		UnblockUpdates(true);
	}
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Functions* dev) {
	if (!dev) {
		// let's find v5 dev...
		for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++) {
			if (conf->afx_dev.fxdevs[i].dev->GetVersion() == 5) {
				dev = conf->afx_dev.fxdevs[i].dev;
				break;
			}
		}
	}
	if (dev && dev->GetVersion() == 5) {
		AlienFX_SDK::afx_act c1{0,0,0,conf->activeProfile->effColor1.r,
			conf->activeProfile->effColor1.g,
			conf->activeProfile->effColor1.b},
			c2{0,0,0,conf->activeProfile->effColor2.r,
			conf->activeProfile->effColor2.g,
			conf->activeProfile->effColor2.b};
		if (conf->activeProfile->flags & PROF_GLOBAL_EFFECTS)
			dev->SetGlobalEffects((byte)conf->activeProfile->globalEffect,
								  (byte)conf->activeProfile->globalDelay, c1, c2);
		else
			dev->SetGlobalEffects(1, 0, c1, c2);
	}
}

//void FXHelper::Flush() {
//
//	if (!lightQuery.empty()) {
//		modifyQuery.lock();
//		deque<LightQueryElement>::iterator qIter = lightQuery.end()-1;
//		if (qIter->update)
//			qIter--;
//		while (qIter > lightQuery.begin() && !qIter->update) qIter--;
//		if (qIter > lightQuery.begin()) {
//			DebugPrint((to_string(qIter - lightQuery.begin()) + " elements flushed.\n").c_str());
//			lightQuery.erase(lightQuery.begin(), qIter);
//		}
//		modifyQuery.unlock();
//	}
//}

void FXHelper::UnblockUpdates(bool newState, bool lock) {
	if (lock)
		updateLock = !newState;
	if (!updateLock || lock) {
		unblockUpdates = newState;
		if (!unblockUpdates) {
			SetEvent(haveNewElement);
			while (updateThread && WaitForSingleObject(queryEmpty, 60000) == WAIT_TIMEOUT);
			DebugPrint("Lights pause on!\n");
		} else {
			DebugPrint("Lights pause off!\n");
		}
	}
}

size_t FXHelper::FillAllDevs(AlienFan_SDK::Control* acc) {
	conf->SetStates();
	conf->haveV5 = false;
	conf->afx_dev.AlienFXAssignDevices(acc ? acc->GetHandle() : NULL, conf->finalBrightness, conf->finalPBState);
	// global effects check
	for (int i=0; i < conf->afx_dev.fxdevs.size(); i++)
		if (conf->afx_dev.fxdevs[i].dev->GetVersion() == 5) {
			conf->haveV5 = true; break;
		}
	return conf->afx_dev.fxdevs.size();
}

void FXHelper::Start() {
	if (!updateThread) {
		DebugPrint("Light updates started.\n");

		stopQuery = CreateEvent(NULL, true, false, NULL);
		haveNewElement = CreateEvent(NULL, false, false, NULL);
		queryEmpty = CreateEvent(NULL, true, true, NULL);
		updateThread = CreateThread(NULL, 0, CLightsProc, this, 0, NULL);
		UnblockUpdates(true, true);
	}
}

void FXHelper::Stop() {
	if (updateThread) {
		DebugPrint("Light updates stopped.\n");

		UnblockUpdates(false, true);
		SetEvent(stopQuery);
		WaitForSingleObject(updateThread, 60000);
		lightQuery.clear();
		CloseHandle(updateThread);
		CloseHandle(stopQuery);
		CloseHandle(haveNewElement);
		CloseHandle(queryEmpty);
		updateThread = NULL;
	}
}

void FXHelper::Refresh(int forced)
{

#ifdef _DEBUG
	if (forced) {
		DebugPrint((string("Forced Refresh initiated in mode ") + to_string(forced) + "\n").c_str());
	}
#endif

	for (auto it = (*conf->active_set).begin(); it < (*conf->active_set).end(); it++) {
		RefreshOne(&(*it), forced, false);
	}
	if (!forced) RefreshMon();

	QueryUpdate(-1, forced == 2);
}

bool FXHelper::SetMode(int mode)
{
	int t = activeMode;
	activeMode = mode;
	for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++)
		conf->afx_dev.fxdevs[i].dev->powerMode = (activeMode == MODE_AC || activeMode == MODE_CHARGE);
	return t == activeMode;
}

bool FXHelper::RefreshOne(groupset* map, int force, bool update)
{
	vector<AlienFX_SDK::afx_act> actions;

	if (!conf->stateOn || !map)
		return false;

	if (map->color.size()) {
		actions = map->color;
	}

	if (!force && conf->enableMon && conf->GetEffect() == 1) {
		if (map->events[1].state || map->events[2].state)
			return false;
		if (map->events[0].state) {
			// use power event;
			if (!map->fromColor)
				actions = { map->events[0].from };
			switch (activeMode) {
			case MODE_BAT:
				actions = { map->events[0].to };
				break;
			case MODE_LOW:
				actions.resize(1);
				actions.push_back(map->events[0].to);
				actions.front().type = AlienFX_SDK::AlienFX_A_Pulse;
				actions.back().type = AlienFX_SDK::AlienFX_A_Pulse;
				break;
			case MODE_CHARGE:
				actions.resize(1);
				actions.push_back(map->events[0].to);
				actions.front().type = AlienFX_SDK::AlienFX_A_Morph;
				actions.back().type = AlienFX_SDK::AlienFX_A_Morph;
				break;
			}
		}
	}

	if (actions.size()) {
		SetGroupLight(map, actions, 0, force == 2);
		if (update)
			QueryUpdate(-1, force == 2);
	}
	return true;
}

void FXHelper::RefreshAmbient(UCHAR *img) {

	if (!unblockUpdates || conf->monDelay > 200) {
		DebugPrint("Ambient update skipped!\n");
		return;
	}

	UINT shift = 255 - conf->amb_shift, gridsize = LOWORD(conf->amb_grid) * HIWORD(conf->amb_grid);
	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});
	bool wasChanged = false;

	for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
		if (it->ambients.size()) {
			UINT r = 0, g = 0, b = 0, dsize = (UINT)it->ambients.size() * 255;
			for (auto cAmb = it->ambients.begin(); cAmb < it->ambients.end(); cAmb++) {
				if (cAmb - it->ambients.begin() < gridsize) {
					wasChanged = true;
					r += img[3 * *cAmb + 2];
					g += img[3 * *cAmb + 1];
					b += img[3 * *cAmb];
				}
				else {
					dsize -= 255;
				}
			}
			// Multilights and brightness correction...
			if (dsize) {
				actions[0].r = (BYTE)((r * shift) / dsize);
				actions[0].g = (BYTE)((g * shift) / dsize);
				actions[0].b = (BYTE)((b * shift) / dsize);
				SetGroupLight(&(*it), actions);
			}
		}
	if (wasChanged)
		QueryUpdate();
}

void FXHelper::RefreshHaptics(int *freq) {

	if (!unblockUpdates || conf->monDelay > 200) {
		DebugPrint("Haptics update skipped!\n");
		return;
	}

	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});
	bool wasChanged = false;

	for (auto mIter = conf->active_set->begin(); mIter < conf->active_set->end(); mIter++) {
		if (mIter->haptics.size()) {
			// Now for each freq block...
			unsigned from_r = 0, from_g = 0, from_b = 0, to_r = 0, to_g = 0, to_b = 0, cur_r = 0, cur_g = 0, cur_b = 0;
			double f_power = 0.0;
			for (auto fIter = mIter->haptics.begin(); fIter < mIter->haptics.end(); fIter++) {
				if (!fIter->freqID.empty() && fIter->hicut > fIter->lowcut) {
					double power = 0.0;

					// here need to check less bars...
					for (auto iIter = fIter->freqID.begin(); iIter < fIter->freqID.end(); iIter++)
						power += (freq[*iIter] > fIter->lowcut ? freq[*iIter] < fIter->hicut ?
							freq[*iIter] - fIter->lowcut : fIter->hicut - fIter->lowcut : 0);
					power = power / (fIter->freqID.size() * (fIter->hicut - fIter->lowcut));

					from_r += fIter->colorfrom.r;
					from_g += fIter->colorfrom.g;
					from_b += fIter->colorfrom.b;

					to_r += fIter->colorto.r;
					to_g += fIter->colorto.g;
					to_b += fIter->colorto.b;

					cur_r += (byte)sqrt((1.0 - power) * fIter->colorfrom.r * fIter->colorfrom.r + power * fIter->colorto.r * fIter->colorto.r);
					cur_g += (byte)sqrt((1.0 - power) * fIter->colorfrom.g * fIter->colorfrom.g + power * fIter->colorto.g * fIter->colorto.g);
					cur_b += (byte)sqrt((1.0 - power) * fIter->colorfrom.b * fIter->colorfrom.b + power * fIter->colorto.b * fIter->colorto.b);

					f_power += power;

					size_t groupsize = fIter->freqID.size();

					f_power /= groupsize;

					actions[0] = { 0,0,0,(byte)(cur_r / groupsize),(byte)(cur_g / groupsize),(byte)(cur_b / groupsize) };

					AlienFX_SDK::afx_act from{ 0,0,0,(byte)(from_r / groupsize),(byte)(from_g / groupsize),(byte)(from_b / groupsize) },
						to{ 0,0,0,(byte)(to_r / groupsize),(byte)(to_g / groupsize),(byte)(to_b / groupsize) };

					SetGroupLight(&(*mIter), actions, f_power);
					wasChanged = true;
				}
			}
		}
	}
	if (wasChanged)
		QueryUpdate();
}

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	LightQueryElement current;

	HANDLE waitArray[2]{src->stopQuery, src->haveNewElement};
	vector<deviceQuery> devs_query;

	// Power button state...
	vector<AlienFX_SDK::afx_act> pbstate;
	pbstate.resize(2);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	AlienFX_SDK::afx_device* dev = NULL;

	while (WaitForMultipleObjects(2, waitArray, false, 500) != WAIT_OBJECT_0) {
		if (!src->lightQuery.empty()) ResetEvent(src->queryEmpty);
		while (!src->lightQuery.empty()) {
			src->modifyQuery.lock();

			if (&src->lightQuery.front())
				current = src->lightQuery.front();

			src->lightQuery.pop_front();
			src->modifyQuery.unlock();

			//#ifdef _DEBUG
			//				char buff[2048];
			//				sprintf_s(buff, 2047, "New light update: (%d,%d),u=%d (%ld remains)...\n", current.did, current.lid, current.update, src->lightQuery.size());
			//				OutputDebugString(buff);
			//#endif
			if (current.update) {
				// update command
				if (conf->stateOn) {
					for (auto devQ=devs_query.begin(); devQ != devs_query.end(); devQ++) {
						if ((dev = src->LocateDev(devQ->devID)) && (current.did == (-1) || devQ->devID == current.did)) {
//#ifdef _DEBUG
//							char buff[2048];
//							sprintf_s(buff, 2047, "Starting update for %d, (%d lights, %d in query)...\n", devQ->devID, devQ->dev_query.size(), src->lightQuery.size());
//							OutputDebugString(buff);
//#endif
							if (dev->dev->GetVersion() == 5 && (conf->activeProfile->flags & PROF_GLOBAL_EFFECTS)) {
								DebugPrint("V5 global effect active!\n");
								src->UpdateGlobalEffect(dev->dev);
							}
							else
								if (devQ->dev_query.size()) {
									dev->dev->SetMultiColor(&devQ->dev_query, current.flags);
									dev->dev->UpdateColors();
								}
						}
						devQ->dev_query.clear();
					}
					//if (current.did == -1)
					//	devs_query.clear();
					if (src->lightQuery.size() > conf->afx_dev.GetMappings()->size() * 5) {
						conf->monDelay += 50;
						DebugPrint(("Query so big (" +
									to_string((int) src->lightQuery.size()) +
									"), delay increased to " +
									to_string(conf->monDelay) +
									" ms!\n"
									).c_str());
					}
				}
			} else {
				// set light
				if (dev = src->LocateDev(current.did)) {
					vector<AlienFX_SDK::afx_act> actions;
					WORD flags = conf->afx_dev.GetFlags(current.did, current.lid);
					for (int i = 0; i < current.actsize; i++) {
						AlienFX_SDK::afx_act action = current.actions[i];
						// gamma-correction...
						if (conf->gammaCorrection) {
							//action.r = (byte)(pow((float)action.r / 255.0, 2.2) * 255 + 0.5);
							//action.g = (byte)(pow((float)action.g / 255.0, 2.2) * 255 + 0.5);
							//action.b = (byte)(pow((float)action.b / 255.0, 2.2) * 255 + 0.5);
							action.r = ((UINT) action.r * action.r * dev->desc->white.r) / (255 * 255);
							action.g = ((UINT) action.g * action.g * dev->desc->white.g) / (255 * 255);
							action.b = ((UINT) action.b * action.b * dev->desc->white.b) / (255 * 255);
						}
						// Dimming...
						// For v0-v3 devices only, v4+ have hardware dimming
						if (dev->dev->GetVersion() < 4 && conf->stateDimmed && (!flags || conf->dimPowerButton)) {
							unsigned delta = 255 - conf->dimmingPower;
							action.r = ((UINT) action.r * delta) / 255;// >> 8;
							action.g = ((UINT) action.g * delta) / 255;// >> 8;
							action.b = ((UINT) action.b * delta) / 255;// >> 8;
						}
						//DebugPrint(("Light for #" + to_string(current.did) + ", ID " + to_string(current.lid) +
						//" to (" + to_string(action.r) + "," + to_string(action.g) + "," + to_string(action.b) + ")\n").c_str());
						actions.push_back(action);
					}

					// Is it power button?
					if (flags & ALIENFX_FLAG_POWER) {
						// Should we update it?
						if (!conf->block_power && current.actsize == 2 &&
							(current.flags || /*(current.flags && dev->GetVersion() < 4) ||*/
							 memcmp(&pbstate[0], &actions[0], sizeof(AlienFX_SDK::afx_act)) ||
							 memcmp(&pbstate[1], &actions[1], sizeof(AlienFX_SDK::afx_act)))) {

							DebugPrint((string("Power button set to ") +
										to_string(actions[0].r) + "-" +
										to_string(actions[0].g) + "-" +
										to_string(actions[0].b) + "/" +
										to_string(actions[1].r) + "-" +
										to_string(actions[1].g) + "-" +
										to_string(actions[1].b) +
										"\n").c_str());

							pbstate = actions;
							actions[0].type = actions[1].type = AlienFX_SDK::AlienFX_A_Power;
						} else {
							DebugPrint("Power button update skipped (blocked or same colors)\n");
							continue;
						}
					}

					// fill query....
					int qn;
					for (qn = 0; qn < devs_query.size(); qn++)
						if (devs_query[qn].devID == current.did) {
							devs_query[qn].dev_query.push_back({(byte)current.lid, actions});
								break;
						}
					if (qn == devs_query.size()) {
						// create new query!
						deviceQuery newQ{current.did, {{(byte) current.lid, actions}}};
						devs_query.push_back(newQ);
					}

				}
			}
		}
		//if (conf->monDelay > 200) {
		//	conf->monDelay -= 10;
		//	DebugPrint((string("Query empty, delay decreased to ") + to_string(conf->monDelay) + " ms!\n").c_str());
		//}
		conf->monDelay = 200;
		SetEvent(src->queryEmpty);
	}
	return 0;
}