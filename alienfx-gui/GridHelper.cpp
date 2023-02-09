#include "GridHelper.h"
#include "EventHandler.h"
#include "FXHelper.h"

extern EventHandler* eve;
extern ConfigHandler* conf;
extern FXHelper* fxhl;

void GridHelper::StartGridRun(groupset* grp, zonemap* cz, int x, int y) {
	if (grp->effect.effectColors.size() > 1) {
		int cx = max(x + 1, cz->xMax - x), cy = max(y + 1, cz->yMax - y);
		switch (grp->gauge) {
		case 1:
			grp->gridop.size = cx;
			break;
		case 2:
			grp->gridop.size = cy;
			break;
		case 0: case 3: case 4:
			grp->gridop.size = cx + cy - 2;
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
		// prepare data
		grp->gridop.size = max(grp->gridop.size + grp->effect.width - 1, 1);
		grp->gridop.gridX = x;
		grp->gridop.gridY = y;
		grp->gridop.current_tact = 0;
		grp->gridop.oldphase = -1;
		grp->gridop.passive = false;
	}
}

LRESULT CALLBACK GridKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

	if (wParam == WM_KEYDOWN && !(GetAsyncKeyState(((LPKBDLLHOOKSTRUCT)lParam)->vkCode) & 0xf000)) {
		eve->modifyProfile.lock();
 		for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++)
			if (it->effect.trigger == 2 && it->gridop.passive) { // keyboard effect
				// Is it have a key pressed?
				AlienFX_SDK::Afx_group* grp = conf->afx_dev.GetGroupById(it->group);
				for (auto lgh = grp->lights.begin(); lgh != grp->lights.end(); lgh++)
					if ((conf->afx_dev.GetMappingByID(lgh->did, lgh->lid)->scancode & 0xff) == ((LPKBDLLHOOKSTRUCT)lParam)->vkCode) {
						zonemap* zone = conf->FindZoneMap(it->group);
						for (auto pos = zone->lightMap.begin(); pos != zone->lightMap.end(); pos++)
							if (pos->light == lgh->lgh) {
								eve->grid->StartGridRun(&(*it), zone, pos->x, pos->y);
								break;
							}
					}
			}
		eve->modifyProfile.unlock();
	}

	return res;
}

void GridUpdate(LPVOID param) {
	if (conf->lightsNoDelay)
		fxhl->RefreshGrid();
}

void GridHelper::StartCommonRun(groupset* ce) {
	zonemap* cz = conf->FindZoneMap(ce->group);
	int srX = 0, srY = 0;
	if (ce->effect.flags & GE_FLAG_RPOS) {
		uniform_int_distribution<int> pntX(0, cz->xMax - 1);
		uniform_int_distribution<int> pntY(0, cz->yMax - 1);
		srX = pntX(conf->rnd); srY = pntY(conf->rnd);
	}
	else
		if (ce->gauge == 5) {
			srX = cz->xMax / 2; srY = cz->yMax / 2;
		}
	StartGridRun(&(*ce), cz, srX, srY);
}

void GridTriggerWatch(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	eve->modifyProfile.lock();
	for (auto ce = conf->activeProfile->lightsets.begin(); ce != conf->activeProfile->lightsets.end(); ce++) {
		if (ce->gridop.passive) {
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
	eve->modifyProfile.unlock();
}

GridHelper::GridHelper() {
	if (eve->capt)
		Sleep(150);
	RestartWatch();
}

GridHelper::~GridHelper()
{
	Stop();
}

void GridHelper::Stop() {
	delete gridTrigger;
	delete gridThread;
	if (kEvent) {
		UnhookWindowsHookEx(kEvent);
		kEvent = NULL;
	}
	if (capt) {
		delete capt; capt = NULL;
	}
	if (sysmon) {
		delete sysmon; sysmon = NULL;
	}
}

void GridHelper::RestartWatch() {
	Stop();

	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		ce->gridop.passive = true;
		switch (ce->effect.trigger) {
		case 2: if (!kEvent)
			kEvent = SetWindowsHookEx(WH_KEYBOARD_LL, GridKeyProc, NULL, 0);
			break;
		case 3: if (!sysmon)
			sysmon = new SysMonHelper();
			break;
		case 4: if (!capt)
			capt = new CaptureHelper(false);
			break;
		}
	}

	gridTrigger = new ThreadHelper(GridTriggerWatch, (LPVOID)this, conf->geTact);
	gridThread = new ThreadHelper(GridUpdate, NULL/*(LPVOID)this*/, conf->geTact);
}
