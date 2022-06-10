#pragma once
#include "ConfigMon.h"
#include "alienfan-SDK.h"

class SenMonHelper
{
private:
	HANDLE eHandle = 0, bHandle = 0;
public:
	HANDLE stopEvents = NULL;
	SenMonHelper();
	~SenMonHelper();
	void StartMon();
	void StopMon();
	void ModifyMon();
};