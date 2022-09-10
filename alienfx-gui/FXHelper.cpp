#include "alienfx-gui.h"
#include "FXHelper.h"
#include "EventHandler.h"

extern AlienFX_SDK::afx_act* Code2Act(AlienFX_SDK::Colorcode* c);
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::group* grp);

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

extern ConfigHandler* conf;
extern EventHandler* eve;

DWORD WINAPI CLightsProc(LPVOID param);

FXHelper::FXHelper() {
	FillAllDevs(NULL);
	//Start();
};
FXHelper::~FXHelper() {
	Stop();
};

//AlienFX_SDK::afx_device* FXHelper::LocateDev(int pid) {
//	AlienFX_SDK::afx_device* dev = conf->afx_dev.GetDeviceById(pid);
//	return dev && dev->dev ? dev : nullptr;
//};

void FXHelper::SetGaugeLight(DWORD id, int x, int max, WORD flags, vector<AlienFX_SDK::afx_act> actions, double power, bool force)
{
	vector<AlienFX_SDK::afx_act> fAct{ actions.front() };
	if (flags & GAUGE_REVERSE)
		x = max - x;
	if (flags & GAUGE_GRADIENT)
		power = (double)x / max;
	else {
		max++;
		double pos = (double)x / max;
		if (pos > power)
			goto setlight;
		else
			if (pos + (1.0 / max) >= power) {
				power = (power - ((double)x) / max) * max;
			}
			else {
				fAct[0] = actions.back();
				goto setlight;
			}
	}
	fAct[0].r = (byte)((1.0 - power) * actions.front().r + power * actions.back().r);
	fAct[0].g = (byte)((1.0 - power) * actions.front().g + power * actions.back().g);
	fAct[0].b = (byte)((1.0 - power) * actions.front().b + power * actions.back().b);
	setlight:
	SetLight(LOWORD(id), HIWORD(id), fAct, force);
}

void FXHelper::SetGroupLight(groupset* grp, vector<AlienFX_SDK::afx_act> actions, double power, bool force) {
	AlienFX_SDK::group* cGrp = conf->afx_dev.GetGroupById(grp->group);
	if (cGrp && cGrp->lights.size()) {
		zonemap* zone = conf->FindZoneMap(grp->group);
		if (!grp->gauge || actions.size() < 2 || !zone) {
			for (auto i = cGrp->lights.begin(); i < cGrp->lights.end(); i++)
				SetLight(LOWORD(*i), HIWORD(*i), actions, force);
		}
		else {
			for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
				switch (grp->gauge) {
				case 1: // horizontal
					SetGaugeLight(t->light, t->x, zone->xMax, grp->flags, actions, power, force);
					break;
				case 2: // vertical
					SetGaugeLight(t->light, t->y, zone->yMax, grp->flags, actions, power, force);
					break;
				case 3: // diagonal
					SetGaugeLight(t->light, t->x + t->y, zone->xMax + zone->yMax, grp->flags, actions, power, force);
					break;
				case 4: // back diagonal
					SetGaugeLight(t->light, zone->xMax - t->x + t->y, zone->xMax + zone->yMax, grp->flags, actions, power, force);
					break;
				case 5: // radial
					float px = abs(((float)zone->xMax)/2 - t->x), py = abs(((float)zone->yMax)/2 - t->y);
					int radius = (int)(sqrt(zone->xMax * zone->xMax + zone->yMax * zone->yMax) / 2),
						weight = (int)sqrt(px * px + py * py);
					SetGaugeLight(t->light, weight, radius, grp->flags, actions, power, force);
					break;
				}
		}
	}
}


