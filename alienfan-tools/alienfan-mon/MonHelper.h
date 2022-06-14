#pragma once
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "..\..\AlienFx-Common\ThreadHelper.h"

class MonHelper {
public:
	ThreadHelper* monThread = NULL;
	short oldPower = 0, oldGmode = 0;
	vector<int> senValues, fanRpm, boostRaw, boostSets, maxTemps, fanSleep;

	MonHelper(ConfigFan*);
	~MonHelper();
	void Start();
	void Stop();
};

