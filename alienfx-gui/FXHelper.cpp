#include "alienfx-gui.h"
#include "FXHelper.h"
#include "EventHandler.h"

extern AlienFX_SDK::afx_act* Code2Act(AlienFX_SDK::Colorcode* c);
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::group* grp);

extern ConfigHandler* conf;
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

AlienFX_SDK::afx_act FXHelper::BlendPower(double power, AlienFX_SDK::afx_act* from, AlienFX_SDK::afx_act* to) {
	return { 0,0,0,
		(byte)((1.0 - power) * from->r + power * to->r),
		(byte)((1.0 - power) * from->g + power * to->g),
		(byte)((1.0 - power) * from->b + power * to->b)
	};
}

void FXHelper::SetZoneLight(DWORD id, int x, int max, WORD flags, vector<AlienFX_SDK::afx_act> actions, double power, bool force)
{
	AlienFX_SDK::afx_act fAct;
	if (flags & GAUGE_REVERSE)
		x = max - x;
	if (flags & GAUGE_GRADIENT)
		power = (double)x / max;
	else {
		max++;
		double pos = (double)x / max;
		if (pos > power) {
			fAct = actions.front();
			goto setlight;
		}
		else
			if (double(x+1) / max > power) {
				power = (power - pos) * max;
			}
			else {
				fAct = actions.back();
				goto setlight;
			}
	}
	fAct = BlendPower(power, &actions.front(), &actions.back());
	setlight:
	SetLight(LOWORD(id), HIWORD(id), { fAct }, force);
}

void FXHelper::SetZone(groupset* grp, vector<AlienFX_SDK::afx_act> actions, double power, bool force) {
	AlienFX_SDK::group* cGrp = conf->afx_dev.GetGroupById(grp->group);
	if (cGrp && cGrp->lights.size()) {
		if (!grp->gauge || actions.size() < 2 /*|| !zone*/) {
			for (auto i = cGrp->lights.begin(); i < cGrp->lights.end(); i++)
				SetLight(LOWORD(*i), HIWORD(*i), actions, force);
		}
		else {
			zonemap* zone = conf->FindZoneMap(grp->group);
			for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
				switch (grp->gauge) {
				case 1: // horizontal
					SetZoneLight(t->light, t->x, zone->xMax, grp->flags, actions, power, force);
					break;
				case 2: // vertical
					SetZoneLight(t->light, t->y, zone->yMax, grp->flags, actions, power, force);
					break;
				case 3: // diagonal
					SetZoneLight(t->light, t->x + t->y, zone->xMax + zone->yMax, grp->flags, actions, power, force);
					break;
				case 4: // back diagonal
					SetZoneLight(t->light, zone->xMax - t->x + t->y, zone->xMax + zone->yMax, grp->flags, actions, power, force);
					break;
				case 5: // radial
					float px = abs(((float)zone->xMax)/2 - t->x), py = abs(((float)zone->yMax)/2 - t->y);
					int radius = (int)(sqrt(zone->xMax * zone->xMax + zone->yMax * zone->yMax) / 2),
						weight = (int)sqrt(px * px + py * py);
					SetZoneLight(t->light, weight, radius, grp->flags, actions, power, force);
					break;
				}
		}
	}
}


void FXHelper::TestLight(AlienFX_SDK::afx_device* dev, int id, bool force, bool wp)
{
	if (dev && dev->dev) {

		AlienFX_SDK::Colorcode c = wp ? dev->white : AlienFX_SDK::Colorcode({ 0 });

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

void FXHelper::ResetPower(AlienFX_SDK::afx_device* dev)
{
	if (dev && dev->dev) {
		vector<AlienFX_SDK::act_block> act{ { (byte)63, {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64}, {AlienFX_SDK::AlienFX_A_Power, 3, 0x64}} } };
		dev->dev->SetPowerAction(&act);
	}
}

void FXHelper::SetCounterColor(EventData *data, bool force)
{

	if (!force) blinkStage = !blinkStage;
	bool wasChanged = false;

#ifdef _DEBUG
	if (force) {
		DebugPrint("Forced Counter update initiated...\n");
	}
#endif

	vector<groupset> active = conf->activeProfile->lightsets;
	AlienFX_SDK::group* grp;
	for (auto Iter = active.begin(); Iter != active.end(); Iter++) {
		vector<AlienFX_SDK::afx_act> actions;
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
							actions[0] = (data->BST & 6) && blinkStage ? AlienFX_SDK::afx_act{ 0 } : e->to;
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

			SetZone(&(*Iter), actions, fCoeff);
		}
	}
	if (wasChanged) {
		QueryUpdate();
	}
}

