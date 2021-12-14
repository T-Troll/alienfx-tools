#include "FXHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)  
#endif

DWORD WINAPI CLightsProc(LPVOID param);

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	afx_dev.LoadMappings();
};
FXHelper::~FXHelper() {
	Stop();
	//for (auto Iter = afx_dev.fxdevs.begin(); Iter < afx_dev.fxdevs.end(); Iter++)
	//	Iter->dev->AlienFXClose();
	afx_dev.SaveMappings();
};

AlienFX_SDK::afx_device* FXHelper::LocateDev(int pid) {
	for (int i = 0; i < afx_dev.fxdevs.size(); i++)
		if (afx_dev.fxdevs[i].dev->GetPID() == pid)
			return &afx_dev.fxdevs[i];
	return nullptr;
};

void FXHelper::SetGroupLight(int groupID, vector<AlienFX_SDK::afx_act> actions, bool force, AlienFX_SDK::afx_act* from, AlienFX_SDK::afx_act* to_c, double power) {
	AlienFX_SDK::group *grp = afx_dev.GetGroupById(groupID);
	if (grp && grp->lights.size()) {
		for (int i = 0; i < grp->lights.size(); i++) {
			if (from) {
				// gauge
				if (((double) i) / grp->lights.size() < power) {
					if (((double) i + 1) / grp->lights.size() < power)
						actions[0] = *to_c;
					else {
						// recalc...
						AlienFX_SDK::afx_act fin;
						double newPower = (power - ((double) i) / grp->lights.size())*grp->lights.size();
						fin.r = (byte) ((1.0 - newPower) * from->r + newPower * to_c->r);
						fin.g = (byte) ((1.0 - newPower) * from->g + newPower * to_c->g);
						fin.b = (byte) ((1.0 - newPower) * from->b + newPower * to_c->b);
						actions[0] = fin;
					}
				} else
					actions[0] = *from;
			}
			SetLight(grp->lights[i]->devid, grp->lights[i]->lightid, actions, force);
		}
	}
}


void FXHelper::TestLight(int did, int id, bool wp)
{
	vector<byte> opLights;

	for (auto lIter = afx_dev.fxdevs[did].lights.begin(); lIter != afx_dev.fxdevs[did].lights.end(); lIter++)
		if ((*lIter)->lightid != id && !((*lIter)->flags & ALIENFX_FLAG_POWER))
			opLights.push_back((byte)(*lIter)->lightid);

	bool dev_ready = false;
	for (int c_count = 0; c_count < 20 && !(dev_ready = afx_dev.fxdevs[did].dev->IsDeviceReady()); c_count++)
		Sleep(20);
	if (!dev_ready) return;

	AlienFX_SDK::Colorcode c = wp ? afx_dev.fxdevs[did].desc->white : AlienFX_SDK::Colorcode({0});
	afx_dev.fxdevs[did].dev->SetMultiLights(&opLights, c);

	afx_dev.fxdevs[did].dev->UpdateColors();
	if (id != -1) {
		afx_dev.fxdevs[did].dev->SetColor(id, config->testColor);
		afx_dev.fxdevs[did].dev->UpdateColors();
	}

}

void FXHelper::ResetPower(int did)
{
	AlienFX_SDK::afx_device* dev = LocateDev(did);
	if (dev != NULL) {
		vector<AlienFX_SDK::act_block> act{{63}};
		dev->dev->SetPowerAction(&act);
	}
}

