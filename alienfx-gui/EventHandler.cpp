#include <pdh.h>
#include <pdhmsg.h>
#include <Psapi.h>
#pragma comment(lib, "pdh.lib")

#include "EventHandler.h"
#include "AlienFX_SDK.h"

DWORD WINAPI CEventProc(LPVOID);
DWORD WINAPI CProfileProc(LPVOID);

EventHandler* even = NULL;

EventHandler::EventHandler(ConfigHandler* confi, MonHelper* f_confi, FXHelper* fx)
{
	conf = confi;
	mon = f_confi;
	even = this;
	fxh = fx;

	StartProfiles();

	if (conf->IsMonitoring()) {
		switch (conf->effectMode) {
		case 0: StartEvents(); break;
		case 1: capt = new CaptureHelper(conf->amb_conf, fxh); break;
			// case 2: haptics
		}
	}
}

EventHandler::~EventHandler()
{
	StopProfiles();
	StopEvents();
	fxh->Refresh(true);
}

void EventHandler::ChangePowerState()
{
	SYSTEM_POWER_STATUS state;
	GetSystemPowerStatus(&state);
	bool sameState = true;
	if (conf->statePower = state.ACLineStatus) {
		// AC line
		switch (state.BatteryFlag) {
		case 8: // charging
			sameState = fxh->SetMode(MODE_CHARGE);
			break;
		default:
			sameState = fxh->SetMode(MODE_AC);
			break;
		}
	}
	else {
		// Battery - check BatteryFlag for details
		switch (state.BatteryFlag) {
		case 1: // ok
			sameState = fxh->SetMode(MODE_BAT);
			break;
		case 2: case 4: // low/critical
			sameState = fxh->SetMode(MODE_LOW);
			break;
		}
	}
	if (!sameState) {
#ifdef _DEBUG
		OutputDebugString("Power state changed\n");
#endif
		fxh->ChangeState();
		fxh->RefreshState();
	}
}

void EventHandler::ChangeScreenState(DWORD state)
{
	if (conf->lightsOn && conf->offWithScreen) {
		if (state == 2) {
			// Dim display
			conf->dimmedScreen = true;
			conf->stateScreen = true;
		}
		else {
			conf->stateScreen = state;
			conf->dimmedScreen = false;
		}
#ifdef _DEBUG
		OutputDebugString("Display state changed\n");
#endif
	} else {
		conf->dimmedScreen = false;
		conf->stateScreen = true;
	}
	fxh->ChangeState();
}

void EventHandler::SwitchActiveProfile(int newID)
{
	if (newID < 0) newID = conf->defaultProfile;
	if (newID != conf->activeProfile) {
		profile* newP = conf->FindProfile(newID);
		if (newP != NULL) {
			modifyProfile.lock();
			//fxh->Flush();
			conf->activeProfile = newID;
			conf->active_set = &newP->lightsets;
			if (mon) {
				if (newP->flags & PROF_FANS)
					mon->conf->lastProf = &newP->fansets;
				else
					mon->conf->lastProf = &mon->conf->prof;
			}
			modifyProfile.unlock();
			fxh->ChangeState();
			ToggleEvents();
#ifdef _DEBUG
			char buff[2048];
			sprintf_s(buff, 2047, "Profile switched to #%d (\"%s\")\n", newP->id, newP->name.c_str());
			OutputDebugString(buff);
#endif
		}
	}
#ifdef _DEBUG
	else {
		char buff[2048];
		sprintf_s (buff, 2047, "Same profile #%d, skipping switch.\n", newID);
		OutputDebugString (buff);
	}
#endif
	return;
}

void EventHandler::StartEvents()
{
	//DWORD dwThreadID;
	if (!dwHandle) {
		fxh->RefreshMon();
		// start thread...
#ifdef _DEBUG
		OutputDebugString("Event thread start.\n");
#endif
		stopEvents = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread(NULL, 0, CEventProc, this, 0, NULL);
	}
}

void EventHandler::StopEvents()
{
	if (dwHandle) {
#ifdef _DEBUG
		OutputDebugString("Event thread stop.\n");
#endif
		SetEvent(stopEvents);
		WaitForSingleObject(dwHandle, 1000);
		CloseHandle(dwHandle);
		CloseHandle(stopEvents);
		dwHandle = 0;
		fxh->Refresh(true);
	}
}

