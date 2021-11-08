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
	FillDevs(config->stateOn, config->offPowerButton);
};
FXHelper::~FXHelper() {
	if (devs.size() > 0) {
		for (int i = 0; i < devs.size(); i++)
			devs[i]->AlienFXClose();
		devs.clear();
	}
};

AlienFX_SDK::Functions* FXHelper::LocateDev(int pid) {
	for (int i = 0; i < devs.size(); i++)
		if (devs[i]->GetPID() == pid)
			return devs[i];
	return nullptr;
};

size_t FXHelper::FillDevs(bool state, bool power) {
	vector<pair<DWORD, DWORD>> devList = afx_dev.AlienFXEnumDevices();
	config->haveV5 = false;

	if (devs.size() > 0) {
		for (int i = 0; i < devs.size(); i++) {
			devs[i]->AlienFXClose();
			delete devs[i];
		}
		devs.clear();
	}
	for (int i = 0; i < devList.size(); i++) {
		AlienFX_SDK::Functions* dev = new AlienFX_SDK::Functions();
		int pid = dev->AlienFXInitialize(devList[i].first, devList[i].second);
		if (pid != -1) {
			devs.push_back(dev);
			dev->ToggleState(state?255:0, afx_dev.GetMappings(), power);
			if (dev->GetVersion() == 5) config->haveV5 = true;
		} else
			delete dev;
	}
	return devs.size();
}

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
						double newPower = (power - ((double) i) / grp->lights.size()) * grp->lights.size();
						fin.r = (unsigned char) ((1.0 - newPower) * from->r + newPower * to_c->r);
						fin.g = (unsigned char) ((1.0 - newPower) * from->g + newPower * to_c->g);
						fin.b = (unsigned char) ((1.0 - newPower) * from->b + newPower * to_c->b);
						actions[0] = fin;
					}
				} else
					actions[0] = *from;
			}
			SetLight(grp->lights[i]->devid, grp->lights[i]->lightid, actions, force);
		}
	}
}


void FXHelper::TestLight(int did, int id)
{
	AlienFX_SDK::Functions* dev = LocateDev(did);
	if (dev != NULL) {
		//int r = (config->testColor.cs.red * config->testColor.cs.red) >> 8,
		//	g = (config->testColor.cs.green * config->testColor.cs.green) >> 8,
		//	b = (config->testColor.cs.blue * config->testColor.cs.blue) >> 8;

		vector<UCHAR> opLights;

		for (vector<AlienFX_SDK::mapping*>::iterator lIter = afx_dev.GetMappings()->begin();
			 lIter != afx_dev.GetMappings()->end(); lIter++)
			if ((*lIter)->devid == did && (*lIter)->lightid != id && !((*lIter)->flags & ALIENFX_FLAG_POWER))
				opLights.push_back((UCHAR)(*lIter)->lightid);

		bool dev_ready = false;
		for (int c_count = 0; c_count < 20 && !(dev_ready = dev->IsDeviceReady()); c_count++)
			Sleep(20);
		if (!dev_ready) return;

		dev->SetMultiLights((int)opLights.size(), opLights.data(), 0, 0, 0);
		if (id != -1)
			dev->SetColor(id, config->testColor.r, config->testColor.g, config->testColor.b);
		dev->UpdateColors();

	}
}

void FXHelper::ResetPower(int did)
{
	AlienFX_SDK::Functions* dev = LocateDev(did);
	if (dev != NULL)
		dev->SetPowerAction(63, 0, 0, 0, 0, 0, 0);
}