void FXHelper::SetCounterColor(EventData *data, bool force)
{

	if (force) {
		DebugPrint("Forced Counter update initiated...\n");
	}

	blinkStage = !blinkStage;
	bool wasChanged = false;
	if (force) {
		eData = {101,0,101,101,101,101,101,101,99};
	}

	if (!config->activeProfile)
		return;
	vector<lightset> active = config->activeProfile->lightsets;

	for (auto Iter = active.begin(); Iter != active.end(); Iter++) {
		vector<AlienFX_SDK::afx_act> actions;
		if ((Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)) {
			int mIndex = (afx_dev.GetFlags(Iter->devid, Iter->lightid) & ALIENFX_FLAG_POWER) && Iter->eve[0].map.size() > 1
				&& activeMode != MODE_AC && activeMode != MODE_CHARGE ? 1 : 0;
			AlienFX_SDK::afx_act fin = Iter->eve[0].fs.b.flags ?
				Iter->eve[0].map[mIndex] :
				Iter->eve[2].fs.b.flags ?
				Iter->eve[2].map[0] :
				Iter->eve[3].map[0],
				from = fin;
			fin.type = from.type = 0;
			double coeff = 0.0;
			bool diffC = false, diffI = false;
			int lVal = 0, cVal = 0;
			if (Iter->eve[2].fs.b.flags) {
				// counter
				int ccut = Iter->eve[2].fs.b.cut;
				switch (Iter->eve[2].source) {
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

				if (lVal != cVal && (lVal > ccut || cVal > ccut)) {
					diffC = true;
					coeff = cVal > ccut ? (cVal - ccut) / (100.0 - ccut) : 0.0;
					//fin.r = (BYTE) (from.r * (1 - coeff) + Iter->eve[2].map[1].r * coeff);
					//fin.g = (BYTE) (from.g * (1 - coeff) + Iter->eve[2].map[1].g * coeff);
					//fin.b = (BYTE) (from.b * (1 - coeff) + Iter->eve[2].map[1].b * coeff);
					fin.r = (byte) sqrt((1 - coeff) * from.r * from.r +
										coeff * Iter->eve[2].map[1].r * Iter->eve[2].map[1].r);
					fin.g = (byte) sqrt((1 - coeff) * from.g * from.g +
										coeff * Iter->eve[2].map[1].g * Iter->eve[2].map[1].g);
					fin.b = (byte) sqrt((1 - coeff) * from.b * from.b +
										coeff * Iter->eve[2].map[1].b * Iter->eve[2].map[1].b);
				}
			}
			if (Iter->eve[3].fs.b.flags) {
				// indicator
				int ccut = Iter->eve[3].fs.b.cut;
				bool blink = Iter->eve[3].fs.b.proc;
				switch (Iter->eve[3].source) {
				case 0: lVal = eData.HDD; cVal = data->HDD; break;
				case 1: lVal = eData.NET; cVal = data->NET; break;
				case 2: lVal = eData.Temp - ccut; cVal = data->Temp - ccut; break; //ccut!
				case 3: lVal = eData.RAM - ccut; cVal = data->RAM - ccut; break; //ccut!
				case 4: lVal = eData.Batt - ccut; cVal = data->Batt - ccut; break; //ccut!
				case 5: lVal = eData.KBD; cVal = data->KBD; break;
				}

				if (force || (lVal != cVal && ((byte)(cVal > 0) + (byte)(lVal > 0)) == 1)) { //check 0 border!
					diffI = true;
					if (cVal > 0 && (!blink || blinkStage))
						fin = Iter->eve[3].map[1];
				} else
					if (cVal > 0 && blink) {
						diffI = true;
						if (blinkStage)
							fin = Iter->eve[3].map[1];
					}
			}

			// check for change.
			if (!diffC && !diffI)
				continue;
			wasChanged = true;

			actions.push_back(fin);

			if (Iter->devid) {
				if (afx_dev.GetFlags(Iter->devid, Iter->lightid) & ALIENFX_FLAG_POWER)
					if (activeMode == MODE_AC || activeMode == MODE_CHARGE) {
						actions.push_back(Iter->eve[0].map[1]);
					} else {
						actions[0] = Iter->eve[0].map[0];
						actions.push_back(fin);
					}
				SetLight(Iter->devid, Iter->lightid, actions);
			} else {
				if (Iter->eve[2].fs.b.flags && Iter->eve[2].fs.b.proc)
					SetGroupLight(Iter->lightid, actions, false, &from, &Iter->eve[2].map[1], coeff);
				else
					SetGroupLight(Iter->lightid, actions);
			}
		}
	}
	if (wasChanged) {
		QueryUpdate();
		memcpy(&eData, data, sizeof(EventData));
	}
}

void FXHelper::QueryUpdate(int did, bool force)
{
	if (unblockUpdates && config->stateOn) {
		LightQueryElement newBlock{did, 0, force, true};// , actions};
		modifyQuery.lock();
		lightQuery.push_back(newBlock);
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

bool FXHelper::SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force)
{
	if (unblockUpdates && config->stateOn) {
		LightQueryElement newBlock{did, id, force, false, (byte)actions.size()};
		for (int i = 0; i < newBlock.actsize; i++)
			newBlock.actions[i] = actions[i];
		if (did && newBlock.actsize) {
			modifyQuery.lock();
			lightQuery.push_back(newBlock);
			modifyQuery.unlock();
			SetEvent(haveNewElement);
		} 
	}
	return true;
}

void FXHelper::RefreshState(bool force)
{
	Refresh(force);
	RefreshMon();
}

void FXHelper::RefreshMon()
{
	config->SetStates();
	if (!config->GetEffect())
		SetCounterColor(&eData, true);
}

void FXHelper::ChangeState() {
	config->SetStates();
	UnblockUpdates(false);

	for (int i = 0; i < afx_dev.fxdevs.size(); i++) {
		afx_dev.fxdevs[i].dev->ToggleState(config->finalBrightness, afx_dev.GetMappings(), config->finalPBState);
		if (config->stateOn)
			switch (afx_dev.fxdevs[i].dev->GetVersion()) {
			case API_L_ACPI: case API_L_V1: case API_L_V2: case API_L_V3: case API_L_V6: case API_L_V7:
					UnblockUpdates(true);
					RefreshState();
					UnblockUpdates(false);
			}
	}
	UnblockUpdates(true);
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Functions* dev) {
	if (!dev) {
		// let's find v5 dev...
		for (int i = 0; i < afx_dev.fxdevs.size(); i++) {
			if (afx_dev.fxdevs[i].dev->GetVersion() == 5) {
				dev = afx_dev.fxdevs[i].dev;
				break;
			}
		}
	}
	if (dev && dev->GetVersion() == 5) {
		AlienFX_SDK::afx_act c1{0,0,0,config->activeProfile->effColor1.r, 
			config->activeProfile->effColor1.g, 
			config->activeProfile->effColor1.b},
			c2{0,0,0,config->activeProfile->effColor2.r,
			config->activeProfile->effColor2.g, 
			config->activeProfile->effColor2.b};
		dev->SetGlobalEffects((byte)config->activeProfile->globalEffect, 
							  (byte)config->activeProfile->globalDelay, c1, c2);
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
			DebugPrint("Lights pause on!\n");
			while (updateThread && WaitForSingleObject(queryEmpty, 60000) == WAIT_TIMEOUT);
		} else {
			DebugPrint("Lights pause off!\n");
		}
	}
}

size_t FXHelper::FillAllDevs(AlienFan_SDK::Control* acc) {
	config->SetStates();
	afx_dev.AlienFXAssignDevices(acc ? acc->GetHandle() : NULL, config->finalBrightness, config->finalPBState);
	// global effects check
	for (int i=0; i < afx_dev.fxdevs.size(); i++)
		if (afx_dev.fxdevs[i].dev->GetVersion() == 5) {
			config->haveV5 = true; break;
		}
	return afx_dev.fxdevs.size();
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
		WaitForSingleObject(updateThread, 10000);
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

	config->SetStates();
	for (int i =0; i < config->active_set->size(); i++) {
		RefreshOne(&config->active_set->at(i), forced);
	}
	QueryUpdate(-1, forced == 2);
}

bool FXHelper::SetMode(int mode)
{
	int t = activeMode;
	activeMode = mode;
	for (int i = 0; i < afx_dev.fxdevs.size(); i++)
		afx_dev.fxdevs[i].dev->powerMode = (activeMode == MODE_AC || activeMode == MODE_CHARGE);
	return t == activeMode;
}

bool FXHelper::RefreshOne(lightset* map, int force, bool update)
{
	vector<AlienFX_SDK::afx_act> actions;

	if (!config->stateOn || !map)
		return false;

	if (map->eve[0].fs.b.flags) {
		actions = map->eve[0].map;
	}

	if (!config->GetEffect() && !force) {
		if (map->eve[1].fs.b.flags) {
			// use power event;
			if (!map->eve[0].fs.b.flags)
				actions = map->eve[1].map;
			else
				actions.push_back(map->eve[1].map[1]);
			switch (activeMode) {
			case MODE_BAT:
				actions.clear();
				actions.push_back(map->eve[1].map[1]);
				break;
			case MODE_LOW:
				actions.front().type = AlienFX_SDK::AlienFX_A_Pulse;
				actions.back().type = AlienFX_SDK::AlienFX_A_Pulse;
				break;
			case MODE_CHARGE:
				actions.front().type = AlienFX_SDK::AlienFX_A_Morph;
				actions.back().type = AlienFX_SDK::AlienFX_A_Morph;
				break;
			}
		}
		if (map->eve[2].fs.b.flags || map->eve[3].fs.b.flags) 
			return false;
	}

	if (!actions.size()) 
		return false;
	
	if (map->devid)
		SetLight(map->devid, map->lightid, actions, force == 2);
	else
		// Group here!
		SetGroupLight(map->lightid, actions, force == 2);
	if (update)
		QueryUpdate(map->devid, force == 2);

	return true;
}

void FXHelper::RefreshAmbient(UCHAR *img) {

	UINT shift = 255 - config->amb_conf->shift;
	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});

	//Flush();

	for (UINT i = 0; i < config->amb_conf->zones.size(); i++) {
		zone map = config->amb_conf->zones[i];
		UINT r = 0, g = 0, b = 0, dsize = (UINT) map.map.size() * 255;
		if (dsize) {
			for (unsigned j = 0; j < map.map.size(); j++) {
				r += img[3 * map.map[j] + 2];
				g += img[3 * map.map[j] + 1];
				b += img[3 * map.map[j]];
			}

			// Multilights and brightness correction...
			actions[0].r = (BYTE) ((r * shift) / dsize);
			actions[0].g = (BYTE) ((g * shift) / dsize);
			actions[0].b = (BYTE) ((b * shift) / dsize);

			if (map.devid)
				SetLight(map.devid, map.lightid, actions);
			else
				SetGroupLight(map.lightid, actions);
		}
	}
	if (config->amb_conf->zones.size())
		QueryUpdate();
}

