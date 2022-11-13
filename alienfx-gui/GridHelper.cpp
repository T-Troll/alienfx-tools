#include "GridHelper.h"
#include "EventHandler.h"

extern EventHandler* eve;

void StartGridRun(groupset* grp, zonemap* cz, int x, int y) {
	int cx = max(x, cz->xMax - x), cy = max(y, cz->yMax - y);
	switch (grp->gauge) {
	case 0: case 1:
		grp->effect.size = cx;
		break;
	case 2:
		grp->effect.size = cy;
		break;
	case 3: case 4:
		grp->effect.size = cx + cy;
		break;
	case 5:
		grp->effect.size = max(cx, cy);
		break;
	}
	grp->gridop.gridX = x;
	grp->gridop.gridY = y;
	grp->gridop.passive = false;
}

LRESULT CALLBACK GridKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

	if (wParam == WM_KEYDOWN && !(GetAsyncKeyState(((LPKBDLLHOOKSTRUCT)lParam)->vkCode) & 0xf000)) {
		char keyname [32];
		GetKeyNameText(MAKELPARAM(0,((LPKBDLLHOOKSTRUCT)lParam)->scanCode), keyname, 31);
 		for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
			if (it->effect.trigger == 3 && it->gridop.passive) { // keyboard effect
				// Is it have a key pressed?
				zonemap* zone = conf->FindZoneMap(it->group);
				for (auto pos = zone->lightMap.begin(); pos != zone->lightMap.end(); pos++)
					if (conf->afx_dev.GetMappingByDev(conf->afx_dev.GetDeviceById(LOWORD(pos->light), 0), HIWORD(pos->light))->name == (string)keyname) {
						StartGridRun(&(*it), zone, pos->x, pos->y);
						break;
					}
			}
	}

	return res;
}

void GridUpdate(LPVOID param) {
	fxhl->RefreshGrid(((GridHelper*)param)->tact++);
}

void StartCommonRun(groupset* ce, zonemap* cz) {
	switch (ce->gauge) {
	case 0: case 1:
		StartGridRun(&(*ce), cz, 0, 0);
		break;
	case 2:
		StartGridRun(&(*ce), cz, 0, 0);
		break;
	case 3: case 4:
		StartGridRun(&(*ce), cz, 0, 0);
		break;
	case 5:
		StartGridRun(&(*ce), cz, cz->xMax / 2, cz->yMax / 2);
		break;
	}
}

void GridTriggerWatch(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->active_set->begin(); ce < conf->active_set->end(); ce++) {
		if (ce->gauge && ce->effect.trigger && ce->gridop.passive) {
			zonemap* cz = conf->FindZoneMap(ce->group);
			switch (ce->effect.trigger) {
			case 1: // Continues
				StartCommonRun(&(*ce), cz);
				break;
			case 2: { // Random
				uniform_int_distribution<int> pntX(0, cz->xMax-1);
				uniform_int_distribution<int> pntY(0, cz->yMax-1);
				StartGridRun(&(*ce), cz, pntX(src->rnd), pntY(src->rnd));
			} break;
			case 4: { // Indicator
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
							StartCommonRun(&(*ce), cz);
						}
					}
			} break;
			}
			ce->gridop.start_tact = src->tact;
			//ce->gridop.oldphase = -1;
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
	UnhookWindowsHookEx(kEvent);
	eve->StopEvents();
	delete gridTrigger;
	delete gridThread;
}

//void GridHelper::UpdateEvent(EventData* data) {
//	//for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
//	//	if (it->effect.trigger == 4 && it->gridop.passive) { // Event trigger
//	//		for (auto ev = it->events.begin(); ev != it->events.end(); ev++)
//	//			if (ev->state == MON_TYPE_IND) {
//	//				int ccut = ev->cut, cVal = 0;
//	//				switch (ev->source) {
//	//				case 0: cVal = data->HDD; break;
//	//				case 1: cVal = data->NET; break;
//	//				case 2: cVal = data->Temp - ccut; break;
//	//				case 3: cVal = data->RAM - ccut; break;
//	//				case 4: cVal = data->Batt - ccut; break;
//	//				case 5: cVal = data->KBD; break;
//	//				}
//
//	//				if (cVal > 0) {
//	//					// Triggering effect...
//	//					zonemap* cz = conf->FindZoneMap(it->group);
//	//					switch (it->gauge) {
//	//					case 1:	case 2:	case 3:
//	//						StartGridRun(&(*it), cz->gMinX, cz->gMinY);
//	//						break;
//	//					case 4:
//	//						StartGridRun(&(*it), cz->gMaxX, cz->gMinY);
//	//						break;
//	//					case 5:
//	//						StartGridRun(&(*it), cz->gMinX + (cz->gMaxX - cz->gMinX) / 2, cz->gMinY + (cz->gMaxY - cz->gMinY) / 2);
//	//						break;
//	//					}
//	//					break;
//	//				}
//	//			}
//	//	}
//	memcpy(&fxhl->eData, data, sizeof(EventData));
//}