void EventHandler::ToggleEvents()
{
	conf->SetStates();
	if (conf->stateOn) {
		if (conf->IsMonitoring()) {
			switch (conf->effectMode) {
			case 0: if (!dwHandle) {
				fxh->Refresh(); StartEvents();
			} else
				fxh->RefreshState(true);
			break;
			case 1: 
			    fxh->Refresh(true);
				if (!capt) capt = new CaptureHelper(conf->amb_conf, fxh); 
			break;
				// case 2: haptics
			}
		} else {
			switch (conf->effectMode) {
			case 0:	
			if (dwHandle)
				StopEvents();
			else
				fxh->Refresh(true);
			break;
			case 1: 
			    if (capt) {
				   delete capt; capt = NULL;
			    }
				fxh->Refresh(true);
				break;
				// case 2: haptics
			}
		}
	}
}

void EventHandler::ChangeEffectMode(int newMode) {
	if (newMode != conf->effectMode) {
		// disable old mode...
		StopEffects();
		conf->effectMode = newMode;
		StartEffects();
	}
}

void EventHandler::StopEffects() {
	switch (conf->effectMode) {
	case 0:	StopEvents(); break;
	case 1: if (capt) {
		delete capt; capt = NULL;
	} break;
		// case 2: haptics
	}
}

void EventHandler::StartEffects() {
	if (conf->IsMonitoring()) {
		// start new mode...
		switch (conf->effectMode) {
		case 0: fxh->Refresh(true); StartEvents(); break;
		case 1: if (!capt) capt = new CaptureHelper(conf->amb_conf, fxh); break;
			// case 2: haptics
		}
	}
}

int ScanTaskList() {
	DWORD maxProcess=256, maxFileName=MAX_PATH, cbNeeded, cProcesses, cFileName = maxFileName;
	DWORD* aProcesses = new DWORD[maxProcess];
	TCHAR* szProcessName=new TCHAR[maxFileName];
	szProcessName[0]=0;
	//=TEXT ("<unknown>");
	int newp = -1;

	if (EnumProcesses(aProcesses, maxProcess * sizeof(DWORD), &cbNeeded))
	{
		while ((cProcesses = cbNeeded/sizeof(DWORD))==maxProcess) {
			maxProcess=maxProcess<<1;
			delete[] aProcesses;
			aProcesses=new DWORD[maxProcess];
			EnumProcesses(aProcesses, maxProcess * sizeof(DWORD), &cbNeeded);
		}
		//HMODULE hMod;
		for (UINT i = 0; i < cProcesses; i++)
		{
			if (aProcesses[i] != 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION |
					PROCESS_VM_READ,
					FALSE, aProcesses[i]);
				if (NULL != hProcess)
				{
					//if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
					//	&cbNeeded))
					//{
						cFileName = GetModuleFileNameEx(hProcess, NULL /*hMod*/, szProcessName, maxFileName);
						while (maxFileName==cFileName) {
							maxFileName=maxFileName<<1;
							delete[] szProcessName;
							szProcessName=new TCHAR[maxFileName];
							cFileName = GetModuleFileNameEx(hProcess, NULL /*hMod*/, szProcessName, maxFileName);
						}
						// is it related to profile?
						if ((newp = even->conf->FindProfileByApp(std::string(szProcessName))) >=0)
							break;
					//}
					CloseHandle(hProcess);
				}
			}
		}
	}
	delete[] szProcessName;
	delete[] aProcesses;
	return newp;
}

// Create - Check process ID, switch if found and no foreground active.
// Foreground - Check process ID, switch if found
// Close - Check process list, switch if found. Switch to dewfault if not.

