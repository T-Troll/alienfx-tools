#include <Pdh.h>
#include <pdhmsg.h>
#include <Psapi.h>
#pragma comment(lib, "pdh.lib")

#include "alienfx-gui.h"
#include "EventHandler.h"
#include "MonHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

extern AlienFan_SDK::Control* acpi;
extern EventHandler* eve;
extern FXHelper* fxhl;
extern MonHelper* mon;

extern void SetTrayTip();

void CEventProc(LPVOID);
VOID CALLBACK CForegroundProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
VOID CALLBACK CCreateProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam);

EventHandler::EventHandler()
{
	eve = this;
	aProcesses = new DWORD[maxProcess];
	ChangePowerState();
	ChangeEffectMode();
}

EventHandler::~EventHandler()
{
	StopProfiles();
	StopEffects();
	delete[] aProcesses;
}

void EventHandler::ChangePowerState()
{
	SYSTEM_POWER_STATUS state;
	GetSystemPowerStatus(&state);
	if ((byte)conf->statePower != state.ACLineStatus) {
		conf->statePower = state.ACLineStatus;
		DebugPrint("Power state changed!\n");
		fxhl->SetState();
		if (cEvent)
			CheckProfileWindow();
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
	fxhl->SetState();
}

void EventHandler::SwitchActiveProfile(profile* newID)
{
	if (keyboardSwitchActive) return;
	if (!newID) newID = conf->FindDefaultProfile();
	if (newID != conf->activeProfile) {
		// reset effects
		fxhl->UpdateGlobalEffect(NULL, true);
		modifyProfile.lock();
		conf->activeProfile = newID;
		//conf->active_set = &newID->lightsets;
		conf->fan_conf->lastProf = newID->flags & PROF_FANS ? &newID->fansets : &conf->fan_conf->prof;
		modifyProfile.unlock();

		if (acpi)
			mon->SetCurrentGmode(conf->fan_conf->lastProf->gmode);

		fxhl->SetState();
		ChangeEffectMode();

		DebugPrint("Profile switched to " + to_string(newID->id) + " (" + newID->name + ")\n");
	}
#ifdef _DEBUG
	else
		DebugPrint("Same profile \"" + newID->name + "\", skipping switch.\n");
#endif
}

void EventHandler::StartEvents()
{
	if (!eventProc && PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS) {
		// Set data source...
		PdhAddCounter(hQuery, COUNTER_PATH_CPU, 0, &hCPUCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_HDD, 0, &hHDDCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_NET, 0, &hNETCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_NETMAX, 0, &hNETMAXCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_GPU, 0, &hGPUCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_HOT, 0, &hTempCounter);
		PdhAddCounter(hQuery, COUNTER_PATH_HOT2, 0, &hTempCounter2);
		PdhAddCounter(hQuery, COUNTER_PATH_PWR, 0, &hPwrCounter);

		fxhl->RefreshCounters();
		// start thread...
		eventProc = new ThreadHelper(CEventProc, this, 300/*, THREAD_PRIORITY_NORMAL*/);
		DebugPrint("Event thread start.\n");
	}
}

void EventHandler::StopEvents()
{
	if (eventProc) {
		delete eventProc;
		eventProc = NULL;
		PdhCloseQuery(hQuery);
		DebugPrint("Event thread stop.\n");
	}
}

void EventHandler::ChangeEffectMode() {
	if (conf->enableMon && conf->stateOn) {
		if (conf->GetEffect() != effMode)
			StopEffects();
		else
			fxhl->Refresh();
		StartEffects();
	}
	else
		StopEffects();
	SetTrayTip();
}

void EventHandler::StopEffects() {
	switch (effMode) {
	case 1:	StopEvents(); break; // Events
	case 2: if (capt) { // Ambient
		delete capt; capt = NULL;
	} break;
	case 3: if (audio) { // Haptics
		delete audio; audio = NULL;
	} break;
	case 4: if (grid) {
		delete grid; grid = NULL;
	} break;
	}
	effMode = 0;
	fxhl->Refresh();
}

void EventHandler::StartEffects() {
	if (conf->enableMon) {
		// start new mode...
		switch (effMode = conf->GetEffect()) {
		case 1:
			StartEvents();
			break;
		case 2:
			if (!capt) capt = new CaptureHelper();
			break;
		case 3:
			if (!audio) audio = new WSAudioIn();
			break;
		case 4:
			if (!grid) grid = new GridHelper();
			break;
		}
	}
}

void EventHandler::ScanTaskList() {
	DWORD cbNeeded;
	char* szProcessName = new char[32768];

	profile* newp = NULL, *finalP = NULL;

	if (EnumProcesses(aProcesses, maxProcess * sizeof(DWORD), &cbNeeded)) {
		while (cbNeeded / sizeof(DWORD) == maxProcess) {
			maxProcess = maxProcess << 1;
			delete[] aProcesses;
			aProcesses = new DWORD[maxProcess];
			EnumProcesses(aProcesses, maxProcess * sizeof(DWORD), &cbNeeded);
		}

		for (UINT i = 0; i < cbNeeded / sizeof(DWORD); i++) {
			if (aProcesses[i]) {
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
				if (hProcess && GetProcessImageFileName(hProcess, szProcessName, 32767)) {
					PathStripPath(szProcessName);
					CloseHandle(hProcess);
					// is it related to profile?
					if (newp = conf->FindProfileByApp(string(szProcessName))) {
						finalP = newp;
						if (conf->IsPriorityProfile(newp))
							break;
					}
				}
			}
			else {
				DebugPrint("Zero process " + to_string(i) + "\n");
			}
		}
	}
#ifdef _DEBUG
	if (finalP) {
		DebugPrint("TaskScan: new profile is " + finalP->name + "\n");
	}
	else
		DebugPrint("TaskScan: no profile\n");
#endif // _DEBUG
	delete[] szProcessName;
	SwitchActiveProfile(finalP);
}

void EventHandler::CheckProfileWindow() {

	char* szProcessName;
	DWORD prcId;

	GetWindowThreadProcessId(GetForegroundWindow(), &prcId);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, prcId);

	if (hProcess) {
		szProcessName = new char[32768];
		GetProcessImageFileName(hProcess, szProcessName, 32767);
		CloseHandle(hProcess);
		PathStripPath(szProcessName);

		DebugPrint(string("Foreground switched to ") + szProcessName + "\n");

		if (!conf->noDesktop || (szProcessName != string("ShellExperienceHost.exe")
			&& szProcessName != string("explorer.exe")
			&& szProcessName != string("SearchApp.exe")
#ifdef _DEBUG
			&& szProcessName != string("devenv.exe")
#endif
			)) {

			profile* newProf;

			if (newProf = conf->FindProfileByApp(szProcessName, true)) {
				if (conf->IsPriorityProfile(newProf) || !conf->IsPriorityProfile(conf->activeProfile))
					SwitchActiveProfile(newProf);
			}
			else
				ScanTaskList();
		}
#ifdef _DEBUG
		else {
			DebugPrint("Forbidden app, switch blocked!\n");
		}
#endif // _DEBUG
		delete[] szProcessName;
	}
}

