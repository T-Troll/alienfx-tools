#include "alienfx-gui.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "common.h"
#include "CaptureHelper.h"
#include "GridHelper.h"
#include "WSAudioIn.h"
#include "MonHelper.h"

extern AlienFX_SDK::Afx_action Code2Act(AlienFX_SDK::Afx_colorcode c);

extern EventHandler* eve;
extern HANDLE haveLightFX;
extern MonHelper* mon;

DWORD WINAPI CLightsProc(LPVOID param);

FXHelper::FXHelper() {
	stopQuery = CreateEvent(NULL, false, false, NULL);
	haveNewElement = CreateEvent(NULL, true, false, NULL);
	haveLightFX = CreateEvent(NULL, true, false, "LightFXActive");
}

FXHelper::~FXHelper() {
	Stop();
	CloseHandle(stopQuery);
	CloseHandle(haveNewElement);
	CloseHandle(haveLightFX);
};

void FXHelper::FillAllDevs() {
	Stop();
	if (conf->afx_dev.AlienFXEnumDevices(mon ? mon->acpi : NULL))
		conf->afx_dev.AlienFXApplyDevices();
	if (conf->afx_dev.activeDevices) {
		Start();
		SetState(true);
	}
}

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
			zonemap* zone = conf->FindZoneMap(grp->group);
			conf->zoneUpdate.lockRead();
			for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
				switch (grp->gauge) {
				case 1: // horizontal
					SetZoneLight(t->light, t->x, zone->gMaxX, zone->scaleX, grp->gaugeflags, actions, power);
					break;
				case 2: // vertical
					SetZoneLight(t->light, t->y, zone->gMaxY, zone->scaleY, grp->gaugeflags, actions, power);
					break;
				case 3: // diagonal
					SetZoneLight(t->light, t->x + t->y, zone->gMaxX + zone->gMaxY, zone->scaleX + zone->scaleY, grp->gaugeflags, actions, power);
					break;
				case 4: // back diagonal
					SetZoneLight(t->light, zone->gMaxX - 1 - t->x + t->y, zone->gMaxX + zone->gMaxY, zone->scaleX + zone->scaleY, grp->gaugeflags, actions, power);
					break;
				case 5: // radial
					SetZoneLight(t->light,
						abs(zone->gMaxX / 2 - t->x) + abs(zone->gMaxY / 2 - t->y),
						zone->gMaxX + zone->gMaxY,
						zone->scaleX + zone->scaleY,
						grp->gaugeflags, actions, power);
					break;
				}
			conf->zoneUpdate.unlockRead();
		}
	}
}


void FXHelper::TestLight(AlienFX_SDK::Afx_device* dev, int id, bool force, bool wp)
{
	DebugPrint("Testing light #" + to_string(id));
	if (dev && dev->dev) {
		DebugPrint(", have device");
		AlienFX_SDK::Afx_action c = { wp ? Code2Act(dev->white) : AlienFX_SDK::Afx_action({0})};

		if (force && dev->lights.size()) {
			vector<byte> opLights;
			DebugPrint(", cleanup");
			for (auto lIter = dev->lights.begin(); lIter != dev->lights.end(); lIter++)
				if (lIter->lightid != id && !(lIter->flags & ALIENFX_FLAG_POWER))
					opLights.push_back((byte)lIter->lightid);
			dev->dev->SetMultiColor(&opLights, c);
			dev->dev->UpdateColors();
		}

		if (id != oldtest) {
			if (oldtest >= 0) {
				DebugPrint(", remove old");
				dev->dev->SetColor(oldtest, c);
				oldtest = -1;
			}
			if (id >= 0) {
				DebugPrint(", set");
				dev->dev->SetColor(id, Code2Act(conf->testColor));
				oldtest = id;
			}
			dev->dev->UpdateColors();
		}
	}
	DebugPrint("\n");
}

bool FXHelper::CheckEvent(LightEventData* eData, event* e) {
#define ccut e->cut
//	byte ccut = e->cut;
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
			modifyQuery.lockWrite();
			if (wasLFX) {
				wasLFX = false;
				lightQuery.push({ 0, 2 });
			}
			lightQuery.push(lqe);
			modifyQuery.unlockWrite();
			SetEvent(haveNewElement);
		}
		else
			wasLFX = true;
}

void FXHelper::QueryUpdate(bool force) {
	QueryCommand(LightQueryElement({ NULL, force, 1 }));
	lightsNoDelay = lightQuery.size() < (conf->afx_dev.activeLights << 3);
#ifdef _DEBUG
	if (!lightsNoDelay)
		DebugPrint("Query so big, delayed!\n");
#endif // _DEBUG
}

