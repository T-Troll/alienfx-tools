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
extern HWND mDlg;

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
	if ((conf->bSensors || conf->eSensors) && !EvaluteToAdmin(mDlg)) {
		conf->bSensors = conf->eSensors = false;
	}
	if ((conf->wSensors || conf->eSensors) && (hQuery || PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS)) {
		if (conf->wSensors) {
			PdhAddCounter(hQuery, COUNTER_PATH_CPU, 0, &hCPUCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_HDD, 0, &hHDDCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_GPU, 0, &hGPUCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_BATT_CHARGE, 0, &hBCCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_BATT_DISCHARGE, 0, &hBDCounter);
			PdhAddCounter(hQuery, COUNTER_PATH_TEMP, 0, &hTempCounter);
		}
		else {
			PdhRemoveCounter(hCPUCounter);
			PdhRemoveCounter(hHDDCounter);
			PdhRemoveCounter(hGPUCounter);
			PdhRemoveCounter(hBCCounter);
			PdhRemoveCounter(hBDCounter);
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
			if (!(conf->bSensors = acpi->Probe(true))) {
				delete acpi;
				acpi = NULL;
			}
		}
	}
	conf->needFullUpdate = true;
}

SENSOR* SenMonHelper::UpdateSensor(SENID sid, long val) {
	if (val > 10000 || val <= NO_SEN_VALUE) return NULL;

	SENSOR* sen = &conf->active_sensors[sid.sid];
	sen->min = sen->min <= NO_SEN_VALUE ? val : min(sen->min, val);
	if (sen->changed = (sen->min > NO_SEN_VALUE && sen->cur != val)) {
		sen->max = max(sen->max, val);
		if (sen->alarm || sen->highlight) {
			bool alarming = sen->direction ? val < sen->ap : val >= sen->ap;
			if (sen->alarm && !sen->alarming && alarming && sen->sname.size())
				ShowNotification(&conf->niData, "Alarm triggered!", "Sensor \"" + sen->sname +
					"\" " + (sen->direction ? "lower " : "higher ") + to_string(sen->ap) + " (Current: " + to_string(val) + ")!");
			sen->alarming = alarming;
		}
		sen->cur = val;
	}
	return sen->sname.empty() ? sen : NULL;
}

int SenMonHelper::GetValuesArray(HCOUNTER counter) {
	PDH_STATUS pdhStatus;
	DWORD count;
	DWORD cs = counterSize;
	while ((pdhStatus = PdhGetFormattedCounterArray(counter, PDH_FMT_LONG /*| PDH_FMT_NOSCALE*/, &cs, &count, (PDH_FMT_COUNTERVALUE_ITEM*)counterValues)) == PDH_MORE_DATA) {
		delete[] counterValues;
		counterSize = cs;
		counterValues = new byte[counterSize];
		cv = (PDH_FMT_COUNTERVALUE_ITEM*)counterValues;
	}

	if (pdhStatus != ERROR_SUCCESS) {
		return 0;
	}
	return count;
}

string SenMonHelper::GetFanName(int index) {
	string fname;
	switch (acpi->fans[index].type)
	{
	case 1: fname = "CPU"; break;
	case 6: fname = "GPU"; break;
	//default: fname = "";
	}
	return fname + " Fan " + to_string(index + 1);
}

