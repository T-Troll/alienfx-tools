#include "SysMonHelper.h"
#include "MonHelper.h"
#include "FXHelper.h"
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

extern FXHelper* fxhl;
extern MonHelper* mon;
extern ConfigHandler* conf;

LightEventData sData;

static SYSTEM_POWER_STATUS state;
static PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
static MEMORYSTATUSEX memStat{ sizeof(MEMORYSTATUSEX) };
static HKL locIDs[10];
static HKL curLocale;

void CEventProc(LPVOID);

SysMonHelper::SysMonHelper() {
	DebugPrint("Starting Event thread\n");
	//PdhSetDefaultRealTimeDataSource(DATA_SOURCE_WBEM);
	if (PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS) {
		// Set data source...
		PdhAddEnglishCounter(hQuery, COUNTER_PATH_CPU, 0, &hCPUCounter);
		PdhAddEnglishCounter(hQuery, COUNTER_PATH_HDD, 0, &hHDDCounter);
		PdhAddEnglishCounter(hQuery, COUNTER_PATH_NET, 0, &hNETCounter);
		PdhAddEnglishCounter(hQuery, COUNTER_PATH_NETMAX, 0, &hNETMAXCounter);
		PdhAddEnglishCounter(hQuery, COUNTER_PATH_GPU, 0, &hGPUCounter);
		PdhAddEnglishCounter(hQuery, COUNTER_PATH_HOT, 0, &hTempCounter);

		PdhAddEnglishCounter(hQuery, COUNTER_PATH_HOT2, 0, &hTempCounter2);
		PdhAddEnglishCounter(hQuery, COUNTER_PATH_PWR, 0, &hPwrCounter);

		PdhCollectQueryData(hQuery);
		// start thread...
		eventProc = new ThreadHelper(CEventProc, this, 300);
		DebugPrint("Event thread start.\n");
	}
}

SysMonHelper::~SysMonHelper() {
	if (eventProc) {
		delete eventProc;
		delete[] counterValues;
		ZeroMemory(&fxhl->eData, sizeof(LightEventData));
		DebugPrint("Event thread stop.\n");
		PdhCloseQuery(hQuery);
	}
}

int SysMonHelper::GetCounterValues(HCOUNTER counter) {
	DWORD cs = 0;
	DWORD count = 0;

	if (PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &cs, &count, NULL) == PDH_MORE_DATA) {
		delete[] counterValues;
		counterValues = new byte[cs];
		PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &cs, &count, (PDH_FMT_COUNTERVALUE_ITEM*)counterValues);
	}
	return count;
}

int SysMonHelper::GetValuesArray(HCOUNTER counter, int delta = 0, int divider = 1) {
	int retVal = 0;
	DWORD count = GetCounterValues(counter);

	for (DWORD i = 0; i < count; i++) {
		retVal = max(retVal, ((PDH_FMT_COUNTERVALUE_ITEM*)counterValues)[i].FmtValue.longValue / divider - delta);
	}

	//maxVal = max(maxVal, retVal);

	return retVal;
}

void CEventProc(LPVOID param)
{
	DebugPrint("Event values update started\n");
	SysMonHelper* src = (SysMonHelper*)param;
	//LightEventData* maxData = &fxhl->maxData;
#define maxData fxhl->maxData

	PdhCollectQueryData(src->hQuery);
	ZeroMemory(&sData, sizeof(LightEventData));

	// CPU load
	PdhGetFormattedCounterValue(src->hCPUCounter, PDH_FMT_LONG, NULL, &cCPUVal);
	// HDD load
	PdhGetFormattedCounterValue(src->hHDDCounter, PDH_FMT_LONG, NULL, &cHDDVal);
	// Network load
	int maxBand = src->GetValuesArray(src->hNETMAXCounter, 0, 2048),
		curBand = src->GetValuesArray(src->hNETCounter);
	if (maxBand) {
		sData.NET = min(100, curBand / maxBand);
		maxData.NET = max(sData.NET, maxData.NET);
	}
	// GPU load
	DWORD count = src->GetCounterValues(src->hGPUCounter);
	// now sort...
	map<string, map<char, int>> gpusubs;
	PDH_FMT_COUNTERVALUE_ITEM* va = (PDH_FMT_COUNTERVALUE_ITEM*)src->counterValues;
	for (DWORD i = 0; i < count; i++) {
		if (va[i].FmtValue.longValue && va[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) {
			string path = va[i].szName;
			char physID = path[path.find("phys_") + 5];
			string type = path.substr(path.find("type_"));
			gpusubs[type][physID] += va[i].FmtValue.longValue;
		}
	}
	for (auto it = gpusubs.begin(); it != gpusubs.end(); it++) {
		// per-adapter
		for (auto sub = it->second.begin(); sub != it->second.end(); sub++) {
			sData.GPU = min(max(sData.GPU, sub->second), 100);
			//DebugPrint("Adapter " + to_string(sub->first) + ", system " + it->first + ": " + to_string(sub->second) + "\n");
			//sub->second = 0;
		}
	}
	// Temperatures
	sData.Temp = src->GetValuesArray(src->hTempCounter, 273);
	// RAM load
	GlobalMemoryStatusEx(&memStat);
	// Power state
	GetSystemPowerStatus(&state);
	// Locale
	if (curLocale = GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), NULL))) {
		GetKeyboardLayoutList(10, locIDs);
		sData.KBD = curLocale == locIDs[0] ? 0 : 100;
	}

	if (mon) {
		// Check fan RPMs
		for (unsigned i = 0; i < mon->fansize; i++) {
			sData.Fan = max(sData.Fan, mon->GetFanPercent(i));
		}
		sData.Fan = min(100, sData.Fan);
		// Sensors
		for (auto i = mon->senValues.begin(); i != mon->senValues.end(); i++)
			sData.Temp = max(sData.Temp, i->second);
		// Power mode
		sData.PWM = mon->powerMode * 100 /	(mon->powerSize + mon->acpi->isGmode - 1);
	}

	// ESIF powers and temps
	if (conf->esif_temp) {
		if (!mon) {
			// ESIF temps (already in fans)
			sData.Temp = max(sData.Temp, src->GetValuesArray(src->hTempCounter2, maxData.Temp));
		}
		// Powers
		sData.PWR = src->GetValuesArray(src->hPwrCounter, 0, 10);
		maxData.PWR = max(sData.PWR, maxData.PWR);
		sData.PWR = sData.PWR * 100 / maxData.PWR;
	}

	// Leveling...
	sData.Temp = min(100, max(0, sData.Temp));
	sData.Batt = state.BatteryLifePercent;
	sData.ACP = state.ACLineStatus;
	sData.BST = state.BatteryFlag;
	sData.HDD = (byte)max(0, 99 - cHDDVal.longValue);
	maxData.RAM = max(maxData.RAM, sData.RAM = (byte)memStat.dwMemoryLoad);
	maxData.CPU = max(maxData.CPU, sData.CPU = (byte)cCPUVal.longValue);
	maxData.GPU = max(maxData.GPU, sData.GPU);
	maxData.Temp = max(sData.Temp, maxData.Temp);

	fxhl->RefreshCounters(&sData);
	memcpy(&fxhl->eData, &sData, sizeof(LightEventData));
	DebugPrint("Event values update finished\n");
}