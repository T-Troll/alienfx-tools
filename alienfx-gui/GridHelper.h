#pragma once
#include "alienfx-gui.h"
#include "common.h"
#include <random>

class GridHelper {
private:
	HHOOK kEvent = NULL;
	ThreadHelper* gridTrigger = NULL, * gridThread = NULL;
public:
	GridHelper();
	~GridHelper();
	void UpdateEvent(EventData*);
	UINT tact = 0;
	mt19937 rnd;
};