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
		COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Idle Time",
		COUNTER_PATH_BATT_CHARGE = "\\BatteryStatus(*)\\ChargeRate",
		COUNTER_PATH_BATT_DISCHARGE = "\\BatteryStatus(*)\\DischargeRate"
		;

	byte* counterValues = new byte[1];
	PDH_FMT_COUNTERVALUE_ITEM* cv=(PDH_FMT_COUNTERVALUE_ITEM*)counterValues;
	DWORD counterSize = 1;

	HQUERY hQuery = NULL;
	HCOUNTER hCPUCounter, hHDDCounter, /*hNETCounter,*/ hGPUCounter, hTempCounter, hTempCounter2, hPwrCounter, hBCCounter, hBDCounter;

	MEMORYSTATUSEX memStat{ sizeof(MEMORYSTATUSEX) };

	SYSTEM_POWER_STATUS state;

	DWORD cType = 0, valCount = 0;

	map<string, map<byte, DWORD>> gpusubs;

	int GetValuesArray(HCOUNTER counter);
	string GetFanName(int index);
	SENSOR* UpdateSensor(SENID sid, long val);
public:
	HANDLE stopEvents = NULL;
	SenMonHelper();
	~SenMonHelper();
	void ModifyMon();
	void UpdateSensors();
};