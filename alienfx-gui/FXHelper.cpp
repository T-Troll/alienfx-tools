#include "alienfx-gui.h"
#include "EventHandler.h"

extern AlienFX_SDK::Afx_action* Code2Act(AlienFX_SDK::Afx_colorcode* c);
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::Afx_group* grp);

extern AlienFan_SDK::Control* acpi;
extern EventHandler* eve;

DWORD WINAPI CLightsProc(LPVOID param);

FXHelper::FXHelper() {
	stopQuery = CreateEvent(NULL, false, false, NULL);
	haveNewElement = CreateEvent(NULL, false, false, NULL);
	FillAllDevs(acpi);
}

FXHelper::~FXHelper() {
	Stop();
	CloseHandle(stopQuery);
	CloseHandle(haveNewElement);
};

AlienFX_SDK::Afx_action FXHelper::BlendPower(double power, AlienFX_SDK::Afx_action* from, AlienFX_SDK::Afx_action* to) {
	return { 0,0,0,
		(byte)((1.0 - power) * from->r + power * to->r),
		(byte)((1.0 - power) * from->g + power * to->g),
		(byte)((1.0 - power) * from->b + power * to->b)
	};
}

void FXHelper::SetZoneLight(DWORD id, int x, int max, WORD flags, vector<AlienFX_SDK::Afx_action>* actions, double power, bool force)
{
	vector<AlienFX_SDK::Afx_action> fAct;
	if (flags & GAUGE_REVERSE)
		x = max - x - 1;
	if (flags & GAUGE_GRADIENT)
		fAct.push_back(BlendPower((double)x / max, &actions->front(), &actions->back()));
	else {
		//max++;
		double pos = (double)x / max;
		if (pos > power) {
			fAct.push_back(actions->front());
		}
		else
			if (double(x+1) / max > power) {
				fAct.push_back(BlendPower((power - pos) * max, &actions->front(), &actions->back()));
			}
			else {
				fAct.push_back(actions->back());
			}
	}
	SetLight(id, &fAct, force);
}

void FXHelper::SetZone(groupset* grp, vector<AlienFX_SDK::Afx_action>* actions, double power, bool force) {
	AlienFX_SDK::Afx_group* cGrp = conf->afx_dev.GetGroupById(grp->group);
	if (cGrp && cGrp->lights.size()) {
		if (!grp->gauge || actions->size() < 2) {
			for (auto i = cGrp->lights.begin(); i < cGrp->lights.end(); i++)
				SetLight(i->lgh, actions, force);
		}
		else {
			zonemap zone = *conf->FindZoneMap(grp->group);
			for (auto t = zone.lightMap.begin(); t < zone.lightMap.end(); t++)
				switch (grp->gauge) {
				case 1: // horizontal
					SetZoneLight(t->light, t->x, zone.xMax, grp->flags, actions, power, force);
					break;
				case 2: // vertical
					SetZoneLight(t->light, t->y, zone.yMax, grp->flags, actions, power, force);
					break;
				case 3: // diagonal
					SetZoneLight(t->light, t->x + t->y, zone.xMax + zone.yMax, grp->flags, actions, power, force);
					break;
				case 4: // back diagonal
					SetZoneLight(t->light, zone.xMax - t->x + t->y, zone.xMax + zone.yMax, grp->flags, actions, power, force);
					break;
				case 5: // radial
					float px = abs(((float)zone.xMax)/2 - t->x), py = abs(((float)zone.yMax)/2 - t->y);
					int radius = (int)(sqrt(zone.xMax * zone.xMax + zone.yMax * zone.yMax) / 2),
						weight = (int)sqrt(px * px + py * py);
					SetZoneLight(t->light, weight, radius, grp->flags, actions, power, force);
					break;
				}
		}
	}
}


