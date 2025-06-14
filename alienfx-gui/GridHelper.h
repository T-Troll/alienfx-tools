#pragma once
#include "ThreadHelper.h"
#include "SysMonHelper.h"
#include "ConfigHandler.h"

class GridHelper {
private:
	HHOOK kEvent = NULL;
	ThreadHelper* gridTrigger = NULL, *gridThread = NULL;
	SysMonHelper* sysmon = NULL;
	void Stop();
public:
	GridHelper();
	~GridHelper();
	void RestartWatch();
	void StartCommonRun(groupset* ce);
	void StartGridRun(groupset* grp, zonemap* cz, int x, int y);
};