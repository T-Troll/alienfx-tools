#include <pdh.h>
#include <pdhmsg.h>
#include <Psapi.h>
#include <shlwapi.h>
#pragma comment(lib, "pdh.lib")

#include "EventHandler.h"
#include "AlienFX_SDK.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)  
#endif

DWORD WINAPI CEventProc(LPVOID);

EventHandler* even = NULL;

EventHandler::EventHandler(ConfigHandler* confi, MonHelper* f_confi, FXHelper* fx)
{
	conf = confi;
	mon = f_confi;
	even = this;
	fxh = fx;

	StartProfiles();
	StartEffects();
}

EventHandler::~EventHandler()
{
	StopProfiles();
	StopEffects();
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
		DebugPrint("Power state changed\n");
		fxh->ChangeState();
		fxh->RefreshState();
	}
}

void EventHandler::ChangeScreenState(DWORD state)
{
	if (conf->lightsOn && (conf->offWithScreen || capt)) {
		if (state == 2) {
			// Dim display
			conf->dimmedScreen = true;
			conf->stateScreen = true;
		}
		else {
			conf->stateScreen = state;
			conf->dimmedScreen = false;
		}
		DebugPrint("Display state changed\n");
	} else {
		conf->dimmedScreen = false;
		conf->stateScreen = true;
	}
	fxh->ChangeState();
}

void EventHandler::SwitchActiveProfile(profile* newID)
{
	if (!newID) newID = conf->FindProfile(conf->defaultProfile);
	if (newID->id != conf->foregroundProfile) conf->foregroundProfile = -1;
	if (newID->id != conf->activeProfile) {
			modifyProfile.lock();
			conf->activeProfile = newID->id;
			conf->active_set = &newID->lightsets;
			if (mon) {
				if (newID->flags & PROF_FANS)
					mon->conf->lastProf = &newID->fansets;
				else
					mon->conf->lastProf = &mon->conf->prof;
			}
			modifyProfile.unlock();
			fxh->ChangeState();
			ToggleEvents();

			DebugPrint((string("Profile switched to ") + to_string(newID->id) + " (" + newID->name + ")\n").c_str());

	} else {
		DebugPrint((string("Same profile \"") + newID->name + "\", skipping switch.\n").c_str());
	}
	return;
}

void EventHandler::StartEvents()
{
	//DWORD dwThreadID;
	if (!dwHandle) {
		fxh->RefreshMon();
		// start thread...

		DebugPrint("Event thread start.\n");

		stopEvents = CreateEvent(NULL, true, false, NULL);
		dwHandle = CreateThread(NULL, 0, CEventProc, this, 0, NULL);
	}
}

void EventHandler::StopEvents()
{
	if (dwHandle) {
		DebugPrint("Event thread stop.\n");

		SetEvent(stopEvents);
		WaitForSingleObject(dwHandle, 1000);
		CloseHandle(dwHandle);
		CloseHandle(stopEvents);
		dwHandle = 0;
	}
}

void EventHandler::ToggleEvents()
{
	conf->SetStates();
	int newMode = conf->FindProfile(conf->activeProfile)->effmode;
	if (conf->stateOn) {
		ChangeEffectMode(newMode);
	}
}

void EventHandler::ChangeEffectMode(int newMode) {
	if (newMode != effMode) {
		StopEffects();
		conf->SetEffect(newMode);
		StartEffects();
	} else
		if (conf->enableMon) {
			fxh->Refresh(true);
			StartEffects();
		}
		else {
			StopEffects();
			fxh->Refresh(true);
		}
}

void EventHandler::StopEffects() {
	switch (effMode) {
	case 0:	StopEvents(); break;
	case 1: if (capt) {
		delete capt; capt = NULL;
	} break;
	case 2: if (audio) {
		delete audio; audio = NULL;
	} break;
	//case 3: break;
	}
	effMode = -1;
	fxh->Refresh(true);
}

void EventHandler::StartEffects() {
	if (conf->enableMon) {
		// start new mode...
		switch (effMode = conf->GetEffect()) {
		case 0: 
			StartEvents(); 
			break;
		case 1: 
			if (!capt) capt = new CaptureHelper(conf->amb_conf, fxh); 
			break;
		case 2: 
			if (!audio) audio = new WSAudioIn(conf->hap_conf, fxh); 
			break;
		//case 3: 
		//	break;
		}
		//effMode = conf->GetEffect();
	}
}

