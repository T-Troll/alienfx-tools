#include "SysMonHelper.h"
#include "MonHelper.h"
//#include <PdhMsg.h>

//#pragma comment(lib, "pdh.lib")

#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

extern FXHelper* fxhl;
extern MonHelper* mon;
extern ConfigHandler* conf;

static SYSTEM_POWER_STATUS state;
//static PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
static MEMORYSTATUSEX memStat{ sizeof(MEMORYSTATUSEX) };
static HKL locIDs[10];
static HKL curLocale;

void CEventProc(LPVOID);

IWbemServices* m_WmiService = NULL, * m_CimService = NULL;

vector<IWbemClassObject*> esifSensors, netInst;
IWbemClassObject* cpuPath, * diskPath;

vector<IWbemClassObject*> GetAllInstances(IWbemServices* srv, const wchar_t* s_name) {
	vector<IWbemClassObject*> result;
	IEnumWbemClassObject* enum_obj;
	if (srv && SUCCEEDED(srv->CreateInstanceEnum((BSTR)s_name, WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_DIRECT_READ, NULL, &enum_obj))) {
		IWbemClassObject* spInstance[64];
		ULONG uNumOfInstances;
		HRESULT res = WBEM_S_NO_ERROR;
		while (res == WBEM_S_NO_ERROR && SUCCEEDED(res = enum_obj->Next(3000, 64, spInstance, &uNumOfInstances)) && uNumOfInstances) {
			for (byte ind = 0; ind < uNumOfInstances; ind++) {
				result.push_back(spInstance[ind]);
			}
		}
		enum_obj->Release();
	}
	return result;
}

IWbemClassObject* FindTotalPath(IWbemServices* srv, const wchar_t* s_name) {
	//IEnumWbemClassObject* enum_obj;
	IWbemClassObject* finalPath = NULL;// (BSTR)L"";
	VARIANT name{ VT_BSTR };// instPath{ VT_BSTR };
	auto inst = GetAllInstances(srv, s_name);
	for (auto& i : inst) {
		if (SUCCEEDED(i->Get((BSTR)L"Name", 0, &name, 0, 0)) && name.bstrVal) {
			if (name.bstrVal[0] == L'_') {
				finalPath = i;
			}
			else
				i->Release();
		}
	}
	return finalPath;
}

SysMonHelper::SysMonHelper() {
	DebugPrint("Starting Event thread\n");
	IWbemLocator* m_WbemLocator;
	CoInitializeEx(nullptr, COINIT::COINIT_MULTITHREADED);
	CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
		RPC_C_AUTHN_LEVEL_NONE, //RPC_C_AUTHN_LEVEL_CONNECT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		nullptr, EOAC_NONE, nullptr);
	CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&m_WbemLocator);
	m_WbemLocator->ConnectServer((BSTR)L"ROOT\\WMI", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &m_WmiService);
	esifSensors = GetAllInstances(m_WmiService, (BSTR)L"EsifDeviceInformation");
	m_WbemLocator->ConnectServer((BSTR)L"ROOT\\CIMV2", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &m_CimService);
	cpuPath = FindTotalPath(m_CimService, (BSTR)L"Win32_PerfFormattedData_PerfOS_Processor");
	diskPath = FindTotalPath(m_CimService, (BSTR)L"Win32_PerfFormattedData_PerfDisk_PhysicalDisk");
	netInst = GetAllInstances(m_CimService, (BSTR)L"Win32_PerfFormattedData_Tcpip_NetworkAdapter");
	m_WbemLocator->Release();
	//PdhSetDefaultRealTimeDataSource(DATA_SOURCE_WBEM);
	//if (PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS) {
	//	// Set data source...
	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_CPU, 0, &hCPUCounter);
	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_HDD, 0, &hHDDCounter);
	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_NET, 0, &hNETCounter);
	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_NETMAX, 0, &hNETMAXCounter);
	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_GPU, 0, &hGPUCounter);
	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_HOT, 0, &hTempCounter);

	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_HOT2, 0, &hTempCounter2);
	//	PdhAddEnglishCounter(hQuery, COUNTER_PATH_PWR, 0, &hPwrCounter);

	//	PdhCollectQueryData(hQuery);
		// start thread...
		eventProc = new ThreadHelper(CEventProc, this, 00);
		DebugPrint("Event thread start.\n");
	//}
}

SysMonHelper::~SysMonHelper() {
	if (eventProc) {
		delete eventProc;
		//delete[] counterValues;
		ZeroMemory(&fxhl->eData, sizeof(LightEventData));
		DebugPrint("Event thread stop.\n");
	//	PdhCloseQuery(hQuery);
	}
	if (m_WmiService)
		m_WmiService->Release();
	if (m_CimService)
		m_CimService->Release();
	CoUninitialize();
}

//int SysMonHelper::GetCounterValues(HCOUNTER counter) {
//	DWORD cs = 0;
//	DWORD count = 0;
//
//	if (PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &cs, &count, NULL) == PDH_MORE_DATA) {
//		delete[] counterValues;
//		counterValues = new byte[cs];
//		PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &cs, &count, (PDH_FMT_COUNTERVALUE_ITEM*)counterValues);
//	}
//	return count;
//}
//
//int SysMonHelper::GetValuesArray(HCOUNTER counter, int delta = 0, int divider = 1) {
//	int retVal = 0;
//	DWORD count = GetCounterValues(counter);
//
//	for (DWORD i = 0; i < count; i++) {
//		retVal = max(retVal, ((PDH_FMT_COUNTERVALUE_ITEM*)counterValues)[i].FmtValue.longValue / divider - delta);
//	}
//
//	//maxVal = max(maxVal, retVal);
//
//	return retVal;
//}