void FXHelper::SetGaugeGrid(groupset* grp, zonemap* zone, int phase, AlienFX_SDK::afx_act fin) {
	for (auto lgh = zone->lightMap.begin(); lgh != zone->lightMap.end(); lgh++) {
		switch (grp->gauge) {
		case 1: // horizontal
			if (lgh->x == phase)
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), { fin });
			break;
		case 2: // vertical
			if (lgh->y == phase)
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), { fin });
			break;
		case 3: // diagonal
			if (lgh->x + lgh->y - grp->gridop.gridX - grp->gridop.gridY == phase)
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), { fin });
			break;
		case 4: // back diagonal
			if (lgh->x + zone->yMax - lgh->y - grp->gridop.gridX - grp->gridop.gridY == phase) // ToDo - add start point!
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), { fin });
			break;
		case 5: // radial
			if ((abs(lgh->x - grp->gridop.gridX) == phase && abs(lgh->y - grp->gridop.gridY) <= phase)
				|| (abs(lgh->y - grp->gridop.gridY) == phase && abs(lgh->x - grp->gridop.gridX) <= phase))
				SetLight(LOWORD(lgh->light), HIWORD(lgh->light), { fin });
			break;
		}
	}
}

void FXHelper::SetGridEffect(groupset* grp)
{
	double power;
	AlienFX_SDK::afx_act from = *Code2Act(&grp->effect.from), to = *Code2Act(&grp->effect.to);

	if (grp->gauge) {
		auto zone = conf->FindZoneMap(grp->group);
		// Old phase cleanup
		if (grp->gridop.oldphase >= 0)
			for (int dist = 0; dist < grp->effect.width; dist++)
				SetGaugeGrid(grp, zone, grp->gridop.oldphase + dist, from);
		else
			SetZone(grp, { from });

		// Check for gradient zones - in this case all phases updated!
		if (grp->flags & GAUGE_GRADIENT && grp->gridop.phase > 0) {
			for (int nf = 0; nf < grp->gridop.phase; nf++) {
				power = (double)nf / grp->gridop.phase;
				SetGaugeGrid(grp, zone, nf, BlendPower(power, &from, &to));
			}
		}

		// Fill new phase colors
		int halfW = grp->effect.width / 2 + 1;
		for (int dist = 1; grp->gridop.phase >= 0 && dist <= grp->effect.width; dist++) {
			// make final color for this distance
			switch (grp->effect.type) {
			case 0: // running light
				power = 1.0;
				break;
			case 1: // wave
				power = 1.0 - (double)abs(halfW - dist) / halfW;
				break;
			case 2: // gradient
				power = 1.0 - (double)(dist - 1) / grp->effect.width; // just for fun for now
			}
			SetGaugeGrid(grp, zone, grp->gridop.phase + dist - 1, BlendPower(power, &from, &to));
		}
	}
	else {
		// flat morph emulation
		if (grp->gridop.phase < 0)
			SetZone(grp, { from });
		else {
			power = (double)grp->gridop.phase / grp->effect.size;
			SetZone(grp, { BlendPower(power, &from, &to) });
		}
	}
}

void FXHelper::RefreshGrid(int tact) {
	//if (!updateThread || conf->monDelay > DEFAULT_MON_DELAY) {
	//	DebugPrint("Grid update skipped!\n");
	//	return;
	//}
	bool wasChanged = false;
	for (auto ce = conf->active_set->begin(); ce < conf->active_set->end(); ce++) {
		if (!ce->gridop.passive) {
			// calculate phase
			int cTact = abs((long)tact - (long)ce->gridop.start_tact);
			ce->gridop.phase = ce->effect.speed < 80 ? cTact / (80 - ce->effect.speed) : cTact * (ce->effect.speed - 79);
			int effsize = (ce->effect.flags & GE_FLAG_CIRCLE ? 2 * ce->effect.size : ce->effect.size);
			if (ce->gridop.phase > effsize) {
				if (ce->effect.trigger == 1)
					ce->gridop.phase %= effsize;
				else {
					ce->gridop.passive = true;
					ce->gridop.phase = -1;
					//SetZone(&(*ce), { *Code2Act(&ce->effect.from) });
					wasChanged = true;
					//continue;
				}
			}
			if (ce->gridop.phase > ce->effect.size) // circle
				ce->gridop.phase = effsize - ce->gridop.phase;
			if (ce->flags & GAUGE_REVERSE)
				ce->gridop.phase = ce->effect.size - ce->gridop.phase;

			// Set lights
			if (ce->gridop.oldphase != ce->gridop.phase) {
				fxhl->SetGridEffect(&(*ce));
				wasChanged = true;
			}
			ce->gridop.oldphase = ce->gridop.phase;
		}
	}
	if (wasChanged)
		QueryUpdate();
}

