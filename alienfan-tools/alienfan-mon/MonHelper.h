#pragma once
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "ThreadHelper.h"

class MonHelper {
private:
	ThreadHelper* monThread = NULL;
	short oldPower = 0;
	bool stopped = true;
	void Run();
	void Finish();
	void ToggleMode();
public:
	AlienFan_SDK::Control* acpi;
	bool inControl = true;
	map<byte,WORD> lastBoost, fanRpm;
	map<byte,byte> fanSleep;
	map<WORD, short> senValues, maxTemps;
	map<byte,map<WORD, byte>> senBoosts;
	WORD powerMode = 0;
	WORD fansize = 0, powerSize = 0, sensorSize = 0;
	bool modified = true;

	MonHelper();
	~MonHelper();
	void Start();
	void Stop();
	void SetCurrentMode(int newMode = -1);
	byte GetFanPercent(byte fanID);
	int GetPowerMode();
	void SetPowerMode(byte newMode);
	int GetFanRPM(int fanID);
	WORD GetSensorData(bool withRPM = true);
	void ResetBoost();
	void SetOC();
};

