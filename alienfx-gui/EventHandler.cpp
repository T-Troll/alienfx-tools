#include "EventHandler.h"
#include "MonHelper.h"
#include "SysMonHelper.h"
#include "CaptureHelper.h"
#include "GridHelper.h"
#include "WSAudioIn.h"
#include "FXHelper.h"
#include <Psapi.h>

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

extern EventHandler* eve;
extern FXHelper* fxhl;
extern MonHelper* mon;
extern ConfigHandler* conf;
extern ConfigFan* fan_conf;
extern ThreadHelper* dxgi_thread;

void CEventProc(LPVOID);
VOID CALLBACK CForegroundProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
VOID CALLBACK CCreateProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam);

EventHandler::EventHandler()
{
	eve = this;
	aProcesses = new DWORD[maxProcess >> 2];

	// Check display...
	DEVMODE current;
	current.dmSize = sizeof(DEVMODE);
	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &current))
		currentFreq = current.dmDisplayFrequency;

	ChangePowerState();
	SwitchActiveProfile(conf->activeProfile, true);
	if (conf->startMinimized)
		StartProfiles();
	acStop = CreateEvent(NULL, false, false, NULL);
	ChangeAction();
}

EventHandler::~EventHandler()
{
	//StopProfiles();
	//ChangeAction(false);
	SetDisplayFreq(currentFreq);
	delete aProcesses;
	CloseHandle(acStop);
}

void EventHandler::ChangePowerState()
{
	SYSTEM_POWER_STATUS state;
	GetSystemPowerStatus(&state);
	if (conf->statePower != (bool)state.ACLineStatus) {
		conf->statePower = fan_conf->acPower = state.ACLineStatus;	 
		DebugPrint("Power state changed!\n");
		if (conf->enableProfSwitch && hEvent) {
			CheckProfileChange();
			SwitchActiveProfile(conf->activeProfile, true);
		} else
			SwitchActiveProfile(conf->SamePower(conf->activeProfile) ? conf->activeProfile : NULL, true);
	}
}

void EventHandler::SetDisplayFreq(int freq) {
	DEVMODE params;
	params.dmSize = sizeof(DEVMODE);
	params.dmFields = DM_DISPLAYFREQUENCY;
	if (params.dmDisplayFrequency = !conf->statePower && conf->dcFreq ? conf->dcFreq :
		freq ? freq : currentFreq)
		ChangeDisplaySettings(&params, CDS_UPDATEREGISTRY);
}

void EventHandler::SwitchActiveProfile(profile* newID, bool force)
{
	if (!newID) newID = conf->FindDefaultProfile();
	if (!keyboardSwitchActive && (force || newID != conf->activeProfile)) {
		//fxhl->UpdateGlobalEffect(NULL, true);
		conf->modifyProfile.lockWrite();
		conf->activeProfile = newID;
		fan_conf->lastProf = newID->flags & PROF_FANS ? (fan_profile*)newID->fansets : &fan_conf->prof;
		conf->modifyProfile.unlockWrite();
		fxhl->UpdateGlobalEffect(NULL, true);
		ToggleFans();
		ChangeEffectMode(true);

		if (newID->flags & PROF_RUN_SCRIPT && !(newID->flags & PROF_ACTIVE) && newID->script.size())
			ShellExecute(NULL, NULL, newID->script.c_str(), NULL, NULL, SW_SHOWDEFAULT);

		// Freqs
		SetDisplayFreq(newID->freqMode);

		DebugPrint("Profile switched to " + to_string(newID->id) + " (" + newID->name + ")\n");
	}
#ifdef _DEBUG
	else
		DebugPrint("Same profile \"" + newID->name + "\", skipping switch.\n");
#endif
}

void EventHandler::ToggleFans() {
	if (mon) {
		//mon->SetCurrentMode();
		if (conf->fansOnBattery || conf->statePower)
			mon->Start();
		else
			mon->Stop();
	}
}

void EventHandler::ChangeEffectMode(bool profile) {
	fxhl->SetState();
	bool oldgrid = grid;
	ChangeEffects();
	if (profile && oldgrid && grid)
		((GridHelper*)grid)->RestartWatch();
}