void FXHelper::QueryUpdate(bool force)
{
	if (conf->stateOn) {
		LightQueryElement newBlock{/*did > 0 ? conf->afx_dev.GetDeviceById(LOWORD(did), HIWORD(did)) :*/ NULL, 0, force, true};
		modifyQuery.lock();
		lightQuery.push(newBlock);
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

void FXHelper::SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force)
{
	if (conf->stateOn && !actions.empty()) {
		AlienFX_SDK::afx_device* dev = conf->afx_dev.GetDeviceById(LOWORD(did), HIWORD(did));
		if (dev && dev->dev) {
			LightQueryElement newBlock{ dev, id, force, false, (byte)actions.size() };
			for (int i = 0; i < newBlock.actsize; i++)
				newBlock.actions[i] = actions[i];
			if (did && newBlock.actsize) {
				modifyQuery.lock();
				lightQuery.push(newBlock);
				modifyQuery.unlock();
				SetEvent(haveNewElement);
			}
		}
	}
}

inline void FXHelper::RefreshMon()
{
	if (updateThread && conf->enableMon && conf->GetEffect() == 1)
		SetCounterColor(&eData, true);
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
				case API_V1: case API_V2: case API_V3:
					if (conf->stateOn)
						Refresh();
					break;
				case API_ACPI: case API_V6: case API_V7:
					Refresh();
				}
			}
		}
		if (updates)
			Start();
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

int FXHelper::FillAllDevs(AlienFan_SDK::Control* acc) {
	conf->SetStates();
	Stop();
	conf->afx_dev.AlienFXAssignDevices(acc, conf->finalBrightness, conf->finalPBState);
	if (conf->afx_dev.activeDevices)
		Start();
	return conf->afx_dev.activeDevices;
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

	for (auto it = (*conf->active_set).begin(); it < (*conf->active_set).end(); it++) {
		RefreshOne(&(*it), false);
	}

	if (!forced && eve)
		switch (eve->effMode) {
		case 1: RefreshMon(); break;
		case 2: if (eve->capt) RefreshAmbient(eve->capt->imgz); break;
		case 3: if (eve->audio) RefreshHaptics(eve->audio->freqs); break;
		case 4: if (eve->grid) RefreshGrid(eve->grid->tact); break;
		default: QueryUpdate();
		}
	else
		QueryUpdate(forced == 2);
}

void FXHelper::RefreshOne(groupset* map, bool update, int force) {

	if (conf->stateOn && map && map->color.size()) {
		SetZone(map, map->color, 0, force == 2);
		if (update)
			QueryUpdate(force == 2);
	}
}

void FXHelper::RefreshAmbient(UCHAR *img) {

	//if (!updateThread || conf->monDelay > DEFAULT_MON_DELAY) {
	//	DebugPrint("Ambient update skipped!\n");
	//	return;
	//}

	UINT shift = 255 - conf->amb_shift, gridsize = LOWORD(conf->amb_grid) * HIWORD(conf->amb_grid);
	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});
	bool wasChanged = false;

	for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
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
				actions[0].r = (BYTE)((r * shift) / dsize);
				actions[0].g = (BYTE)((g * shift) / dsize);
				actions[0].b = (BYTE)((b * shift) / dsize);
				SetZone(&(*it), actions);
			}
		}
	if (wasChanged)
		QueryUpdate();
}

void FXHelper::RefreshHaptics(int *freq) {

	vector<AlienFX_SDK::afx_act> actions;
	bool wasChanged = false;

	for (auto mIter = conf->active_set->begin(); mIter < conf->active_set->end(); mIter++) {
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

				SetZone(&(*mIter), actions, f_power / groupsize);
				wasChanged = true;
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
						AlienFX_SDK::afx_act* action = &current.actions[i];
						// gamma-correction...
						if (conf->gammaCorrection) {
							action->r = ((UINT)action->r * action->r * current.dev->white.r) / 65025; // (255 * 255);
							action->g = ((UINT)action->g * action->g * current.dev->white.g) / 65025; // (255 * 255);
							action->b = ((UINT)action->b * action->b * current.dev->white.b) / 65025; // (255 * 255);
						}
						// Dimming...
						// For v0-v3 and v7 devices only, other have hardware dimming
						if (conf->IsDimmed() && (!flags || conf->dimPowerButton))
							switch (current.dev->dev->GetVersion()) {
							case 0: case 1: case 2: case 3: case 7: {
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
						if (!conf->block_power && (current.flags || memcmp(src->pbstate, current.actions, 2 * sizeof(AlienFX_SDK::afx_act)))) {

							DebugPrint("Power button set to " +
										to_string(current.actions[0].r) + "-" +
										to_string(current.actions[0].g) + "-" +
										to_string(current.actions[0].b) + "/" +
										to_string(current.actions[1].r) + "-" +
										to_string(current.actions[1].g) + "-" +
										to_string(current.actions[1].b) +
										"\n");

							memcpy(src->pbstate, current.actions, 2 * sizeof(AlienFX_SDK::afx_act));
						} else {
							DebugPrint("Power button update skipped (blocked or same colors)\n");
							continue;
						}
					}

					// form actblock...
					AlienFX_SDK::act_block ablock{ (byte)current.lid };
					ablock.act.resize(current.actsize);
					memcpy(ablock.act.data(), current.actions, current.actsize * sizeof(AlienFX_SDK::afx_act));
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