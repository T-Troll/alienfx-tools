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
	map<byte,WORD> lastBoost, fanRpm;
	map<byte,byte> fanSleep;
	map<WORD, short> senValues, maxTemps;
	map<byte,map<WORD, byte>> senBoosts;
	WORD powerMode = 0;
	WORD fansize, powerSize, sensorSize;
	bool modified;

	MonHelper();
	~MonHelper();
	void Start();
	void Stop();
	void SetCurrentMode(int newMode = -1);
	byte GetFanPercent(byte fanID);
	int GetPowerMode();
	void SetPowerMode(byte newMode);
	void ResetBoost();
	void SetOC();
};

