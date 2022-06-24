#include "GridHelper.h"

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
							it->effect.gridX = x;
							it->effect.gridY = y;
							it->effect.passive = false;
							return res;
						}
					}
			}
	}

	return res;
}

void GridUpdate(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		if (ce->effect.trigger && !ce->effect.passive) {
			// calculate phase
			// not exactly, need to count slower speed and over-zero tact
			ce->effect.oldphase = ce->effect.phase;
			ce->effect.phase = (byte)abs((long)src->tact - (long)ce->effect.start_tact);
			ce->effect.phase = ce->effect.speed < 80 ? ce->effect.phase / (80 - ce->effect.speed) : ce->effect.phase * (ce->effect.speed - 79);
			if (ce->effect.phase > (ce->effect.flags & GE_FLAG_CIRCLE ? 2 * ce->effect.size : ce->effect.size)) {
				ce->effect.passive = true;
			}
			else
				if (ce->effect.phase > ce->effect.size) // circle
					ce->effect.phase = 2*ce->effect.size - ce->effect.phase;
			// Set lights
			fxhl->SetGridEffect(&(*ce));
		}
	}
	src->tact++;
}

void GridTriggerWatch(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		if (ce->gauge && ce->effect.trigger && ce->effect.passive) {
			zonemap* cz = conf->FindZoneMap(ce->group);
			switch (ce->effect.trigger) {
			case 1: // Continues
				switch (ce->gauge) {
				case 1:	case 2:	case 3:
					ce->effect.gridX = cz->gMinX; ce->effect.gridY = cz->gMinY;
					break;
				case 4:
					ce->effect.gridX = cz->gMaxX; ce->effect.gridY = cz->gMinY;
					break;
				case 5:
					ce->effect.gridX = cz->gMinX + (cz->gMaxX - cz->gMinX) / 2;
					ce->effect.gridY = cz->gMinY + (cz->gMaxY - cz->gMinY) / 2;
					break;
				}
				ce->effect.passive = false;
				break;
			case 2: { // Random
				uniform_int_distribution<int> pntX(cz->gMinX, cz->gMaxX);
				uniform_int_distribution<int> pntY(cz->gMinY, cz->gMaxY);
				ce->effect.gridX = pntX(src->rnd);
				ce->effect.gridY = pntY(src->rnd);
				ce->effect.passive = false;
			} break;
			}
			ce->effect.oldphase = 0;
			ce->effect.start_tact = src->tact;
		}
	}
}

GridHelper::GridHelper() {
	// check current profile sets - do we need events or haptics?
	// Also start keyboard hook if needed.
	tact = 0;
	gridTrigger = new ThreadHelper(GridTriggerWatch, (LPVOID)this, 100);
	gridThread = new ThreadHelper(GridUpdate, (LPVOID)this, 100);
	kEvent = SetWindowsHookExW(WH_KEYBOARD_LL, GridKeyProc, NULL, 0);
}

GridHelper::~GridHelper()
{
	delete gridTrigger;
	delete gridThread;
	gridTrigger = gridThread = NULL;
	// Clean haptics, events, hook
	UnhookWindowsHookEx(kEvent);
}