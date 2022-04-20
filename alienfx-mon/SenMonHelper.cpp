#include "SenMonHelper.h"
#include <Pdh.h>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)  
#endif

DWORD WINAPI CEventProc(LPVOID);
DWORD WINAPI CBiosProc(LPVOID);

SenMonHelper::SenMonHelper(ConfigMon* cf)
{
	conf = cf;
	stopEvents = CreateEvent(NULL, true, false, NULL);

	if (conf->bSensors) {
		acpi = new AlienFan_SDK::Control();
		validB = acpi->IsActivated() && acpi->Probe();
	}
	//if (conf->wSensors || conf->eSensors) {
	StartMon();
	//}
}

SenMonHelper::~SenMonHelper()
{
	StopMon();
	CloseHandle(stopEvents);
	if (acpi)
		delete acpi;
}

void SenMonHelper::StartMon()
{
	if (!eHandle) {
		ResetEvent(stopEvents);
		eHandle = CreateThread(NULL, 0, CEventProc, this, 0, NULL);
	}
}

void SenMonHelper::StopMon()
{
	if (eHandle) {
		SetEvent(stopEvents);
		WaitForSingleObject(eHandle, 1000);
		CloseHandle(eHandle);
		eHandle = 0;
	}
}

void SenMonHelper::ModifyMon()
{
	if (conf->bSensors) {
		acpi = new AlienFan_SDK::Control();
		validB = acpi->IsActivated() && acpi->Probe();
	}
	else {
		delete acpi;
		acpi = NULL;
		validB = false;
	}
}

void AddUpdateSensor(ConfigMon* conf, int grp, byte type, DWORD id, long val, string name) {
	SENSOR* sen;
	if (sen = conf->FindSensor(grp, type, id)) {
		sen->cur = val;
		if (sen->cur < sen->min)
			sen->min = sen->cur;
		if (sen->cur > sen->max)
			sen->max = sen->cur;
	}
	else {
		// add sensor
		sen = new SENSOR({ grp, type, id, name, val, val, val });
		conf->active_sensors.push_back(*sen);
		conf->needFullUpdate = true;
	}
}

PDH_FMT_COUNTERVALUE_ITEM* counterValues = new PDH_FMT_COUNTERVALUE_ITEM[1];
DWORD counterSize = sizeof(PDH_FMT_COUNTERVALUE_ITEM);

int GetValuesArray(HCOUNTER counter) {
	PDH_STATUS pdhStatus;
	DWORD count;
	while ((pdhStatus = PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &counterSize, &count, counterValues)) == PDH_MORE_DATA) {
		delete[] counterValues;
		counterValues = new PDH_FMT_COUNTERVALUE_ITEM[counterSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1];
	}

	if (pdhStatus != ERROR_SUCCESS) {
		return 0;
	}
	return count;
}