profile* EventHandler::ScanTaskList() {
	DWORD maxProcess=256, maxFileName=MAX_PATH, cbNeeded, cProcesses, cFileName = maxFileName;
	DWORD* aProcesses = new DWORD[maxProcess];
	TCHAR *szProcessName = new TCHAR[maxFileName]{0};

	profile* newp = NULL, *finalP = NULL;

	if (EnumProcesses(aProcesses, maxProcess * sizeof(DWORD), &cbNeeded))
	{
		while ((cProcesses = cbNeeded/sizeof(DWORD))==maxProcess) {
			maxProcess=maxProcess<<1;
			delete[] aProcesses;
			aProcesses=new DWORD[maxProcess];
			EnumProcesses(aProcesses, maxProcess * sizeof(DWORD), &cbNeeded);
		}

		for (UINT i = 0; i < cProcesses; i++)
		{
			if (aProcesses[i])
			{
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION |
					PROCESS_VM_READ,
					FALSE, aProcesses[i]);
				if (hProcess)
				{
					cFileName = GetProcessImageFileName(hProcess, szProcessName, maxFileName); //GetModuleFileNameEx(hProcess, NULL /*hMod*/, szProcessName, maxFileName);
					while (maxFileName==cFileName) {
						maxFileName=maxFileName<<1;
						delete[] szProcessName;
						szProcessName=new TCHAR[maxFileName];
						cFileName = GetProcessImageFileName(hProcess, szProcessName, maxFileName);// GetModuleFileNameEx(hProcess, NULL /*hMod*/, szProcessName, maxFileName);
					}
					PathStripPath(szProcessName);
					// is it related to profile?
					if (newp = even->conf->FindProfileByApp(string(szProcessName)))
						if (!finalP || !(finalP->flags & PROF_PRIORITY))
							finalP = newp;
					CloseHandle(hProcess);
				}
			}
		}
	}
	delete[] szProcessName;
	delete[] aProcesses;
	return finalP;
}

// Create - Check process ID, switch if found and no foreground active.
// Foreground - Check process ID, switch if found, clear foreground if not.
// Close - Check process list, switch if found and no foreground active.

VOID CALLBACK CForegroundProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	
	DWORD nameSize = MAX_PATH, cFileName = nameSize;// , cbNeeded;
	TCHAR* szProcessName = new TCHAR[nameSize]{0};
	DWORD prcId = 0;
	//HMODULE hMod;
	profile* newp = NULL;

	GUITHREADINFO activeThread;
	activeThread.cbSize = sizeof(GUITHREADINFO);

	GetGUIThreadInfo(NULL, &activeThread);

	if (activeThread.hwndActive != 0) {
		GetWindowThreadProcessId(activeThread.hwndActive, &prcId);
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION |
									  PROCESS_VM_READ,
									  FALSE, prcId);

		cFileName = GetProcessImageFileName(hProcess, szProcessName, nameSize); //GetModuleFileNameEx(hProcess, NULL /*hMod*/, szProcessName, nameSize);
		while (nameSize == cFileName) {
			nameSize = nameSize << 1;
			delete[] szProcessName;
			szProcessName = new TCHAR[nameSize];
			cFileName = GetProcessImageFileName(hProcess, szProcessName, nameSize); //GetModuleFileNameEx(hProcess, NULL/*hMod*/, szProcessName, nameSize);
		}
		CloseHandle(hProcess);
		PathStripPath(szProcessName);

		DebugPrint((string("Foreground switched to ") + szProcessName + "\n").c_str());

		string pName = szProcessName;

		newp = even->conf->FindProfileByApp(pName, true);
		even->conf->foregroundProfile = newp ? newp->id : -1;

		if (newp || !even->conf->noDesktop || (pName != "ShellExperienceHost.exe"
					 && pName != "alienfx-gui.exe"
					 && pName != "explorer.exe"
					 && pName != "SearchApp.exe"
					#ifdef _DEBUG
					 && pName != "devenv.exe"
					#endif
					 )) {

			if (!newp) {
				even->SwitchActiveProfile(even->ScanTaskList());
			} else {
				if (even->conf->IsPriorityProfile(newp->id) || !even->conf->IsPriorityProfile(even->conf->activeProfile))
					even->SwitchActiveProfile(newp);
			}
		} else {
			DebugPrint("Forbidden app, switch blocked!\n");
		}
	}
	delete[] szProcessName;
}