void FXHelper::SetLight(DWORD lgh, vector<AlienFX_SDK::Afx_action>* actions)
{
	auto dev = conf->afx_dev.GetDeviceById(LOWORD(lgh));
	if (lightsNoDelay && dev && dev->dev && actions->size()) {
		LightQueryElement newBlock{ dev->pid, (byte)HIWORD(lgh), (byte)
			(conf->afx_dev.GetFlags(dev, HIWORD(lgh)) & ALIENFX_FLAG_POWER ? 3 : 0),
			(byte)actions->size() };
		memcpy(newBlock.actions, actions->data(), newBlock.actsize * sizeof(AlienFX_SDK::Afx_action));
		for (int i = 0; i < newBlock.actsize; i++) {
			AlienFX_SDK::Afx_action* action = &newBlock.actions[i];
			// Check and change if Accent
			if (action->type & 0xf0) {
				action->type &= 0xf;
				AlienFX_SDK::Afx_colorcode c;
				c.ci = conf->accentColor;
				action->r = c.b; action->g = c.g; action->b = c.r;
			}
			// gamma-correction...
			if (conf->gammaCorrection) {
				action->r = ((UINT)action->r * action->r * dev->white.r) / 65025; // (255 * 255);
				action->g = ((UINT)action->g * action->g * dev->white.g) / 65025; // (255 * 255);
				action->b = ((UINT)action->b * action->b * dev->white.b) / 65025; // (255 * 255);
			}
		}
		QueryCommand(newBlock);
	}
}

void FXHelper::SetState(bool force) {
	int oldBr = finalBrightness; bool oldPM = finalPBState;
	// Lights on state...
	conf->stateOn = conf->lightsOn && stateScreen && (!conf->offOnBattery || conf->statePower) && stateAction;
	// Dim state...
	conf->stateDimmed = conf->dimmed || stateDim || conf->activeProfile->flags & PROF_DIMMED || (conf->dimmedBatt && !conf->statePower);
	// Brightness
	finalBrightness = (byte)(conf->stateOn ? conf->stateDimmed ? conf->dimmingPower : conf->fullPower : 0);
	// Power button state
	finalPBState = conf->stateOn ? !conf->stateDimmed || conf->dimPowerButton : conf->offPowerButton;
	DebugPrint(string("Status: ") + (force ? "Forced, " : "") + to_string(conf->stateOn) + ", " + to_string(finalBrightness) + "\n");
	if (force || oldBr != finalBrightness || oldPM != finalPBState) {
		if (mDlg)
			conf->SetIconState();
		if (force && !finalPBState)
			QueryCommand(LightQueryElement({ NULL, 1, 2 }));
		QueryCommand(LightQueryElement({ NULL, 0, 2 }));
	}
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Afx_device* dev, bool reset) {
	conf->modifyProfile.lockRead();

	for (auto cdev = conf->afx_dev.fxdevs.begin(); cdev != conf->afx_dev.fxdevs.end(); cdev++) {
		if (cdev->dev && cdev->dev->IsHaveGlobal() && (!dev || (dev->devID == cdev->devID))) {
			if (reset && cdev->version == AlienFX_SDK::API_V5)
				cdev->dev->SetGlobalEffects(0, 1, 1, 0, { 0 }, { 0 });
			for (auto it = conf->activeProfile->effects[cdev->devID].begin(); it != conf->activeProfile->effects[cdev->devID].end(); it++) {
				cdev->dev->SetGlobalEffects(it->globalEffect, it->globalMode, it->colorMode, it->globalDelay,
					it->effColor1,
					it->effColor2);
			}
		}
	}
	conf->modifyProfile.unlockRead();
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
	conf->modifyProfile.lockRead();
	for (groupset& it : conf->activeProfile->lightsets) {
		RefreshZone(&it, false);
	}
	conf->modifyProfile.unlockRead();
	if (!forced && conf->stateEffects && eve) {
		RefreshCounters(NULL, true);
		RefreshAmbient(true);
		RefreshHaptics(true);
		RefreshGrid(true);
	}
	QueryUpdate(forced);
}

void FXHelper::RefreshZone(groupset* map, bool update) {

	if (map && map->color.size()) {
		SetZone(map, &map->color, 0.0);
		if (update)
			QueryUpdate();
	}
}