DWORD WINAPI CEventProc(LPVOID param)
{
	SenMonHelper* src = (SenMonHelper*)param;

	// locales block
	//HKL* locIDs = new HKL[10];
	//int locNum = GetKeyboardLayoutList(10, locIDs);

	LPCTSTR COUNTER_PATH_CPU = "\\Processor Information(_Total)\\% Processor Time",
		COUNTER_PATH_NET = "\\Network Interface(*)\\Bytes Total/sec",
		COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage",
		COUNTER_PATH_HOT = "\\Thermal Zone Information(*)\\Temperature",
		COUNTER_PATH_HOT2 = "\\EsifDeviceInformation(*)\\Temperature",
		COUNTER_PATH_PWR = "\\EsifDeviceInformation(*)\\RAPL Power",
		COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Idle Time";

	HQUERY hQuery = NULL;
	HLOG hLog = NULL;
	//PDH_STATUS pdhStatus;
	DWORD dwLogType = PDH_LOG_TYPE_CSV;
	HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hGPUCounter, hTempCounter, hTempCounter2, hPwrCounter;

	MEMORYSTATUSEX memStat;
	memStat.dwLength = sizeof(MEMORYSTATUSEX);

	SYSTEM_POWER_STATUS state;

	//ULONGLONG maxnet = 1;
	//DWORD maxPower = 100;

	DWORD cType = 0, valCount = 0;

	// Set data source...
	//PdhSetDefaultRealTimeDataSource(DATA_SOURCE_WBEM);

	if (PdhOpenQuery(NULL, 0, &hQuery) != ERROR_SUCCESS)
	{
		goto cleanup;
	}

	PdhAddCounter(hQuery, COUNTER_PATH_CPU, 0, &hCPUCounter);
	PdhAddCounter(hQuery, COUNTER_PATH_HDD, 0, &hHDDCounter);
	PdhAddCounter(hQuery, COUNTER_PATH_NET, 0, &hNETCounter);
	PdhAddCounter(hQuery, COUNTER_PATH_GPU, 0, &hGPUCounter);
	PdhAddCounter(hQuery, COUNTER_PATH_HOT, 0, &hTempCounter);
	PdhAddCounter(hQuery, COUNTER_PATH_HOT2, 0, &hTempCounter2);
	PdhAddCounter(hQuery, COUNTER_PATH_PWR, 0, &hPwrCounter);

	PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;

	while (WaitForSingleObject(src->stopEvents, src->conf->refreshDelay) == WAIT_TIMEOUT) {
		// get indicators...
		PdhCollectQueryData(hQuery);

		if (src->conf->wSensors) { // group 0
			PdhGetFormattedCounterValue(hCPUCounter, PDH_FMT_LONG, &cType, &cCPUVal); // CPU, code 0
			AddUpdateSensor(src->conf, 0, 0, 0, cCPUVal.longValue, "CPU load");

			GlobalMemoryStatusEx(&memStat); // RAM, code 1
			AddUpdateSensor(src->conf, 0, 1, 0, memStat.dwMemoryLoad, "Memory usage");

			PdhGetFormattedCounterValue(hHDDCounter, PDH_FMT_LONG, &cType, &cHDDVal); // HDD, code 2
			AddUpdateSensor(src->conf, 0, 2, 0, 100 - cHDDVal.longValue, "HDD load");

			GetSystemPowerStatus(&state); // Battery, code 3
			AddUpdateSensor(src->conf, 0, 3, 0, state.BatteryLifePercent, "Battery");

			valCount = GetValuesArray(hGPUCounter); // GPU, code 4
			long valLast = 0;
			for (unsigned i = 0; i < valCount && counterValues[i].szName != NULL; i++) {
				if (valLast < counterValues[i].FmtValue.longValue)
					valLast = (byte)counterValues[i].FmtValue.longValue;
			}
			AddUpdateSensor(src->conf, 0, 4, 0, valLast, "GPU load");

			valCount = GetValuesArray(hTempCounter); // Temps, code 5
			for (unsigned i = 0; i < valCount; i++) {
				AddUpdateSensor(src->conf, 0, 5, i, counterValues[i].FmtValue.longValue - 273, (string)counterValues[i].szName);
			}
		}

		if (src->conf->eSensors) { // group 1
			// ESIF temperatures and power
			valCount = GetValuesArray(hTempCounter2); // Esif temps, code 0
			// Added other set maximum temp...
			for (unsigned i = 0; i < valCount; i++) {
				AddUpdateSensor(src->conf, 1, 0, i, counterValues[i].FmtValue.longValue, (string)"Temp " + counterValues[i].szName);
			}
			valCount = GetValuesArray(hPwrCounter); // Esif powers, code 1
			for (unsigned i = 0; i < valCount; i++) {
				AddUpdateSensor(src->conf, 1, 1, i, counterValues[i].FmtValue.longValue, (string)"Power " + counterValues[i].szName);
			}
		}

		if (src->conf->bSensors && src->validB) { // group 2
			// Fan data and BIOS temperatures
			for (int i = 0; i < src->acpi->HowManySensors(); i++) { // BIOS temps, code 0
				AddUpdateSensor(src->conf, 2, 0, i, src->acpi->GetTempValue(i), src->acpi->sensors[i].name);
			}

			for (int i = 0; i < src->acpi->HowManyFans(); i++) { // BIOS fans, code 1-2
				int fpst = src->acpi->GetFanPercent(i);
				AddUpdateSensor(src->conf, 2, 1, i, src->acpi->GetFanRPM(i), (string)"Fan " + to_string(i) + " RPM");
				AddUpdateSensor(src->conf, 2, 2, i, fpst > 100 ? 100 : fpst, (string)"Fan " + to_string(i) + " percent");
			}
		}
	}

cleanup:

	if (hQuery)
		PdhCloseQuery(hQuery);

	//delete[] locIDs;

	return 0;
}