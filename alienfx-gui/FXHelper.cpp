#include "FXHelper.h"

DWORD WINAPI CLightsProc(LPVOID param);

void FXHelper::TestLight(int did, int id)
{
	AlienFX_SDK::Functions* dev = LocateDev(did);
	if (dev != NULL) {
		int r = (config->testColor.cs.red * config->testColor.cs.red) >> 8,
			g = (config->testColor.cs.green * config->testColor.cs.green) >> 8,
			b = (config->testColor.cs.blue * config->testColor.cs.blue) >> 8;

		vector<UCHAR> opLights;

		for (vector<AlienFX_SDK::mapping>::iterator lIter = afx_dev.GetMappings()->begin();
			 lIter != afx_dev.GetMappings()->end(); lIter++)
			if (lIter->devid == did && lIter->lightid != id && !(lIter->flags & ALIENFX_FLAG_POWER))
				opLights.push_back((UCHAR)lIter->lightid);

		bool dev_ready = false;
		for (int c_count = 0; c_count < 20 && !(dev_ready = dev->IsDeviceReady()); c_count++)
			Sleep(20);
		if (!dev_ready) return;

		dev->SetMultiLights((int)opLights.size(), opLights.data(), 0, 0, 0);
		if (id != -1)
			dev->SetColor(id, r, g, b);
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
#ifdef _DEBUG
	//char buff[2048];
	//sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
	//OutputDebugString(buff);
	if (force)
		OutputDebugString("Forced Counter update initiated...\n");
#endif

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

	//bool tHDD = (lHDD && !cHDD) || (!lHDD && cHDD),
	//	tNet = (lNET && !cNet) || (!lNET && cNet);

	vector<AlienFX_SDK::afx_act> actions;
	
	profile* cprof = config->FindProfile(config->activeProfile);
	if (!cprof)
		return;
	vector<lightset> active = cprof->lightsets;

	for (Iter = active.begin(); Iter != active.end(); Iter++)
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

			if (Iter->devid) {
				if (afx_dev.GetFlags(Iter->devid, Iter->lightid) & ALIENFX_FLAG_POWER)
					if (activeMode == MODE_AC || activeMode == MODE_CHARGE) {
						actions.push_back(fin);
						actions.push_back(Iter->eve[0].map[1]);
					} else {
						actions.push_back(Iter->eve[0].map[0]);
						actions.push_back(fin);
					}
				else {
					actions.push_back(fin);
				}
				SetLight(Iter->devid, Iter->lightid, actions);
			}
			else {
				actions.push_back(fin);
				AlienFX_SDK::group* grp = afx_dev.GetGroupById(Iter->lightid);
				if (grp)
					for (int i = 0; i < grp->lights.size(); i++) {
						if (!(afx_dev.GetFlags(grp->lights[i]->devid, grp->lights[i]->lightid) & ALIENFX_FLAG_POWER)) {
							if (Iter->eve[2].fs.b.proc && !(diffI && cVal > 0)) {
								// set as gauge...
								if (((double) i) / grp->lights.size() < coeff) {
									if (((double) i + 1) / grp->lights.size() < coeff) {
										actions.front() = Iter->eve[2].map[1];
									} else {
										// recalc...
										double newCoeff = (coeff - ((double) i) / grp->lights.size()) * grp->lights.size();
										fin.r = (BYTE) (from.r * (1 - newCoeff) + Iter->eve[2].map[1].r * newCoeff);
										fin.g = (BYTE) (from.g * (1 - newCoeff) + Iter->eve[2].map[1].g * newCoeff);
										fin.b = (BYTE) (from.b * (1 - newCoeff) + Iter->eve[2].map[1].b * newCoeff);
										actions.front() = fin;
									}
								} else
									actions[0] = from;

							}
							SetLight(grp->lights[i]->devid, grp->lights[i]->lightid, actions);
						}
					}
			}
		}
	if (wasChanged) {
		UpdateColors();
		lCPU = cCPU; lRAM = cRAM; lHDD = cHDD; lGPU = cGPU; lNET = cNet; lTemp = cTemp; lBatt = cBatt; lFan = cFan;
	}
}

