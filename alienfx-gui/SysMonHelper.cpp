#include "SysMonHelper.h"
#include "MonHelper.h"
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
	delete eventProc;
	ZeroMemory(&fxhl->eData, sizeof(LightEventData));
	DebugPrint("Event thread stop.\n");
	PdhCloseQuery(hQuery);
}

int SysMonHelper::GetCounterValues(HCOUNTER counter, int index) {
	DWORD cs = 0;// counterSizes[index];
	DWORD count = 0;
	if (PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &cs, &count, NULL) == PDH_MORE_DATA) {
		delete[] counterValues[index];
		//counterSizes[index] = cs;
		counterValues[index] = new byte[cs];
		PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &cs, &count, (PDH_FMT_COUNTERVALUE_ITEM*)counterValues[index]);
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

static LightEventData sData;

void CEventProc(LPVOID param)
{
	DebugPrint("Event values update started\n");
	SysMonHelper* src = (SysMonHelper*)param;

	PdhCollectQueryData(src->hQuery);
	ZeroMemory(&sData, sizeof(LightEventData));

	// CPU load
	PdhGetFormattedCounterValue(src->hCPUCounter, PDH_FMT_LONG, NULL, &cCPUVal);
	// HDD load
	PdhGetFormattedCounterValue(src->hHDDCounter, PDH_FMT_LONG, NULL, &cHDDVal);
	// Network load
	sData.NET = src->GetValuesArray(src->hNETCounter, fxhl->maxData.NET, 0, 1, src->hNETMAXCounter);
	// GPU load
	DWORD count = src->GetCounterValues(src->hGPUCounter);
	// now sort...
	map<string, map<char, int>> gpusubs;
	PDH_FMT_COUNTERVALUE_ITEM* va = (PDH_FMT_COUNTERVALUE_ITEM*)src->counterValues[0];
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
	sData.Temp = src->GetValuesArray(src->hTempCounter, fxhl->maxData.Temp, 273);
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
		fxhl->maxData.Temp = max(sData.Temp, fxhl->maxData.Temp);
		// Power mode
		sData.PWM = mon->powerMode * 100 /	(mon->powerSize + mon->acpi->isGmode - 1);
	}

	// ESIF powers and temps
	if (conf->esif_temp) {
		if (!mon) {
			// ESIF temps (already in fans)
			sData.Temp = max(sData.Temp, src->GetValuesArray(src->hTempCounter2, fxhl->maxData.Temp));
		}
		// Powers
		sData.PWR = src->GetValuesArray(src->hPwrCounter, fxhl->maxData.PWR, 0, 10) * 100 / fxhl->maxData.PWR;
	}

	// Leveling...
	sData.Temp = min(100, max(0, sData.Temp));
	sData.Batt = state.BatteryLifePercent;
	sData.ACP = state.ACLineStatus;
	sData.BST = state.BatteryFlag;
	sData.HDD = (byte)max(0, 99 - cHDDVal.longValue);
	fxhl->maxData.RAM = max(fxhl->maxData.RAM, sData.RAM = (byte)memStat.dwMemoryLoad);
	fxhl->maxData.CPU = max(fxhl->maxData.CPU, sData.CPU = (byte)cCPUVal.longValue);
	fxhl->maxData.GPU = max(fxhl->maxData.GPU, sData.GPU);

	fxhl->RefreshCounters(&sData);
	memcpy(&fxhl->eData, &sData, sizeof(LightEventData));
	DebugPrint("Event values update finished\n");
}