void SenMonHelper::UpdateSensors()
{
	PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
	SENSOR* sen;

	if (conf->wSensors || conf->eSensors)
		// get indicators...
		PdhCollectQueryData(hQuery);

	if (conf->wSensors) { // group 0
		if (PdhGetFormattedCounterValue(hCPUCounter, PDH_FMT_LONG, &cType, &cCPUVal) == ERROR_SUCCESS && // CPU, code 0
			(sen = UpdateSensor({ 0, 0, 0 }, cCPUVal.longValue)))
			sen->sname = "CPU load";

		if (GlobalMemoryStatusEx(&memStat) && // RAM, code 1
		   (sen = UpdateSensor({ 0, 1, 0 }, memStat.dwMemoryLoad)))
			sen->sname = "Memory usage";

		if (PdhGetFormattedCounterValue(hHDDCounter, PDH_FMT_LONG, &cType, &cHDDVal) == ERROR_SUCCESS && // HDD, code 2
			(sen = UpdateSensor({ 0, 2, 0 }, 99 - cHDDVal.longValue)))
			sen->sname = "HDD load";

		if (GetSystemPowerStatus(&state) && // Battery, code 3
			(sen = UpdateSensor({ 0, 3, 0 }, state.BatteryLifePercent)))
			sen->sname = "Battery";

		// New: battery charge/discharge rate
		valCount = GetValuesArray(hBDCounter); // Temps, code 5
		for (WORD i = 0; i < valCount; i++) {
			if (cv[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA &&
				(sen = UpdateSensor({ (WORD)(i + 1), 3, 0 }, cv[i].FmtValue.longValue / 1000)))
				sen->sname = "Battery " + to_string(i+1) + " Discharge rate";
		}
		valCount = GetValuesArray(hBCCounter); // Temps, code 5
		for (WORD i = 0; i < valCount; i++) {
			if (cv[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA &&
				(sen = UpdateSensor({ (WORD)(i + 0x101), 3, 0 }, cv[i].FmtValue.longValue / 1000)))
				sen->sname = "Battery " + to_string(i + 1) + " Charge rate";
		}

		valCount = GetValuesArray(hGPUCounter); // GPU, code 4
		for (DWORD i = 0; i < valCount; i++) {
			if (cv[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) {
				string path = cv[i].szName;
				string phys = path.substr(path.find("luid_") + 18, 8);
				int physID = strtol(path.substr(path.find("luid_") + 18, 8).c_str(), NULL, 16);
				string type = path.substr(path.find("type_") + 5);
				gpusubs[type][physID] += cv[i].FmtValue.longValue;
			}
		}
		WORD sstype = 0;
		for (auto it = gpusubs.begin(); it != gpusubs.end(); it++) {
			// per-value
			WORD adnum = 0;
			for (auto sub = it->second.begin(); sub != it->second.end(); sub++) {
				if (sen = UpdateSensor({ (WORD)((adnum << 4) + sstype), 4, 0 }, min(sub->second, 100)))
					sen->sname = "GPU " + to_string(adnum) + " " + it->first;
				sub->second = 0;
				adnum++;
			}
			sstype++;
		}

		valCount = GetValuesArray(hTempCounter); // Temps, code 5
		for (WORD i = 0; i < valCount; i++) {
			if (cv[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA &&
				(sen = UpdateSensor({ i, 5, 0 }, cv[i].FmtValue.longValue - 273)))
				sen->sname = cv[i].szName;
		}
	}

	if (conf->bSensors) { // group 2
		// BIOS/WMI temperatures
		for (WORD i = 0; i < acpi->sensors.size(); i++) { // BIOS temps, code 0
			if (sen = UpdateSensor({ acpi->sensors[i].sid, 0, 2 }, acpi->GetTempValue(i)))
				sen->sname = conf->fan_conf.GetSensorName(&acpi->sensors[i]);
		}
		// Fan data
		for (WORD i = 0; i < acpi->fans.size(); i++) { // BIOS fans, code 1-3
			if (sen = UpdateSensor({ i, 1, 2 }, acpi->GetFanRPM(i)))
				sen->sname = GetFanName(i) + " RPM";
			if (sen = UpdateSensor({ i, 2, 2 }, acpi->GetFanPercent(i)))
				sen->sname = GetFanName(i) + " percent";
			if (sen = UpdateSensor({ i, 3, 2 }, acpi->GetFanBoost(i)))
				sen->sname = GetFanName(i) + " boost";
		}
	} else
		if (conf->eSensors) {
			// ESIF temperatures...
			valCount = GetValuesArray(hTempCounter2); // Esif temps, code 0
			for (WORD i = 0; i < valCount; i++) {
				if ((cv[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) && cv[i].FmtValue.longValue &&
					(sen = UpdateSensor({ i, 0, 1 }, cv[i].FmtValue.longValue)))
					sen->sname = conf->fan_conf.sensors[i].empty() ? "ESIF " + to_string(i + 1) : conf->fan_conf.sensors[i];
			}
		}

	if (conf->eSensors) { // group 1
		// ESIF temperatures and power
		valCount = GetValuesArray(hPwrCounter); // Esif powers, code 1
		for (WORD i = 0; i < valCount; i++) {
			if (cv[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA &&
				(sen = UpdateSensor({ i, 1, 1 }, cv[i].FmtValue.longValue / 10)))
				sen->sname = "ESIF Power " + to_string(i + 1);
		}
	}
}