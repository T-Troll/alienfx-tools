#include "FXHelper.h"

DWORD WINAPI CLightsProc(LPVOID param);

void FXHelper::TestLight(int did, int id)
{
	AlienFX_SDK::Functions* dev = LocateDev(did);
	if (dev != NULL) {
		int r = (config->testColor.cs.red * config->testColor.cs.red) >> 8,
			g = (config->testColor.cs.green * config->testColor.cs.green) >> 8,
			b = (config->testColor.cs.blue * config->testColor.cs.blue) >> 8;

		bool dev_ready = false;
		for (int c_count = 0; c_count < 20 && !(dev_ready = dev->IsDeviceReady()); c_count++)
			Sleep(20);
		if (!dev_ready) return;

		if (id != lastTest) {
			if (lastTest >= 0) {
				dev->SetColor(lastTest, 0, 0, 0);
				dev->UpdateColors();
			}
			lastTest = id;
		}
		if (id != -1) {
			dev->SetColor(id, r, g, b);
			dev->UpdateColors();
		}
	}
}

void FXHelper::ResetPower(int did)
{
	AlienFX_SDK::Functions* dev = LocateDev(did);
	if (dev != NULL)
		dev->SetPowerAction(63, 0, 0, 0, 0, 0, 0, true);
}

void FXHelper::SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force)
{
#ifdef _DEBUG
	//char buff[2048];
	//sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
	//sprintf_s(buff, 2047, "CounterUpdate: S%d,", afx_dev->AlienfxGetDeviceStatus());
	//OutputDebugString(buff);
	if (force)
		OutputDebugString("Forced Counter update initiated...\n");
#endif

	if (config->autoRefresh) {
		Refresh();
		force = true;
	}

	std::vector <lightset>::iterator Iter;
	bStage = !bStage;
	bool wasChanged = false;
	if (force) {
		lCPU = 101; lRAM = 0; lHDD = 101; lGPU = 101; lNET = 101; lTemp = 101; lBatt = 101;
	}

	bool tHDD = (lHDD && !cHDD) || (!lHDD && cHDD),
		tNet = (lNET && !cNet) || (!lNET && cNet);

	for (Iter = config->active_set.begin(); Iter != config->active_set.end(); Iter++)
		if ((Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)) {
			int mIndex = (afx_dev.GetFlags(Iter->devid, Iter->lightid) & ALIENFX_FLAG_POWER) && Iter->eve[0].map.size() > 1 && activeMode != MODE_AC && activeMode != MODE_CHARGE ? 1 : 0;
			AlienFX_SDK::afx_act fin = Iter->eve[0].fs.b.flags ? Iter->eve[0].map[mIndex] : Iter->eve[2].fs.b.flags ?
				Iter->eve[2].map[0] : Iter->eve[3].map[0], from = fin;
			fin.type = 0;
			bool valid = force ? false : Iter->valid;
			double coeff = 0.0;
			bool gauge = false;
			if (Iter->eve[2].fs.b.flags) {
				// counter
				gauge = Iter->eve[2].fs.b.proc;
				double ccut = Iter->eve[2].fs.b.cut;
				switch (Iter->eve[2].source) {
				case 0: if (valid && (lCPU == cCPU || lCPU < ccut && cCPU < ccut)) continue; coeff = cCPU; break;
				case 1: if (valid && (lRAM == cRAM || lRAM < ccut && cRAM < ccut)) continue; coeff = cRAM; break;
				case 2: if (valid && (lHDD == cHDD || lHDD < ccut && cHDD < ccut)) continue; coeff = cHDD; break;
				case 3: if (valid && (lGPU == cGPU || lGPU < ccut && cGPU < ccut)) continue; coeff = cGPU; break;
				case 4: if (valid && (lNET == cNet || lNET < ccut && cNet < ccut)) continue; coeff = cNet; break;
				case 5: if (valid && (lTemp == cTemp || lTemp < ccut && cTemp < ccut)) continue; coeff = cTemp; break;
				case 6: if (valid && (lBatt == cBatt || lBatt > ccut && cBatt > ccut)) continue; coeff = 100-cBatt; break;
				}
				coeff = coeff > ccut ? (coeff - ccut) / (100.0 - ccut) : 0.0;
				fin.r = (BYTE)(from.r * (1 - coeff) + Iter->eve[2].map[1].r * coeff);
				fin.g = (BYTE)(from.g * (1 - coeff) + Iter->eve[2].map[1].g * coeff);
				fin.b = (BYTE)(from.b * (1 - coeff) + Iter->eve[2].map[1].b * coeff);
			}
			if (Iter->eve[3].fs.b.flags) {
				// indicator
				long indi = 0, ccut = Iter->eve[3].fs.b.cut;
				bool blink = Iter->eve[3].fs.b.proc;
				switch (Iter->eve[3].source) {
				case 0: if (!tHDD && valid && !blink) continue; indi = cHDD; break;
				case 1: if (!tNet && valid && !blink) continue; indi = cNet; break;
				case 2: if (valid && !blink &&
					((lTemp <= ccut && cTemp <= ccut) ||
						(cTemp > ccut && lTemp > ccut))) continue;
					indi = cTemp - ccut; break;
				case 3: if (valid && !blink &&
					((lRAM <= ccut && cRAM <= ccut) || (lRAM > ccut && cRAM > ccut))) continue;
					indi = cRAM - ccut; break;
				case 4: if (valid && !blink &&
					((lBatt >= ccut && cBatt >= ccut) || (lBatt < ccut && cBatt < ccut))) continue;
					indi = ccut - cBatt; break;
				}
				fin = indi > 0 ?
					blink ?
					bStage ?
					Iter->eve[3].map[1] : fin
					: Iter->eve[3].map[1]
					: fin;
			}
			wasChanged = true;
			std::vector<AlienFX_SDK::afx_act> actions;
			if (afx_dev.GetFlags(Iter->devid, Iter->lightid) & ALIENFX_FLAG_POWER)
				if (activeMode != MODE_AC && activeMode != MODE_CHARGE) {
					actions.push_back(Iter->eve[0].map[0]);
					actions.push_back(fin);
				}
				else {
					actions.push_back(fin);
					actions.push_back(Iter->eve[0].map.size() > 1 ? Iter->eve[0].map[1] : Iter->eve[0].map[0]);
				}
			else
				actions.push_back(fin);
			if (!Iter->devid) {
				AlienFX_SDK::group* grp = afx_dev.GetGroupById(Iter->lightid);
				if (grp)
					for (int i = 0; i < grp->lights.size(); i++) {
						int lFlags = afx_dev.GetFlags(grp->lights[i]->devid, grp->lights[i]->lightid) & ALIENFX_FLAG_POWER;
						if (!lFlags)
							if (gauge) {
								// set as gauge...
								actions.clear();
								if (((double) i) / grp->lights.size() < coeff) {
									if (((double) i + 1) / grp->lights.size() < coeff) {
										actions.push_back(Iter->eve[2].map[1]);
									} else {
										// recalc...
										double newCoeff = (coeff - ((double) i) / grp->lights.size()) * grp->lights.size();
										fin.r = (BYTE)(from.r * (1 - newCoeff) + Iter->eve[2].map[1].r * newCoeff);
										fin.g = (BYTE)(from.g * (1 - newCoeff) + Iter->eve[2].map[1].g * newCoeff);
										fin.b = (BYTE)(from.b * (1 - newCoeff) + Iter->eve[2].map[1].b * newCoeff);
										actions.push_back(fin);
									}
								} else
									actions.push_back(from);

							}
							SetLight(grp->lights[i]->devid, grp->lights[i]->lightid, lFlags, actions, &(*Iter), force);
					}
			} else
				Iter->valid = SetLight(Iter->devid, Iter->lightid, afx_dev.GetFlags(Iter->devid, Iter->lightid) & ALIENFX_FLAG_POWER, actions, &(*Iter), force);
		}
	if (wasChanged) {
		UpdateColors();
		lCPU = cCPU; lRAM = cRAM; lHDD = cHDD; lGPU = cGPU; lNET = cNet; lTemp = cTemp; lBatt = cBatt;
	}
}

