#pragma once
#include <windowsx.h>
#include "ConfigFan.h"
#include "alienfan-SDK.h"
#include "ThreadHelper.h"

class MonHelper {
private:
	ThreadHelper* monThread = NULL;
public:
	ConfigFan* conf;

	//HANDLE stopEvent = 0;
	short oldPower = 0;
	AlienFan_SDK::Control* acpi;
	vector<int> senValues, fanValues, boostValues, boostSets, maxTemps;

	MonHelper(ConfigFan*, AlienFan_SDK::Control*);
	~MonHelper();
	void Start();
	void Stop();
};

