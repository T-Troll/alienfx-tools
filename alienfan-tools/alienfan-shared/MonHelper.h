#pragma once
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "ThreadHelper.h"

class MonHelper {
private:
	ThreadHelper* monThread = NULL;
public:
	ConfigFan* conf;

	//HANDLE stopEvent = 0;
	short oldPower = 0;
	AlienFan_SDK::Control* acpi;
	vector<int> senValues, fanValues, boostValues, boostRaw, boostSets, maxTemps, fanSleep;

	MonHelper(ConfigFan*, AlienFan_SDK::Control*);
	~MonHelper();
	void Start();
	void Stop();
};