void EventHandler::ChangeEffects(bool stop) {
	bool noMon = true, noAmb = true, noHap = true, noGrid = true;
	// Effects state...
	conf->stateEffects = conf->stateOn && conf->enableEffects && (conf->effectsOnBattery || conf->statePower) && conf->activeProfile->effmode;
	if (!stop && conf->stateEffects) {
		for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++) {
			noMon = noMon && it->events.empty();
			noAmb = noAmb && it->ambients.empty();
			noHap = noHap && it->haptics.empty();
			noGrid = noGrid && !it->effect.trigger;
		}
		if (!(noMon || sysmon))
			sysmon = new SysMonHelper();
		if (!(noAmb || capt))
			capt = new CaptureHelper(true);
		if (!(noHap || audio))
			audio = new WSAudioIn();
		if (!(noGrid || grid))
			grid = new GridHelper();
	}
	DebugPrint(string("Profile state: ") + (noGrid ? "" : "Grid") +
		(noMon ? "" : ", Mon") +
		(noAmb ? "" : ", Amb") +
		(noHap ? "\n" : ", Hap\n"));
	if (noGrid && grid) {	// Grid
		delete (GridHelper*)grid; grid = NULL;
	}
	if (noMon && sysmon) {	// System monitoring
		delete (SysMonHelper*)sysmon; sysmon = NULL;
	}
	if ((noAmb || !dxgi_thread) && capt) {	// Ambient
		delete (CaptureHelper*)capt; capt = NULL;
	}
	if (noHap && audio) {	// Haptics
		delete (WSAudioIn*)audio; audio = NULL;
	}
	fxhl->Refresh();
}

string EventHandler::GetProcessName(DWORD proc) {
	char szProcessName[MAX_PATH]{ 0 };
	HANDLE hProcess;
	if (hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, proc)) {
		if (GetProcessImageFileName(hProcess, szProcessName, MAX_PATH))
			PathStripPath(szProcessName);
		CloseHandle(hProcess);
	}
	return szProcessName;
}

static const string forbiddenApps[] = { "ShellExperienceHost.exe"
									,"explorer.exe"
									,"SearchApp.exe"
									,"StartMenuExperienceHost.exe"
//									,"alienfx-gui.exe"
#ifdef _DEBUG
									,"devenv.exe"
#endif
									, ""
									};

void EventHandler::CheckProfileChange(bool isRun) {

	DWORD prcId;

	GetWindowThreadProcessId(GetForegroundWindow(), &prcId);
	string procName = GetProcessName(prcId);

	profile* newProf;

	//if (procName.empty() && isRun)
	//	return;
	if (conf->noDesktop && isRun) {
		if (procName.empty())
			return;
		for (int i = 0; forbiddenApps[i].size(); i++)
			if (forbiddenApps[i] == procName)
				return;
	}

	DebugPrint("Foreground switched to " + procName + "\n");
	if ((newProf = conf->FindProfileByApp(procName, true)) &&
		(conf->IsPriorityProfile(newProf) || !conf->IsPriorityProfile(conf->activeProfile))) {
		SwitchActiveProfile(newProf);
		return;
	}
#ifdef _DEBUG
	else {
		if (newProf) {
			DebugPrint("Blocked by priority\n");
		}
		else
			DebugPrint("No foreground profile\n");

	}
#endif
	DebugPrint("TaskScan initiated.\n");
	profile* finalP = NULL;
	DWORD cbNeeded;
	if (EnumProcesses(aProcesses, maxProcess, &cbNeeded)) {
		while (cbNeeded == maxProcess) {
			maxProcess = maxProcess << 1;
			delete[] aProcesses;
			aProcesses = new DWORD[maxProcess >> 2];
			EnumProcesses(aProcesses, maxProcess , &cbNeeded);
		}
		cbNeeded = cbNeeded >> 2;
		for (UINT i = 0; i < cbNeeded; i++) {
			if (aProcesses[i] && 
					(newProf = conf->FindProfileByApp(GetProcessName(aProcesses[i])))) {
				finalP = newProf;
				if (conf->IsPriorityProfile(newProf))
					break;
			}
		}
	}
	SwitchActiveProfile(finalP);
}

void EventHandler::StartProfiles()
{
	if (conf->enableProfSwitch && !hEvent) {
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
		kEvent = SetWindowsHookEx(WH_KEYBOARD_LL, KeyProc, NULL, 0);
#endif
		// Need to switch if already running....
		CheckProfileChange();
	}
}

