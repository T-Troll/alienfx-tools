#include "GridHelper.h"
#include "EventHandler.h"

extern EventHandler* eve;

void StartGridRun(groupset* grp, int x, int y) {
	grp->gridop.gridX = x;
	grp->gridop.gridY = y;
	grp->gridop.passive = false;
}

LRESULT CALLBACK GridKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

	if (wParam == WM_KEYDOWN /*&& !(GetAsyncKeyState(((LPKBDLLHOOKSTRUCT)lParam)->vkCode) & 0xf000)*/) {
		char keyname [32];
		GetKeyNameText(MAKELPARAM(0,((LPKBDLLHOOKSTRUCT)lParam)->scanCode), keyname, 31);
 		for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
			if (it->effect.trigger == 3 && it->gridop.passive) { // keyboard effect
				// Is it have a key pressed?
				AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(it->group);
				for (auto light = grp->lights.begin(); light != grp->lights.end(); light++)
					if (conf->afx_dev.GetMappingByDev(conf->afx_dev.GetDeviceById(LOWORD(*light), 0), HIWORD(*light))->name == (string)keyname) {
						zonemap* zone = conf->FindZoneMap(it->group);
						AlienFX_SDK::lightgrid* grid = conf->afx_dev.GetGridByID((byte)zone->gridID);
						for (int x = zone->gMinX; x < zone->gMaxX; x++)
							for (int y = zone->gMinY; y < zone->gMaxY; y++) {
								if (grid->grid[ind(x, y)] == *light) {
									// start effect
									StartGridRun(&(*it), x, y);
									return res;
								}
							}
						break;
					}
			}
	}

	return res;
}

void GridUpdate(LPVOID param) {
	fxhl->RefreshGrid(((GridHelper*)param)->tact++);
}

void GridTriggerWatch(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->active_set->begin(); ce < conf->active_set->end(); ce++) {
		if (ce->gauge && ce->effect.trigger && ce->gridop.passive) {
			zonemap* cz = conf->FindZoneMap(ce->group);
			switch (ce->effect.trigger) {
			case 1: // Continues
				switch (ce->gauge) {
				case 1:	case 2:	case 3: case 4:
					StartGridRun(&(*ce), cz->gMinX, cz->gMinY);
					break;
				//case 4:
				//	ce->gridop.gridX = cz->gMaxX-1; ce->gridop.gridY = cz->gMinY;
				//	break;
				case 5:
					StartGridRun(&(*ce), cz->gMinX + (cz->gMaxX - cz->gMinX) / 2, cz->gMinY + (cz->gMaxY - cz->gMinY) / 2);
					break;
				}
				break;
			case 2: { // Random
				uniform_int_distribution<int> pntX(cz->gMinX, cz->gMaxX);
				uniform_int_distribution<int> pntY(cz->gMinY, cz->gMaxY);
				StartGridRun(&(*ce), pntX(src->rnd), pntY(src->rnd));
			} break;
			}
			ce->gridop.start_tact = src->tact;
			ce->gridop.oldphase = 0;
		}
	}
}

GridHelper::GridHelper() {
	tact = 0;
	eve->StartEvents();
	gridTrigger = new ThreadHelper(GridTriggerWatch, (LPVOID)this, 100);
	gridThread = new ThreadHelper(GridUpdate, (LPVOID)this, 100);
	kEvent = SetWindowsHookExW(WH_KEYBOARD_LL, GridKeyProc, NULL, 0);
}

GridHelper::~GridHelper()
{
	UnhookWindowsHookEx(kEvent);
	eve->StopEvents();
	delete gridTrigger;
	delete gridThread;
}

void GridHelper::UpdateEvent(EventData* data) {
	for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
		if (it->effect.trigger == 4 && it->gridop.passive) { // Event trigger
			for (auto ev = it->events.begin(); ev != it->events.end(); ev++)
				if (ev->state == MON_TYPE_IND) {
					int ccut = ev->cut, cVal = 0;
					switch (ev->source) {
					case 0: cVal = data->HDD; break;
					case 1: cVal = data->NET; break;
					case 2: cVal = data->Temp - ccut; break;
					case 3: cVal = data->RAM - ccut; break;
					case 4: cVal = data->Batt - ccut; break;
					case 5: cVal = data->KBD; break;
					}

					if (cVal > 0) {
						// Triggering effect...
						zonemap* cz = conf->FindZoneMap(it->group);
						switch (it->gauge) {
						case 1:	case 2:	case 3:
							StartGridRun(&(*it), cz->gMinX, cz->gMinY);
							break;
						case 4:
							StartGridRun(&(*it), cz->gMaxX, cz->gMinY);
							break;
						case 5:
							StartGridRun(&(*it), cz->gMinX + (cz->gMaxX - cz->gMinX) / 2, cz->gMinY + (cz->gMaxY - cz->gMinY) / 2);
							break;
						}
						break;
					}
				}
		}
	memcpy(&fxhl->eData, data, sizeof(EventData));
}