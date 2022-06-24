#pragma once
#include "alienfx-gui.h"
#include "common.h"
#include <random>

class GridHelper {
public:
	GridHelper();
	~GridHelper();
	UINT tact = 0;
	mt19937 rnd;
	HHOOK kEvent = NULL;
	ThreadHelper* gridTrigger = NULL, *gridThread = NULL;
};