VOID CALLBACK CCreateProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	DWORD nameSize = MAX_PATH, cFileName = nameSize;
	TCHAR* szProcessName=new TCHAR[nameSize];
	szProcessName[0]=0;
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
			cFileName = GetProcessImageFileName(hProcess, szProcessName, nameSize);// GetModuleFileNameEx(hProcess, NULL, szProcessName, nameSize);
			while (nameSize==cFileName) {
				nameSize=nameSize<<1;
				delete[] szProcessName;
				szProcessName=new TCHAR[nameSize];
				cFileName = GetProcessImageFileName(hProcess, szProcessName, nameSize); //GetModuleFileNameEx(hProcess, NULL, szProcessName, nameSize);
			}

			PathStripPath(szProcessName);

			DWORD procstatus = 0;

			switch (dwEvent) {
			case EVENT_OBJECT_CREATE:

				if (even->conf->foregroundProfile != even->conf->activeProfile &&
					even->conf->FindProfileByApp(string(szProcessName))) {
					even->SwitchActiveProfile(even->ScanTaskList());
				}
				break;

			case EVENT_OBJECT_DESTROY:
				//GetExitCodeProcess(hProcess, &procstatus);
				//DebugPrint((string("Process (") + szProcessName + ") status " + to_string((int) procstatus) + "\n").c_str());
				
				if (even->conf->foregroundProfile != even->conf->activeProfile &&
					even->conf->FindProfileByApp(string(szProcessName))) {
					even->SwitchActiveProfile(even->ScanTaskList());
				}

				break;
			}
			CloseHandle(hProcess);
		}
		CloseHandle(hThread);
	}
	delete[] szProcessName;
}

