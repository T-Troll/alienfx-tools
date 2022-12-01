#pragma once
#include "ConfigHandler.h"
#include "ThreadHelper.h"

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
};