void FXHelper::RefreshCounters(LightEventData* data, bool fromRefresh)
{
	if (eve->sysmon) {
		//DebugPrint("Counter refresh started\n");
		bool force = !data, wasChanged = false, havePower;
		AlienFX_SDK::Afx_group* grp;
		if (force)
			data = &eData;
		else
			blinkStage = !blinkStage;
		conf->modifyProfile.lockRead();
		for (auto& Iter : conf->activeProfile->lightsets) {
			if (Iter.events.size() && (grp = conf->afx_dev.GetGroupById(Iter.group))) {
				havePower = conf->FindZoneMap(Iter.group)->havePower;
				vector<AlienFX_SDK::Afx_action> actions;
				bool hasDiff = false;
				int lVal = 0, cVal = 0;
				double fCoeff = 0.0;
				if (Iter.fromColor && Iter.color.size()) {
					actions.push_back(havePower && conf->statePower ? Iter.color.back() : Iter.color.front());
					actions.back().type = 0;
				}
				for (auto e = Iter.events.begin(); e != Iter.events.end(); e++) {
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
						if (force || (lVal != cVal && (cVal > e->cut || lVal >= e->cut))) {
							hasDiff = true;
							cVal -= e->cut;
							fCoeff = cVal > 0 ? cVal / (100.0 - e->cut) : 0.0;
							actions.push_back(!Iter.gauge || (Iter.gaugeflags & GAUGE_GRADIENT) ? BlendPower(fCoeff, &actions.back(), &e->to) : e->to);
							if (!Iter.gauge)
								actions.erase(actions.begin());
						}
						break;
					case MON_TYPE_IND: { // indicator
						if (e->source == 7) {
							if (force || eData.ACP != data->ACP || eData.BST != data->BST || (data->BST & 14)) {
								hasDiff = true;
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
							actions.push_back(Iter.color.back());
						}
						else {
							actions.insert(actions.begin(), Iter.color.front());
						}

					SetZone(&Iter, &actions, fCoeff);
				}
			}
		}
		conf->modifyProfile.unlockRead();
		if (wasChanged && !fromRefresh) {
			DebugPrint("Counters changed, updating\n");
			QueryUpdate();
			//memcpy(&eData, data, sizeof(LightEventData));
		}
		//DebugPrint("Counters update finished\n");
	}
}

void FXHelper::RefreshAmbient(bool fromRefresh) {
	if (eve->capt) {
		UCHAR* img = ((CaptureHelper*)eve->capt)->imgz;
		UINT shift = 255 - conf->amb_shift, gridsize = conf->amb_grid.x * conf->amb_grid.y;
		vector<AlienFX_SDK::Afx_action> actions{ {0} };
		bool wasChanged = false;
		conf->modifyProfile.lockRead();
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
		conf->modifyProfile.unlockRead();
		if (wasChanged && !fromRefresh)
			QueryUpdate();
	}
}

void FXHelper::RefreshHaptics(bool fromRefresh) {
	if (eve->audio) {
		int* freq = ((WSAudioIn*)eve->audio)->freqs;
		vector<AlienFX_SDK::Afx_action> actions;
		bool wasChanged = false;
		conf->modifyProfile.lockRead();
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
		conf->modifyProfile.unlockRead();
		if (wasChanged && !fromRefresh)
			QueryUpdate();
	}
}

void FXHelper::RefreshGrid(bool fromRefresh) {
	if (eve->grid) {
		bool wasChanged = false;
		vector<AlienFX_SDK::Afx_action> cur{ {0} };
		conf->modifyProfile.lockRead();
		for (auto ce = conf->activeProfile->lightsets.begin(); ce != conf->activeProfile->lightsets.end(); ce++) {
			if (ce->effect.trigger && !ce->gridop.passive) {
				if (ce->effect.trigger == 4) { // ambient
					CaptureHelper* capt = (CaptureHelper*)ce->effect.capt;
					// update lights
					if (capt && capt->needUpdate) {
						capt->needUpdate = false;
						UINT shift = 255 - conf->amb_shift;
						auto zone = *conf->FindZoneMap(ce->group);
						for (auto lgh = zone.lightMap.begin(); lgh != zone.lightMap.end(); lgh++) {
							int ind = (lgh->x + (lgh->y * zone.gMaxX)) * 3;
							cur.front().r = (capt->imgz[ind + 2] * shift) / 255;
							cur.front().g = (capt->imgz[ind + 1] * shift) / 255;
							cur.front().b = (capt->imgz[ind] * shift) / 255;
							SetLight(lgh->light, &cur);
						}
						wasChanged = true;
					}
				} else {
					if (ce->effect.effectColors.size()) {
						// prepare vars..
						grideffop* effop = &ce->gridop;
						grideffect* eff = &ce->effect;
						// check for initial repaint
						if (effop->stars.empty() || fromRefresh) {
							cur.front() = Code2Act(eff->effectColors.front());
							SetZone(&(*ce), &cur);
							effop->stars.resize(eff->width);
							wasChanged = true;
						}
						// calculate phase
						int cTact = effop->current_tact++;
						int phase = eff->speed < 80 ? cTact / (80 - eff->speed) : cTact * (eff->speed - 79);

						if (phase == effop->effsize * effop->lmp) {
							effop->passive = true;
							continue;
						}

						int backIndex = (eff->flags & GE_FLAG_PHASE ? phase : (phase / effop->effsize)) % (eff->effectColors.size());
						AlienFX_SDK::Afx_action from = Code2Act(eff->flags & GE_FLAG_BACK ? eff->effectColors.front() :
							eff->effectColors[backIndex]);
						backIndex++;
						AlienFX_SDK::Afx_action to = Code2Act(eff->effectColors[backIndex != eff->effectColors.size() ? backIndex : 0]);

						phase %= effop->effsize;

						if (phase > effop->size) // circle by color and direction
							phase = effop->effsize - phase - 1;

						if (ce->gaugeflags & GAUGE_REVERSE)
							phase = effop->size - phase - 2;

						// Set lights
						if (effop->oldphase != phase) {
							// Set grid effect
							double power = 0;
							if (eff->type == 4) {
								// Star field
								auto grp = conf->afx_dev.GetGroupById(ce->group);
								uniform_int_distribution<int> id(0, (int)grp->lights.size() - 1);
								uniform_int_distribution<int> count(5, 25);
								uniform_int_distribution<int> clr(1, (int)eff->effectColors.size() - 1);
								for (auto star = effop->stars.begin(); star != effop->stars.end(); star++) {
									if (star->count >= 0 && star->lightID) {
										// change star color phase
										int halfW = star->maxCount >> 1;
										power = 1.0 - (double)abs(halfW - star->count) / halfW;
										cur.front() = { BlendPower(power,
											&Code2Act(eff->effectColors.front()),
											&Code2Act(eff->effectColors.at(star->colorIndex))) };
										SetLight(star->lightID, &cur);
										star->count--;
									}
									else {
										star->lightID = grp->lights.at(id(conf->rnd)).lgh;
										star->maxCount = count(conf->rnd) << 1;
										star->count = eff->flags & GE_FLAG_CIRCLE ? star->maxCount : (star->maxCount >> 1);
										star->colorIndex = clr(conf->rnd);
									}
								}
							}
							else {
								if (ce->gauge) {
									auto zone = *conf->FindZoneMap(ce->group);

									// Define update size
									int maxPhase = 0, halfW = 0;
									switch (eff->type) {
									case 0: case 5:
										maxPhase = eff->width; break;
									case 1: // Wave
										maxPhase = 2 * eff->width + 1; halfW = maxPhase >> 1; break;
									case 2: case 3: // Fill
										maxPhase = phase+1;
									}
									// cleanup
									for (int dist = 0; dist < maxPhase; dist++)
										SetGaugeGrid(&(*ce), &zone, effop->oldphase - dist, &from);

									if (ce->gaugeflags & GAUGE_GRADIENT) {
										// Gradient fill
										AlienFX_SDK::Afx_action grad = Code2Act(eff->flags & GE_FLAG_BACK ? eff->effectColors.front() :
											backIndex > 0 ? eff->effectColors[backIndex - 1] : eff->effectColors.back());
										for (int nf = 0; nf < effop->size - phase; nf++) {
											// Gradient to previous color
											SetGaugeGrid(&(*ce), &zone, effop->size - nf, &BlendPower((double)nf / (effop->size - phase), &grad, &to));
										}
									}

									for (int dist = 0; dist < maxPhase; dist++) {
										if (phase - dist >= 0) {
											switch (eff->type) {
											case 0: case 3: power = 0; break;
											case 1: power = (double)abs(halfW - dist) / halfW; break;
											case 2: power = (maxPhase ? (double)dist / maxPhase : 0.0); break;
											case 5: power = (double)(dist) / eff->width; break;
											}
											SetGaugeGrid(&(*ce), &zone, phase - dist, &BlendPower(power, &to, &from));
										}
									}
								}
								else {
									// flat morph emulation
									cur.front() = { BlendPower((double)phase / eff->width, &from, &to) };
									SetZone(&(*ce), &cur);
								}
							}
							wasChanged = true;
							effop->oldphase = phase;
						}
					}
				}
			}
		}
		conf->modifyProfile.unlockRead();
		if (wasChanged && !fromRefresh)
			QueryUpdate();
	}
}

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	LightQueryElement current;

	HANDLE waitArray[2]{ src->haveNewElement, src->stopQuery };
	map<WORD, vector<AlienFX_SDK::Afx_lightblock>> devs_query;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	while (WaitForMultipleObjects(2, waitArray, false, INFINITE) == WAIT_OBJECT_0) {
		while (src->lightQuery.size()) {

			src->modifyQuery.lockWrite();
			current = src->lightQuery.front();
			src->lightQuery.pop();
			src->modifyQuery.unlockWrite();

			switch (current.command) {
			case 2: { // set brightness
				bool pbstate = current.light || src->finalPBState, needRefresh = false;
				byte fbright = (byte)(current.light ?
					conf->stateDimmed && conf->dimPowerButton ? conf->dimmingPower : conf->fullPower
					: src->finalBrightness);
					//(byte)(current.light ? !conf->lightsOn && conf->stateDimmed && conf->dimPowerButton ? conf->dimmingPower : conf->fullPower : src->finalBrightness);
				for (auto dev = conf->afx_dev.fxdevs.begin(); dev != conf->afx_dev.fxdevs.end(); dev++)
					if (dev->dev) {
						//DebugPrint("Set brightness " + to_string(src->finalBrightness) + " for device " + to_string(dev->pid) + "\n");
						needRefresh |= dev->dev->SetBrightness(fbright, &dev->lights, pbstate);
					}
				if (needRefresh)
					src->Refresh();
			} break;
			case 1: { // update command
				for (auto devQ = devs_query.begin(); devQ != devs_query.end(); devQ++) {
					AlienFX_SDK::Afx_device* dev = conf->afx_dev.GetDeviceById(devQ->first);
					//DebugPrint("Updating device " + to_string(devQ->first) + ", " + to_string(devQ->second.size()) + " lights\n");
					if (devQ->second.size() && dev->version != AlienFX_SDK::API_UNKNOWN && conf->activeProfile->effects[dev->devID].empty()) {
						dev->dev->SetMultiAction(&devQ->second, current.light);
						//DebugPrint("Set action complete\n");
						dev->dev->UpdateColors();
					}
					devQ->second.clear();
					//DebugPrint("Update complete\n");
				}
			} break;
			case 3: { // set power button
				// Does it have 2 colors?
				if (current.actsize < 2)
					memcpy(&current.actions[1], current.actions, sizeof(AlienFX_SDK::Afx_action));
				// Should we update it?
				current.actions[0].type = current.actions[1].type = AlienFX_SDK::AlienFX_A_Power;
				current.actsize = 2;
				if (memcmp(src->pbstate[current.pid], current.actions, 2 * sizeof(AlienFX_SDK::Afx_action))) {

					DebugPrint("Power button set to " +
						to_string(current.actions[0].r) + "-" +
						to_string(current.actions[0].g) + "-" +
						to_string(current.actions[0].b) + "/" +
						to_string(current.actions[1].r) + "-" +
						to_string(current.actions[1].g) + "-" +
						to_string(current.actions[1].b) + "\n");

					memcpy(src->pbstate[current.pid], current.actions, 2 * sizeof(AlienFX_SDK::Afx_action));
				}
				else {
					DebugPrint("Power button update skipped (blocked or same colors)\n");
					continue;
				}
			} // no break here, need set light!
			case 0: { // set light

				// form actblock...
				AlienFX_SDK::Afx_lightblock ablock{ current.light };
				ablock.act.resize(current.actsize);
				memcpy(ablock.act.data(), current.actions, current.actsize * sizeof(AlienFX_SDK::Afx_action));

				// do we have another set for same light?
				for (auto lp = devs_query[current.pid].begin(); lp != devs_query[current.pid].end(); lp++) {
					if (lp->index == current.light) {
						devs_query[current.pid].erase(lp);
						break;
					}
				}

				devs_query[current.pid].push_back(ablock);
			}
			}
		}
		src->lightsNoDelay = true;
		ResetEvent(src->haveNewElement);
	}
	return 0;
}