void FXHelper::TestLight(AlienFX_SDK::afx_device* dev, int id, bool force, bool wp)
{
	if (dev) {

		/*bool dev_ready = false;
		for (int c_count = 0; c_count < 200 && !(dev_ready = conf->afx_dev.fxdevs[did].dev->IsDeviceReady()); c_count++)
			Sleep(20);
		if (!dev_ready) return;*/

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

	blinkStage = !blinkStage;
	bool wasChanged = false;

#ifdef _DEBUG
	if (force) {
		DebugPrint("Forced Counter update initiated...\n");
	}
#endif

	vector<groupset> active = conf->activeProfile->lightsets;

	for (auto Iter = active.begin(); Iter != active.end(); Iter++) {
		vector<AlienFX_SDK::afx_act> actions;
		bool noDiff = true;
		int lVal = 0, cVal = 0;
		AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(Iter->group);
		if (grp) {
			if (Iter->fromColor && Iter->color.size()) {
				actions.push_back(grp->have_power && conf->statePower ? Iter->color.back() : Iter->color.front());
				actions.back().type = 0;
			}
			if (Iter->events[0].state) {
				// Power, just need to prepare colors
				if (actions.empty())
					actions.push_back(Iter->events[0].from);
				if (!conf->statePower)
					actions[0] = Iter->events[0].to;
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
				}
				if (Iter->gauge && !(Iter->flags & GAUGE_GRADIENT))
					actions.push_back(Iter->events[1].to);
				else {
					actions.push_back({ 0,0,0,
						(BYTE)(actions.front().r * (1 - Iter->events[1].coeff) + Iter->events[1].to.r * Iter->events[1].coeff),
						(BYTE)(actions.front().g * (1 - Iter->events[1].coeff) + Iter->events[1].to.g * Iter->events[1].coeff),
						(BYTE)(actions.front().b * (1 - Iter->events[1].coeff) + Iter->events[1].to.b * Iter->events[1].coeff) });
					if (!Iter->gauge)
						actions.erase(actions.begin());
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
				case 4: lVal = ccut - eData.Batt; cVal = ccut - data->Batt; break;
				case 5: lVal = eData.KBD; cVal = data->KBD; break;
				}

				if (force || ((byte)(cVal > 0) + (byte)(lVal > 0)) == 1) {
					noDiff = false;
					if (cVal > 0) {
						actions.erase(actions.begin());
						actions.push_back(Iter->events[2].to);
					}
				}
				else
					if (blink && cVal > 0) {
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

			if (grp->have_power)
				if (conf->statePower) {
					actions.push_back(Iter->color[1]);
				}
				else {
					actions.insert(actions.begin(), Iter->color[0]);
				}

			SetGroupLight(&(*Iter), actions, Iter->events[1].state ? Iter->events[1].coeff : 0);
		}
	}
	if (wasChanged) {
		QueryUpdate();
	}
	memcpy(&eData, data, sizeof(EventData));
}

void FXHelper::SetGridLight(groupset* grp, zonemap* zone, AlienFX_SDK::lightgrid* grid, int x, int y, AlienFX_SDK::Colorcode fin, vector<DWORD>* setLights) {
	if (x < zone->gMaxX && x >= zone->gMinX && y < zone->gMaxY && y >= zone->gMinY) {
		DWORD gridval = grid->grid[ind(x, y)];
		if (gridval && grp->effect.flags & GE_FLAG_ZONE) { // zone lights only
			if (!IsLightInGroup(gridval, conf->afx_dev.GetGroupById(grp->group))) {
				gridval = 0;
			}
		}
		if (gridval && find(setLights->begin(), setLights->end(), gridval) == setLights->end()) {
			SetLight(LOWORD(gridval), HIWORD(gridval), { *Code2Act(&fin) });
			setLights->push_back(gridval);
		}
	}
}

void FXHelper::SetGaugeGrid(groupset* grp, zonemap* zone, AlienFX_SDK::lightgrid* grid, int phase, int dist, AlienFX_SDK::Colorcode fin, vector<DWORD>* setLights) {
	switch (grp->gauge) {
	case 1: // horizontal
		for (int y = zone->gMinY; y < zone->gMaxY; y++) { // GridOp point!
			SetGridLight(grp, zone, grid, grp->gridop.gridX + phase + dist, y, fin, setLights);
			if (dist) {
				SetGridLight(grp, zone, grid, grp->gridop.gridX + phase - dist, y, fin, setLights);
			}
		}
		break;
	case 2: // vertical
		for (int x = zone->gMinX; x < zone->gMaxX; x++) {
			SetGridLight(grp, zone, grid, x, grp->gridop.gridY + phase + dist, fin, setLights);
			if (dist) {
				SetGridLight(grp, zone, grid, x, grp->gridop.gridY + phase - dist, fin, setLights);
			}
		}
		break;
	case 3: // diagonal
		for (int x = zone->gMinX; x < zone->gMaxX; x++)
			for (int y = zone->gMinY; y < zone->gMaxY; y++) {
				if (x + y - grp->gridop.gridX - grp->gridop.gridY == phase + dist)
					SetGridLight(grp, zone, grid, x, y, fin, setLights);
				if (dist && x + y - grp->gridop.gridX - grp->gridop.gridY == phase - dist)
					SetGridLight(grp, zone, grid, x, y, fin, setLights);
			}
		break;
	case 4: // back diagonal
		for (int x = zone->gMinX; x < zone->gMaxX; x++)
			for (int y = zone->gMinY; y < zone->gMaxY; y++) {
				if (zone->gMaxX - x + y - grp->gridop.gridX - grp->gridop.gridY == phase + dist)
					SetGridLight(grp, zone, grid, x, y, fin, setLights);
				if (dist && zone->gMaxX - x + y - grp->gridop.gridX - grp->gridop.gridY == phase - dist)
					SetGridLight(grp, zone, grid, x, y, fin, setLights);
			}
		break;
	case 5: // radial
		for (int x = zone->gMinX; x < zone->gMaxX; x++)
			for (int y = zone->gMinY; y < zone->gMaxY; y++) {
				int radius = (int)sqrt((x - grp->gridop.gridX) * (x - grp->gridop.gridX) + (y - grp->gridop.gridY) * (x - grp->gridop.gridY));
				if (radius == phase + dist)
					SetGridLight(grp, zone, grid, x, y, fin, setLights);
				if (dist && radius == phase - dist)
					SetGridLight(grp, zone, grid, x, y, fin, setLights);
			}
		break;
	}
}

void FXHelper::SetGridEffect(groupset* grp)
{
	auto zone = conf->FindZoneMap(grp->group);
	if (zone) {
		AlienFX_SDK::lightgrid* grid = conf->afx_dev.GetGridByID((byte)zone->gridID);
		if (grid) {
			vector<DWORD> setLights;
			double power = 1.0;
			AlienFX_SDK::Colorcode fin;
			int phase = grp->flags & GAUGE_REVERSE ? grp->effect.size - grp->gridop.phase : grp->gridop.phase,
				oldphase = grp->flags & GAUGE_REVERSE ? grp->effect.size - grp->gridop.oldphase : grp->gridop.oldphase;
			// Only involve lights at "width" from phase point!
			// First phase lights...
			for (int dist = 0; dist <= grp->effect.width; dist++) {
				// make final color for this distance
				switch (grp->effect.type) {
				case 0: // running light
					power = 1.0;
					break;
				case 1: // wave
					power = ((double)grp->effect.width - dist) / grp->effect.width; // maybe 0.5 for next!
					break;
				case 2: // gradient
					power = 1.0 - (((double)dist) / grp->effect.width); // just for fun for now
				}
				fin.r = (byte)((1.0 - power) * grp->effect.from.r + power * grp->effect.to.r);
				fin.g = (byte)((1.0 - power) * grp->effect.from.g + power * grp->effect.to.g);
				fin.b = (byte)((1.0 - power) * grp->effect.from.b + power * grp->effect.to.b);
				// make a list of position depends on type
				SetGaugeGrid(grp, zone, grid, phase, dist, fin, &setLights);
			}
			// Then oldphase lights for reset
			for (int dist = 0; dist <= grp->effect.width; dist++) {
				SetGaugeGrid(grp, zone, grid, oldphase, dist, grp->effect.from, &setLights);
			}
			QueryUpdate();
		}
	}
}

void FXHelper::QueryUpdate(int did, bool force)
{
	if (conf->stateOn) {
		LightQueryElement newBlock{did, 0, force, true};
		modifyQuery.lock();
		lightQuery.push_back(newBlock);
		modifyQuery.unlock();
		SetEvent(haveNewElement);
	}
}

void FXHelper::SetLight(int did, int id, vector<AlienFX_SDK::afx_act> actions, bool force)
{
	if (conf->stateOn && !actions.empty()) {
//		if (did) {
			LightQueryElement newBlock{ did, id, force, false, (byte)actions.size() };
			for (int i = 0; i < newBlock.actsize; i++)
				newBlock.actions[i] = actions[i];
			if (did && newBlock.actsize) {
				modifyQuery.lock();
				lightQuery.push_back(newBlock);
				modifyQuery.unlock();
				SetEvent(haveNewElement);
			}
//		}
	}
}

inline void FXHelper::RefreshMon()
{
	if (conf->enableMon && conf->GetEffect() == 1)
		SetCounterColor(&eData, true);
	//eData = EventData();
}

void FXHelper::ChangeState() {
	if (conf->SetStates()) {
		Stop();
		for (auto i = conf->afx_dev.fxdevs.begin(); i < conf->afx_dev.fxdevs.end(); i++) {
			if (i->dev) {

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
		Start();
	}
}

void FXHelper::UpdateGlobalEffect(AlienFX_SDK::Functions* dev) {
	if (conf->haveGlobal) {
		if (!dev) {
			auto pos = find_if(conf->afx_dev.fxdevs.begin(), conf->afx_dev.fxdevs.end(),
				[](auto t) {
					return t.dev ? t.dev->IsHaveGlobal() : false;
				});
			if (pos != conf->afx_dev.fxdevs.end())
				dev = pos->dev;
		}
		if (dev && dev->IsHaveGlobal()) {
			if (conf->activeProfile->effmode == 99)
				dev->SetGlobalEffects(conf->activeProfile->globalEffect, conf->activeProfile->globalMode, conf->activeProfile->globalDelay,
					{ 0,0,0,conf->activeProfile->effColor1.r, conf->activeProfile->effColor1.g,	conf->activeProfile->effColor1.b },
					{ 0,0,0,conf->activeProfile->effColor2.r, conf->activeProfile->effColor2.g,	conf->activeProfile->effColor2.b });
			else
				dev->SetGlobalEffects(1, dev->GetVersion() == 8 ? 0 : 1, 0, { 0 }, { 0 });
		}
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

//void FXHelper::UnblockUpdates(bool newState) {
//
//	unblockUpdates = newState;
//	if (!unblockUpdates) {
//		while (updateThread && lightQuery.size())
//			Sleep(100);
//		DebugPrint("Lights pause on!\n");
//	} else {
//		DebugPrint("Lights pause off!\n");
//	}
//}

size_t FXHelper::FillAllDevs(AlienFan_SDK::Control* acc) {
	conf->SetStates();
	conf->haveGlobal = false;
	numActiveDevs = 0;
	Stop();
	conf->afx_dev.AlienFXAssignDevices(acc, conf->finalBrightness, conf->finalPBState);
	// global effects check
	for (auto i = conf->afx_dev.fxdevs.begin(); i < conf->afx_dev.fxdevs.end(); i++)
		if (i->dev) {
			numActiveDevs++;
			if (i->dev->IsHaveGlobal())
				conf->haveGlobal = true;
			//if (i->dev->GetVersion() == API_V6)
			//	// reset device will make all white...
			//	Refresh();
		}
	if (numActiveDevs)
		Start();
	return numActiveDevs;
}

void FXHelper::Start() {
	if (!updateThread) {
		DebugPrint("Light updates started.\n");

		stopQuery = CreateEvent(NULL, true, false, NULL);
		haveNewElement = CreateEvent(NULL, false, false, NULL);
		updateThread = CreateThread(NULL, 0, CLightsProc, this, 0, NULL);
	}
}

void FXHelper::Stop() {
	if (updateThread) {
		DebugPrint("Light updates stopped.\n");

		SetEvent(stopQuery);
		WaitForSingleObject(updateThread, 60000);
		lightQuery.clear();
		CloseHandle(updateThread);
		CloseHandle(stopQuery);
		CloseHandle(haveNewElement);
		updateThread = NULL;
	}
}

void FXHelper::Refresh(int forced)
{

#ifdef _DEBUG
	if (forced) {
		DebugPrint((("Forced Refresh initiated in mode ") + to_string(forced) + "\n").c_str());
	} else
		DebugPrint("Refresh initiated.\n");
#endif

	for (auto it = (*conf->active_set).begin(); it < (*conf->active_set).end(); it++) {
		RefreshOne(&(*it), forced, false);
	}

	QueryUpdate(-1, forced == 2);

	if (!forced && eve)
		switch (conf->GetEffect()) {
		case 1: RefreshMon(); break;
		case 2: if (eve->capt) RefreshAmbient(eve->capt->imgz); break;
		case 3: if (eve->audio) RefreshHaptics(eve->audio->freqs); break;
		}
}

bool FXHelper::SetPowerMode(int mode)
{
	int t = activePowerMode;
	activePowerMode = mode;
	for (auto i = conf->afx_dev.fxdevs.begin(); i < conf->afx_dev.fxdevs.end(); i++)
		if (i->dev)
			i->dev->powerMode = conf->statePower;
	return t == activePowerMode;
}

bool FXHelper::RefreshOne(groupset* map, int force, bool update)
{
	vector<AlienFX_SDK::afx_act> actions = map->color;

	if (!conf->stateOn || !map)
		return false;

	if (!force && conf->enableMon && conf->GetEffect() == 1) {
		if (map->events[0].state) {
			// use power event;
			if (!map->fromColor)
				actions = { map->events[0].from };
			switch (activePowerMode) {
			case MODE_AC:
				break;
			case MODE_BAT:
				actions = { map->events[0].to };
				break;
			case MODE_LOW:
				actions.resize(1);
				actions.push_back(map->events[0].to);
				actions.front().type = actions.back().type = AlienFX_SDK::AlienFX_A_Pulse;
				actions.front().time = actions.back().time = 3;
				actions.front().tempo = actions.back().tempo = 0x64;
				break;
			case MODE_CHARGE:
				actions.resize(1);
				actions.push_back(map->events[0].to);
				actions.front().type = actions.back().type = AlienFX_SDK::AlienFX_A_Morph;
				actions.front().time = actions.back().time = 3;
				actions.front().tempo = actions.back().tempo = 0x64;
				break;
			}
		} else
			if (map->events[1].state || map->events[2].state)
				return false;

	}

	if (actions.size()) {
		SetGroupLight(map, actions, 0, force == 2);
		if (update)
			QueryUpdate(-1, force == 2);
	}
	return true;
}

void FXHelper::RefreshAmbient(UCHAR *img) {

	if (!updateThread || conf->monDelay > DEFAULT_MON_DELAY) {
		DebugPrint("Ambient update skipped!\n");
		return;
	}

	UINT shift = 255 - conf->amb_shift, gridsize = LOWORD(conf->amb_grid) * HIWORD(conf->amb_grid);
	vector<AlienFX_SDK::afx_act> actions;
	actions.push_back({0});
	bool wasChanged = false;

	for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
		if (it->ambients.size()) {
			ULONG r = 0, g = 0, b = 0, dsize = (UINT)it->ambients.size() * 255;
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
			// Multi-lights and brightness correction...
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

	if (!updateThread || conf->monDelay > DEFAULT_MON_DELAY) {
		DebugPrint("Haptics update skipped!\n");
		return;
	}

	vector<AlienFX_SDK::afx_act> actions;
	bool wasChanged = false;

	for (auto mIter = conf->active_set->begin(); mIter < conf->active_set->end(); mIter++) {
		if (mIter->haptics.size()) {
			// Now for each freq block...
			unsigned from_r = 0, from_g = 0, from_b = 0, to_r = 0, to_g = 0, to_b = 0, cur_r = 0, cur_g = 0, cur_b = 0, groupsize = 0;
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

					from_r += fIter->colorfrom.r;
					from_g += fIter->colorfrom.g;
					from_b += fIter->colorfrom.b;

					to_r += fIter->colorto.r;
					to_g += fIter->colorto.g;
					to_b += fIter->colorto.b;

					cur_r += (byte)sqrt((1.0 - power) * fIter->colorfrom.r * fIter->colorfrom.r + power * fIter->colorto.r * fIter->colorto.r);
					cur_g += (byte)sqrt((1.0 - power) * fIter->colorfrom.g * fIter->colorfrom.g + power * fIter->colorto.g * fIter->colorto.g);
					cur_b += (byte)sqrt((1.0 - power) * fIter->colorfrom.b * fIter->colorfrom.b + power * fIter->colorto.b * fIter->colorto.b);
				}
				//else
				//	DebugPrint("HiCut issue\n");
			}

			if (groupsize) {
				f_power /= groupsize;

				if (mIter->gauge) {
					actions = { { 0,0,0,(byte)(from_r / groupsize),(byte)(from_g / groupsize),(byte)(from_b / groupsize)},
						{0,0,0,(byte)(to_r / groupsize),(byte)(to_g / groupsize),(byte)(to_b / groupsize)} };
				}
				else
					actions = { { 0,0,0,(byte)(cur_r / groupsize),(byte)(cur_g / groupsize),(byte)(cur_b / groupsize) } };

				SetGroupLight(&(*mIter), actions, f_power);
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

	HANDLE waitArray[2]{src->stopQuery, src->haveNewElement};
	vector<deviceQuery> devs_query;

	// Power button state...
	vector<AlienFX_SDK::afx_act> pbstate;
	pbstate.resize(2);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	AlienFX_SDK::afx_device* dev = NULL;

	while (WaitForMultipleObjects(2, waitArray, false, INFINITE) != WAIT_OBJECT_0) {
		while (!src->lightQuery.empty()) {
			src->modifyQuery.lock();

			//if (&src->lightQuery.front())
				current = src->lightQuery.front();
			//else
			//	DebugPrint("Incorrect front element!\n");

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
						if (current.did == (-1) || devQ->dev->pid == current.did) {
//#ifdef _DEBUG
//							char buff[2048];
//							sprintf_s(buff, 2047, "Starting update for %d, (%d lights, %d in query)...\n", devQ->devID, devQ->dev_query.size(), src->lightQuery.size());
//							OutputDebugString(buff);
//#endif
							if (devQ->dev_query.size()) {
								devQ->dev->dev->SetMultiAction(&devQ->dev_query, current.flags);
								devQ->dev->dev->UpdateColors();
							}
							if (devQ->dev->dev->IsHaveGlobal() && (conf->activeProfile->effmode == 99)) {
								DebugPrint("Global effect active!\n");
								src->UpdateGlobalEffect(devQ->dev->dev);
							}
							devQ->dev_query.clear();
						}
					}

					if (src->lightQuery.size() > conf->afx_dev.activeLights * 5) {
						conf->monDelay += 50;
						DebugPrint(("Query so big (" +
									to_string((int) src->lightQuery.size()) +
									"), delay increased to " +
									to_string(conf->monDelay) +
									" ms!\n").c_str());
					}
					//else
					//	DebugPrint(("Query size " + to_string(src->lightQuery.size()) + "\n").c_str());
				}
			} else {
				// set light
				if ((dev = conf->afx_dev.GetDeviceById(current.did)) && dev->dev) {
					vector<AlienFX_SDK::afx_act> actions;
					WORD flags = conf->afx_dev.GetFlags(dev, current.lid);
					for (int i = 0; i < current.actsize; i++) {
						AlienFX_SDK::afx_act action = current.actions[i];
						// gamma-correction...
						if (conf->gammaCorrection) {
							//action.r = (byte)(pow((float)action.r / 255.0, 2.2) * 255 + 0.5);
							//action.g = (byte)(pow((float)action.g / 255.0, 2.2) * 255 + 0.5);
							//action.b = (byte)(pow((float)action.b / 255.0, 2.2) * 255 + 0.5);
							action.r = ((UINT)action.r * action.r * dev->white.r) / (255 * 255);
							action.g = ((UINT)action.g * action.g * dev->white.g) / (255 * 255);
							action.b = ((UINT)action.b * action.b * dev->white.b) / (255 * 255);
						}
						// Dimming...
						// For v0-v3 and v7 devices only, other have hardware dimming
						if (!flags || conf->dimPowerButton)
							switch (dev->dev->GetVersion()) {
							case 0: case 1: case 2: case 3: case 7: {
								unsigned delta = 255 - conf->dimmingPower;
								action.r = ((UINT)action.r * delta) / 255;// >> 8;
								action.g = ((UINT)action.g * delta) / 255;// >> 8;
								action.b = ((UINT)action.b * delta) / 255;// >> 8;
							}
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
					auto qn = find_if(devs_query.begin(), devs_query.end(),
						[current](auto t) {
							return t.dev->pid == current.did;
						});
					if (qn != devs_query.end())
						qn->dev_query.push_back({ (byte)current.lid, actions });
					else {
						devs_query.push_back({ dev, {{(byte)current.lid, actions}} });
					}
				}
			}
		}
		conf->monDelay = DEFAULT_MON_DELAY;
		//DebugPrint("Query empty, delay reset\n");
	}
	return 0;
}