#include "alienfx-gui.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "common.h"
#include "GridHelper.h"
#include "WSAudioIn.h"

extern AlienFX_SDK::Afx_action Code2Act(AlienFX_SDK::Afx_colorcode* c);
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::Afx_group* grp);

extern EventHandler* eve;
extern HANDLE haveLightFX;

DWORD WINAPI CLightsProc(LPVOID param);

FXHelper::FXHelper() {
	stopQuery = CreateEvent(NULL, false, false, NULL);
	haveNewElement = CreateEvent(NULL, false, false, NULL);
	haveLightFX = CreateEvent(NULL, true, false, "LightFXActive");
}

FXHelper::~FXHelper() {
	Stop();
	CloseHandle(stopQuery);
	CloseHandle(haveNewElement);
	CloseHandle(haveLightFX);
};

AlienFX_SDK::Afx_action FXHelper::BlendPower(double power, AlienFX_SDK::Afx_action* from, AlienFX_SDK::Afx_action* to) {
	return { 0,0,0,
		(byte)((1.0 - power) * from->r + power * to->r),
		(byte)((1.0 - power) * from->g + power * to->g),
		(byte)((1.0 - power) * from->b + power * to->b)
	};
}

void FXHelper::SetZoneLight(DWORD id, int x, int max, int scale, WORD flags, vector<AlienFX_SDK::Afx_action>* actions, double power)
{
	vector<AlienFX_SDK::Afx_action> fAct;
	if (flags & GAUGE_REVERSE)
		x = max - x - 1;
	if (flags & GAUGE_GRADIENT)
		fAct.push_back(BlendPower((double)x / max, &actions->front(), &actions->back()));
	else {
		double powerStep = 1.0 / scale;
		double pos = (double)x / max;
		if (pos > power) {
			fAct.push_back(actions->front());
		}
		else
			if (pos + powerStep > power) {
				fAct.push_back(BlendPower((power - pos) * scale, &actions->front(), &actions->back()));
			}
			else {
				fAct.push_back(actions->back());
			}
	}
	SetLight(id, &fAct);
}

void FXHelper::SetZone(groupset* grp, vector<AlienFX_SDK::Afx_action>* actions, double power) {
	AlienFX_SDK::Afx_group* cGrp = conf->afx_dev.GetGroupById(grp->group);
	if (cGrp && cGrp->lights.size()) {
		if (!grp->gauge || actions->size() < 2) {
			for (auto i = cGrp->lights.begin(); i < cGrp->lights.end(); i++)
				SetLight(i->lgh, actions);
		}
		else {
			zonemap zone = *conf->FindZoneMap(grp->group);
			for (auto t = zone.lightMap.begin(); t < zone.lightMap.end(); t++)
				switch (grp->gauge) {
				case 1: // horizontal
					SetZoneLight(t->light, t->x, zone.gMaxX, zone.scaleX, grp->gaugeflags, actions, power);
					break;
				case 2: // vertical
					SetZoneLight(t->light, t->y, zone.gMaxY, zone.scaleY, grp->gaugeflags, actions, power);
					break;
				case 3: // diagonal
					SetZoneLight(t->light, t->x + t->y, zone.gMaxX + zone.gMaxY, zone.scaleX + zone.scaleY, grp->gaugeflags, actions, power);
					break;
				case 4: // back diagonal
					SetZoneLight(t->light, zone.gMaxX - 1 - t->x + t->y, zone.gMaxX + zone.gMaxY, zone.scaleX + zone.scaleY, grp->gaugeflags, actions, power);
					break;
				case 5: // radial
					//float px = abs(((float)zone.gMaxX)/2 - t->x), py = abs(((float)zone.gMaxY)/2 - t->y);
					//int radius = (int)(sqrt(zone.gMaxX * zone.gMaxX + zone.gMaxY * zone.gMaxY) / 2),
					//	weight = (int)sqrt(px * px + py * py),
					//	scale = (int)(sqrt(zone.scaleX * zone.scaleX + zone.scaleY * zone.scaleY) / 2);
					//SetZoneLight(t->light, weight, radius, scale, grp->gaugeflags, actions, power);
					SetZoneLight(t->light, 
						abs(zone.gMaxX / 2 - t->x) + abs(zone.gMaxY / 2 - t->y),
						zone.gMaxX + zone.gMaxY,
						zone.scaleX + zone.scaleY,
						grp->gaugeflags, actions, power);
					break;
				}
		}
	}
}


