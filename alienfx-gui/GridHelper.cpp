#include "GridHelper.h"

void GridUpdate(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		if (ce->effect.trigger && !ce->effect.passive) {
			// Set lights
			// update phase
			// Phase - tact % size - if zero stop non-continues
		}
	}
	src->tact++;
}

void GridTriggerWatch(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		if (ce->effect.trigger && ce->effect.passive) {
			zonemap* cz = conf->FindZoneMap(ce->group);
			switch (ce->effect.trigger) {
			case 1: // Continues
				ce->effect.passive = false;
				ce->effect.gridX = 255;
				ce->effect.gridY = 255;
				break;
			case 2: { // Random
				uniform_int_distribution<int> pntX(cz->gMinX, cz->gMaxX);
				uniform_int_distribution<int> pntY(cz->gMinY, cz->gMaxY);
				ce->effect.passive = false;
				ce->effect.gridX = pntX(src->rnd);
				ce->effect.gridY = pntY(src->rnd);
			} break;
			}
		}
	}
}

GridHelper::GridHelper() {
	// check current profile sets - do we need events or haptics?
	// Also start keyboard hook if needed.
	gridTrigger = new ThreadHelper(GridTriggerWatch, (LPVOID)this, 100);
	gridThread = new ThreadHelper(GridUpdate, (LPVOID)this, 50);
}

GridHelper::~GridHelper()
{
	delete gridTrigger;
	delete gridThread;
	gridTrigger = gridThread = NULL;
	// Clean haptics, events, hook
}