void EventHandler::StopProfiles()
{
	if (hEvent) {
		DebugPrint("Profile hooks stop.\n");
		UnhookWinEvent(hEvent);
		UnhookWinEvent(cEvent);
		UnhookWindowsHookEx(kEvent);
		hEvent = NULL;
		keyboardSwitchActive = false;
	}
}

DWORD WINAPI acFunc(LPVOID lpParam) {
	DWORD res = 0;
	HANDLE hArray[2]{ eve->acStop, eve->wasAction };
	while ((res = WaitForMultipleObjects(2, hArray, false, conf->actionTimeout * 1000)) != WAIT_OBJECT_0) {
		switch (res) {
			case WAIT_OBJECT_0 + 1: // was action
				if (!fxhl->stateAction) {
					fxhl->stateAction = true;
					fxhl->SetState();
				}
				break;
			case WAIT_TIMEOUT: // timeout expired
				if (fxhl->stateAction) {
					fxhl->stateAction = false;
					fxhl->SetState();
				}
				break;
		}
	}
	fxhl->stateAction = true;
	fxhl->SetState();
	return 0;
}

LRESULT CALLBACK acProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0)
		SetEvent(eve->wasAction);
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void EventHandler::ChangeAction(bool run)
{
	if (ackEvent) {
		// stop hooks and waiting procedure
		SetEvent(acStop);
		UnhookWindowsHookEx(ackEvent);
		UnhookWindowsHookEx(acmEvent);
		ackEvent = NULL;
		CloseHandle(wasAction);
	}

	if (run && conf->actionLights) {
		// Set hooks and waiting procedure
		wasAction = CreateEvent(NULL, false, false, NULL);
		ackEvent = SetWindowsHookEx(WH_KEYBOARD_LL, acProc, NULL, 0);
		acmEvent = SetWindowsHookEx(WH_MOUSE_LL, acProc, NULL, 0);
		acThread = CreateThread(NULL, 0, acFunc, this, 0, NULL);
	}
}

// Create/destroy callback - switch profile if new/closed process in app list
static VOID CALLBACK CCreateProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {

	DWORD prcId;
	profile* prof = NULL;

	GetWindowThreadProcessId(hwnd, &prcId);
	string szProcessName = eve->GetProcessName(prcId);

	if (idChild == CHILDID_SELF && szProcessName.size()) {
		//DebugPrint("C/D: " + szProcessName + ", child is " + to_string(idChild == CHILDID_SELF) + "\n");
		if (prof = conf->FindProfileByApp(szProcessName, true)) {
			if (dwEvent == EVENT_OBJECT_DESTROY && prof->id == conf->activeProfile->id) {
				// Wait for termination
				HANDLE hProcess;
				if (hProcess = OpenProcess(SYNCHRONIZE, FALSE, prcId)) {
					DebugPrint("C/D: Active profile app closed, delay activated.\n");
					WaitForSingleObject(hProcess, 500);
					CloseHandle(hProcess);
					DebugPrint("C/D: Quit wait over.\n");
				}
			}
			eve->CheckProfileChange(dwEvent != EVENT_OBJECT_DESTROY);
		}
	}
}

// Foreground app process change callback
static VOID CALLBACK CForegroundProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	eve->CheckProfileChange();
}

LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0)
		switch (wParam) {
		case WM_KEYDOWN: case WM_SYSKEYDOWN:
			if (!eve->keyboardSwitchActive) {
				for (auto prof = conf->profiles.begin(); prof != conf->profiles.end(); prof++)
					if (((LPKBDLLHOOKSTRUCT)lParam)->vkCode == ((*prof)->triggerkey & 0xff) && conf->SamePower(*prof)) {
						eve->SwitchActiveProfile(*prof);
						eve->keyboardSwitchActive = true;
						break;
					}
			}
			break;
		case WM_KEYUP: case WM_SYSKEYUP:
			if (eve->keyboardSwitchActive && ((LPKBDLLHOOKSTRUCT)lParam)->vkCode == (conf->activeProfile->triggerkey & 0xff)) {
				eve->keyboardSwitchActive = false;
				eve->CheckProfileChange();
			}
			break;
		}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