void FXHelper::UpdateColors(int did)
{
	std::vector<AlienFX_SDK::afx_act> actions;
	LightQuewryElement newBlock = {did, 0, false, false, true, actions, NULL };
	lightQuery.push(newBlock);
	SetEvent(haveNewElement);
}

bool FXHelper::SetLight(int did, int id, bool power, std::vector<AlienFX_SDK::afx_act> actions, lightset* map, bool force)
{
	LightQuewryElement newBlock = {did, id, force, power, false, actions, map };
	lightQuery.push(newBlock);
	SetEvent(haveNewElement);
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
	if (config->enableMon)
		SetCounterColor(lCPU, lRAM, lGPU, lNET, lHDD, lTemp, lBatt, true);
}

void FXHelper::ChangeState(bool newState) {
	for (int i = 0; i < devs.size(); i++) {
		//if (devs[i]->GetVersion() > 3 || config->offPowerButton)
			devs[i]->ToggleState(newState, afx_dev.GetMappings(), config->offPowerButton);
		if (newState && devs[i]->GetVersion() < 4)
			RefreshState();
	}
}

void FXHelper::Start() {
	if (!updateThread) {
		stopQuery = CreateEvent(NULL, false, false, NULL);
		haveNewElement = CreateEvent(NULL, true, false, NULL);
		updateThread = CreateThread(NULL, 0, CLightsProc, this, 0, NULL);
	}
}

void FXHelper::Stop() {
	if (updateThread) {
		SetEvent(stopQuery);
		WaitForSingleObject(updateThread, 3000);
		CloseHandle(stopQuery);
		CloseHandle(updateThread);
		updateThread = NULL;
	}
}