int GetInstanceValue(IWbemClassObject* i_name, const wchar_t* v_name) {
	VARIANT value;
	i_name->Get((BSTR)v_name, 0, &value, 0, 0);
	//i_name->Release();
	return _wtoi(value.bstrVal);
}

void CEventProc(LPVOID param)
{
	DebugPrint("Event values update started\n");
	SysMonHelper* src = (SysMonHelper*)param;
	LightEventData* sData = &src->sData;
#define maxData fxhl->maxData

	//PdhCollectQueryData(src->hQuery);
	ZeroMemory(sData, sizeof(LightEventData));

	// CPU load
	//PdhGetFormattedCounterValue(src->hCPUCounter, PDH_FMT_LONG, NULL, &cCPUVal);
	sData->CPU = GetInstanceValue(cpuPath, (BSTR)L"PercentProcessorTime");
	// HDD load
	//PdhGetFormattedCounterValue(src->hHDDCounter, PDH_FMT_LONG, NULL, &cHDDVal);
	sData->HDD = GetInstanceValue(diskPath, (BSTR)L"PercentDiskTime");
	// Network load
	//auto netInst = GetAllInstances(m_CimService, (BSTR)L"Win32_PerfFormattedData_Tcpip_NetworkAdapter");
	for (auto& i : netInst) {
		//VARIANT maxBand{ VT_BSTR }, curBand{ VT_BSTR };
		int mBand = GetInstanceValue(i, (BSTR)L"CurrentBandwidth");
		if (mBand) {
			int cBand = GetInstanceValue(i, (BSTR)L"BytesTotalPerSec");
			if (cBand) {
				sData->NET = max(sData->NET, (cBand * 100 / mBand) + 1);
				maxData.NET = max(sData->NET, maxData.NET);
			}
		}
		//i->Release();
	}
	//
	//int maxBand = src->GetValuesArray(src->hNETMAXCounter, 0, 2048),
	//	curBand = src->GetValuesArray(src->hNETCounter);
	//if (maxBand) {
	//	sData->NET = min(100, curBand / maxBand);
	//	maxData.NET = max(sData->NET, maxData.NET);
	//}
	// GPU load
	auto gpus = GetAllInstances(m_CimService, (BSTR)L"Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine");
	//DWORD count = src->GetCounterValues(src->hGPUCounter);
	// now sort...
	map<string, map<char, int>> gpusubs;
	//PDH_FMT_COUNTERVALUE_ITEM* va = (PDH_FMT_COUNTERVALUE_ITEM*)src->counterValues;
	//for (DWORD i = 0; i < count; i++) {
	//	if (va[i].FmtValue.longValue && va[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA) {
	//		string path = va[i].szName;
	//		char physID = path[path.find("phys_") + 5];
	//		string type = path.substr(path.find("type_"));
	//		gpusubs[type][physID] += va[i].FmtValue.longValue;
	//	}
	//}
	for (auto it = gpusubs.begin(); it != gpusubs.end(); it++) {
		// per-adapter
		for (auto sub = it->second.begin(); sub != it->second.end(); sub++) {
			sData->GPU = min(max(sData->GPU, sub->second), 100);
			//DebugPrint("Adapter " + to_string(sub->first) + ", system " + it->first + ": " + to_string(sub->second) + "\n");
			//sub->second = 0;
		}
	}
	// Temperatures
	//sData->Temp = src->GetValuesArray(src->hTempCounter, 273);
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
		int cPwr = 0;
		for (auto& i : esifSensors) {
			if (!mon)
				sData->Temp = max(sData->Temp, GetInstanceValue(i, (BSTR)L"Temperature"));
			cPwr += GetInstanceValue(i, (BSTR)L"Power") / 10;
		}
		//if (!mon) {
		//	// ESIF temps (already in fans)
		//	sData->Temp = max(sData->Temp, src->GetValuesArray(src->hTempCounter2, maxData.Temp));
		//}
		// Powers
		//sData->PWR = src->GetValuesArray(src->hPwrCounter, 0, 10);
		maxData.PWR = max(cPwr, maxData.PWR);
		sData->PWR = cPwr * 100 / maxData.PWR;
		//maxData.PWR = max(sData->PWR, maxData.PWR);
		//sData->PWR = sData->PWR * 100 / maxData.PWR;
	}

	// Leveling...
	sData->Temp = min(100, max(0, sData->Temp));
	sData->Batt = state.BatteryLifePercent;
	sData->ACP = state.ACLineStatus;
	sData->BST = state.BatteryFlag;
	//sData->HDD = (byte)max(0, 99 - cHDDVal.longValue);
	maxData.RAM = max(maxData.RAM, sData->RAM = (byte)memStat.dwMemoryLoad);
	maxData.CPU = max(maxData.CPU, sData->CPU);// = (byte)cCPUVal.longValue);
	maxData.GPU = max(maxData.GPU, sData->GPU);
	maxData.Temp = max(sData->Temp, maxData.Temp);

	fxhl->RefreshCounters();
	memcpy(&fxhl->eData, sData, sizeof(LightEventData));
	DebugPrint("Event values update finished\n");
}