void FXHelper::TestLight(AlienFX_SDK::Afx_device* dev, int id, bool force, bool wp)
{
	if (dev && dev->dev) {

		vector<AlienFX_SDK::Afx_action> c{ {0} };
		if (wp)
			c.front() = { 0, 0, 0, dev->white.r, dev->white.g, dev->white.b };

		if (force) {
			vector<byte> opLights;
			for (auto lIter = dev->lights.begin(); lIter != dev->lights.end(); lIter++)
				if (lIter->lightid != id && !(lIter->flags & ALIENFX_FLAG_POWER))
					opLights.push_back((byte)lIter->lightid);
			dev->dev->SetMultiColor(&opLights, c.front());
			dev->dev->UpdateColors();
		}

		if (id != oldtest) {
			if (oldtest != -1)
				dev->dev->SetAction(oldtest, &c);
			oldtest = id;

			if (id != -1) {
				c.front() = { 0,0,0,conf->testColor.r,conf->testColor.g,conf->testColor.b };
				dev->dev->SetAction(id, &c);
			}
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
			needSet = zone->gMaxX - lgh->x - 1 + lgh->y - grp->gridop.gridX - grp->gridop.gridY == phase;
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

void FXHelper::QueryCommand(LightQueryElement &lqe) {
	if (updateThread)
		if (WaitForSingleObject(haveLightFX, 0) == WAIT_TIMEOUT) {
			modifyQuery.lock();
			if (wasLFX) {
				wasLFX = false;
				lightQuery.push({ 0, 2 });
			}
			lightQuery.push(lqe);
			modifyQuery.unlock();
			SetEvent(haveNewElement);
		}
		else
			wasLFX = true;
}

void FXHelper::QueryUpdate(bool force) {
	QueryCommand(LightQueryElement({ force, 1 }));
	lightsNoDelay = lightQuery.size() < (conf->afx_dev.activeLights << 3);
#ifdef _DEBUG
	if (!lightsNoDelay)
		DebugPrint("Query so big, delayed!\n");
#endif // _DEBUG
}

void FXHelper::SetLight(DWORD lgh, vector<AlienFX_SDK::Afx_action>* actions)
{
	if (actions->size()) {
		LightQueryElement newBlock{ lgh, 0, (byte)actions->size() };
		memcpy(newBlock.actions, actions->data(), newBlock.actsize * sizeof(AlienFX_SDK::Afx_action));
		QueryCommand(newBlock);
	}
}

void FXHelper::SetState(bool force) {
	int oldBr = finalBrightness; bool oldPM = finalPBState;
	// Lights on state...
	conf->stateOn = conf->lightsOn && stateScreen && (!conf->offOnBattery || conf->statePower);
	// Dim state...
	conf->stateDimmed = conf->dimmed || conf->activeProfile->flags & PROF_DIMMED || (conf->dimmedBatt && !conf->statePower);
	// Brightness
	finalBrightness = (byte)(conf->lightsOn ? 255 - (conf->stateDimmed ? conf->dimmingPower : 0) : 0);
	// Power button state
	finalPBState = conf->stateOn ? !conf->stateDimmed || conf->dimPowerButton : conf->offPowerButton;

	if (force) {
		QueryCommand(LightQueryElement({ 1, 2 }));
		QueryCommand(LightQueryElement({ 0, 2 }));
	} else
		if (oldBr != finalBrightness || oldPM != finalPBState) {
			if (mDlg)
				conf->SetIconState();
			QueryCommand(LightQueryElement({ 0, 2 }));
		}
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Afx_device* dev, bool reset) {
	for (auto it = conf->activeProfile->effects.begin(); it < conf->activeProfile->effects.end(); it++) {
		auto cdev = dev ? dev : conf->afx_dev.GetDeviceById(it->pid, it->vid);
		if (cdev->dev)
			cdev->dev->SetGlobalEffects(reset ? 0 : it->globalEffect, it->globalMode, it->colorMode, it->globalDelay,
				{ 0,0,0,it->effColor1.r, it->effColor1.g, it->effColor1.b },
				{ 0,0,0,it->effColor2.r, it->effColor2.g, it->effColor2.b });
	}
}

void FXHelper::Start() {
	if (!updateThread) {
		lightsNoDelay = true;
		updateThread = CreateThread(NULL, 0, CLightsProc, this, 0, NULL);
		DebugPrint("Light updates started.\n");
	}
}

void FXHelper::Stop() {
	if (updateThread) {
		HANDLE oldUpate = updateThread;
		updateThread = NULL;
		SetEvent(stopQuery);
		WaitForSingleObject(oldUpate, 20000);
		CloseHandle(oldUpate);
		DebugPrint("Light updates stopped.\n");
	}
}

void FXHelper::Refresh(bool forced)
{

#ifdef _DEBUG
	if (forced)
		DebugPrint("Forced ");
	DebugPrint("Refresh initiated.\n");
#endif

	for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++) {
		RefreshOne(&(*it), false);
	}
	if (!forced) {
		RefreshCounters();
		RefreshAmbient();
		RefreshHaptics();
		RefreshGrid();
	}
	QueryUpdate(forced);
}

void FXHelper::RefreshOne(groupset* map, bool update) {

	if (map && map->color.size()) {
		SetZone(map, &map->color, 0);
		if (update)
			QueryUpdate();
	}
}

void FXHelper::RefreshCounters(LightEventData* data)
{
	if (lightsNoDelay && eve->sysmon) {
		bool force = !data, wasChanged = false;
		AlienFX_SDK::Afx_group* grp;
		if (!data)
			data = &eData;
		else
			blinkStage = !blinkStage;

		eve->modifyProfile.lock();
		for (auto Iter = conf->activeProfile->lightsets.begin(); Iter != conf->activeProfile->lightsets.end(); Iter++) {
			if (Iter->events.size() && (grp = conf->afx_dev.GetGroupById(Iter->group))) {
				bool havePower = conf->FindZoneMap(Iter->group)->havePower;
				vector<AlienFX_SDK::Afx_action> actions;
				bool hasDiff = false;
				int lVal = 0, cVal = 0;
				double fCoeff = 0.0;
				if (Iter->fromColor && Iter->color.size()) {
					actions.push_back(havePower && conf->statePower ? Iter->color.back() : Iter->color.front());
					actions.back().type = 0;
				}
				for (auto e = Iter->events.begin(); e != Iter->events.end(); e++) {
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
						if (force || (lVal != cVal && (cVal > e->cut || lVal >= e->cut))) {
							hasDiff = true;
							cVal -= e->cut;
							fCoeff = cVal > 0 ? cVal / (100.0 - e->cut) : 0.0;
							if (actions.empty())
								actions.push_back(e->from);
							actions.push_back(!Iter->gauge || (Iter->gaugeflags & GAUGE_GRADIENT) ? BlendPower(fCoeff, &actions.back(), &e->to) : e->to);
							if (!Iter->gauge)
								actions.erase(actions.begin());
						}
						break;
					case MON_TYPE_IND: { // indicator
						if (e->source == 7) {
							if (force || eData.ACP != data->ACP || eData.BST != data->BST || (data->BST & 14)) {
								hasDiff = true;
								if (actions.empty())
									actions.push_back(e->from);
								if (!data->ACP || ((data->BST & 8) && blinkStage)) {
									actions.erase(actions.begin());
									actions.push_back((data->BST & 6) && blinkStage ? AlienFX_SDK::Afx_action{ 0 } : e->to);
								}
							}
						}
						else {
							cVal = CheckEvent(data, &(*e));

							if (force || (cVal + CheckEvent(&eData, &(*e))) == 1 || (e->mode && cVal)) {
								hasDiff = true;
								if (actions.empty())
									actions.push_back(e->from);
								if (cVal && (!e->mode || blinkStage)) {
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

					if (havePower)
						// ToDo: check about 2 lights in actions
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

void FXHelper::RefreshAmbient() {
	if (lightsNoDelay && eve->capt) {
		UCHAR* img = ((CaptureHelper*)eve->capt)->imgz;
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
	if (lightsNoDelay && eve->audio) {
		int* freq = ((WSAudioIn*)eve->audio)->freqs;
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

void FXHelper::RefreshGrid() {
	if (lightsNoDelay && eve->grid) {
		bool wasChanged = false, noAmb = true;
		vector<AlienFX_SDK::Afx_action> cur{ {0} };
		eve->modifyProfile.lock();
		for (auto ce = conf->activeProfile->lightsets.begin(); ce != conf->activeProfile->lightsets.end(); ce++) {
			if (!ce->gridop.passive) {
				if (ce->effect.trigger == 4) { // ambient
					CaptureHelper* capt = ((GridHelper*)eve->grid)->capt;
					// update lights
					if (capt->needUpdate && noAmb) {
						capt->needUpdate = false;
						UINT shift = 255 - conf->amb_shift;
						auto zone = *conf->FindZoneMap(ce->group);
						// resize grid if zone changed
						if (capt->gridX != zone.gMaxX || capt->gridY != zone.gMaxY)
							capt->SetLightGridSize(zone.gMaxX, zone.gMaxY);
						for (auto lgh = zone.lightMap.begin(); lgh != zone.lightMap.end(); lgh++) {
							int ind = (lgh->x + (lgh->y * zone.gMaxX)) * 3;
							cur.front().r = (capt->imgz[ind + 2] * shift) / 255;
							cur.front().g = (capt->imgz[ind + 1] * shift) / 255;
							cur.front().b = (capt->imgz[ind] * shift) / 255;
							SetLight(lgh->light, &cur);
						}
						wasChanged = true;
						noAmb = false;
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

					if (phase == effop->effsize * effop->lmp) {
						effop->passive = true;
						continue;
					}

					int backIndex = (eff->flags & GE_FLAG_PHASE ? phase : (phase / effop->effsize)) % (eff->effectColors.size());
					AlienFX_SDK::Afx_action from = Code2Act(eff->flags & GE_FLAG_BACK ? &eff->effectColors.front() :
						&eff->effectColors[backIndex]);
					backIndex++;
					AlienFX_SDK::Afx_action to = Code2Act(&eff->effectColors[backIndex != eff->effectColors.size() ? backIndex : 0]);

					phase %= effop->effsize;

					if (phase > effop->size) // circle by color and direction
						phase = effop->effsize - phase - 1;

					if (ce->gaugeflags & GAUGE_REVERSE)
						phase = effop->size - phase - 2;

					// Set lights
					if (effop->oldphase != phase || !ce->gauge) {
						// Set grid effect
						double power = 0;

						if (ce->gauge) {
							auto zone = *conf->FindZoneMap(ce->group);
							// Old phase cleanup
							//if (!cTact) {
							//	cur.front() = from;
							//	SetZone(&(*ce), &cur);
							//}
							//else
							for (int dist = 0; dist < eff->width; dist++)
								SetGaugeGrid(&(*ce), &zone, effop->oldphase - dist, &from);

							// Check for gradient zones - in this case all phases updated!
							if (ce->gaugeflags & GAUGE_GRADIENT) {
								for (int nf = 0; nf < effop->size - phase; nf++) {
									power = (double)nf / (effop->size - phase);
									SetGaugeGrid(&(*ce), &zone, effop->size - nf, &BlendPower(power, &from, &to));
								}
							}

							// Fill new phase colors
							if (eff->type == 3) { // Filled
								for (int nf = 0; nf <= phase; nf++) {
									power = ce->gaugeflags & GAUGE_GRADIENT && phase ? (double)nf / phase : 1.0;
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
							power = (double)phase / eff->width;
							cur.front() = { BlendPower((double)phase / eff->width, &from, &to) };
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

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	LightQueryElement current;

	HANDLE waitArray[2]{ src->haveNewElement, src->stopQuery };
	map<WORD, vector<AlienFX_SDK::Afx_lightblock>> devs_query;

	AlienFX_SDK::Afx_device* dev;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	while (WaitForMultipleObjects(2, waitArray, false, INFINITE) == WAIT_OBJECT_0) {
		while (src->lightQuery.size()) {

			src->modifyQuery.lock();
			current = src->lightQuery.front();
			src->lightQuery.pop();
			src->modifyQuery.unlock();

			switch (current.command) {
			case 2: { // set brightness
				bool pbstate = current.light || src->finalPBState, needRefresh = false;
				byte fbright = (byte)(current.light ? 255 - (!conf->lightsOn && conf->stateDimmed && conf->dimPowerButton ? conf->dimmingPower : 0) : src->finalBrightness);
				for (auto dev = conf->afx_dev.fxdevs.begin(); dev != conf->afx_dev.fxdevs.end(); dev++)
					if (dev->dev) {
						byte oldBr = dev->dev->bright;
						dev->dev->SetBrightness(fbright, &dev->lights, pbstate);
						switch (dev->version) {
						case AlienFX_SDK::API_V2: case AlienFX_SDK::API_V3: case AlienFX_SDK::API_V6: case AlienFX_SDK::API_V7:
							// They don't have hardware brightness, so need to set each light again.
							needRefresh = needRefresh || !oldBr || dev->version > AlienFX_SDK::API_V3;
						}
					}
				if (needRefresh)
					src->Refresh();
			} break;
			case 1: // update command
				for (auto devQ = devs_query.begin(); devQ != devs_query.end(); devQ++)
					if ((dev = conf->afx_dev.GetDeviceById(devQ->first)) && dev->dev) {
						if (devQ->second.size()) {
							dev->dev->SetMultiAction(&devQ->second, current.light);
							dev->dev->UpdateColors();
							devQ->second.clear();
						}
						if (dev->dev->IsHaveGlobal())
							src->UpdateGlobalEffect(dev);
					}
				break;
			case 0: { // set light
				WORD pid = LOWORD(current.light);
				if ((dev = conf->afx_dev.GetDeviceById(pid)) && dev->dev) {
					WORD lid = HIWORD(current.light);
					auto map = conf->afx_dev.GetMappingByDev(dev, lid);
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
						// For v7 devices only, other have hardware dimming
						if (conf->stateDimmed && (!flags || conf->dimPowerButton))
							switch (dev->version) {
								/*case AlienFX_SDK::API_V2:
								case AlienFX_SDK::API_V3: */case AlienFX_SDK::API_V7: {
									unsigned delta = 255 - conf->dimmingPower;
									action->r = ((UINT)action->r * delta) / 255;// >> 8;
									action->g = ((UINT)action->g * delta) / 255;// >> 8;
									action->b = ((UINT)action->b * delta) / 255;// >> 8;
								}
							}
					}

					// Is it power button?
					if ((flags & ALIENFX_FLAG_POWER) && dev->version && dev->version < AlienFX_SDK::API_V5) {
						// Should we update it?
						current.actions[0].type = current.actions[1].type = AlienFX_SDK::AlienFX_A_Power;
						current.actsize = 2;
						if (memcmp(src->pbstate[pid], current.actions, 2 * sizeof(AlienFX_SDK::Afx_action))) {

							DebugPrint("Power button set to " +
								to_string(current.actions[0].r) + "-" +
								to_string(current.actions[0].g) + "-" +
								to_string(current.actions[0].b) + "/" +
								to_string(current.actions[1].r) + "-" +
								to_string(current.actions[1].g) + "-" +
								to_string(current.actions[1].b) +
								"\n");

							memcpy(src->pbstate[pid], current.actions, 2 * sizeof(AlienFX_SDK::Afx_action));
						}
						else {
							DebugPrint("Power button update skipped (blocked or same colors)\n");
							continue;
						}
					}

					// form actblock...
					AlienFX_SDK::Afx_lightblock ablock{ (byte)lid };
					ablock.act.resize(current.actsize);
					memcpy(ablock.act.data(), current.actions, current.actsize * sizeof(AlienFX_SDK::Afx_action));
					// do we have another set for same light?
					auto dv = &devs_query[pid];
					auto lp = dv->begin();
					for (; lp != dv->end(); lp++)
						if (lp->index == lid) {
							//DebugPrint("Light " + to_string(lid) + " already in query, updating data.\n");
							lp->act = ablock.act;
							break;
						}
					if (lp == dv->end())
						dv->push_back(ablock);
				}
			}
			}
		}
		src->lightsNoDelay = true;
	}
	return 0;
}