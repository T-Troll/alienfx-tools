#pragma once
#include <pdh.h>
#include "FXHelper.h"
#include "ThreadHelper.h"

class SysMonHelper {
private:
	const LPCTSTR COUNTER_PATH_CPU = "\\Processor Information(_Total)\\% Processor Time",
		COUNTER_PATH_NET = "\\Network Interface(*)\\Bytes Total/sec",
		COUNTER_PATH_NETMAX = "\\Network Interface(*)\\Current BandWidth",
		COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage",
		COUNTER_PATH_HOT = "\\Thermal Zone Information(*)\\Temperature",
		COUNTER_PATH_HOT2 = "\\EsifDeviceInformation(*)\\Temperature",
		COUNTER_PATH_PWR = "\\EsifDeviceInformation(*)\\RAPL Power",
		COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Idle Time";

	ThreadHelper* eventProc = NULL;
public:

	HQUERY hQuery = NULL;
	HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hNETMAXCounter, hGPUCounter, hTempCounter, hTempCounter2, hPwrCounter;
	PDH_FMT_COUNTERVALUE_ITEM* counterValues[2]{ new PDH_FMT_COUNTERVALUE_ITEM[1], new PDH_FMT_COUNTERVALUE_ITEM[1] };
	DWORD counterSizes[2]{ 1,1 };
	LightEventData cData;

	SysMonHelper();
	~SysMonHelper();
	int GetCounterValues(HCOUNTER counter, int index = 0);
	int GetValuesArray(HCOUNTER counter, byte& maxVal, int delta, int divider, HCOUNTER c2);
};