void FXHelper::UpdateColors(int did, bool force)
{
	if (unblockUpdates && config->stateOn) {
		LightQueryElement newBlock = {did, 0, force, true};// , actions};
		lightQuery.push_back(newBlock);
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
		} else {
#ifdef _DEBUG
			char buff[2048];
			sprintf_s(buff, 2047, "ERROR! Light (%d, %d) have group, not color!\n", did, id);
			OutputDebugString(buff);
#endif
			return false;
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
	if (config->IsMonitoring())
		SetCounterColor(lCPU, lRAM, lGPU, lNET, lHDD, lTemp, lBatt, lFan, true);
}

void FXHelper::ChangeState() {
	config->SetStates();
	UnblockUpdates(false);
	byte bright = (byte) (config->stateOn ? config->stateDimmed ? 255 - config->dimmingPower : 255 : 0);
	for (int i = 0; i < devs.size(); i++) {
		devs[i]->ToggleState(bright, afx_dev.GetMappings(), config->offPowerButton);
		if (config->stateOn && devs[i]->GetVersion() < 4) {
			UnblockUpdates(true);
			RefreshState();
			UnblockUpdates(false);
		}
	}
	UnblockUpdates(true);
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Functions* dev) {
	if (!dev)
		dev = LocateDev(config->lastActive);
	if (dev) {
		AlienFX_SDK::afx_act c1 = {0,0,0,config->effColor1.cs.red, config->effColor1.cs.green, config->effColor1.cs.blue},
			c2 = {0,0,0,config->effColor2.cs.red, config->effColor2.cs.green, config->effColor2.cs.blue};
		dev->SetGlobalEffects((byte)config->globalEffect, config->globalDelay, c1, c2);
	}
}

void FXHelper::Flush() {
#ifdef _DEBUG
	OutputDebugString("Flushing light query...\n");
#endif
	unblockUpdates = false;
	modifyQuery.lock();
	deque<LightQueryElement>::iterator qIter;
	for (qIter = lightQuery.begin(); qIter != lightQuery.end() && !qIter->update; qIter++);
	if (qIter != lightQuery.end())
		// flush the rest of query
		lightQuery.erase(qIter, lightQuery.end());
	modifyQuery.unlock();
	while (updateThread && !lightQuery.empty()) Sleep(20);
}

void FXHelper::UnblockUpdates(bool newState, bool lock) {
	if (lock)
		updateLock = !newState;
	if (!updateLock || lock) {
		unblockUpdates = newState;
		if (!unblockUpdates) {
#ifdef _DEBUG
			OutputDebugString("Lights pause on!\n");
#endif
			while (updateThread && !lightQuery.empty()) Sleep(20);
		}
#ifdef _DEBUG
		else
			OutputDebugString("Lights pause off!\n");
#endif
	}
}

size_t FXHelper::FillAllDevs(bool state, bool power, HANDLE acc) {
	size_t res = FillDevs(state, power);
	if (acc) {
		// add ACPI device...
		AlienFX_SDK::Functions *dev = new AlienFX_SDK::Functions();
		int pid = dev->AlienFXInitialize(acc);
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
#ifdef _DEBUG
		OutputDebugString("Light updates started.\n");
#endif
		stopQuery = CreateEvent(NULL, true, false, NULL);
		UnblockUpdates(true);
		haveNewElement = CreateEvent(NULL, true, false, NULL);
		updateThread = CreateThread(NULL, 0, CLightsProc, this, 0, NULL);
	}
}

void FXHelper::Stop() {
	if (updateThread) {
#ifdef _DEBUG
		OutputDebugString("Light updates stopped.\n");
#endif
		UnblockUpdates(false, true);
		SetEvent(stopQuery);
		WaitForSingleObject(updateThread, 10000);
		lightQuery.clear();
		CloseHandle(stopQuery);
		CloseHandle(updateThread);
		updateThread = NULL;
	}
}

int FXHelper::Refresh(bool forced)
{
#ifdef _DEBUG
	if (forced) {

		OutputDebugString("Forced Refresh initiated...\n");

	}
#endif
	config->SetStates();
	for (int i =0; i < config->active_set->size(); i++) {
		RefreshOne(&config->active_set->at(i), forced);
	}
	UpdateColors(-1, forced);
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

	if (config->IsMonitoring() && !force) {
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
	
	if (!map->devid) {
		// Group here!
		AlienFX_SDK::group* grp = afx_dev.GetGroupById(map->lightid);
		if (grp) {
			for (int i = 0; i < grp->lights.size(); i++) {
				SetLight(grp->lights[i]->devid, grp->lights[i]->lightid, actions, force);
			}
			if (grp->lights.size() > 0 && update)
				UpdateColors();
		}
	} else {
		SetLight(map->devid, map->lightid, actions, force);
		if (update)
			UpdateColors(map->devid, true);
	}
	//map->lastColor = actions[0];
	return true;
}

int FXHelper::RefreshAmbient(UCHAR *img) {
	unsigned i = 0;
	unsigned shift = 255 - config->amb_conf->shift;

	for (i = 0; i < config->amb_conf->mappings.size(); i++) {
		mapping map = config->amb_conf->mappings[i];
		vector<AlienFX_SDK::afx_act> actions;
		AlienFX_SDK::afx_act fin = {0};
		unsigned r = 0, g = 0, b = 0, size = (unsigned) map.map.size();
		if (size > 0) {
			for (unsigned j = 0; j < size; j++) {
				r += img[3 * map.map[j] + 2];
				g += img[3 * map.map[j] + 1];
				b += img[3 * map.map[j]];
			}

			// Multilights correction...
			fin.r = ((r * shift) / size) >> 8;
			fin.g = ((g * shift) / size) >> 8;
			fin.b = ((b * shift) / size) >> 8;

			actions.push_back(fin);
			if (map.lightid > 0xffff) {
				AlienFX_SDK::group *grp = afx_dev.GetGroupById(map.lightid);
				if (grp && grp->lights.size()) {
					for (int i = 0; i < grp->lights.size(); i++) {
						SetLight(grp->lights[i]->devid, grp->lights[i]->lightid, actions);
					}
				}
			} else {
				SetLight(map.devid, map.lightid, actions);
			}
		}
	}
	UpdateColors();
	return 0;
}

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	LightQueryElement current;
	bool wasDelay = false;

	HANDLE waitArray[2] = {src->stopQuery, src->haveNewElement};
	vector<deviceQuery> devs_query;

	while (WaitForMultipleObjects(2, waitArray, false, 200) != WAIT_OBJECT_0) {
		while (!src->lightQuery.empty()) {
			int maxQlights = (int)src->afx_dev.GetMappings()->size() * 5;
			if (!wasDelay && src->lightQuery.size() > maxQlights) {
				src->GetConfig()->monDelay += 50;
				wasDelay = true;
#ifdef _DEBUG
				char buff[2048];
				sprintf_s(buff, 2047, "Query so big, delay increased to %d ms!\n", src->GetConfig()->monDelay);
				OutputDebugString(buff);
#endif
			}
			src->modifyQuery.lock();
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
				//#ifdef _DEBUG
				//					OutputDebugString("Update:\n");
				//#endif
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
								//#ifdef _DEBUG
								//									OutputDebugString("Updating device...\n");
								//#endif
								if (dev->IsDeviceReady()) {
#ifdef _DEBUG
									if (lights.size() != acts.size())
										OutputDebugString("Data size mismatch!\n");
#endif
									dev->SetMultiColor((int)lights.size(), lights.data(), acts, current.flags);
									dev->UpdateColors();
									src->UpdateGlobalEffect(dev);
								} //else
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
				//src->lightQuery.pop_front();
			} else {
				// set light
				const unsigned delta = 256 - src->GetConfig()->dimmingPower;
				vector<AlienFX_SDK::afx_act> actions;// = current.actions;
				AlienFX_SDK::Functions* dev = src->LocateDev(current.did);
				byte flags = src->afx_dev.GetFlags(current.did, current.lid);
				if (dev) {
					for (int i = 0; i < current.actsize; i++) {
						AlienFX_SDK::afx_act action = current.actions[i];
						// gamma-correction...
						if (src->GetConfig()->gammaCorrection) {
							// TODO - fix max=254 (>>8 not right, 255 in fact)
							action.r = (action.r * action.r) / 255;
							action.g = (action.g * action.g) / 255;
							action.b = (action.b * action.b) / 255;
						}
						// Dimming...
						// Only for v1-v3 devices!
						if (dev->GetVersion() < 4 && src->GetConfig()->stateDimmed && (!flags || src->GetConfig()->dimPowerButton)) {
							action.r = (action.r * delta) >> 8;
							action.g = (action.g * delta) >> 8;
							action.b = (action.b * delta) >> 8;
						}
						actions.push_back(action);
					}

					if (flags & ALIENFX_FLAG_POWER) {
						// Set power immediately!
						if (!src->GetConfig()->block_power && current.actsize > 1) {
#ifdef _DEBUG
							char buff[2048];
							sprintf_s(buff, 2047, "Setting power button to %d-%d-%d/%d-%d-%d\n",
									  actions[0].r, actions[0].g, actions[0].b,
									  actions[1].r, actions[1].g, actions[1].b);
							OutputDebugString(buff);
#endif
							if (!current.flags)
								dev->SetPowerAction(current.lid,
													actions[0].r, actions[0].g, actions[0].b,
													actions[1].r, actions[1].g, actions[1].b);
							else
								actions[0].type = AlienFX_SDK::AlienFX_A_Power;

						}
					}
					if (current.flags || !(flags & ALIENFX_FLAG_POWER)) {
						// find query....
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
		}
		if (src->GetConfig()->monDelay > 200) {
			src->GetConfig()->monDelay -= 10;
#ifdef _DEBUG
			char buff[2048];
			sprintf_s(buff, 2047, "Query empty, delay decreased to %d ms!\n", src->GetConfig()->monDelay);
			OutputDebugString(buff);
#endif
		}
		ResetEvent(src->haveNewElement);
	}
	return 0;
}