VOID CALLBACK CForegroundProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	
	DWORD nameSize = MAX_PATH, cFileName = nameSize;// , cbNeeded;
	TCHAR* szProcessName = new TCHAR[nameSize]{0};
	DWORD prcId = 0;
	//HMODULE hMod;
	int newp = -1;

	GUITHREADINFO activeThread;
	activeThread.cbSize = sizeof(GUITHREADINFO);

	GetGUIThreadInfo(NULL, &activeThread);

	if (activeThread.hwndActive != 0) {
		// is it related to profile?
		GetWindowThreadProcessId(activeThread.hwndActive, &prcId);
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION |
			PROCESS_VM_READ,
			FALSE, prcId);
		//QueryFullProcessImageName(hProcess, 0, szProcessName, &nameSize);
		//EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded);
		cFileName = GetModuleFileNameEx(hProcess, NULL /*hMod*/, szProcessName, nameSize);
		while (nameSize==cFileName) {
			nameSize=nameSize<<1;
			delete[] szProcessName;
			szProcessName=new TCHAR[nameSize];
			cFileName = GetModuleFileNameEx(hProcess, NULL/*hMod*/, szProcessName, nameSize);
		}
		CloseHandle(hProcess);
//#ifdef _DEBUG
//		char buff[2048];
//		sprintf_s(buff, 2047, "Active app switched to %s\n", szProcessName);
//		OutputDebugString(buff);
//#endif
		even->conf->foregroundProfile = newp = even->conf->FindProfileByApp(std::string(szProcessName), true);
		if (newp < 0) {
			newp = ScanTaskList();
//#ifdef _DEBUG
//			char buff[2048];
//			sprintf_s(buff, 2047, "Active app unknown, switching to ID=%d\n", newp);
//			OutputDebugString(buff);
//#endif
		}
		even->SwitchActiveProfile(newp);
	}
	delete[] szProcessName;
}

VOID CALLBACK CCreateProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	DWORD nameSize = MAX_PATH, cFileName = nameSize;// , cbNeeded;
	TCHAR* szProcessName=new TCHAR[nameSize];
	szProcessName[0]=0;
	//HMODULE hMod;
	DWORD prcId = 0;
	int newp = -1;

	HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION,
		FALSE, dwEventThread);
	if (hThread) {
		prcId = GetProcessIdOfThread(hThread);
		if (prcId) {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
				PROCESS_VM_READ,
				FALSE, prcId);
			//EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded);
			cFileName = GetModuleFileNameEx(hProcess, NULL/*hMod*/, szProcessName, nameSize);
			while (nameSize==cFileName) {
				nameSize=nameSize<<1;
				delete[] szProcessName;
				szProcessName=new TCHAR[nameSize];
				cFileName = GetModuleFileNameEx(hProcess, NULL/*hMod*/, szProcessName, nameSize);
			}
			CloseHandle(hProcess);
			switch (dwEvent) {
			case EVENT_OBJECT_CREATE:
				if (even->conf->foregroundProfile < 0) {
#ifdef _DEBUG
					char buff[2048];
					sprintf_s(buff, 2047, "Switching to %s\n", szProcessName);
					OutputDebugString(buff);
#endif
					even->SwitchActiveProfile(even->conf->FindProfileByApp(std::string(szProcessName)));
				}
				break;
			case EVENT_OBJECT_DESTROY:
				if (even->conf->foregroundProfile < 0 && even->conf->FindProfileByApp(std::string(szProcessName)) == even->conf->activeProfile) {
					newp = ScanTaskList();
#ifdef _DEBUG
					char buff[2048];
					sprintf_s(buff, 2047, "Switching by close to ID=%d\n", newp);
					OutputDebugString(buff);
#endif
					even->SwitchActiveProfile(newp);
				}
				break;
			}
		}
		CloseHandle(hThread);
	}
	delete[] szProcessName;
}

