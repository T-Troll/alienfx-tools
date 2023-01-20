#pragma once
#include <Pdh.h>
#include <PdhMsg.h>
#include "ConfigMon.h"
#include "alienfan-SDK.h"

class SenMonHelper
{
private:
	HANDLE eHandle = 0, bHandle = 0;
	LPCTSTR COUNTER_PATH_CPU = "\\Processor Information(_Total)\\% Processor Time",
		//COUNTER_PATH_NET = "\\Network Interface(*)\\Bytes Total/sec",
		COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage",
		COUNTER_PATH_TEMP = "\\Thermal Zone Information(*)\\Temperature",
		COUNTER_PATH_ESIF = "\\EsifDeviceInformation(*)\\Temperature",
		COUNTER_PATH_PWR = "\\EsifDeviceInformation(*)\\RAPL Power",
		COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Idle Time";

	PDH_FMT_COUNTERVALUE_ITEM* counterValues = new PDH_FMT_COUNTERVALUE_ITEM[1];
	DWORD counterSize = sizeof(PDH_FMT_COUNTERVALUE_ITEM);

	HQUERY hQuery = NULL;
	HCOUNTER hCPUCounter, hHDDCounter, /*hNETCounter,*/ hGPUCounter, hTempCounter, hTempCounter2, hPwrCounter;

	MEMORYSTATUSEX memStat{ sizeof(MEMORYSTATUSEX) };

	SYSTEM_POWER_STATUS state;

	DWORD cType = 0, valCount = 0;

	int GetValuesArray(HCOUNTER counter);
	void AddUpdateSensor(SENID sid, long val, string name);
public:
	HANDLE stopEvents = NULL;
	SenMonHelper();
	~SenMonHelper();
	void ModifyMon();
	void UpdateSensors();
};