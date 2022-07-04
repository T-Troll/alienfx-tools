#include "GridHelper.h"
#include "EventHandler.h"

extern EventHandler* eve;

LRESULT CALLBACK GridKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

	if (wParam == WM_KEYDOWN && !(GetAsyncKeyState(((LPKBDLLHOOKSTRUCT)lParam)->vkCode) & 0xf000)) {
		char keyname [32];
		GetKeyNameText(MAKELPARAM(0,((LPKBDLLHOOKSTRUCT)lParam)->scanCode), keyname, 31);
 		for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
			if (it->effect.trigger == 3) { // keyboard effect
				// check grid
				zonemap* zone = conf->FindZoneMap(it->group);
				for (int x = zone->gMinX; x < zone->gMaxX; x++)
					for (int y = zone->gMinY; y < zone->gMaxY; y++) {
						DWORD gridval = conf->afx_dev.GetGridByID((byte)zone->gridID)->grid[ind(x, y)];
						if (gridval && conf->afx_dev.GetMappingById(conf->afx_dev.GetDeviceById(LOWORD(gridval)), HIWORD(gridval))->name == (string)keyname) {
							// start effect
							it->gridop.gridX = x;
							it->gridop.gridY = y;
							it->gridop.passive = false;
							return res;
						}
					}
			}
	}

	return res;
}

void GridUpdate(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->active_set->begin(); ce < conf->active_set->end(); ce++) {
		if (ce->effect.trigger && !ce->gridop.passive) {
			// calculate phase
			// not exactly, need to count slower speed and over-zero tact
			ce->gridop.oldphase = ce->gridop.phase;
			ce->gridop.phase = (byte)abs((long)src->tact - (long)ce->gridop.start_tact);
			ce->gridop.phase = ce->effect.speed < 80 ? ce->gridop.phase / (80 - ce->effect.speed) : ce->gridop.phase * (ce->effect.speed - 79);
			if (ce->gridop.phase > (ce->effect.flags & GE_FLAG_CIRCLE ? 2 * ce->effect.size : ce->effect.size)) {
				ce->gridop.passive = true;
			}
			else
				if (ce->gridop.phase > ce->effect.size) // circle
					ce->gridop.phase = 2*ce->effect.size - ce->gridop.phase;
			// Set lights
			fxhl->SetGridEffect(&(*ce));
		}
	}
	src->tact++;
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
					ce->gridop.gridX = cz->gMinX; ce->gridop.gridY = cz->gMinY;
					break;
				//case 4:
				//	ce->gridop.gridX = cz->gMaxX-1; ce->gridop.gridY = cz->gMinY;
				//	break;
				case 5:
					ce->gridop.gridX = cz->gMinX + (cz->gMaxX - cz->gMinX) / 2;
					ce->gridop.gridY = cz->gMinY + (cz->gMaxY - cz->gMinY) / 2;
					break;
				}
				ce->gridop.passive = false;
				break;
			case 2: { // Random
				uniform_int_distribution<int> pntX(cz->gMinX, cz->gMaxX);
				uniform_int_distribution<int> pntY(cz->gMinY, cz->gMaxY);
				ce->gridop.gridX = pntX(src->rnd);
				ce->gridop.gridY = pntY(src->rnd);
				ce->gridop.passive = false;
			} break;
			}
			ce->gridop.oldphase = 0;
			ce->gridop.start_tact = src->tact;
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
	gridTrigger = gridThread = NULL;
	// Clean haptics, events, hook
}

void GridHelper::UpdateEvent(EventData* data) {
	for (auto it = conf->active_set->begin(); it < conf->active_set->end(); it++)
		if (it->effect.trigger == 4 && it->events[2].state && it->gridop.passive) { // Event trigger
			int ccut = it->events[2].cut, cVal = 0;
			switch (it->events[2].source) {
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
					it->gridop.gridX = cz->gMinX; it->gridop.gridY = cz->gMinY;
					break;
				case 4:
					it->gridop.gridX = cz->gMaxX; it->gridop.gridY = cz->gMinY;
					break;
				case 5:
					it->gridop.gridX = cz->gMinX + (cz->gMaxX - cz->gMinX) / 2;
					it->gridop.gridY = cz->gMinY + (cz->gMaxY - cz->gMinY) / 2;
					break;
				}
				it->gridop.passive = false;
			}
		}
	memcpy(&fxhl->eData, data, sizeof(EventData));
}