void EventHandler::StartProfiles()
{
	if (hEvent == 0 && conf->enableProf) {
#ifdef _DEBUG
		OutputDebugString("Profile hooks starting.\n");
#endif
		// Need to switch if already running....
		even->SwitchActiveProfile(ScanTaskList());

		hEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND,
			EVENT_SYSTEM_FOREGROUND, NULL,
			CForegroundProc, 0, 0,
			WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
		cEvent = SetWinEventHook(EVENT_OBJECT_CREATE,
			EVENT_OBJECT_DESTROY, NULL,
			CCreateProc, 0, 0,
			WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
	}
}

void EventHandler::StopProfiles()
{
	if (hEvent) {
#ifdef _DEBUG
		OutputDebugString("Profile hooks stop.\n");
#endif
		UnhookWinEvent(cEvent);
		UnhookWinEvent(hEvent);
		hEvent = 0;
		cEvent = 0;
	}
}

DWORD WINAPI CEventProc(LPVOID param)
{
	EventHandler* src = (EventHandler*)param;

	LPCTSTR COUNTER_PATH_CPU = "\\Processor Information(_Total)\\% Processor Time",
		COUNTER_PATH_NET = "\\Network Interface(*)\\Bytes Total/sec",
		COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage",
		COUNTER_PATH_HOT = "\\Thermal Zone Information(*)\\Temperature",
		COUNTER_PATH_HOT2 = "\\EsifDeviceInformation(*)\\Temperature",
		COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Idle Time";

	HQUERY hQuery = NULL;
	HLOG hLog = NULL;
	PDH_STATUS pdhStatus;
	DWORD dwLogType = PDH_LOG_TYPE_CSV;
	HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hGPUCounter, hTempCounter, hTempCounter2;

	MEMORYSTATUSEX memStat;
	memStat.dwLength = sizeof(MEMORYSTATUSEX);

	SYSTEM_POWER_STATUS state;

	ULONGLONG maxnet = 1;
	long max_rpm = 5500;

	long maxgpuarray = 10, maxnetarray = 10, maxtemparray = 10;
	PDH_FMT_COUNTERVALUE_ITEM* gpuArray = new PDH_FMT_COUNTERVALUE_ITEM[maxgpuarray];
	PDH_FMT_COUNTERVALUE_ITEM* netArray = new PDH_FMT_COUNTERVALUE_ITEM[maxnetarray];
	PDH_FMT_COUNTERVALUE_ITEM* tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];

	// Set data source...
	//PdhSetDefaultRealTimeDataSource(DATA_SOURCE_WBEM);

	// Open a query object.
	pdhStatus = PdhOpenQuery(NULL, 0, &hQuery);

	if (pdhStatus != ERROR_SUCCESS)
	{
		//wprintf(L"PdhOpenQuery failed with 0x%x\n", pdhStatus);
		goto cleanup;
	}

	// Add one counter that will provide the data.
	pdhStatus = PdhAddCounter(hQuery,
		COUNTER_PATH_CPU,
		0,
		&hCPUCounter);

	pdhStatus = PdhAddCounter(hQuery,
		COUNTER_PATH_HDD,
		0,
		&hHDDCounter);

	pdhStatus = PdhAddCounter(hQuery,
		COUNTER_PATH_NET,
		0,
		&hNETCounter);

	pdhStatus = PdhAddCounter(hQuery,
		COUNTER_PATH_GPU,
		0,
		&hGPUCounter);
	pdhStatus = PdhAddCounter(hQuery,
		COUNTER_PATH_HOT,
		0,
		&hTempCounter);

	pdhStatus = PdhAddCounter(hQuery,
		COUNTER_PATH_HOT2,
		0,
		&hTempCounter2);

	while (WaitForSingleObject(src->stopEvents, src->conf->monDelay) == WAIT_TIMEOUT) {
		// get indicators...
		PdhCollectQueryData(hQuery);
		PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
		DWORD cType = 0,
			netbSize = maxnetarray * sizeof(PDH_FMT_COUNTERVALUE_ITEM), netCount = 0,
			gpubSize = maxgpuarray * sizeof(PDH_FMT_COUNTERVALUE_ITEM), gpuCount = 0,
			tempbSize = maxtemparray * sizeof(PDH_FMT_COUNTERVALUE_ITEM), tempCount = 0;
		pdhStatus = PdhGetFormattedCounterValue(
			hCPUCounter,
			PDH_FMT_LONG,
			&cType,
			&cCPUVal
		);
		pdhStatus = PdhGetFormattedCounterValue(
			hHDDCounter,
			PDH_FMT_LONG,
			&cType,
			&cHDDVal
		);

		pdhStatus = PdhGetFormattedCounterArray(
			hNETCounter,
			PDH_FMT_LONG,
			&netbSize,
			&netCount,
			netArray
		);

		if (pdhStatus != ERROR_SUCCESS) {
			if (pdhStatus == PDH_MORE_DATA) {
				maxnetarray = netbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
				delete[] netArray;
				netArray = new PDH_FMT_COUNTERVALUE_ITEM[maxnetarray];
			}
			netCount = 0;
		}

		// Normilizing net values...
		ULONGLONG totalNet = 0;
		for (unsigned i = 0; i < netCount; i++) {
			totalNet += netArray[i].FmtValue.longValue;
		}

		if (maxnet < totalNet) maxnet = totalNet;
		//if (maxnet / 4 > totalNet) maxnet /= 2; TODO: think about decay!
		totalNet = totalNet > 0 ? (totalNet * 100) / maxnet > 0 ? (totalNet * 100) / maxnet : 1 : 0;

		pdhStatus = PdhGetFormattedCounterArray(
			hGPUCounter,
			PDH_FMT_LONG,
			&gpubSize,
			&gpuCount,
			gpuArray
		);

		if (pdhStatus != ERROR_SUCCESS) {
			if (pdhStatus == PDH_MORE_DATA) {
				maxgpuarray = gpubSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
				delete[] gpuArray;
				gpuArray = new PDH_FMT_COUNTERVALUE_ITEM[maxgpuarray];
			}
			gpuCount = 0;
		}

		// Getting maximum GPU load value...
		long maxGPU = 0;
		for (unsigned i = 0; i < gpuCount && gpuArray[i].szName != NULL; i++) {
			if (maxGPU < gpuArray[i].FmtValue.longValue)
				maxGPU = gpuArray[i].FmtValue.longValue;
		}

		pdhStatus = PdhGetFormattedCounterArray(
			hTempCounter,
			PDH_FMT_LONG,
			&tempbSize,
			&tempCount,
			tempArray
		);

		if (pdhStatus != ERROR_SUCCESS) {
			if (pdhStatus == PDH_MORE_DATA) {
				maxtemparray = tempbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
				delete[] tempArray;
				tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];
			}
			tempCount = 0;
		}

		// Getting maximum temp...
		long maxTemp = 0, maxRpm = 0;
		if (src->conf->fanControl && src->mon) { 
			// Let's get temperatures from fan sensors
			for (unsigned i = 0; i < src->mon->senValues.size(); i++)
				if (maxTemp < src->mon->senValues[i])
					maxTemp = src->mon->senValues[i];
			// And also fan RPMs
			for (unsigned i = 0; i < src->mon->fanValues.size(); i++)
				if (maxRpm < src->mon->fanValues[i])
					maxRpm = src->mon->fanValues[i];
			if (maxRpm > max_rpm)
				max_rpm = maxRpm;
		} else {
			for (unsigned i = 0; i < tempCount; i++) {
				if (maxTemp + 273 < tempArray[i].FmtValue.longValue)
					maxTemp = tempArray[i].FmtValue.longValue - 273;
			}
		}

		// Now other temp sensor block...
		if (src->conf->esif_temp) {
			tempbSize = maxtemparray * sizeof(PDH_FMT_COUNTERVALUE_ITEM); tempCount = 0;
			pdhStatus = PdhGetFormattedCounterArray(
				hTempCounter2,
				PDH_FMT_LONG,
				&tempbSize,
				&tempCount,
				tempArray
			);

			if (pdhStatus != ERROR_SUCCESS) {
				if (pdhStatus == PDH_MORE_DATA) {
					maxtemparray = tempbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
					delete[] tempArray;
					tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];
				}
				tempCount = 0;
			}

			// Added other set maximum temp...
			for (unsigned i = 0; i < tempCount; i++) {
				if (maxTemp < tempArray[i].FmtValue.longValue)
					maxTemp = tempArray[i].FmtValue.longValue;
			}
		}

		GlobalMemoryStatusEx(&memStat);
		GetSystemPowerStatus(&state);

		// Leveling...
		maxTemp = min(100, max(0, maxTemp));
		long battLife = min(100, max(0, state.BatteryLifePercent));
		long hddLoad = max(0, 99 - cHDDVal.longValue);
		long fanLoad = maxRpm * 100 / max_rpm;

		src->modifyProfile.lock();
		src->fxh->SetCounterColor(cCPUVal.longValue, memStat.dwMemoryLoad, maxGPU, (long)totalNet, hddLoad, maxTemp, battLife, fanLoad);
		src->modifyProfile.unlock();
	}

	delete[] gpuArray; delete[] netArray; delete[] tempArray;

cleanup:

	if (hQuery)
		PdhCloseQuery(hQuery);

	return 0;
}