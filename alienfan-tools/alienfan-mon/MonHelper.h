#pragma once
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "ThreadHelper.h"

class MonHelper {
public:
	ThreadHelper* monThread = NULL;
	short oldPower = -1, oldGmode = 0;
	bool inControl = true;
	vector<WORD> fanRpm;
	vector<byte> boostRaw, boostSets, fanSleep;
	map<WORD, short> senValues, maxTemps;
	vector<map<WORD, byte>> senBoosts;

	MonHelper();
	~MonHelper();
	void Start();
	void Stop();
	void SetCurrentGmode(WORD newMode);
	byte GetFanPercent(byte fanID);
};

