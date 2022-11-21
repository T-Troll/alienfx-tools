#include "SenMonHelper.h"
#include "common.h"

#pragma comment(lib, "pdh.lib")

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

DWORD WINAPI UpdateSensors(LPVOID);
DWORD WINAPI CBiosProc(LPVOID);

extern ConfigMon* conf;
extern AlienFan_SDK::Control* acpi;

SenMonHelper::SenMonHelper()
{
	// Set data source...
	PdhSetDefaultRealTimeDataSource(DATA_SOURCE_WBEM);
	ModifyMon();

}

SenMonHelper::~SenMonHelper()
{
	if (hQuery)
		PdhCloseQuery(hQuery);
	if (acpi)
		delete acpi;
	delete[] counterValues;
}

void SenMonHelper::ModifyMon()
{
	if ((conf->wSensors || conf->eSensors) && (hQuery || PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS)) {
		if (conf->wSensors) {
			PdhAddCounter(hQuery, COUNTER_PATH_CPU, 0, &hCPUCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_HDD, 0, &hHDDCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_GPU, 0, &hGPUCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_TEMP, 0, &hTempCounter);
		}
		else {
			PdhRemoveCounter(hCPUCounter);
			PdhRemoveCounter(hHDDCounter);
			PdhRemoveCounter(hGPUCounter);
			PdhRemoveCounter(hTempCounter);
		}
		if (conf->eSensors) {
			PdhAddCounter(hQuery, COUNTER_PATH_ESIF, 0, &hTempCounter2);
			PdhAddCounter(hQuery, COUNTER_PATH_PWR, 0, &hPwrCounter);
		}
		else {
			PdhRemoveCounter(hTempCounter2);
			PdhRemoveCounter(hPwrCounter);
		}
		DebugPrint("PDH query on.\n");
		// Bugfix for incorrect value order
		PdhCollectQueryData(hQuery);
	} else
		if (hQuery) {
			DebugPrint("PDH query off.\n");
			PdhCloseQuery(hQuery);
			hQuery = NULL;
		}
	if (conf->bSensors) {
		if (!acpi) {
			acpi = new AlienFan_SDK::Control();
			if (!(conf->bSensors = acpi->Probe())) {
				delete acpi;
				acpi = NULL;
			}
			DebugPrint("ACPI on.\n");
		}
	}
	else {
		if (acpi) {
			delete acpi;
			acpi = NULL;
			DebugPrint("ACPI off.\n");
		}
	}
	conf->needFullUpdate = true;
}

void AddUpdateSensor(ConfigMon* conf, SENID sid, long val, string name) {
	SENSOR* sen;
	if (val > 10000 || val <= NO_SEN_VALUE) return;
	if (sen = conf->FindSensor(sid.sid)) {
		if (sen->sname.empty()) {
			sen->sname = name;
			conf->needFullUpdate = true;
		}
		sen->cur = val;
		sen->min = sen->min == NO_SEN_VALUE ? val : min(sen->min, val);
		sen->max = max(sen->max, val);
		if (sen->alarm && sen->oldCur != NO_SEN_VALUE) {
			// Check alarm
			if ((sen->direction ? (sen->cur < (int)sen->ap) + (sen->oldCur >= (int)sen->ap) == 2 :
				(sen->cur > (int)sen->ap) + (sen->oldCur <= (int)sen->ap) == 2)) {
				// Set alarm
				ShowNotification(&conf->niData, "Alarm triggered!", "Sensor \"" + sen->name +
					"\" " + (sen->direction ? "lower " : "higher ") + to_string(sen->ap) + " (Current: " + to_string(sen->cur) + ")!", true);
			}
		}
	}
	else {
		// add sensor
		conf->active_sensors[sid.sid] = { name, val, val, val, NO_SEN_VALUE };
		conf->needFullUpdate = true;
	}
}

