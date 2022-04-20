#pragma once
#include "ConfigMon.h"
#include "../alienfan-tools/alienfan-SDK/alienfan-SDK.h"

class SenMonHelper
{
private:
	HANDLE eHandle = 0, bHandle = 0;
public:
	ConfigMon* conf = NULL;
	AlienFan_SDK::Control* acpi = NULL;
	bool validB = false;
	HANDLE stopEvents = NULL;
	SenMonHelper(ConfigMon*);
	~SenMonHelper();
	void StartMon();
	void StopMon();
	void ModifyMon();
};