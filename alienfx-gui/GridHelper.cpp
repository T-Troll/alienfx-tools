#include "GridHelper.h"
#include "EventHandler.h"
#include "FXHelper.h"

extern EventHandler* eve;
extern ConfigHandler* conf;
extern FXHelper* fxhl;

//extern AlienFX_SDK::Afx_action* Code2Act(AlienFX_SDK::Afx_colorcode* c);

void GridHelper::StartGridRun(groupset* grp, zonemap* cz, int x, int y) {
	if (grp->effect.effectColors.size() > 1) {
		int cx = max(x, cz->xMax - x), cy = max(y, cz->yMax - y);
		switch (grp->gauge) {
		case 1:
			grp->gridop.size = cx;
			break;
		case 2:
			grp->gridop.size = cy;
			break;
		case 0: case 3: case 4:
			grp->gridop.size = cx + cy;
			break;
		case 5:
			grp->gridop.size = max(cx, cy);
			break;
		}
		if (grp->effect.flags & GE_FLAG_RANDOM) {
			// set color to random
			for (auto cl = grp->effect.effectColors.begin(); cl != grp->effect.effectColors.end(); cl++)
				conf->SetRandomColor(&(*cl));
		}
		grp->gridop.gridX = x;
		grp->gridop.gridY = y;
		grp->gridop.start_tact = tact;
		grp->gridop.oldphase = -1;
		grp->gridop.passive = false;
	}
}

LRESULT CALLBACK GridKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

	if (wParam == WM_KEYDOWN && !(GetAsyncKeyState(((LPKBDLLHOOKSTRUCT)lParam)->vkCode) & 0xf000)) {
		char keyname[32];
		GetKeyNameText(MAKELPARAM(0,((LPKBDLLHOOKSTRUCT)lParam)->scanCode), keyname, 31);
		eve->modifyProfile.lock();
 		for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++)
			if (it->effect.trigger == 2 && it->gridop.passive) { // keyboard effect
				// Is it have a key pressed?
				zonemap* zone = conf->FindZoneMap(it->group);
				for (auto pos = zone->lightMap.begin(); pos != zone->lightMap.end(); pos++)
					if (conf->afx_dev.GetMappingByID(LOWORD(pos->light), HIWORD(pos->light))->name == (string)keyname) {
						eve->grid->StartGridRun(&(*it), zone, pos->x, pos->y);
						break;
					}
			}
		eve->modifyProfile.unlock();
	}

	return res;
}

void GridUpdate(LPVOID param) {
	GridHelper* gh = (GridHelper*)param;
	if (gh->tact < 0)
		gh->tact = 0;
	if (conf->lightsNoDelay)
		fxhl->RefreshGrid(gh->tact++);
}

void GridHelper::StartCommonRun(groupset* ce) {
	zonemap* cz = conf->FindZoneMap(ce->group);
	if (ce->flags & GE_FLAG_RPOS) {
		uniform_int_distribution<int> pntX(0, cz->xMax - 1);
		uniform_int_distribution<int> pntY(0, cz->yMax - 1);
		StartGridRun(&(*ce), cz, pntX(conf->rnd), pntY(conf->rnd));
	} else
		if (ce->gauge == 5)
			StartGridRun(&(*ce), cz, cz->xMax / 2, cz->yMax / 2);
		else
			StartGridRun(&(*ce), cz, 0, 0);
}

void GridTriggerWatch(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	eve->modifyProfile.lock();
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		if (/*ce->effect.trigger && */ce->gridop.passive) {
			switch (ce->effect.trigger) {
			case 1: src->StartCommonRun(&(*ce));
				break;
			case 3:
				for (auto ev = ce->events.begin(); ev != ce->events.end(); ev++)
					if (ev->state == MON_TYPE_IND && fxhl->CheckEvent(&fxhl->eData, &(*ev)) > 0) {
						// Triggering effect...
						src->StartCommonRun(&(*ce));
						break;
					}
				break;
			}
		}
	}
	eve->modifyProfile.unlock();
}

GridHelper::GridHelper() {
	tact = 0;
	gridTrigger = new ThreadHelper(GridTriggerWatch, (LPVOID)this, 100);
	gridThread = new ThreadHelper(GridUpdate, (LPVOID)this, 100);
	RestartWatch();
}

GridHelper::~GridHelper()
{
	delete gridTrigger;
	delete gridThread;
	if (kEvent)
		UnhookWindowsHookEx(kEvent);
	eve->StopEvents();
}

void GridHelper::RestartWatch() {
	bool haveEvents = false, haveKeys = false;
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		switch (ce->effect.trigger) {
		case 2: haveKeys = true; break;
		case 3: haveEvents = true; break;
		}
	}
	if (haveKeys && !kEvent)
		kEvent = SetWindowsHookExW(WH_KEYBOARD_LL, GridKeyProc, NULL, 0);
	else {
		UnhookWindowsHookEx(kEvent);
		kEvent = NULL;
	}
	if (haveEvents)
		eve->StartEvents();
	else
		eve->StopEvents();
}
