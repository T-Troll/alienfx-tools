#pragma once
//#include <pdh.h>
#include "ThreadHelper.h"
#include "FXHelper.h"

class SysMonHelper {
private:
	//const LPCTSTR COUNTER_PATH_CPU = "\\Processor Information(_Total)\\% Processor Time", // Win32_PerfFormattedData_PerfOS_Processor name: _Total, column: PercentProcessorTime
	//	COUNTER_PATH_NET = "\\Network Interface(*)\\Bytes Total/sec", // Win32_PerfFormattedData_Tcpip_NetworkAdapter BytesTotalPerSec
	//	COUNTER_PATH_NETMAX = "\\Network Interface(*)\\Current BandWidth", // Win32_PerfFormattedData_Tcpip_NetworkAdapter
	//	COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage", // Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine
	//	COUNTER_PATH_HOT = "\\Thermal Zone Information(*)\\Temperature", // Deprecated?
	//	COUNTER_PATH_HOT2 = "\\EsifDeviceInformation(*)\\Temperature", // EsifDeviceInformation Temperature
	//	COUNTER_PATH_PWR = "\\EsifDeviceInformation(*)\\RAPL Power", // EsifDeviceInformation Power
	//	COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Idle Time"; // Win32_PerfFormattedData_PerfDisk_PhysicalDisk name: _Total PercentDiskTime
	ThreadHelper* eventProc = NULL;
	IWbemServices* m_WmiService = NULL, * m_CimService = NULL;
	IWbemHiPerfEnum* netInsts, * esifInsts;
public:

//	HQUERY hQuery;
//	HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hNETMAXCounter, hGPUCounter, hTempCounter, hTempCounter2, hPwrCounter;
//	unsigned char* counterValues{ new unsigned char[1] };
	LightEventData sData;
	IWbemRefresher* refresher;
	IWbemHiPerfEnum* gpuInsts;
	vector<IWbemObjectAccess*> esifSensors, netInst;
	IWbemClassObject* cpuPath, * diskPath;

	SysMonHelper();
	~SysMonHelper();
//	int GetCounterValues(HCOUNTER counter);
//	int GetValuesArray(HCOUNTER counter, int delta, int divider);
};