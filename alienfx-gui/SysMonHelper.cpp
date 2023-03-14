#include "SysMonHelper.h"
#include "MonHelper.h"
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

extern FXHelper* fxhl;
extern MonHelper* mon;
extern ConfigFan* fan_conf;
extern ConfigHandler* conf;

void CEventProc(LPVOID);

SysMonHelper::SysMonHelper() {
	if (PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS) {
		// Set data source...
		PdhAddCounter(hQuery, COUNTER_PATH_CPU, 0, &hCPUCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_HDD, 0, &hHDDCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_NET, 0, &hNETCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_NETMAX, 0, &hNETMAXCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_GPU, 0, &hGPUCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_HOT, 0, &hTempCounter);
		// ToDo: start only if esif enabled
		PdhAddCounter(hQuery, COUNTER_PATH_HOT2, 0, &hTempCounter2);
		PdhAddCounter(hQuery, COUNTER_PATH_PWR, 0, &hPwrCounter);
		// start thread...
		eventProc = new ThreadHelper(CEventProc, this, 300);
		DebugPrint("Event thread start.\n");
	}
}

SysMonHelper::~SysMonHelper() {
	delete eventProc;
	memset(&fxhl->eData, 0, sizeof(LightEventData));
	DebugPrint("Event thread stop.\n");
	PdhCloseQuery(hQuery);
}

int SysMonHelper::GetCounterValues(HCOUNTER counter, int index) {
	DWORD cs = counterSizes[index];
	DWORD count;
	while (PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &cs, &count, (PDH_FMT_COUNTERVALUE_ITEM*)counterValues[index]) == PDH_MORE_DATA) {
		delete[] counterValues[index];
		counterSizes[index] = cs;
		counterValues[index] = new byte[cs];
	}
	return count;
}

int SysMonHelper::GetValuesArray(HCOUNTER counter, byte& maxVal, int delta = 0, int divider = 1, HCOUNTER c2 = NULL) {
	int retVal = 0;

	if (c2) {
		GetCounterValues(c2, 1);
	}

	DWORD count = GetCounterValues(counter);

	for (DWORD i = 0; i < count; i++) {
		int cval = c2 && ((PDH_FMT_COUNTERVALUE_ITEM*)counterValues[1])[i].FmtValue.longValue ?
			((PDH_FMT_COUNTERVALUE_ITEM*)counterValues[0])[i].FmtValue.longValue * 800 / 
				((PDH_FMT_COUNTERVALUE_ITEM*)counterValues[1])[i].FmtValue.longValue :
				((PDH_FMT_COUNTERVALUE_ITEM*)counterValues[0])[i].FmtValue.longValue / divider - delta;
		retVal = max(retVal, cval);
	}

	maxVal = max(maxVal, retVal);

	return retVal;
}

void CEventProc(LPVOID param)
{
	static HKL locIDs[10];
	static map<string, map<string, int>> gpusubs;

	if (fxhl->lightsNoDelay) {
		SysMonHelper* src = (SysMonHelper*)param;
		LightEventData* cData = &src->cData;

		SYSTEM_POWER_STATUS state;
		PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
		MEMORYSTATUSEX memStat{ sizeof(MEMORYSTATUSEX) };
		HKL curLocale;

		PdhCollectQueryData(src->hQuery);
		memset(cData,0,sizeof(LightEventData));

		// CPU load
		PdhGetFormattedCounterValue(src->hCPUCounter, PDH_FMT_LONG, NULL, &cCPUVal);
		// HDD load
		PdhGetFormattedCounterValue(src->hHDDCounter, PDH_FMT_LONG, NULL, &cHDDVal);
		// Network load
		cData->NET = src->GetValuesArray(src->hNETCounter, fxhl->maxData.NET, 0, 1, src->hNETMAXCounter);
		// GPU load
		DWORD count = src->GetCounterValues(src->hGPUCounter);
		// now sort...
		for (DWORD i = 0; i < count; i++) {
			if (((PDH_FMT_COUNTERVALUE_ITEM*)src->counterValues[0])[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) {
				string path = ((PDH_FMT_COUNTERVALUE_ITEM*)src->counterValues[0])[i].szName;
				string physID = path.substr(path.find("phys_") + 5, 1);
				string type = path.substr(path.find("type_") + 5);
				gpusubs[type][physID] += ((PDH_FMT_COUNTERVALUE_ITEM*)src->counterValues[0])[i].FmtValue.longValue;
			}
		}
		for (auto it = gpusubs.begin(); it != gpusubs.end(); it++) {
			// per-adapter
			for (auto sub = it->second.begin(); sub != it->second.end(); sub++) {
				cData->GPU = min(max(cData->GPU, sub->second), 100);
				//DebugPrint("Adapter " + to_string(sub->first) + ", system " + it->first + ": " + to_string(sub->second) + "\n");
				sub->second = 0;
			}
		}
		// Temperatures
		cData->Temp = src->GetValuesArray(src->hTempCounter, fxhl->maxData.Temp, 273);
		// RAM load
		GlobalMemoryStatusEx(&memStat);
		// Power state
		GetSystemPowerStatus(&state);
		// Locale
		if (curLocale = GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), NULL))) {
			GetKeyboardLayoutList(10, locIDs);
			cData->KBD = curLocale == locIDs[0] ? 0 : 100;
		}

		if (mon) {
			// Check fan RPMs
			for (unsigned i = 0; i < mon->fanRpm.size(); i++) {
				cData->Fan = max(cData->Fan, mon->GetFanPercent(i));
			}
			cData->Fan = min(100, cData->Fan);
			// Sensors
			for (auto i = mon->senValues.begin(); i != mon->senValues.end(); i++)
				cData->Temp = max(cData->Temp, i->second);
			// Power mode
			cData->PWM = mon->powerMode * 100 /	(mon->powerSize + mon->acpi->isGmode - 1);
		}

		// ESIF powers and temps
		if (conf->esif_temp) {
			if (!mon) {
				// ESIF temps (already in fans)
				cData->Temp = max(cData->Temp, src->GetValuesArray(src->hTempCounter2, fxhl->maxData.Temp));
			}
			// Powers
			cData->PWR = src->GetValuesArray(src->hPwrCounter, fxhl->maxData.PWR, 0, 10) * 100 / fxhl->maxData.PWR;
		}

		// Leveling...
		cData->Temp = min(100, max(0, cData->Temp));
		cData->Batt = state.BatteryLifePercent;
		cData->ACP = state.ACLineStatus;
		cData->BST = state.BatteryFlag;
		cData->HDD = (byte)max(0, 99 - cHDDVal.longValue);
		fxhl->maxData.RAM = max(fxhl->maxData.RAM, cData->RAM = (byte)memStat.dwMemoryLoad);
		fxhl->maxData.CPU = max(fxhl->maxData.CPU, cData->CPU = (byte)cCPUVal.longValue);
		fxhl->maxData.GPU = max(fxhl->maxData.GPU, cData->GPU);

		if (fxhl->lightsNoDelay) { // update lights
			fxhl->RefreshCounters(cData);
			memcpy(&fxhl->eData, cData, sizeof(LightEventData));
		}
	}
}