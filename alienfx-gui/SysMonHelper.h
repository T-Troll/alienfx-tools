#pragma once
#include <pdh.h>
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

	HQUERY hQuery;
	HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hNETMAXCounter, hGPUCounter, hTempCounter, hTempCounter2, hPwrCounter;
	byte* counterValues{ new byte[1] };

	SysMonHelper();
	~SysMonHelper();
	int GetCounterValues(HCOUNTER counter);
	int GetValuesArray(HCOUNTER counter, int delta, int divider);
};