void FXHelper::TestLight(AlienFX_SDK::Afx_device* dev, int id, bool force, bool wp)
{
	if (dev && dev->dev) {

		AlienFX_SDK::Afx_colorcode c = wp ? dev->white : AlienFX_SDK::Afx_colorcode({ 0 });

		if (force) {
			vector<byte> opLights;
			for (auto lIter = dev->lights.begin(); lIter != dev->lights.end(); lIter++)
				if (lIter->lightid != id && !(lIter->flags & ALIENFX_FLAG_POWER))
					opLights.push_back((byte)lIter->lightid);
			dev->dev->SetMultiColor(&opLights, c);
			dev->dev->UpdateColors();
		}

		if (id != oldtest) {
			if (oldtest != -1)
				dev->dev->SetColor(oldtest, c);
			oldtest = id;

			if (id != -1)
				dev->dev->SetColor(id, conf->testColor);
			dev->dev->UpdateColors();
		}
	}
}

void FXHelper::ResetPower(AlienFX_SDK::Afx_device* dev)
{
	if (dev && dev->dev) {
		vector<AlienFX_SDK::Afx_lightblock> act{ { (byte)63, {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64}, {AlienFX_SDK::AlienFX_A_Power, 3, 0x64}} } };
		dev->dev->SetPowerAction(&act);
	}
}

bool FXHelper::CheckEvent(LightEventData* eData, event* e) {
	byte ccut = e->cut;
	switch (e->source) {
	case 0: return eData->HDD; break;
	case 1: return eData->NET; break;
	case 2: return eData->Temp - ccut > 0; break;
	case 3: return eData->RAM - ccut > 0; break;
	case 4: return ccut - eData->Batt > 0; break;
	case 5: return eData->KBD; break;
	case 6: return eData->PWM - ccut > 0; break;
	}
	return false;
}

void FXHelper::RefreshCounters(LightEventData *data)
{
	if (eve->sysmon) {
		bool force = false, wasChanged = false;
		AlienFX_SDK::Afx_group* grp;
		if (!data) {
			data = &eData;
			force = true;
		} else
			blinkStage = !blinkStage;

		eve->modifyProfile.lock();
		for (auto Iter = conf->activeProfile->lightsets.begin(); Iter != conf->activeProfile->lightsets.end(); Iter++) {
			vector<AlienFX_SDK::Afx_action> actions;
			bool hasDiff = false;
			int lVal = 0, cVal = 0;
			double fCoeff = 0.0;
			if (grp = conf->afx_dev.GetGroupById(Iter->group)) {
				if (Iter->fromColor && Iter->color.size()) {
					actions.push_back(grp->have_power && conf->statePower ? Iter->color.back() : Iter->color.front());
					actions.back().type = 0;
				}
				for (auto e = Iter->events.begin(); e != Iter->events.end(); e++) {
					if (actions.empty())
						actions.push_back(e->from);
					switch (e->state) {
					case MON_TYPE_PERF: // counter
						switch (e->source) {
						case 0: lVal = eData.CPU; cVal = data->CPU; break;
						case 1: lVal = eData.RAM; cVal = data->RAM; break;
						case 2: lVal = eData.HDD; cVal = data->HDD; break;
						case 3: lVal = eData.GPU; cVal = data->GPU; break;
						case 4: lVal = eData.NET; cVal = data->NET; break;
						case 5: lVal = eData.Temp; cVal = data->Temp; break;
						case 6: lVal = eData.Batt; cVal = data->Batt; break;
						case 7: lVal = eData.Fan; cVal = data->Fan; break;
						case 8: lVal = eData.PWR; cVal = data->PWR; break;
						case 9: lVal = eData.PWM; cVal = data->PWM; break;
						}

						if (force || (lVal != cVal && (lVal > e->cut || cVal > e->cut))) {
							hasDiff = true;
							fCoeff = /*e->coeff =*/ cVal > e->cut ? (cVal - e->cut) / (100.0 - e->cut) : 0.0;
							if (Iter->gauge && !(Iter->flags & GAUGE_GRADIENT))
								actions.push_back(e->to);
							else {
								actions.push_back(BlendPower(fCoeff/*e->coeff*/, &actions.back(), &e->to));
								if (!Iter->gauge)
									actions.erase(actions.begin());
							}
						}
						break;
					case MON_TYPE_IND: { // indicator
						if (e->source == 7) {
							if (force || eData.ACP != data->ACP || eData.BST != data->BST || data->BST & 14) {
								if (data->ACP) {
									if ((data->BST & 8) && blinkStage)
										actions[0] = e->to;
								}
								else
									actions[0] = (data->BST & 6) && blinkStage ? AlienFX_SDK::Afx_action{ 0 } : e->to;
								hasDiff = true;
							}
						}
						else {
							//lVal = CheckEvent(&eData, &(*e));
							cVal = CheckEvent(data, &(*e));

							if (force || (cVal + CheckEvent(&eData, &(*e))) == 1) {
								hasDiff = true;
								if (cVal) {
									actions.erase(actions.begin());
									actions.push_back(e->to);
								}
							}
							else
								if (e->mode && cVal) {
									hasDiff = true;
									if (blinkStage) {
										actions.erase(actions.begin());
										actions.push_back(e->to);
									}
								}
						}
					} break;
					}
				}

				// set if changed
				if (hasDiff) {
					wasChanged = true;

					if (grp->have_power)
						if (conf->statePower) {
							actions.push_back(Iter->color.back());
						}
						else {
							actions.insert(actions.begin(), Iter->color.front());
						}

					SetZone(&(*Iter), &actions, fCoeff);
				}
			}
		}
		eve->modifyProfile.unlock();
		if (wasChanged) {
			QueryUpdate();
		}
	}
}