void FXHelper::SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, long cFan, bool force)
{

	//char buff[2048];
	//sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
	//OutputDebugString(buff);
	if (force) {
		DebugPrint("Forced Counter update initiated...\n");
	}

	if (config->autoRefresh) {
		Refresh();
		force = true;
	}

	std::vector <lightset>::iterator Iter;
	blinkStage = !blinkStage;
	bool wasChanged = false;
	if (force) {
		lCPU = 101; lRAM = 0; lHDD = 101; lGPU = 101; lNET = 101; lTemp = 101; lBatt = 101, cFan = 101;
	}

	profile* cprof = config->FindProfile(config->activeProfile);
	if (!cprof)
		return;
	vector<lightset> active = cprof->lightsets;

	for (Iter = active.begin(); Iter != active.end(); Iter++) {
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
			fin.type = 0;
			double coeff = 0.0;
			bool diffC = false, diffI = false;
			long lVal = 0, cVal = 0;
			if (Iter->eve[2].fs.b.flags) {
				// counter
				double ccut = Iter->eve[2].fs.b.cut;
				//long lVal = 0, cVal = 0;
				switch (Iter->eve[2].source) {
				case 0: lVal = lCPU; cVal = cCPU; break;
				case 1: lVal = lRAM; cVal = cRAM; break;
				case 2: lVal = lHDD; cVal = cHDD; break;
				case 3: lVal = lGPU; cVal = cGPU; break;
				case 4: lVal = lNET; cVal = cNet; break;
				case 5: lVal = lTemp; cVal = cTemp; break;
				case 6: lVal = lBatt; cVal = cBatt; break;
				case 7: lVal = lFan; cVal = cFan; break;
				}

				if (lVal != cVal && (lVal > ccut || cVal > ccut)) {
					diffC = true;
					coeff = cVal > ccut ? (cVal - ccut) / (100.0 - ccut) : 0.0;
					fin.r = (BYTE) (from.r * (1 - coeff) + Iter->eve[2].map[1].r * coeff);
					fin.g = (BYTE) (from.g * (1 - coeff) + Iter->eve[2].map[1].g * coeff);
					fin.b = (BYTE) (from.b * (1 - coeff) + Iter->eve[2].map[1].b * coeff);
				}
			}
			if (Iter->eve[3].fs.b.flags) {
				// indicator
				long indi = 0, ccut = Iter->eve[3].fs.b.cut;
				bool blink = Iter->eve[3].fs.b.proc;
				switch (Iter->eve[3].source) {
				case 0: lVal = lHDD; cVal = cHDD; break;
				case 1: lVal = lNET; cVal = cNet; break;
				case 2: lVal = lTemp - ccut; cVal = cTemp - ccut; break; //ccut!
				case 3: lVal = lRAM - ccut; cVal = cRAM - ccut; break; //ccut!
				case 4: lVal = lBatt - ccut; cVal = cBatt - ccut; break; //ccut!
				}

				if (cVal > 0 || (lVal > 0 && !(cVal > 0))) {
					if (cVal > 0) {
						diffC = false;
						if (!(lVal > 0) || blink)
							diffI = true;
						if (!blink || (blink && blinkStage))
							fin = Iter->eve[3].map[1];
					} else {
						diffI = !diffC;
					}
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
		lCPU = cCPU; lRAM = cRAM; lHDD = cHDD; lGPU = cGPU; lNET = cNet; lTemp = cTemp; lBatt = cBatt; lFan = cFan;
	}
}

void FXHelper::QueryUpdate(int did, bool force)
{
	if (unblockUpdates && config->stateOn) {
		LightQueryElement newBlock = {did, 0, force, true};// , actions};
		modifyQuery.lock();
		lightQuery.push_back(newBlock);
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

bool FXHelper::SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force)
{
	if (unblockUpdates && config->stateOn) {
		LightQueryElement newBlock = {did, id, force, false, (byte)actions.size()};
		for (int i = 0; i < newBlock.actsize; i++)
			newBlock.actions[i] = actions[i];
		if (did && newBlock.actsize) {
			modifyQuery.lock();
			lightQuery.push_back(newBlock);
			modifyQuery.unlock();
			SetEvent(haveNewElement);
		} /*else {
			DebugPrint((string("ERROR! Light (") + to_string(did) + ", " + to_string(id) + ") have group, not color!\n").c_str());
			return false;
		}*/
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
		SetCounterColor(lCPU, lRAM, lGPU, lNET, lHDD, lTemp, lBatt, lFan, true);
}

void FXHelper::ChangeState() {
	config->SetStates();
	UnblockUpdates(false);
	byte bright = (byte) (config->stateOn ? config->stateDimmed ? 255 - config->dimmingPower : 255 : 0);

	for (int i = 0; i < devs.size(); i++) {
		devs[i]->ToggleState(bright, afx_dev.GetMappings(), bright > 0 ? config->dimPowerButton : config->offPowerButton);
		switch (devs[i]->GetVersion()) {
		case 0: case 1: case 2: case 3: case 6:
			if (config->stateOn) {
				UnblockUpdates(true);
				RefreshState();
				UnblockUpdates(false);
			}
		}
	}
	UnblockUpdates(true);
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Functions* dev) {
	if (!dev) {
		// let's find v5 dev...
		for (int i = 0; i < devs.size(); i++) {
			if (devs[i]->GetVersion() == 5) {
				dev = devs[i];
				break;
			}
		}
	}
	if (dev && dev->GetVersion() == 5) {
		AlienFX_SDK::afx_act c1 = {0,0,0,config->effColor1.r, config->effColor1.g, config->effColor1.b},
			c2 = {0,0,0,config->effColor2.r, config->effColor2.g, config->effColor2.b};
		dev->SetGlobalEffects((byte)config->globalEffect, config->globalDelay, c1, c2);
	}
}

void FXHelper::Flush() {

	//DebugPrint("Flushing light query...\n");

	modifyQuery.lock();
	deque<LightQueryElement>::iterator qIter;
	for (qIter = lightQuery.begin(); qIter != lightQuery.end() && !qIter->update; qIter++);
	if (qIter != lightQuery.end())
		// flush the rest of query
		lightQuery.erase(qIter+1, lightQuery.end());
	modifyQuery.unlock();
	//while (updateThread && !lightQuery.empty()) Sleep(20);
}

void FXHelper::UnblockUpdates(bool newState, bool lock) {
	if (lock)
		updateLock = !newState;
	if (!updateLock || lock) {
		unblockUpdates = newState;
		if (!unblockUpdates) {
			DebugPrint("Lights pause on!\n");
			while (updateThread && WaitForSingleObject(queryEmpty, 60000) == WAIT_TIMEOUT);
			//while (updateThread && !lightQuery.empty()) Sleep(20);
		} else {
			DebugPrint("Lights pause off!\n");
		}
	}
}

size_t FXHelper::FillAllDevs(AlienFan_SDK::Control* acc) {
	size_t res = FillDevs(config->stateOn, config->offPowerButton);
	if (acc) {
		// add ACPI device...
		AlienFX_SDK::Functions *dev = new AlienFX_SDK::Functions();
		int pid = dev->AlienFXInitialize(acc->GetHandle());
		if (pid != -1) {
			devs.push_back(dev);
			//dev->ToggleState(state ? 255 : 0, afx_dev.GetMappings(), power);
		} else
			delete dev;
	}
	return res;
}

void FXHelper::Start() {
	if (!updateThread) {
		DebugPrint("Light updates started.\n");

		stopQuery = CreateEvent(NULL, true, false, NULL);
		UnblockUpdates(true);
		haveNewElement = CreateEvent(NULL, false, false, NULL);
		queryEmpty = CreateEvent(NULL, true, true, NULL);
		updateThread = CreateThread(NULL, 0, CLightsProc, this, 0, NULL);
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

int FXHelper::Refresh(bool forced)
{
	if (forced) {
		DebugPrint("Forced Refresh initiated...\n");
	}

	config->SetStates();
	for (int i =0; i < config->active_set->size(); i++) {
		RefreshOne(&config->active_set->at(i), forced);
	}
	QueryUpdate(-1, forced);
	return 0;
}

bool FXHelper::SetMode(int mode)
{
	int t = activeMode;
	activeMode = mode;
	return t == activeMode;
}

bool FXHelper::RefreshOne(lightset* map, bool force, bool update)
{
	vector<AlienFX_SDK::afx_act> actions;
	AlienFX_SDK::afx_act action;

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
		SetLight(map->devid, map->lightid, actions, force);
	else
		// Group here!
		SetGroupLight(map->lightid, actions, force);
	if (update)
		QueryUpdate(map->devid, true);

	return true;
}

void FXHelper::RefreshAmbient(UCHAR *img) {

	UINT shift = 255 - config->amb_conf->shift;
	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});

	Flush();

	for (UINT i = 0; i < config->amb_conf->mappings.size(); i++) {
		mapping map = config->amb_conf->mappings[i];
		UINT r = 0, g = 0, b = 0, size = (UINT) map.map.size(), dsize = size * 255;
		if (size > 0) {
			for (unsigned j = 0; j < size; j++) {
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
	if (config->amb_conf->mappings.size())
		QueryUpdate(-1, false);
}

struct devset {
	WORD did;
	vector<UCHAR> lIDs;
	vector<vector<AlienFX_SDK::afx_act>> fullSets;
};

void FXHelper::RefreshHaptics(int *freq) {
	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});

	Flush();

	for (unsigned i = 0; i < config->hap_conf->mappings.size(); i++) {
		haptics_map map = config->hap_conf->mappings[i];

		if (!map.map.empty() && map.hicut > map.lowcut) {
			double power = 0.0;
			AlienFX_SDK::afx_act from = {0,0,0,map.colorfrom.r,map.colorfrom.g, map.colorfrom.b},
				to = {0,0,0,map.colorto.r,map.colorto.g, map.colorto.b},
				fin = {0};
			// here need to check less bars...
			for (int j = 0; j < map.map.size(); j++)
				power += (freq[map.map[j]] > map.lowcut ? freq[map.map[j]] < map.hicut ? freq[map.map[j]] - map.lowcut : map.hicut - map.lowcut : 0);
			//if (map.map.size() > 1)
				power = power / (map.map.size() * (map.hicut - map.lowcut));
			fin.r = (unsigned char) ((1.0 - power) * from.r + power * to.r);
			fin.g = (unsigned char) ((1.0 - power) * from.g + power * to.g);
			fin.b = (unsigned char) ((1.0 - power) * from.b + power * to.b);

			actions[0] = fin;

			if (map.devid) {
				SetLight(map.devid, map.lightid, actions);
			} else {
				// group
				if (map.flags)
					SetGroupLight(map.lightid, actions, false, &from, &to, power);
				else
					SetGroupLight(map.lightid, actions);
			}
		}
	}
	if (config->hap_conf->mappings.size())
		QueryUpdate(-1, false);
}

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	LightQueryElement current;
	bool wasDelay = false;

	HANDLE waitArray[2] = {src->stopQuery, src->haveNewElement};
	vector<deviceQuery> devs_query;

	// Power button state...
	vector<AlienFX_SDK::afx_act> pbstate;
	pbstate.resize(2);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	// ULONGLONG ladt = GetTickCount64();

	while (WaitForMultipleObjects(2, waitArray, false, 200) != WAIT_OBJECT_0) {
		if (!src->lightQuery.empty()) ResetEvent(src->queryEmpty);
		while (!src->lightQuery.empty()) {
			int maxQlights = (int)src->afx_dev.GetMappings()->size() * 5;
			if (!wasDelay && src->lightQuery.size() > maxQlights) {
				src->GetConfig()->monDelay += 50;
				wasDelay = true;
				DebugPrint((string("Query so big (") +
								   to_string((int) src->lightQuery.size()) +
								   "), delay increased to " +
								   to_string(src->GetConfig()->monDelay) +
								   " ms!\n"
								   ).c_str());
			}
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
				wasDelay = false;
				//ULONGLONG upst = GetTickCount64();
				//DebugPrint((string("Update begins after ") + to_string(upst - ladt) + " ms\n").c_str());
				if (src->GetConfig()->stateOn) {
					for (int i = 0; i < devs_query.size(); i++) {
						AlienFX_SDK::Functions* dev = src->LocateDev(devs_query[i].devID);
						deviceQuery* devQ = &devs_query[i];
						if (dev && (current.did == (-1) || devQ->devID == current.did)) {
							vector<UCHAR>lights;
							std::vector<vector<AlienFX_SDK::afx_act>> acts;
//#ifdef _DEBUG
//							char buff[2048];
//							sprintf_s(buff, 2047, "Starting update for %d, (%d lights, %d in query)...\n", devQ->devID, devQ->dev_query.size(), src->lightQuery.size());
//							OutputDebugString(buff);
//#endif
							if (devQ->dev_query.size() > 0) {
								for (int j = 0; j < devQ->dev_query.size(); j++) {
									if (devQ->dev_query[j].second.size() > 0) {
										lights.push_back(devQ->dev_query[j].first);
										acts.push_back(devQ->dev_query[j].second);
									}
								}

								//if (dev->IsDeviceReady()) {
#ifdef _DEBUG
									if (lights.size() != acts.size())
										OutputDebugString("Data size mismatch!\n");
#endif
									dev->SetMultiColor((int)lights.size(), lights.data(), acts, current.flags);
									dev->UpdateColors();
									src->UpdateGlobalEffect(dev);
								//} //else
									//dev->Reset(true);
								devQ->dev_query.clear();
								lights.clear();
								acts.clear();
							}
						}
					}
					if (current.did == -1)
						devs_query.clear();
				}
				//ULONGLONG uft = GetTickCount64();
				//DebugPrint((string("Last update time ") + to_string(uft - upst) + ", circle time " + to_string(uft - ladt) + " ms\n").c_str());
				//ladt = uft;
			} else {
				// set light
				AlienFX_SDK::Functions* dev = src->LocateDev(current.did);

				if (dev) {
					vector<AlienFX_SDK::afx_act> actions;// = current.actions;
					byte flags = src->afx_dev.GetFlags(current.did, current.lid);
					for (int i = 0; i < current.actsize; i++) {
						AlienFX_SDK::afx_act action = current.actions[i];
						// gamma-correction...
						if (src->GetConfig()->gammaCorrection) {
							action.r = ((UINT) action.r * action.r) / 255;
							action.g = ((UINT) action.g * action.g) / 255;
							action.b = ((UINT) action.b * action.b) / 255;
						}
						// Dimming...
						// For v0-v3 devices only, v4 and v5 have hardware dimming
						if (dev->GetVersion() < 4 && src->GetConfig()->stateDimmed && (!flags || src->GetConfig()->dimPowerButton)) {
							unsigned delta = 255 - src->GetConfig()->dimmingPower;
							action.r = ((UINT) action.r * delta) / 255;// >> 8;
							action.g = ((UINT) action.g * delta) / 255;// >> 8;
							action.b = ((UINT) action.b * delta) / 255;// >> 8;
						}
						actions.push_back(action);
					}

					if (flags & ALIENFX_FLAG_POWER) {
						// Set power immediately!
						if (!src->GetConfig()->block_power && current.actsize > 1) {

							// Do we have the same color?
							if ((current.flags /*&& dev->GetVersion() != 4*/) ||
								pbstate[0].r != actions[0].r || pbstate[1].r != actions[1].r ||
								pbstate[0].g != actions[0].g || pbstate[1].g != actions[1].g ||
								pbstate[0].b != actions[0].b || pbstate[1].b != actions[1].b) {

								DebugPrint((string("Setting power button to ") +
											to_string(actions[0].r) + "-" +
											to_string(actions[0].g) + "-" + 
											to_string(actions[0].b) + "/" +
											to_string(actions[1].r) + "-" +
											to_string(actions[1].g) + "-" +
											to_string(actions[1].b) + 
											"\n").c_str());

								pbstate = actions;
								actions[0].type = AlienFX_SDK::AlienFX_A_Power;
							} else {
								DebugPrint("Setting power button skipped, same colors\n");
								continue;
							}
						} else
							continue;
					}

					// fill query....
					int qn;
					for (qn = 0; qn < devs_query.size(); qn++)
						if (devs_query[qn].devID == current.did) {
							devs_query[qn].dev_query.push_back(pair<int, vector<AlienFX_SDK::afx_act>>(current.lid, actions));
								break;
						}
					if (qn == devs_query.size()) {
						// create new query!
						deviceQuery newQ;
						newQ.devID = current.did;
						newQ.dev_query.push_back(pair<int, vector<AlienFX_SDK::afx_act>>(current.lid, actions));
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