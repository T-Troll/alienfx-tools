#include "GridHelper.h"

void GridUpdate(LPVOID param) {
	GridHelper* src = (GridHelper*)param;
	for (auto ce = conf->activeProfile->lightsets.begin(); ce < conf->activeProfile->lightsets.end(); ce++) {
		if (ce->effect.trigger && !ce->effect.passive) {
			// calculate phase
			// not exactly, need to count slower speed and over-zero tact
			ce->effect.phase = abs((long)src->tact - (long)ce->effect.start_tact) * ce->effect.speed;
			if (ce->effect.phase > (ce->effect.flags & GE_FLAG_CIRCLE ? 2 * ce->effect.size : ce->effect.size)) {
				ce->effect.passive = true;
				ce->effect.phase = 0;
			}
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
				ce->effect.passive = false;
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
				break;
			case 2: { // Random
				uniform_int_distribution<int> pntX(cz->gMinX, cz->gMaxX);
				uniform_int_distribution<int> pntY(cz->gMinY, cz->gMaxY);
				ce->effect.passive = false;
				ce->effect.gridX = pntX(src->rnd);
				ce->effect.gridY = pntY(src->rnd);
			} break;
			}
			ce->effect.start_tact = src->tact;
		}
	}
}

GridHelper::GridHelper() {
	// check current profile sets - do we need events or haptics?
	// Also start keyboard hook if needed.
	tact = 0;
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