void FXHelper::SetGaugeGrid(groupset* grp, zonemap* zone, int phase, AlienFX_SDK::Afx_action* fin) {
	vector<AlienFX_SDK::Afx_action> cur{ *fin };
	for (auto lgh = zone->lightMap.begin(); lgh != zone->lightMap.end(); lgh++) {
		bool needSet = false;
		switch (grp->gauge) {
		case 1: // horizontal
			needSet = lgh->x == phase;
			break;
		case 2: // vertical
			needSet = lgh->y == phase;
			break;
		case 3: // diagonal
			needSet = lgh->x + lgh->y - grp->gridop.gridX - grp->gridop.gridY == phase;
			break;
		case 4: // back diagonal
			needSet = lgh->x + zone->yMax - lgh->y - grp->gridop.gridX - grp->gridop.gridY == phase;
			break;
		case 5: // radial
			needSet = (abs(lgh->x - grp->gridop.gridX) == phase && abs(lgh->y - grp->gridop.gridY) <= phase)
				|| (abs(lgh->y - grp->gridop.gridY) == phase && abs(lgh->x - grp->gridop.gridX) <= phase);
			break;
		}
		if (needSet)
			SetLight(lgh->light, &cur);
	}
}

void FXHelper::RefreshGrid() {
	if (eve->grid) {
		bool wasChanged = false;
		vector<AlienFX_SDK::Afx_action> cur{ {0} };
		eve->modifyProfile.lock();
		for (auto ce = conf->activeProfile->lightsets.begin(); ce != conf->activeProfile->lightsets.end(); ce++) {
			if (!ce->gridop.passive) {
				if (ce->effect.trigger == 4) { // ambient
					auto capt = eve->grid->capt;
					if (capt->needUpdate) {
						UINT shift = 255 - conf->amb_shift;
						auto zone = *conf->FindZoneMap(ce->group);
						// resize grid if zone changed
						if (capt->gridX != zone.xMax || capt->gridY != zone.yMax)
							capt->SetLightGridSize(zone.xMax, zone.yMax);
						for (auto lgh = zone.lightMap.begin(); lgh != zone.lightMap.end(); lgh++) {
							int ind = (lgh->x + (lgh->y * zone.xMax)) * 3;
							cur.front().r = (capt->imgz[ind + 2] * shift) / 255;
							cur.front().g = (capt->imgz[ind + 1] * shift) / 255;
							cur.front().b = (capt->imgz[ind] * shift) / 255;
							SetLight(lgh->light, &cur);
						}
						wasChanged = true;
					}
					continue;
				}
				if (ce->effect.effectColors.size()) {
					// prepare vars..
					grideffop* effop = &ce->gridop;
					grideffect* eff = &ce->effect;
					// calculate phase
					int cTact = effop->current_tact++;
					int phase = eff->speed < 80 ? cTact / (80 - eff->speed) : cTact * (eff->speed - 79);
					int lmp = eff->flags & GE_FLAG_PHASE ? 1 : (int)eff->effectColors.size() - 1;
					int effsize = eff->flags & GE_FLAG_CIRCLE ? (effop->size << 1) : effop->size;

					if (phase > effsize * lmp) {
						effop->passive = true;
						continue;
					}

					int colorIndex = (eff->flags & GE_FLAG_PHASE ? phase : phase / effsize) % (eff->effectColors.size());

					AlienFX_SDK::Afx_action from = *Code2Act(eff->flags & GE_FLAG_BACK ? &eff->effectColors.front() :
						&eff->effectColors[colorIndex]),
						to = *Code2Act(&eff->effectColors[colorIndex + 1 < eff->effectColors.size() ? colorIndex + 1 : 0]);

					phase %= effsize;

					if (phase >= effop->size) // circle by color and direction
						phase = effsize - phase - 1;

					if (ce->flags & GAUGE_REVERSE)
						phase = effop->size - phase - 1;

					// Set lights
					if (effop->oldphase != phase) {
						// Set grid effect
						double power = 0;

						if (ce->gauge) {
							auto zone = *conf->FindZoneMap(ce->group);
							// Old phase cleanup
							if (!cTact/*effop->oldphase < 0*/) {
								cur.front() = from;
								SetZone(&(*ce), &cur);
							}
							else
								for (int dist = 0; dist < eff->width; dist++)
									SetGaugeGrid(&(*ce), &zone, effop->oldphase - dist, &from);

							// Check for gradient zones - in this case all phases updated!
							if (ce->flags & GAUGE_GRADIENT) {
								for (int nf = 0; nf < effop->size - phase; nf++) {
									power = (double)nf / (effop->size - phase);
									SetGaugeGrid(&(*ce), &zone, effop->size - nf, &BlendPower(power, &from, &to));
								}
							}

							// Fill new phase colors
							if (eff->type == 3) { // Filled
								for (int nf = 0; nf <= phase; nf++) {
									power = ce->flags & GAUGE_GRADIENT && phase ? (double)(nf) / (phase) : 1.0;
									SetGaugeGrid(&(*ce), &zone, nf, &BlendPower(power, &from, &to));
								}
							}
							else {
								int halfW = eff->width / 2;
								for (int dist = 0; dist < eff->width; dist++) {
									if (phase - dist >= 0) {
										// make final color for this distance
										power = 1.0;
										if (halfW) // do not need if size < 2
											switch (eff->type) {
											case 1: // wave
												power -= (double)abs(halfW - dist) / halfW;
												break;
											case 2: // gradient
												power -= (double)(dist) / eff->width;
											}
										SetGaugeGrid(&(*ce), &zone, phase - dist, &BlendPower(power, &from, &to));
									}
								}
							}
						}
						else {
							// flat morph emulation
							power = (double)phase / eff->width;// effsize;
							cur.front() = { BlendPower(power, &from, &to) };
							SetZone(&(*ce), &cur);
						}
						wasChanged = true;
						effop->oldphase = phase;
					}
				}
			}
		}
		eve->modifyProfile.unlock();
		if (wasChanged)
			QueryUpdate();
	}
}

