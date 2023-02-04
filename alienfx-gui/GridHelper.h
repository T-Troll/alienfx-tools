#pragma once
#include "ConfigHandler.h"
#include "ThreadHelper.h"
#include "CaptureHelper.h"
#include "SysMonHelper.h"

class GridHelper {
private:
	HHOOK kEvent = NULL;
	ThreadHelper* gridTrigger = NULL, *gridThread = NULL;
	SysMonHelper* sysmon = NULL;
	void Stop();
public:
	CaptureHelper* capt = NULL;
	GridHelper();
	~GridHelper();
	void RestartWatch();
	void StartCommonRun(groupset* ce);
	void StartGridRun(groupset* grp, zonemap* cz, int x, int y);
};