#pragma once
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "ThreadHelper.h"

class MonHelper {
public:
	ThreadHelper* monThread = NULL;
	AlienFan_SDK::Control* acpi;
	short oldPower = -1;
	bool inControl = true;
	vector<WORD> fanRpm, lastBoost;
	vector<byte> boostRaw, fanSleep;
	map<WORD, short> senValues, maxTemps;
	vector<map<WORD, byte>> senBoosts;

	MonHelper();
	~MonHelper();
	void Start();
	void Stop();
	void SetCurrentMode(size_t newMode);
	void SetCurrentGmode(bool newMode);
	byte GetFanPercent(byte fanID);
};