void FXHelper::QueryUpdate(bool force)
{
	if (conf->stateOn) {
		//LightQueryElement newBlock{0, 0, force, true};
		modifyQuery.lock();
		lightQuery.push({ 0, force, true });
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

void FXHelper::SetLight(DWORD lgh, vector<AlienFX_SDK::Afx_action>* actions, bool force)
{
	if (conf->stateOn && !actions->empty()) {
		LightQueryElement newBlock{ lgh, force, false, (byte)actions->size() };
		for (int i = 0; i < newBlock.actsize; i++)
			newBlock.actions[i] = actions->at(i);
		modifyQuery.lock();
		lightQuery.push(newBlock);
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

void FXHelper::SetState(bool force) {
	if (conf->SetStates() || force) {
		bool updates = updateThread;
		Stop();
		for (auto i = conf->afx_dev.fxdevs.begin(); i < conf->afx_dev.fxdevs.end(); i++) {
			if (i->dev) {
				i->dev->powerMode = conf->statePower;
				i->dev->ToggleState(conf->finalBrightness, &i->lights, conf->finalPBState);
				switch (i->dev->GetVersion()) {
				case AlienFX_SDK::API_V1: case AlienFX_SDK::API_V2: case AlienFX_SDK::API_V3:
					Refresh();
					break;
				case AlienFX_SDK::API_V6: case AlienFX_SDK::API_V7:
					Refresh(true);
				}
			}
		}
		if (updates) Start();
	}
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Functions* dev, bool reset) {
	for (auto it = conf->activeProfile->effects.begin(); it < conf->activeProfile->effects.end(); it++)
	{
		AlienFX_SDK::Functions* cdev = dev ? dev : conf->afx_dev.GetDeviceById(it->pid, it->vid)->dev;

		if (cdev && (cdev->GetPID() == it->pid && cdev->GetVID() == it->vid)) {
			// set this effect for device
			if (reset)
				cdev->SetGlobalEffects(0, it->globalMode, it->globalDelay, { 0 }, { 0 });
			else
				cdev->SetGlobalEffects(it->globalEffect, it->globalMode, it->globalDelay,
					{ 0,0,0,it->effColor1.r, it->effColor1.g, it->effColor1.b },
					{ 0,0,0,it->effColor2.r, it->effColor2.g, it->effColor2.b });
		}
	}
}

void FXHelper::FillAllDevs(AlienFan_SDK::Control* acc) {
	conf->SetStates();
	Stop();
	conf->afx_dev.AlienFXAssignDevices(acc, conf->finalBrightness, conf->finalPBState);
	if (conf->afx_dev.activeDevices)
		Start();
}

void FXHelper::Start() {
	if (!updateThread) {
		conf->lightsNoDelay = true;
		updateThread = CreateThread(NULL, 0, CLightsProc, this, 0, NULL);
		DebugPrint("Light updates started.\n");
	}
}

void FXHelper::Stop() {
	if (updateThread) {
		if (lightQuery.size())
			QueryUpdate();
		SetEvent(stopQuery);
		//conf->lightsNoDelay = false;
		WaitForSingleObject(updateThread, 60000);
		CloseHandle(updateThread);
		updateThread = NULL;
		DebugPrint("Light updates stopped.\n");
	}
}

void FXHelper::Refresh(int forced)
{

#ifdef _DEBUG
	if (forced) {
		DebugPrint("Forced Refresh initiated in mode " + to_string(forced) + "\n");
	} else
		DebugPrint("Refresh initiated.\n");
#endif
	for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++) {
		RefreshOne(&(*it), false, forced);
	}
	if (!forced) {
		RefreshCounters();
		RefreshAmbient();
		RefreshHaptics();
		RefreshGrid();
		QueryUpdate();
	}
	else
		QueryUpdate(forced == 2);
}

void FXHelper::RefreshOne(groupset* map, bool update, int force) {

	if (map && map->color.size()) {
		SetZone(map, &map->color, 0, force == 2);
		if (update)
			QueryUpdate(force == 2);
	}
}

void FXHelper::RefreshAmbient() {
	if (eve->capt) {
		UCHAR* img = eve->capt->imgz;
		UINT shift = 255 - conf->amb_shift, gridsize = conf->amb_grid.x * conf->amb_grid.y;
		vector<AlienFX_SDK::Afx_action> actions{ {0} };
		bool wasChanged = false;
		eve->modifyProfile.lock();
		for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++)
			if (it->ambients.size()) {
				ULONG r = 0, g = 0, b = 0, dsize = (UINT)it->ambients.size();
				for (auto cAmb = it->ambients.begin(); dsize && cAmb != it->ambients.end(); cAmb++) {
					if (*cAmb < gridsize) {
						int ind = *cAmb * 3;
						r += img[ind + 2];
						g += img[ind + 1];
						b += img[ind];
					}
					else {
						dsize--;
					}
				}
				// Multi-lights and brightness correction...
				if (dsize) {
					wasChanged = true;
					dsize *= 255;
					actions.front().r = (BYTE)((r * shift) / dsize);
					actions.front().g = (BYTE)((g * shift) / dsize);
					actions.front().b = (BYTE)((b * shift) / dsize);
					if (it->gauge)
						actions.push_back({ 0 });
					SetZone(&(*it), &actions);
				}
			}
		eve->modifyProfile.unlock();
		if (wasChanged)
			QueryUpdate();
	}
}

void FXHelper::RefreshHaptics() {
	if (eve->audio) {
		int* freq = eve->audio->freqs;
		vector<AlienFX_SDK::Afx_action> actions;
		bool wasChanged = false;
		eve->modifyProfile.lock();
		for (auto mIter = conf->activeProfile->lightsets.begin(); mIter != conf->activeProfile->lightsets.end(); mIter++) {
			if (mIter->haptics.size()) {
				// Now for each freq block...
				long from_r = 0, from_g = 0, from_b = 0, to_r = 0, to_g = 0, to_b = 0, cur_r = 0, cur_g = 0, cur_b = 0, groupsize = 0;
				double f_power = 0.0;
				for (auto fIter = mIter->haptics.begin(); fIter < mIter->haptics.end(); fIter++) {
					if (!fIter->freqID.empty() && fIter->hicut > fIter->lowcut) {
						double power = 0.0;
						groupsize++;
						// here need to check less bars...
						for (auto iIter = fIter->freqID.begin(); iIter < fIter->freqID.end(); iIter++)
							power += (freq[*iIter] > fIter->lowcut ? freq[*iIter] < fIter->hicut ?
								freq[*iIter] - fIter->lowcut : fIter->hicut - fIter->lowcut : 0);
						power = power / (fIter->freqID.size() * (fIter->hicut - fIter->lowcut));
						f_power += power;

						if (mIter->gauge) {
							from_r += fIter->colorfrom.r;
							from_g += fIter->colorfrom.g;
							from_b += fIter->colorfrom.b;

							to_r += fIter->colorto.r;
							to_g += fIter->colorto.g;
							to_b += fIter->colorto.b;
						}
						else {
							cur_r += (byte)sqrt((1.0 - power) * fIter->colorfrom.r * fIter->colorfrom.r + power * fIter->colorto.r * fIter->colorto.r);
							cur_g += (byte)sqrt((1.0 - power) * fIter->colorfrom.g * fIter->colorfrom.g + power * fIter->colorto.g * fIter->colorto.g);
							cur_b += (byte)sqrt((1.0 - power) * fIter->colorfrom.b * fIter->colorfrom.b + power * fIter->colorto.b * fIter->colorto.b);
						}

						// change color if peak
						if (fIter->colorto.br && power == 1.0) {
							conf->SetRandomColor(&fIter->colorto);
						}
					}
				}

				if (groupsize) {

					if (mIter->gauge) {
						actions = { { 0,0,0,(byte)(from_r / groupsize),(byte)(from_g / groupsize),(byte)(from_b / groupsize)},
							{0,0,0,(byte)(to_r / groupsize),(byte)(to_g / groupsize),(byte)(to_b / groupsize)} };
					}
					else
						actions = { { 0,0,0,(byte)(cur_r / groupsize),(byte)(cur_g / groupsize),(byte)(cur_b / groupsize) } };

					SetZone(&(*mIter), &actions, f_power / groupsize);
					wasChanged = true;
				}
			}
		}
		eve->modifyProfile.unlock();
		if (wasChanged)
			QueryUpdate();
	}
}

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	LightQueryElement current;

	HANDLE waitArray[2]{ src->haveNewElement, src->stopQuery };
	map<WORD, vector<AlienFX_SDK::Afx_lightblock>> devs_query;

	AlienFX_SDK::Afx_device* dev;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	while (WaitForMultipleObjects(2, waitArray, false, INFINITE) == WAIT_OBJECT_0) {
		while (src->lightQuery.size() /*&& conf->stateOn*/) {

			src->modifyQuery.lock();
			current = src->lightQuery.front();
			src->lightQuery.pop();
			src->modifyQuery.unlock();

			//#ifdef _DEBUG
			//				char buff[2048];
			//				sprintf_s(buff, 2047, "New light update: (%d,%d),u=%d (%ld remains)...\n", current.did, current.lid, current.update, src->lightQuery.size());
			//				OutputDebugString(buff);
			//#endif
			if (current.update) {
				// update command
				for (auto devQ=devs_query.begin(); devQ != devs_query.end(); devQ++) {
					if (devQ->second.size()) {
//#ifdef _DEBUG
//							char buff[2048];
//							sprintf_s(buff, 2047, "Starting update for %d, (%d lights, %d in query)...\n", devQ->devID, devQ->dev_query.size(), src->lightQuery.size());
//							OutputDebugString(buff);
//#endif
						if ((dev = conf->afx_dev.GetDeviceById(devQ->first)) && dev->dev) {
							dev->dev->SetMultiAction(&devQ->second, current.flags);
							dev->dev->UpdateColors();
							if (dev->dev->IsHaveGlobal())
								src->UpdateGlobalEffect(dev->dev);
						}
						devQ->second.clear();
					}
				}

				conf->lightsNoDelay = src->lightQuery.size() < conf->afx_dev.activeLights * 5;
#ifdef _DEBUG
				if (!conf->lightsNoDelay)
					DebugPrint("Query so big, delayed!\n");
#endif // _DEBUG

				//if (src->lightQuery.size() > conf->afx_dev.activeLights * 5) {
				//	conf->lightsNoDelay = false;
				//	DebugPrint("Query so big, delayed!\n");
				//}

			} else {
				// set light
				WORD pid = LOWORD(current.light);
				WORD lid = HIWORD(current.light);
				if ((dev = conf->afx_dev.GetDeviceById(pid)) && dev->dev) {
					WORD flags = conf->afx_dev.GetFlags(dev, lid);
					for (int i = 0; i < current.actsize; i++) {
						AlienFX_SDK::Afx_action* action = &current.actions[i];
						// gamma-correction...
						if (conf->gammaCorrection) {
							action->r = ((UINT)action->r * action->r * dev->white.r) / 65025; // (255 * 255);
							action->g = ((UINT)action->g * action->g * dev->white.g) / 65025; // (255 * 255);
							action->b = ((UINT)action->b * action->b * dev->white.b) / 65025; // (255 * 255);
						}
						// Dimming...
						// For v1-v3 and v7 devices only, other have hardware dimming
						if (conf->IsDimmed() && (!flags || conf->dimPowerButton))
							switch (dev->dev->GetVersion()) {
							case AlienFX_SDK::API_V1: case AlienFX_SDK::API_V2: case AlienFX_SDK::API_V3: case AlienFX_SDK::API_V7: {
								unsigned delta = 255 - conf->dimmingPower;
								action->r = ((UINT)action->r * delta) / 255;// >> 8;
								action->g = ((UINT)action->g * delta) / 255;// >> 8;
								action->b = ((UINT)action->b * delta) / 255;// >> 8;
							}
							}
						//DebugPrint(("Light for #" + to_string(current.did) + ", ID " + to_string(current.lid) +
						//" to (" + to_string(action.r) + "," + to_string(action.g) + "," + to_string(action.b) + ")\n").c_str());
						//actions.push_back(action);
					}

					// Is it power button?
					if (flags & ALIENFX_FLAG_POWER) {
						// Should we update it?
						current.actions[0].type = current.actions[1].type = AlienFX_SDK::AlienFX_A_Power;
						current.actsize = 2;
						if (!conf->block_power && (current.flags || memcmp(src->pbstate[pid], current.actions, 2 * sizeof(AlienFX_SDK::Afx_action)))) {

							DebugPrint("Power button set to " +
										to_string(current.actions[0].r) + "-" +
										to_string(current.actions[0].g) + "-" +
										to_string(current.actions[0].b) + "/" +
										to_string(current.actions[1].r) + "-" +
										to_string(current.actions[1].g) + "-" +
										to_string(current.actions[1].b) +
										"\n");

							memcpy(src->pbstate[pid], current.actions, 2 * sizeof(AlienFX_SDK::Afx_action));
						} else {
							DebugPrint("Power button update skipped (blocked or same colors)\n");
							continue;
						}
					}

					// form actblock...
					AlienFX_SDK::Afx_lightblock ablock{ (byte)lid };
					ablock.act.resize(current.actsize);
					memcpy(ablock.act.data(), current.actions, current.actsize * sizeof(AlienFX_SDK::Afx_action));
					// do we have another set for same light?
					vector<AlienFX_SDK::Afx_lightblock>* dv = &devs_query[pid];
					auto lp = find_if(dv->begin(), dv->end(),
						[lid](auto acb) {
							return acb.index == lid;
						});
					if (lp == dv->end())
						dv->push_back(ablock);
					else {
						//DebugPrint("Light already in query, updating data.\n");
						lp->act = ablock.act;
					}
				}
			}
		}
	}
	return 0;
}