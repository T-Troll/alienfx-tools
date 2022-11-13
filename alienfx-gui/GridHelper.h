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
	void StartCommonRun(groupset* ce, zonemap* cz);
	void StartGridRun(groupset* grp, zonemap* cz, int x, int y);
	UINT tact = 0;
	mt19937 rnd;
};