void EventHandler::StartProfiles()
{
	if (conf->enableProf && !cEvent) {

		DebugPrint("Profile hooks starting.\n");

		hEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND,
								 EVENT_SYSTEM_FOREGROUND, NULL,
								 CForegroundProc, 0, 0,
								 WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

		cEvent = SetWinEventHook(EVENT_OBJECT_CREATE,
			EVENT_OBJECT_DESTROY, NULL,
			CCreateProc, 0, 0,
			WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

#ifndef _DEBUG
		kEvent = SetWindowsHookExW(WH_KEYBOARD_LL, KeyProc, NULL, 0);
#endif

		// Need to switch if already running....
		CheckProfileWindow();
	}
}

void EventHandler::StopProfiles()
{
	if (cEvent) {
		DebugPrint("Profile hooks stop.\n");
		UnhookWinEvent(hEvent);
		UnhookWinEvent(cEvent);
		UnhookWindowsHookEx(kEvent);
		cEvent = 0;
		keyboardSwitchActive = false;
	}
}

DWORD counterSize = sizeof(PDH_FMT_COUNTERVALUE_ITEM);
PDH_FMT_COUNTERVALUE_ITEM* counterValues = new PDH_FMT_COUNTERVALUE_ITEM[1], * counterValuesMax = new PDH_FMT_COUNTERVALUE_ITEM[1];

int GetValuesArray(HCOUNTER counter, byte& maxVal, int delta = 0, int divider = 1, HCOUNTER c2 = NULL) {
	PDH_STATUS pdhStatus;
	DWORD count;
	int retVal = 0;

	if (c2) {
		counterSize = sizeof(counterValuesMax);
		while ((pdhStatus = PdhGetFormattedCounterArray(c2, PDH_FMT_LONG, &counterSize, &count, counterValuesMax)) == PDH_MORE_DATA) {
			delete[] counterValuesMax;
			counterValuesMax = new PDH_FMT_COUNTERVALUE_ITEM[counterSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1];
		}
	}

	counterSize = sizeof(counterValues);
	while ((pdhStatus = PdhGetFormattedCounterArray( counter, PDH_FMT_LONG, &counterSize, &count, counterValues)) == PDH_MORE_DATA) {
		delete[] counterValues;
		counterValues = new PDH_FMT_COUNTERVALUE_ITEM[counterSize/sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1];
	}

	for (DWORD i = 0; i < count; i++) {
		int cval = c2 && counterValuesMax[i].FmtValue.longValue ?
			counterValues[i].FmtValue.longValue * 800 / counterValuesMax[i].FmtValue.longValue :
			counterValues[i].FmtValue.longValue / divider - delta;
		retVal = max(retVal, cval);
	}

	maxVal = max(maxVal, retVal);

	return retVal;
}

// Callback and event processing hooks
// Create - Check process ID, switch if found and no foreground active.
// Foreground - Check process ID, switch if found, clear foreground if not.
// Close - Check process list, switch if found and no foreground active.

static VOID CALLBACK CCreateProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {

	char* szProcessName;
	DWORD prcId;

	GetWindowThreadProcessId(hwnd, &prcId);

	HANDLE hProcess;

	if (idObject == OBJID_WINDOW && !GetParent(hwnd) &&
		(hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, prcId))) {
		szProcessName = new char[32768];
		GetProcessImageFileName(hProcess, szProcessName, 32767);
		PathStripPath(szProcessName);
		//DebugPrint(("C/D: " + to_string(dwEvent) + " - " + szProcessName + "\n").c_str());
		if (conf->FindProfileByApp(szProcessName)) {
			if (dwEvent == EVENT_OBJECT_DESTROY) {
				DebugPrint("C/D: Delay activated.\n");
				// Wait for termination
				WaitForSingleObject(hProcess, 1000);
				DebugPrint("C/D: Delay done.\n");
			}
			//eve->ScanTaskList();
			eve->CheckProfileWindow();
		}
		delete[] szProcessName;
		CloseHandle(hProcess);
	}
}

