#include "alienfx-gui.h"
#include "EventHandler.h"

extern AlienFX_SDK::Afx_action* Code2Act(AlienFX_SDK::Afx_colorcode* c);
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::Afx_group* grp);

extern EventHandler* eve;

DWORD WINAPI CLightsProc(LPVOID param);

FXHelper::FXHelper() {
	stopQuery = CreateEvent(NULL, false, false, NULL);
	haveNewElement = CreateEvent(NULL, false, false, NULL);
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
		x = max - x;
	if (flags & GAUGE_GRADIENT)
		power = (double)x / max;
	else {
		max++;
		double pos = (double)x / max;
		if (pos > power) {
			fAct.push_back(actions->front());
			goto setlight;
		}
		else
			if (double(x+1) / max > power) {
				power = (power - pos) * max;
			}
			else {
				fAct.push_back(actions->back());
				goto setlight;
			}
	}
	fAct.push_back(BlendPower(power, &actions->front(), &actions->back()));
	setlight:
	SetLight(LOWORD(id), HIWORD(id), &fAct, force);
}

void FXHelper::SetZone(groupset* grp, vector<AlienFX_SDK::Afx_action>* actions, double power, bool force) {
	AlienFX_SDK::Afx_group* cGrp = conf->afx_dev.GetGroupById(grp->group);
	if (cGrp && cGrp->lights.size()) {
		if (!grp->gauge || actions->size() < 2) {
			for (auto i = cGrp->lights.begin(); i < cGrp->lights.end(); i++)
				SetLight(i->did, i->lid, actions, force);
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

void FXHelper::RefreshCounters(LightEventData *data, bool force)
{
#ifdef _DEBUG
	if (force) {
		DebugPrint("Forced Counter update initiated...\n");
	}
#endif

	if (conf->enableMon && conf->GetEffect() == 1) {
		if (!data) {
			data = &eData;
			force = true;
		}
		if (!force) blinkStage = !blinkStage;
		bool wasChanged = false;

		eve->modifyProfile.lock();
		vector<groupset> active = conf->activeProfile->lightsets;
		eve->modifyProfile.unlock();
		AlienFX_SDK::Afx_group* grp;

		for (auto Iter = active.begin(); Iter != active.end(); Iter++) {
			vector<AlienFX_SDK::Afx_action> actions;
			bool noDiff = true;
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
					case MON_TYPE_POWER: // power
						if (force || eData.ACP != data->ACP || eData.BST != data->BST || data->BST & 14) {
							if (data->ACP) {
								if ((data->BST & 8) && blinkStage)
									actions[0] = e->to;
							}
							else
								actions[0] = (data->BST & 6) && blinkStage ? AlienFX_SDK::Afx_action{ 0 } : e->to;
							noDiff = false;
						}
						break;
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
						}

						if (force || (lVal != cVal && (lVal > e->cut || cVal > e->cut))) {
							noDiff = false;
							fCoeff = e->coeff = cVal > e->cut ? (cVal - e->cut) / (100.0 - e->cut) : 0.0;
						}
						if (Iter->gauge && !(Iter->flags & GAUGE_GRADIENT))
							actions.push_back(e->to);
						else {
							actions.push_back(BlendPower(e->coeff, &actions.front(), &e->to));
							if (!Iter->gauge)
								actions.erase(actions.begin());
						}
						break;
					case MON_TYPE_IND: { // indicator
						int ccut = e->cut;
						switch (e->source) {
						case 0: lVal = eData.HDD; cVal = data->HDD; break;
						case 1: lVal = eData.NET; cVal = data->NET; break;
						case 2: lVal = eData.Temp - ccut; cVal = data->Temp - ccut; break;
						case 3: lVal = eData.RAM - ccut; cVal = data->RAM - ccut; break;
						case 4: lVal = ccut - eData.Batt; cVal = ccut - data->Batt; break;
						case 5: lVal = eData.KBD; cVal = data->KBD; break;
						}

						if (force || ((byte)(cVal > 0) + (byte)(lVal > 0)) == 1) {
							noDiff = false;
							if (cVal > 0) {
								actions.erase(actions.begin());
								actions.push_back(e->to);
							}
						}
						else
							if (e->mode && cVal > 0) {
								noDiff = false;
								if (blinkStage) {
									actions.erase(actions.begin());
									actions.push_back(e->to);
								}
							}
					} break;
					}
				}

				// check for change.
				if (noDiff)	continue;
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
		if (wasChanged) {
			QueryUpdate();
		}
	}
}

void FXHelper::SetGaugeGrid(groupset* grp, zonemap* zone, int phase, AlienFX_SDK::Afx_action* fin) {
	vector<AlienFX_SDK::Afx_action> cur{ *fin };
	for (auto lgh = zone->lightMap.begin(); lgh != zone->lightMap.end(); lgh++) {
		switch (grp->gauge) {
		case 1: // horizontal
			if (lgh->x == phase)
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), &cur);
			break;
		case 2: // vertical
			if (lgh->y == phase)
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), &cur);
			break;
		case 3: // diagonal
			if (lgh->x + lgh->y - grp->gridop.gridX - grp->gridop.gridY == phase)
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), &cur);
			break;
		case 4: // back diagonal
			if (lgh->x + zone->yMax - lgh->y - grp->gridop.gridX - grp->gridop.gridY == phase)
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), &cur);
			break;
		case 5: // radial
			if ((abs(lgh->x - grp->gridop.gridX) == phase && abs(lgh->y - grp->gridop.gridY) <= phase)
				|| (abs(lgh->y - grp->gridop.gridY) == phase && abs(lgh->x - grp->gridop.gridX) <= phase))
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), &cur);
			break;
		}
	}
}

