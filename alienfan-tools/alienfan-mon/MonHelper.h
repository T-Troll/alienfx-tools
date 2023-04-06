#pragma once
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "ThreadHelper.h"

class MonHelper {
private:
	ThreadHelper* monThread = NULL;
	short oldPower = 0;
public:
	AlienFan_SDK::Control* acpi;
	bool inControl = true;
	map<byte,WORD> fanRpm, lastBoost;
	map<byte,byte> boostRaw, fanSleep;
	map<WORD, short> senValues, maxTemps;
	map<byte,map<WORD, byte>> senBoosts;
	WORD powerMode = 0;
	WORD fansize, powerSize;

	MonHelper();
	~MonHelper();
	void Start();
	void Stop();
	void SetCurrentMode(WORD newMode);
	byte GetFanPercent(byte fanID);
	int GetPowerMode();
	void SetPowerMode(WORD newMode);
	void ResetBoost();
	void SetProfilePower();
};

