#pragma once
#include "alienfx-gui.h"
#include "common.h"
#include <random>

class GridHelper {
private:
	HHOOK kEvent;
	ThreadHelper* gridTrigger, * gridThread;
public:
	GridHelper();
	~GridHelper();
	void UpdateEvent(EventData*);
	UINT tact = 0;
	mt19937 rnd;
};