int FXHelper::Refresh(bool forced)
{
#ifdef _DEBUG
	if (forced)
		OutputDebugString("Forced Refresh initiated...\n");
#endif
	config->SetStates();
	for (int i =0; i < config->active_set.size(); i++) {
		config->active_set[i].valid = RefreshOne(&config->active_set[i], forced);
	}
	UpdateColors();
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
	bool ret = false;

	std::vector<AlienFX_SDK::afx_act> actions; AlienFX_SDK::afx_act action;
	actions = map->eve[0].map;
	if (config->monState && !force) {
		if (map->eve[1].fs.b.flags) {
			// use power event;
			if (!map->eve[0].fs.b.flags)
				actions = map->eve[1].map;
			else
				if (actions.size() < 2)
					actions.push_back(map->eve[1].map[1]);
			switch (activeMode) {
			case MODE_BAT:
				action = actions[0]; actions[0] = actions[1]; actions[1] = action;
				actions[0].type = 0;
				break;
			case MODE_LOW:
				action = actions[0]; actions[0] = actions[1]; actions[1] = action;
				actions[0].type = actions[1].type = 1;
				break;
			case MODE_CHARGE:
				actions[0].type = actions[1].type = 2;
				break;
			}
		}
		if ((map->eve[2].fs.b.flags || map->eve[3].fs.b.flags)
			&& config->lightsOn && config->stateOn) return map->valid;
	}
	if (!map->devid) {
		// Group here!
		AlienFX_SDK::group* grp = afx_dev.GetGroupById(map->lightid);
		if (grp) {
			for (int i = 0; i < grp->lights.size(); i++) {
				int lFlags = afx_dev.GetFlags(grp->lights[i]->devid, grp->lights[i]->lightid) & ALIENFX_FLAG_POWER;
				ret = SetLight(grp->lights[i]->devid, grp->lights[i]->lightid, lFlags, actions, map, force);
			}
			if (update)
				UpdateColors();
		}
	} else {
		int lFlags = afx_dev.GetFlags(map->devid, map->lightid) & ALIENFX_FLAG_POWER;
		ret = SetLight(map->devid, map->lightid, lFlags, actions, map, force);
		if (update)
			UpdateColors(map->devid);
	}
	return ret;
}

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	while (WaitForSingleObject(src->stopQuery, 20) == WAIT_TIMEOUT) {
		if (WaitForSingleObject(src->haveNewElement, 0) == WAIT_OBJECT_0)
			if (!src->lightQuery.empty()) {
				LightQuewryElement current = src->lightQuery.front();
				src->lightQuery.pop();
				if (current.update) {
					// update command
					if (src->GetConfig()->stateOn) {
						for (int i = 0; i < src->devs.size(); i++)
							if (current.did == -1 || current.did == src->devs[i]->GetPID())
								src->devs[i]->UpdateColors();
					}
				} else {
					// set light
					const unsigned delta = 256 - src->GetConfig()->dimmingPower;
					std::vector<AlienFX_SDK::afx_act> actions = current.actions;
					AlienFX_SDK::Functions* dev = src->LocateDev(current.did);
					if (dev) {
						for (int i = 0; i < actions.size(); i++) {
							// gamma-correction...
							if (src->GetConfig()->gammaCorrection) {
								// TODO - fix max=254 (>>8 not right, 255 in fact)
								actions[i].r = (actions[i].r * actions[i].r) >> 8;
								actions[i].g = (actions[i].g * actions[i].g) >> 8;
								actions[i].b = (actions[i].b * actions[i].b) >> 8;
							}
							// Dimming...
							if (src->GetConfig()->stateDimmed && (!current.power || src->GetConfig()->dimPowerButton)) {
								actions[i].r = (actions[i].r * delta) >> 8;
								actions[i].g = (actions[i].g * delta) >> 8;
								actions[i].b = (actions[i].b * delta) >> 8;
							}
						}
						bool devReady = false;// = dev->IsDeviceReady();// false;
						int wcount;
						for (wcount = 0; wcount < 20 && !(devReady = dev->IsDeviceReady()) && current.force; wcount++)
							Sleep(20);
						if (devReady) {
							if (current.power) {
								if (!src->GetConfig()->block_power && actions.size() > 1) {
									if (src->GetConfig()->stateOn)
										current.map->valid = dev->SetPowerAction(current.lid, actions[0].r, actions[0].g, actions[0].b,
																actions[1].r, actions[1].g, actions[1].b, current.force);
								}
							}
							else
								if (src->GetConfig()->stateOn) {
									if (actions[0].type == 0)
										current.map->valid = dev->SetColor(current.lid, actions[0].r, actions[0].g, actions[0].b);
									else {
										current.map->valid = dev->SetAction(current.lid, actions);
									}
								}
						}
						else {
							current.map->valid = false;
						#ifdef _DEBUG
							OutputDebugString("Light update skipped!\n");
							if (current.force)
								OutputDebugString("Forced one!!!\n");
						#endif
						}
					}
				}
			} else
				ResetEvent(src->haveNewElement);
	}
	return 0;
}