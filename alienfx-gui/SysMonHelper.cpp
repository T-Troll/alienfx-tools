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

IWbemServices* m_WmiService = NULL, *m_CimService = NULL;

vector<BSTR> esifSensors;

SysMonHelper::SysMonHelper() {
	DebugPrint("Starting Event thread\n");
	IWbemLocator* m_WbemLocator;
	CoInitializeEx(nullptr, COINIT::COINIT_MULTITHREADED);
	CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
		RPC_C_AUTHN_LEVEL_NONE, //RPC_C_AUTHN_LEVEL_CONNECT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		nullptr, EOAC_NONE, nullptr);
	IEnumWbemClassObject* enum_obj;
	CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&m_WbemLocator);
	m_WbemLocator->ConnectServer((BSTR)L"ROOT\\WMI", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &m_WmiService);
	if (m_WmiService && SUCCEEDED(m_WmiService->CreateInstanceEnum((BSTR)L"EsifDeviceInformation", WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj))) {
		IWbemClassObject* spInstance[32];
		ULONG uNumOfInstances;
		HRESULT res = WBEM_S_NO_ERROR;
		VARIANT instPath{ VT_BSTR };
		while (res == WBEM_S_NO_ERROR && SUCCEEDED(res = enum_obj->Next(3000, 32, spInstance, &uNumOfInstances)) && uNumOfInstances) {
			for (byte ind = 0; ind < uNumOfInstances; ind++) {
				if (SUCCEEDED(spInstance[ind]->Get((BSTR)L"__Path", 0, &instPath, 0, 0)) && instPath.bstrVal) {
					esifSensors.push_back(instPath.bstrVal);
				}
				spInstance[ind]->Release();
			}
		}
		enum_obj->Release();
	}
	m_WbemLocator->ConnectServer((BSTR)L"ROOT\\CIMV2", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &m_CimService);

	m_WbemLocator->Release();
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
	LightEventData* sData = &src->sData;
#define maxData fxhl->maxData

	PdhCollectQueryData(src->hQuery);
	ZeroMemory(sData, sizeof(LightEventData));

	// CPU load
	PdhGetFormattedCounterValue(src->hCPUCounter, PDH_FMT_LONG, NULL, &cCPUVal);
	// HDD load
	PdhGetFormattedCounterValue(src->hHDDCounter, PDH_FMT_LONG, NULL, &cHDDVal);
	// Network load
	int maxBand = src->GetValuesArray(src->hNETMAXCounter, 0, 2048),
		curBand = src->GetValuesArray(src->hNETCounter);
	if (maxBand) {
		sData->NET = min(100, curBand / maxBand);
		maxData.NET = max(sData->NET, maxData.NET);
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
			sData->GPU = min(max(sData->GPU, sub->second), 100);
			//DebugPrint("Adapter " + to_string(sub->first) + ", system " + it->first + ": " + to_string(sub->second) + "\n");
			//sub->second = 0;
		}
	}
	// Temperatures
	sData->Temp = src->GetValuesArray(src->hTempCounter, 273);
	// RAM load
	GlobalMemoryStatusEx(&memStat);
	// Power state
	GetSystemPowerStatus(&state);
	// Locale
	if (curLocale = GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), NULL))) {
		GetKeyboardLayoutList(10, locIDs);
		sData->KBD = curLocale == locIDs[0] ? 0 : 100;
	}

	if (mon) {
		mon->GetSensorData();
		// Check fan RPMs
		for (unsigned i = 0; i < mon->fansize; i++) {
			sData->Fan = max(sData->Fan, mon->GetFanPercent(i));
		}
		sData->Fan = min(100, sData->Fan);
		// Sensors
		for (auto i = mon->senValues.begin(); i != mon->senValues.end(); i++)
			sData->Temp = max(sData->Temp, i->second);
		// Power mode
		sData->PWM = mon->powerMode * 100 /	(mon->powerSize + mon->acpi->isGmode - 1);
	}

	// ESIF powers and temps
	if (conf->esif_temp) {
		if (!mon) {
			// ESIF temps (already in fans)
			sData->Temp = max(sData->Temp, src->GetValuesArray(src->hTempCounter2, maxData.Temp));
		}
		// Powers
		sData->PWR = src->GetValuesArray(src->hPwrCounter, 0, 10);
		maxData.PWR = max(sData->PWR, maxData.PWR);
		sData->PWR = sData->PWR * 100 / maxData.PWR;
	}

	// Leveling...
	sData->Temp = min(100, max(0, sData->Temp));
	sData->Batt = state.BatteryLifePercent;
	sData->ACP = state.ACLineStatus;
	sData->BST = state.BatteryFlag;
	sData->HDD = (byte)max(0, 99 - cHDDVal.longValue);
	maxData.RAM = max(maxData.RAM, sData->RAM = (byte)memStat.dwMemoryLoad);
	maxData.CPU = max(maxData.CPU, sData->CPU = (byte)cCPUVal.longValue);
	maxData.GPU = max(maxData.GPU, sData->GPU);
	maxData.Temp = max(sData->Temp, maxData.Temp);

	fxhl->RefreshCounters();
	memcpy(&fxhl->eData, sData, sizeof(LightEventData));
	DebugPrint("Event values update finished\n");
}