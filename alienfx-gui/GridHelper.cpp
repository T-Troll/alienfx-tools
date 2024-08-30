#include "GridHelper.h"
#include "EventHandler.h"
#include "FXHelper.h"

extern EventHandler* eve;
extern ConfigHandler* conf;
extern FXHelper* fxhl;

extern AlienFX_SDK::Afx_action Code2Act(AlienFX_SDK::Afx_colorcode* c);

void GridHelper::StartGridRun(groupset* grp, zonemap* cz, int x, int y) {
	if (cz->lightMap.size() && (grp->effect.trigger == 4 || grp->effect.effectColors.size())) {
		grideffop* gridop = &grp->gridop;
		int cx = max(x + 1, cz->gMaxX - x), cy = max(y + 1, cz->gMaxY - y), esize = 0;
		if (grp->gauge) {
			switch (grp->gauge) {
			case 1:
				esize = cx;
				break;
			case 2:
				esize = cy;
				break;
			case 0: case 3: case 4:
				esize = cx + cy - 2;
				break;
			case 5:
				esize = max(cx, cy);
				break;
			}
			gridop->size = max(esize + grp->effect.width, 1);
		}
		else
			gridop->size = grp->effect.width;
		if (grp->effect.flags & GE_FLAG_RANDOM) {
			// set color to random
			for (auto cl = grp->effect.effectColors.begin() + ((grp->effect.flags & GE_FLAG_BACK) > 0);
				cl != grp->effect.effectColors.end(); cl++)
				conf->SetRandomColor(&(*cl));
		}
		// prepare data
		gridop->gridX = x;
		gridop->gridY = y;
		gridop->current_tact = 0;
		gridop->oldphase = -1;
		gridop->lmp = grp->effect.flags & GE_FLAG_PHASE ? 1 : ((int)grp->effect.effectColors.size() - ((grp->effect.flags & GE_FLAG_BACK) != 0));
		gridop->effsize = grp->effect.flags & GE_FLAG_CIRCLE ? gridop->size << 1 : gridop->size;
		grp->gridop.passive = false;
	}
}

LRESULT CALLBACK GridKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

	if (wParam == WM_KEYDOWN && !(GetAsyncKeyState(((LPKBDLLHOOKSTRUCT)lParam)->vkCode) & 0xf000)) {
		conf->modifyProfile.lock();
 		for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++)
			if (it->effect.trigger == 2 && it->gridop.passive) { // keyboard effect
				// Is it have a key pressed?
				AlienFX_SDK::Afx_group* grp = conf->afx_dev.GetGroupById(it->group);
				for (auto lgh = grp->lights.begin(); lgh != grp->lights.end(); lgh++)
					if ((conf->afx_dev.GetMappingByID(lgh->did, lgh->lid)->scancode & 0xff) == ((LPKBDLLHOOKSTRUCT)lParam)->vkCode) {
						zonemap zone = *conf->FindZoneMap(it->group);
						for (auto pos = zone.lightMap.begin(); pos != zone.lightMap.end(); pos++)
							if (pos->light == lgh->lgh) {
								((GridHelper*)eve->grid)->StartGridRun(&(*it), &zone, pos->x, pos->y);
								break;
							}
					}
			}
		conf->modifyProfile.unlock();
	}

	return res;
}

void GridUpdate(LPVOID param) {
	fxhl->RefreshGrid();
}

void GridHelper::StartCommonRun(groupset* ce) {
	zonemap cz = *conf->FindZoneMap(ce->group);
	int srX = 0, srY = 0;
	if (ce->effect.flags & GE_FLAG_RPOS) {
		uniform_int_distribution<int> pntX(0, cz.gMaxX - 1);
		uniform_int_distribution<int> pntY(0, cz.gMaxY - 1);
		srX = pntX(conf->rnd); srY = pntY(conf->rnd);
	}
	else
		if (ce->gauge == 5) {
			srX = cz.gMaxX / 2; srY = cz.gMaxY / 2;
		}
	StartGridRun(&(*ce), &cz, srX, srY);
}

void GridTriggerWatch(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	conf->modifyProfile.lock();
	for (auto ce = conf->activeProfile->lightsets.begin(); ce != conf->activeProfile->lightsets.end(); ce++) {
		if (ce->effect.trigger && ce->gridop.passive) {
			switch (ce->effect.trigger) {
			case 4: case 1:
				src->StartCommonRun(&(*ce));
				break;
			case 3:
				for (auto ev = ce->events.begin(); ev != ce->events.end(); ev++)
					if (ev->state == MON_TYPE_IND && fxhl->CheckEvent(&fxhl->eData, &(*ev))) {
						// Triggering effect...
						src->StartCommonRun(&(*ce));
						break;
					}
				break;
			}
		}

	}
	conf->modifyProfile.unlock();
}

GridHelper::GridHelper() {
	RestartWatch();
}

GridHelper::~GridHelper()
{
	Stop();
}

void GridHelper::Stop() {
	if (gridTrigger) {
		delete gridTrigger;
		gridTrigger = NULL;
		delete gridThread;
		if (kEvent) {
			UnhookWindowsHookEx(kEvent);
			kEvent = NULL;
		}
		if (capt) {
			delete (CaptureHelper*)capt; capt = NULL;
		}
		if (sysmon) {
			delete sysmon; sysmon = NULL;
		}
	}
}

void GridHelper::RestartWatch() {
	Stop();
	conf->modifyProfile.lock();
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		if (ce->effect.trigger) {
			// Reset zone
			ce->gridop.passive = true;
			ce->gridop.stars.clear();
			switch (ce->effect.trigger) {
			case 2: if (!kEvent)
				kEvent = SetWindowsHookEx(WH_KEYBOARD_LL, GridKeyProc, NULL, 0);
				break;
			case 3: if (!sysmon)
				sysmon = new SysMonHelper();
				break;
			case 4: if (!capt) {
				capt = new CaptureHelper(false);
				auto zone = *conf->FindZoneMap(ce->group);
				capt->SetLightGridSize(zone.gMaxX, zone.gMaxY);
			} break;
			}
		}
	}
	conf->modifyProfile.unlock();

	gridTrigger = new ThreadHelper(GridTriggerWatch, (LPVOID)this, conf->geTact);
	gridThread = new ThreadHelper(GridUpdate, NULL, conf->geTact);
}