void FXHelper::RefreshGrid(long tact) {
	bool wasChanged = false;
	for (auto ce = conf->activeProfile->lightsets.begin(); ce != conf->activeProfile->lightsets.end(); ce++) {
		if (!ce->gridop.passive && ce->effect.effectColors.size()) {
			// prepare vars..
			grideffop* effop = &ce->gridop;
			grideffect* eff = &ce->effect;
			// calculate phase
			int cTact = abs(tact - effop->start_tact);
			int phase = eff->speed < 80 ? cTact / (80 - eff->speed) : cTact * (eff->speed - 79);
			int lmp = eff->flags & GE_FLAG_PHASE ? 1 : (int)eff->effectColors.size() /*- 1*/;
			int realsize = effop->size + eff->width;
			int effsize = eff->flags & GE_FLAG_CIRCLE ? (realsize << 1): realsize;

			int colorIndex = (eff->flags & GE_FLAG_PHASE ? phase :
				phase / effsize) % (eff->effectColors.size());

			AlienFX_SDK::Afx_action from = *Code2Act(eff->flags & GE_FLAG_BACK ? &eff->effectColors.front() :
				&eff->effectColors[colorIndex]),
				to = *Code2Act(&eff->effectColors[colorIndex + 1 < eff->effectColors.size() ? colorIndex + 1 : 0]);

			if (phase == effsize * lmp || !memcmp(&from, &to, sizeof(AlienFX_SDK::Afx_action))) {
				effop->passive = true;
				continue;
			}

			phase %= effsize;

			if (phase >= realsize) // circle by color and direction
				phase = effsize - phase;

			if (ce->flags & GAUGE_REVERSE)
				phase = realsize - phase - 1;

			// Set lights
			if (effop->oldphase != phase) {
				// Set grid effect
				double power = 0;

				if (ce->gauge) {
					auto zone = *conf->FindZoneMap(ce->group);
					// Old phase cleanup
					if (effop->oldphase < 0) {
						vector<AlienFX_SDK::Afx_action> fin{ from };
						SetZone(&(*ce), &fin);
					} else
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
						for (int nf = 0; nf < phase; nf++) {
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
					power = (double)phase / effsize;
					vector<AlienFX_SDK::Afx_action> fin = { BlendPower(power, &from, &to) };
					SetZone(&(*ce), &fin);
				}
				wasChanged = true;
				effop->oldphase = phase;
			}
		}
	}
	if (wasChanged)
		QueryUpdate();
}

void FXHelper::QueryUpdate(bool force)
{
	if (conf->stateOn) {
		LightQueryElement newBlock{NULL, 0, force, true};
		modifyQuery.lock();
		lightQuery.push(newBlock);
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

void FXHelper::SetLight(int did, int id, vector<AlienFX_SDK::Afx_action>* actions, bool force)
{
	if (conf->stateOn && !actions->empty()) {
		AlienFX_SDK::Afx_device* dev = conf->afx_dev.GetDeviceById(LOWORD(did), HIWORD(did));
		if (dev && dev->dev) {
			LightQueryElement newBlock{ dev, id, force, false, (byte)actions->size() };
			for (int i = 0; i < newBlock.actsize; i++)
				newBlock.actions[i] = actions->at(i);
			modifyQuery.lock();
			lightQuery.push(newBlock);
			modifyQuery.unlock();
			SetEvent(haveNewElement);
		}
	}
}

void FXHelper::SetState(bool force) {
	if (conf->SetStates() || force) {
		HANDLE updates = updateThread;
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
		conf->lightsNoDelay = false;
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

	if (!forced && eve)
		switch (eve->effMode) {
		case 1: RefreshCounters(); break;
		case 2: if (eve->capt) RefreshAmbient(eve->capt->imgz); break;
		case 3: if (eve->audio) RefreshHaptics(eve->audio->freqs); break;
		case 4: if (eve->grid) RefreshGrid(eve->grid->tact); break;
		default: QueryUpdate();
		}
	else
		QueryUpdate(forced == 2);
}

void FXHelper::RefreshOne(groupset* map, bool update, int force) {

	if ((conf->stateOn || force) && map && map->color.size()) {
		SetZone(map, &map->color, 0, force == 2);
		if (update)
			QueryUpdate(force == 2);
	}
}

void FXHelper::RefreshAmbient(UCHAR *img) {

	UINT shift = 255 - conf->amb_shift, gridsize = LOWORD(conf->amb_grid) * HIWORD(conf->amb_grid);
	vector<AlienFX_SDK::Afx_action> actions{ {0} };
	bool wasChanged = false;

	for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++)
		if (it->ambients.size()) {
			ULONG r = 0, g = 0, b = 0, dsize = (UINT)it->ambients.size();
			for (auto cAmb = it->ambients.begin(); dsize && cAmb != it->ambients.end(); cAmb++) {
				if (*cAmb < gridsize) {
					r += img[3 * *cAmb + 2];
					g += img[3 * *cAmb + 1];
					b += img[3 * *cAmb];
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
	if (wasChanged)
		QueryUpdate();
}

void FXHelper::RefreshHaptics(int *freq) {

	vector<AlienFX_SDK::Afx_action> actions;
	bool wasChanged = false;

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
				}
			}

			if (groupsize) {

				if (mIter->gauge) {
					actions = { { 0,0,0,(byte)(from_r / groupsize),(byte)(from_g / groupsize),(byte)(from_b / groupsize)},
						{0,0,0,(byte)(to_r / groupsize),(byte)(to_g / groupsize),(byte)(to_b / groupsize)} };
				}
				else
					actions = { { 0,0,0,(byte)(cur_r / groupsize),(byte)(cur_g / groupsize),(byte)(cur_b / groupsize) } };

				// Do we need to change color?
				//if (mIter->lastHapPower != f_power || mIter->lastHap.size() != actions.size() ||
				//	memcmp(mIter->lastHap.data(), actions.data(), sizeof(AlienFX_SDK::afx_act) * actions.size())) {
				//	mIter->lastHap = actions;
				//	mIter->lastHapPower = f_power;
					SetZone(&(*mIter), &actions, f_power / groupsize);
					wasChanged = true;
				//}
//#ifdef _DEBUG
//				else
//					DebugPrint("Skip haptic update, same colors.\n");
//#endif // _DEBUG

			}
			//else
			//	DebugPrint("Skip group\n");
		}
	}
	if (wasChanged)
		QueryUpdate();
}

DWORD WINAPI CLightsProc(LPVOID param) {
	FXHelper* src = (FXHelper*) param;
	LightQueryElement current;

	HANDLE waitArray[2]{ src->haveNewElement, src->stopQuery };
	vector<deviceQuery> devs_query;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	while (WaitForMultipleObjects(2, waitArray, false, INFINITE) == WAIT_OBJECT_0) {
		while (src->lightQuery.size() && conf->stateOn) {

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
					if (devQ->dev && devQ->dev_query.size() /*&& (!current.dev || devQ->dev->pid == current.dev->pid)*/) {
//#ifdef _DEBUG
//							char buff[2048];
//							sprintf_s(buff, 2047, "Starting update for %d, (%d lights, %d in query)...\n", devQ->devID, devQ->dev_query.size(), src->lightQuery.size());
//							OutputDebugString(buff);
//#endif
						devQ->dev->dev->SetMultiAction(&devQ->dev_query, current.flags);
						devQ->dev->dev->UpdateColors();
						if (devQ->dev->dev->IsHaveGlobal())
							src->UpdateGlobalEffect(devQ->dev->dev);
						devQ->dev_query.clear();
					}
				}

				if (src->lightQuery.size() > conf->afx_dev.activeLights * 5) {
					conf->lightsNoDelay = false;
					DebugPrint("Query so big, delayed!\n");
				}

			} else {
				// set light
				if (current.dev->dev) {
					WORD flags = conf->afx_dev.GetFlags(current.dev, current.lid);
					for (int i = 0; i < current.actsize; i++) {
						AlienFX_SDK::Afx_action* action = &current.actions[i];
						// gamma-correction...
						if (conf->gammaCorrection) {
							action->r = ((UINT)action->r * action->r * current.dev->white.r) / 65025; // (255 * 255);
							action->g = ((UINT)action->g * action->g * current.dev->white.g) / 65025; // (255 * 255);
							action->b = ((UINT)action->b * action->b * current.dev->white.b) / 65025; // (255 * 255);
						}
						// Dimming...
						// For v1-v3 and v7 devices only, other have hardware dimming
						if (conf->IsDimmed() && (!flags || conf->dimPowerButton))
							switch (current.dev->dev->GetVersion()) {
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
						if (!conf->block_power && (current.flags || memcmp(src->pbstate, current.actions, 2 * sizeof(AlienFX_SDK::Afx_action)))) {

							DebugPrint("Power button set to " +
										to_string(current.actions[0].r) + "-" +
										to_string(current.actions[0].g) + "-" +
										to_string(current.actions[0].b) + "/" +
										to_string(current.actions[1].r) + "-" +
										to_string(current.actions[1].g) + "-" +
										to_string(current.actions[1].b) +
										"\n");

							memcpy(src->pbstate, current.actions, 2 * sizeof(AlienFX_SDK::Afx_action));
						} else {
							DebugPrint("Power button update skipped (blocked or same colors)\n");
							continue;
						}
					}

					// form actblock...
					AlienFX_SDK::Afx_lightblock ablock{ (byte)current.lid };
					ablock.act.resize(current.actsize);
					memcpy(ablock.act.data(), current.actions, current.actsize * sizeof(AlienFX_SDK::Afx_action));
					// find query....
					auto qn = find_if(devs_query.begin(), devs_query.end(),
						[ablock,current](auto t) {
							return t.dev->pid == current.dev->pid;
						});
					if (qn != devs_query.end()) {
						// do we have another set for same light?
						auto lp = find_if(qn->dev_query.begin(), qn->dev_query.end(),
							[current](auto acb) {
								return acb.index == current.lid;
							});
						if (lp == qn->dev_query.end())
							qn->dev_query.push_back(ablock);
						else {
							//DebugPrint("Light already in query, updating data.\n");
							lp->act = ablock.act;
						}
					}
					else
						devs_query.push_back({ current.dev, { ablock } });
				}
			}
		}
		conf->lightsNoDelay = true;
		//DebugPrint("Query empty, delay reset\n");
	}
	return 0;
}