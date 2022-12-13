#include "GridHelper.h"
#include "EventHandler.h"
#include "FXHelper.h"

extern EventHandler* eve;
extern ConfigHandler* conf;
extern FXHelper* fxhl;

extern AlienFX_SDK::Afx_action* Code2Act(AlienFX_SDK::Afx_colorcode* c);

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
			uniform_int_distribution<DWORD> ccomp(0x00404040, 0x00ffffff);
			for (auto cl = grp->effect.effectColors.begin(); cl != grp->effect.effectColors.end(); cl++)
				cl->ci = ccomp(conf->rnd);
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
		char keyname [32];
		GetKeyNameText(MAKELPARAM(0,((LPKBDLLHOOKSTRUCT)lParam)->scanCode), keyname, 31);
 		for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++)
			if (it->effect.trigger == 3 && it->gridop.passive) { // keyboard effect
				// Is it have a key pressed?
				zonemap* zone = conf->FindZoneMap(it->group);
				for (auto pos = zone->lightMap.begin(); pos != zone->lightMap.end(); pos++)
					if (conf->afx_dev.GetMappingByDev(conf->afx_dev.GetDeviceById(LOWORD(pos->light), 0), HIWORD(pos->light))->name == (string)keyname) {
						eve->grid->StartGridRun(&(*it), zone, pos->x, pos->y);
						break;
					}
			}
	}

	return res;
}

void GridUpdate(LPVOID param) {
	if (conf->lightsNoDelay)
		fxhl->RefreshGrid(((GridHelper*)param)->tact++);
}

void GridHelper::StartCommonRun(groupset* ce) {
	zonemap* cz = conf->FindZoneMap(ce->group);
	if (ce->effect.trigger == 2) {
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
	vector<groupset> active = conf->activeProfile->lightsets;
	eve->modifyProfile.unlock();
	for (auto ce = active.begin(); ce < active.end(); ce++) {
		if (ce->effect.trigger && ce->gridop.passive) {
			if (ce->effect.trigger == 4) { // indicator
				for (auto ev = ce->events.begin(); ev != ce->events.end(); ev++)
					if (ev->state == MON_TYPE_IND) {
						int ccut = ev->cut, cVal = 0;
						switch (ev->source) {
						case 0: cVal = fxhl->eData.HDD; break;
						case 1: cVal = fxhl->eData.NET; break;
						case 2: cVal = fxhl->eData.Temp - ccut; break;
						case 3: cVal = fxhl->eData.RAM - ccut; break;
						case 4: cVal = fxhl->eData.Batt - ccut; break;
						case 5: cVal = fxhl->eData.KBD; break;
						}

						if (cVal > 0) {
							// Triggering effect...
							src->StartCommonRun(&(*ce));
						}
					}
			} else
				src->StartCommonRun(&(*ce));
		}
	}

}

GridHelper::GridHelper() {
	tact = 0;
	eve->StartEvents();
	gridTrigger = new ThreadHelper(GridTriggerWatch, (LPVOID)this, 100);
	gridThread = new ThreadHelper(GridUpdate, (LPVOID)this, 100);
#ifndef _DEBUG
	kEvent = SetWindowsHookExW(WH_KEYBOARD_LL, GridKeyProc, NULL, 0);
#endif // !_DEBUG
}

GridHelper::~GridHelper()
{
#ifndef _DEBUG
	UnhookWindowsHookEx(kEvent);
#endif
	eve->StopEvents();
	delete gridTrigger;
	delete gridThread;
}