void EventHandler::StartProfiles()
{
	if (hEvent == 0 && conf->enableProf) {

		DebugPrint("Profile hooks starting.\n");

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

		DebugPrint("Profile hooks stop.\n");

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

	long maxgpuarray = 10, maxnetarray = 10, maxtemparray = 10;
	PDH_FMT_COUNTERVALUE_ITEM* gpuArray = new PDH_FMT_COUNTERVALUE_ITEM[maxgpuarray];
	PDH_FMT_COUNTERVALUE_ITEM* netArray = new PDH_FMT_COUNTERVALUE_ITEM[maxnetarray];
	PDH_FMT_COUNTERVALUE_ITEM* tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];

	// Set data source...
	//PdhSetDefaultRealTimeDataSource(DATA_SOURCE_WBEM);

	// Open a query object.
	if (PdhOpenQuery(NULL, 0, &hQuery) != ERROR_SUCCESS)
	{
		goto cleanup;
	}

	// Add one counter that will provide the data.
	/*pdhStatus =*/ PdhAddCounter(hQuery,
		COUNTER_PATH_CPU,
		0,
		&hCPUCounter);

	/*pdhStatus =*/ PdhAddCounter(hQuery,
		COUNTER_PATH_HDD,
		0,
		&hHDDCounter);

	/*pdhStatus =*/ PdhAddCounter(hQuery,
		COUNTER_PATH_NET,
		0,
		&hNETCounter);

	/*pdhStatus =*/ PdhAddCounter(hQuery,
		COUNTER_PATH_GPU,
		0,
		&hGPUCounter);
	/*pdhStatus =*/ PdhAddCounter(hQuery,
		COUNTER_PATH_HOT,
		0,
		&hTempCounter);

	/*pdhStatus =*/ PdhAddCounter(hQuery,
		COUNTER_PATH_HOT2,
		0,
		&hTempCounter2);

	PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
	DWORD cType = 0,
		netbSize = maxnetarray * sizeof(PDH_FMT_COUNTERVALUE_ITEM), netCount = 0,
		gpubSize = maxgpuarray * sizeof(PDH_FMT_COUNTERVALUE_ITEM), gpuCount = 0,
		tempbSize = maxtemparray * sizeof(PDH_FMT_COUNTERVALUE_ITEM), tempCount = 0;

	while (WaitForSingleObject(src->stopEvents, src->conf->monDelay) == WAIT_TIMEOUT) {
		// get indicators...
		PdhCollectQueryData(hQuery);

		/*pdhStatus =*/ PdhGetFormattedCounterValue(
			hCPUCounter,
			PDH_FMT_LONG,
			&cType,
			&cCPUVal
		);
		/*pdhStatus =*/ PdhGetFormattedCounterValue(
			hHDDCounter,
			PDH_FMT_LONG,
			&cType,
			&cHDDVal
		);

		while ((pdhStatus = PdhGetFormattedCounterArray(
			hNETCounter,
			PDH_FMT_LONG,
			&netbSize,
			&netCount,
			netArray
		)) == PDH_MORE_DATA) {
			maxnetarray = netbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
			delete[] netArray;
			netArray = new PDH_FMT_COUNTERVALUE_ITEM[maxnetarray];
			netCount = 0;
		}

		if (pdhStatus != ERROR_SUCCESS) {
		//	if (pdhStatus == PDH_MORE_DATA) {
		//		maxnetarray = netbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
		//		delete[] netArray;
		//		netArray = new PDH_FMT_COUNTERVALUE_ITEM[maxnetarray];
		//	}
			netCount = 0;
		}

		// Normilizing net values...
		ULONGLONG totalNet = 0;
		for (unsigned i = 0; i < netCount; i++) {
			totalNet += netArray[i].FmtValue.longValue;
		}

		if (maxnet < totalNet) 
			maxnet = totalNet;
		//if (maxnet / 4 > totalNet) maxnet /= 2; TODO: think about decay!
		totalNet = totalNet > 0 ? (totalNet * 100) / maxnet > 0 ? (totalNet * 100) / maxnet : 1 : 0;

		while ((pdhStatus = PdhGetFormattedCounterArray(
			hGPUCounter,
			PDH_FMT_LONG,
			&gpubSize,
			&gpuCount,
			gpuArray
		)) == PDH_MORE_DATA) {
			maxgpuarray = gpubSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
			delete[] gpuArray;
			gpuArray = new PDH_FMT_COUNTERVALUE_ITEM[maxgpuarray];
			gpuCount = 0;
		}

		if (pdhStatus != ERROR_SUCCESS) {
		//	if (pdhStatus == PDH_MORE_DATA) {
		//		maxgpuarray = gpubSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
		//		delete[] gpuArray;
		//		gpuArray = new PDH_FMT_COUNTERVALUE_ITEM[maxgpuarray];
		//	}
			gpuCount = 0;
		}

		// Getting maximum GPU load value...
		long maxGPU = 0;
		for (unsigned i = 0; i < gpuCount && gpuArray[i].szName != NULL; i++) {
			if (maxGPU < gpuArray[i].FmtValue.longValue)
				maxGPU = gpuArray[i].FmtValue.longValue;
		}

		while ((pdhStatus = PdhGetFormattedCounterArray(
			hTempCounter,
			PDH_FMT_LONG,
			&tempbSize,
			&tempCount,
			tempArray
		)) == PDH_MORE_DATA) {
			maxtemparray = tempbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
			delete[] tempArray;
			tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];
		}

		if (pdhStatus != ERROR_SUCCESS) {
			//if (pdhStatus == PDH_MORE_DATA) {
			//	maxtemparray = tempbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
			//	delete[] tempArray;
			//	tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];
			//}
			tempCount = 0;
		}

		// Getting maximum temp...
		int maxTemp = 0, maxRpm = 0;
		if (src->mon) { 
			// Let's get temperatures from fan sensors
			for (unsigned i = 0; i < src->mon->senValues.size(); i++)
				if (maxTemp < src->mon->senValues[i])
					maxTemp = src->mon->senValues[i];
			// And also fan RPMs
			for (unsigned i = 0; i < src->mon->fanValues.size(); i++) {
				// maxRPM is a percent now!
				int newRpm = src->mon->fanValues[i] * 100 / src->conf->fan_conf->boosts[i].maxRPM;
				if (maxRpm < newRpm)
					maxRpm = newRpm;
			}
		} else {
			for (unsigned i = 0; i < tempCount; i++) {
				if (maxTemp + 273 < tempArray[i].FmtValue.longValue)
					maxTemp = tempArray[i].FmtValue.longValue - 273;
			}
		}

		// Now other temp sensor block...
		if (src->conf->esif_temp) {
			tempbSize = maxtemparray * sizeof(PDH_FMT_COUNTERVALUE_ITEM); tempCount = 0;
			while ((pdhStatus = PdhGetFormattedCounterArray(
				hTempCounter2,
				PDH_FMT_LONG,
				&tempbSize,
				&tempCount,
				tempArray
			)) == PDH_MORE_DATA) {
				maxtemparray = tempbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
				delete[] tempArray;
				tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];
			}

			if (pdhStatus != ERROR_SUCCESS) {
				//if (pdhStatus == PDH_MORE_DATA) {
				//	maxtemparray = tempbSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
				//	delete[] tempArray;
				//	tempArray = new PDH_FMT_COUNTERVALUE_ITEM[maxtemparray];
				//}
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
		long fanLoad = min(100, maxRpm);

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