int SenMonHelper::GetValuesArray(HCOUNTER counter) {
	PDH_STATUS pdhStatus;
	DWORD count;
	while ((pdhStatus = PdhGetFormattedCounterArray(counter, PDH_FMT_LONG /*| PDH_FMT_NOSCALE*/, &counterSize, &count, counterValues)) == PDH_MORE_DATA) {
		delete[] counterValues;
		counterValues = new PDH_FMT_COUNTERVALUE_ITEM[(counterSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM)) + 1];
	}

	if (pdhStatus != ERROR_SUCCESS) {
		return 0;
	}
	return count;
}

void SenMonHelper::UpdateSensors()
{
	PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;

	if (conf->wSensors || conf->eSensors)
		// get indicators...
		PdhCollectQueryData(hQuery);

	if (conf->wSensors) { // group 0
		if (PdhGetFormattedCounterValue(hCPUCounter, PDH_FMT_LONG, &cType, &cCPUVal) == ERROR_SUCCESS) // CPU, code 0
			AddUpdateSensor(conf, { 0, 0, 0 }, cCPUVal.longValue, "CPU load");

		GlobalMemoryStatusEx(&memStat); // RAM, code 1
		AddUpdateSensor(conf, { 0, 1, 0 }, memStat.dwMemoryLoad, "Memory usage");

		if (PdhGetFormattedCounterValue(hHDDCounter, PDH_FMT_LONG, &cType, &cHDDVal) == ERROR_SUCCESS) // HDD, code 2
			AddUpdateSensor(conf, { 0, 2, 0 }, 100 - cHDDVal.longValue, "HDD load");

		GetSystemPowerStatus(&state); // Battery, code 3
		AddUpdateSensor(conf, { 0, 3, 0 }, state.BatteryLifePercent, "Battery");

		valCount = GetValuesArray(hGPUCounter); // GPU, code 4
		long valLast = 0;
		for (unsigned i = 0; i < valCount && counterValues[i].szName != NULL; i++) {
			if ((counterValues[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA))
				valLast = max(valLast, counterValues[i].FmtValue.longValue);
		}
		AddUpdateSensor(conf, { 0, 4, 0 }, valLast, "GPU load");

		valCount = GetValuesArray(hTempCounter); // Temps, code 5
		for (WORD i = 0; i < valCount; i++) {
			if (counterValues[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA)
				AddUpdateSensor(conf, { i, 5, 0 }, counterValues[i].FmtValue.longValue - 273, counterValues[i].szName);
		}
	}

	if (conf->eSensors) { // group 1
		// ESIF temperatures and power
		valCount = GetValuesArray(hPwrCounter); // Esif powers, code 1
		for (WORD i = 0; i < valCount; i++) {
			if (counterValues[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA)
				AddUpdateSensor(conf, { i, 1, 1 }, counterValues[i].FmtValue.longValue / 10, "Power " + to_string(i));
		}
	}

	if (acpi) { // group 2
		// Fan data and BIOS temperatures
		for (WORD i = 0; i < acpi->sensors.size(); i++) { // BIOS temps, code 0
			AddUpdateSensor(conf, { acpi->sensors[i].sid, 0, 2 }, acpi->GetTempValue(i), acpi->sensors[i].name);
		}

		for (WORD i = 0; i < acpi->fans.size(); i++) { // BIOS fans, code 1-3
			AddUpdateSensor(conf, { i, 1, 2 }, acpi->GetFanRPM(i), "Fan " + to_string(i + 1) + " RPM");
			AddUpdateSensor(conf, { i, 2, 2 }, acpi->GetFanPercent(i), "Fan " + to_string(i + 1) + " percent");
			AddUpdateSensor(conf, { i, 3, 2 }, acpi->GetFanBoost(i, true), "Fan " + to_string(i + 1) + " boost");
		}
	} else
		if (conf->eSensors) {
			// Added other tempset ...
			valCount = GetValuesArray(hTempCounter2); // Esif temps, code 0
			for (WORD i = 0; i < valCount; i++) {
				if ((counterValues[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) && counterValues[i].FmtValue.longValue)
					AddUpdateSensor(conf, { i, 0, 1 }, counterValues[i].FmtValue.longValue, "Temp " + to_string(i));
			}
		}
}