void FXHelper::RefreshHaptics(int *freq) {
	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});

	//Flush();
	if (config->monDelay > 200) {
		DebugPrint("Haptics update skipped!\n");
		return;
	}

	for (auto mIter = config->hap_conf->haptics.begin(); mIter < config->hap_conf->haptics.end(); mIter++) {
		// Now for each freq block...
		unsigned from_r = 0, from_g = 0, from_b = 0, to_r = 0, to_g = 0, to_b = 0, cur_r = 0, cur_g = 0, cur_b = 0;
		double f_power = 0.0;
		for (auto fIter = mIter->freqs.begin(); fIter < mIter->freqs.end(); fIter++) {
			if (!fIter->freqID.empty() && fIter->hicut > fIter->lowcut) {
				double power = 0.0;
				//AlienFX_SDK::afx_act from{0,0,0,fIter->colorfrom.r,fIter->colorfrom.g, fIter->colorfrom.b},
				//	to{0,0,0,fIter->colorto.r,fIter->colorto.g, fIter->colorto.b}, current;
				// here need to check less bars...
				for (auto iIter = fIter->freqID.begin(); iIter < fIter->freqID.end(); iIter++)
					power += (freq[*iIter] > fIter->lowcut ? freq[*iIter] < fIter->hicut ?
							  freq[*iIter] - fIter->lowcut : fIter->hicut - fIter->lowcut : 0);
				power = power / (fIter->freqID.size() * (fIter->hicut - fIter->lowcut));
				//actions[0].r = (byte) ((1.0 - power) * from.r + power * to.r);
				//actions[0].g = (byte) ((1.0 - power) * from.g + power * to.g);
				//actions[0].b = (byte) ((1.0 - power) * from.b + power * to.b);
				//current.r = (byte) sqrt((1.0 - power) * fIter->colorfrom.r * fIter->colorfrom.r + power * fIter->colorto.r * fIter->colorto.r);
				//current.g = (byte) sqrt((1.0 - power) * fIter->colorfrom.g * fIter->colorfrom.g + power * fIter->colorto.g * fIter->colorto.g);
				//current.b = (byte) sqrt((1.0 - power) * fIter->colorfrom.b * fIter->colorfrom.b + power * fIter->colorto.b * fIter->colorto.b);

				from_r += fIter->colorfrom.r;
				from_g += fIter->colorfrom.g;
				from_b += fIter->colorfrom.b;

				to_r += fIter->colorto.r;
				to_g += fIter->colorto.g;
				to_b += fIter->colorto.b;

				cur_r += (byte) sqrt((1.0 - power) * fIter->colorfrom.r * fIter->colorfrom.r + power * fIter->colorto.r * fIter->colorto.r);
				cur_g += (byte) sqrt((1.0 - power) * fIter->colorfrom.g * fIter->colorfrom.g + power * fIter->colorto.g * fIter->colorto.g);
				cur_b += (byte) sqrt((1.0 - power) * fIter->colorfrom.b * fIter->colorfrom.b + power * fIter->colorto.b * fIter->colorto.b);

				f_power += power;

			}

			size_t groupsize = mIter->freqs.size();

			f_power /= groupsize;

			actions[0] = {0,0,0,(byte)(cur_r/groupsize),(byte)(cur_g/groupsize),(byte)(cur_b/groupsize)};

			AlienFX_SDK::afx_act from{0,0,0,(byte) (from_r / groupsize),(byte) (from_g / groupsize),(byte) (from_b / groupsize)},
				to{0,0,0,(byte) (to_r / groupsize),(byte) (to_g / groupsize),(byte) (to_b / groupsize)};

			if (mIter->devid) {
				SetLight(mIter->devid, mIter->lightid, actions);
			} else {
				// group
				if (mIter->flags & MAP_GAUGE)
					SetGroupLight(mIter->lightid, actions, false, &from, &to, f_power);
				else
					SetGroupLight(mIter->lightid, actions);
			}
		}
	}
	if (config->hap_conf->haptics.size())
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

	while (WaitForMultipleObjects(2, waitArray, false, 50) != WAIT_OBJECT_0) {
		if (!src->lightQuery.empty()) ResetEvent(src->queryEmpty);
		int maxQlights = (int)src->afx_dev.GetMappings()->size() * 5;
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
				if (src->GetConfig()->stateOn) {
					for (auto devQ=devs_query.begin(); devQ != devs_query.end(); devQ++) {
						AlienFX_SDK::afx_device* dev = src->LocateDev(devQ->devID);
						if (dev && (current.did == (-1) || devQ->devID == current.did)) {
//#ifdef _DEBUG
//							char buff[2048];
//							sprintf_s(buff, 2047, "Starting update for %d, (%d lights, %d in query)...\n", devQ->devID, devQ->dev_query.size(), src->lightQuery.size());
//							OutputDebugString(buff);
//#endif
							if (devQ->dev_query.size() > 0) {
								dev->dev->SetMultiColor(&devQ->dev_query, current.flags);
								dev->dev->UpdateColors();
								if (src->config->activeProfile && src->config->activeProfile->flags & PROF_GLOBAL_EFFECTS)
									src->UpdateGlobalEffect(dev->dev);
								devQ->dev_query.clear();
							}
						}
					}
					if (current.did == -1)
						devs_query.clear();
					if (src->lightQuery.size() > maxQlights) {
						src->GetConfig()->monDelay += 50;
						DebugPrint((string("Query so big (") +
									to_string((int) src->lightQuery.size()) +
									"), delay increased to " +
									to_string(src->GetConfig()->monDelay) +
									" ms!\n"
									).c_str());
					}
				}
			} else {
				// set light
				AlienFX_SDK::afx_device* dev = src->LocateDev(current.did);

				if (dev) {
					vector<AlienFX_SDK::afx_act> actions;// = current.actions;
					byte flags = src->afx_dev.GetFlags(current.did, current.lid);
					for (int i = 0; i < current.actsize; i++) {
						AlienFX_SDK::afx_act action = current.actions[i];
						// gamma-correction...
						if (src->GetConfig()->gammaCorrection) {
							action.r = ((UINT) action.r * action.r * dev->desc->white.r) / (255 * 255);
							action.g = ((UINT) action.g * action.g * dev->desc->white.g) / (255 * 255);
							action.b = ((UINT) action.b * action.b * dev->desc->white.b) / (255 * 255);
						}
						// Dimming...
						// For v0-v3 devices only, v4+ have hardware dimming
						if (dev->dev->GetVersion() < 4 && src->GetConfig()->stateDimmed && (!flags || src->GetConfig()->dimPowerButton)) {
							unsigned delta = 255 - src->GetConfig()->dimmingPower;
							action.r = ((UINT) action.r * delta) / 255;// >> 8;
							action.g = ((UINT) action.g * delta) / 255;// >> 8;
							action.b = ((UINT) action.b * delta) / 255;// >> 8;
						}
						actions.push_back(action);
					}

					// Is it power button?
					if (flags & ALIENFX_FLAG_POWER) {
						// Should we update it?
						if (!src->GetConfig()->block_power && current.actsize == 2 &&
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
		if (src->GetConfig()->monDelay > 200) {
			src->GetConfig()->monDelay -= 10;
			DebugPrint((string("Query empty, delay decreased to ") + to_string(src->GetConfig()->monDelay) + " ms!\n").c_str());
		}
		SetEvent(src->queryEmpty);
	}
	return 0;
}