static VOID CALLBACK CForegroundProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	eve->CheckProfileWindow();
}

LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

	switch (wParam) {
	case WM_KEYDOWN: case WM_SYSKEYDOWN:
		if (!eve->keyboardSwitchActive) {
			for (auto pos = conf->profiles.begin(); pos != conf->profiles.end(); pos++)
				if (((LPKBDLLHOOKSTRUCT)lParam)->vkCode == (*pos)->triggerkey && conf->SamePower((*pos)->triggerFlags, true)) {
					eve->SwitchActiveProfile(*pos);
					eve->keyboardSwitchActive = true;
					break;
				}
		}
		break;
	case WM_KEYUP: case WM_SYSKEYUP:
		if (eve->keyboardSwitchActive && ((LPKBDLLHOOKSTRUCT)lParam)->vkCode == conf->activeProfile->triggerkey) {
			eve->keyboardSwitchActive = false;
			eve->CheckProfileWindow();
		}
		break;
	}

	return res;
}

void CEventProc(LPVOID param)
{
	EventHandler* src = (EventHandler*)param;

	static HKL locIDs[10];
	static MEMORYSTATUSEX memStat{ sizeof(MEMORYSTATUSEX) };
	static SYSTEM_POWER_STATUS state;
	static PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
	static HKL curLocale;

	LightEventData* cData = &src->cData;

	if (conf->lightsNoDelay) {

			PdhCollectQueryData(src->hQuery);
			src->cData = { 0 };

			// CPU load
			PdhGetFormattedCounterValue(src->hCPUCounter, PDH_FMT_LONG, NULL, &cCPUVal);
			// HDD load
			PdhGetFormattedCounterValue(src->hHDDCounter, PDH_FMT_LONG, NULL, &cHDDVal);
			// Network load
			cData->NET = GetValuesArray(src->hNETCounter, fxhl->maxData.NET, 0, 1, src->hNETMAXCounter);
			// GPU load
			cData->GPU = GetValuesArray(src->hGPUCounter, fxhl->maxData.GPU);
			// Temperatures
			cData->Temp = GetValuesArray(src->hTempCounter, fxhl->maxData.Temp, 273);
			// RAM load
			GlobalMemoryStatusEx(&memStat);
			// Power state
			GetSystemPowerStatus(&state);
			// Locale
			if (curLocale = GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), NULL))) {
				GetKeyboardLayoutList(10, locIDs);
				cData->KBD = curLocale == locIDs[0] ? 0 : 100;
			}

			if (acpi) {
				// Check fan RPMs
				for (unsigned i = 0; i < mon->fanRpm.size(); i++) {
					cData->Fan = max(cData->Fan, mon->GetFanPercent(i));
				}
				// Sensors
				for (auto i = mon->senValues.begin(); i != mon->senValues.end(); i++)
					cData->Temp = max(cData->Temp, i->second);
				// Power mode
				cData->PWM = conf->fan_conf->lastProf->gmode ? 100 :
					conf->fan_conf->lastProf->powerStage * 100 /
					((int)acpi->powers.size() + acpi->isGmode - 1);
			}

			// ESIF powers and temps
			short totalPwr = 0;
			if (conf->esif_temp) {
				if (!acpi) {
					// ESIF temps (already in fans)
					cData->Temp = max(cData->Temp, GetValuesArray(src->hTempCounter2, fxhl->maxData.Temp));
				}
				// Powers
				cData->PWR = GetValuesArray(src->hPwrCounter, fxhl->maxData.PWR, 0, 10);
			}

			// Leveling...
			cData->Temp = min(100, max(0, cData->Temp));
			cData->Batt = state.BatteryLifePercent;
			cData->ACP = state.ACLineStatus;
			cData->BST = state.BatteryFlag;
			cData->HDD = (byte)max(0, 99 - cHDDVal.longValue);
			cData->Fan = min(100, cData->Fan);
			cData->CPU = (byte)cCPUVal.longValue;
			cData->RAM = (byte)memStat.dwMemoryLoad;
			cData->PWR = (byte)cData->PWR * 100 / fxhl->maxData.PWR;
			fxhl->maxData.RAM = max(fxhl->maxData.RAM, cData->RAM);
			fxhl->maxData.CPU = max(fxhl->maxData.CPU, cData->CPU);

			if (conf->lightsNoDelay) { // update lights
				if (!src->grid)
					fxhl->RefreshCounters(cData);
				memcpy(&fxhl->eData, cData, sizeof(LightEventData));
			}
	}
}