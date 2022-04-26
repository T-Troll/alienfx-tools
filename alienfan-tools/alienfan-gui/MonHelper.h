#pragma once
#include <windowsx.h>
#include "ConfigFan.h"
#include "alienfan-SDK.h"

class MonHelper {
private:
	HANDLE dwHandle = 0;
public:
	ConfigFan* conf;

	HANDLE stopEvent = 0;
	short oldPower = 0;
	AlienFan_SDK::Control* acpi;
	vector<int> senValues, fanValues, boostValues, boostSets, maxTemps;

	MonHelper(ConfigFan*, AlienFan_SDK::Control*);
	~MonHelper();
	void Start();
	void Stop();
};

