#pragma once
#include "ConfigHandler.h"
#include "ThreadHelper.h"

class GridHelper {
private:
	HHOOK kEvent = NULL;
	ThreadHelper* gridTrigger = NULL, *gridThread = NULL;
	void Stop();
public:
	GridHelper();
	~GridHelper();
	void RestartWatch();
	void StartCommonRun(groupset* ce);
	void StartGridRun(groupset* grp, zonemap* cz, int x, int y);
	//unsigned long tact = 0;
};