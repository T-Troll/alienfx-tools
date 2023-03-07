#include "alienfx-gui.h"
#include "EventHandler.h"
#include "MonHelper.h"
#include "SysMonHelper.h"
#include "CaptureHelper.h"
#include "GridHelper.h"
#include "WSAudioIn.h"
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
extern ConfigFan* fan_conf;

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
	if (conf->startMinimized)
		StartProfiles();
}

EventHandler::~EventHandler()
{
	StopProfiles();
	ChangeEffects(true);
	delete[] aProcesses;
}

void EventHandler::ChangePowerState()
{
	SYSTEM_POWER_STATUS state;
	GetSystemPowerStatus(&state);
	if (conf->statePower != (bool)state.ACLineStatus) {
		conf->statePower = state.ACLineStatus;
		DebugPrint("Power state changed!\n");
		fxhl->SetState();
		if (cEvent)
			CheckProfileChange();
	}
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
		fan_conf->lastProf = newID->flags & PROF_FANS ? (fan_profile*)newID->fansets : &fan_conf->prof;
		modifyProfile.unlock();

		// deprecated, remove soon
		if (mon && fan_conf->lastProf->gmode_stage) {
			fan_conf->lastProf->gmode_stage = 0;
			fan_conf->lastProf->powerSet = (WORD)mon->acpi->powers.size();
		}

		ChangeEffectMode();

		DebugPrint("Profile switched to " + to_string(newID->id) + " (" + newID->name + ")\n");
	}
#ifdef _DEBUG
	else
		DebugPrint("Same profile \"" + newID->name + "\", skipping switch.\n");
#endif
}

void EventHandler::ChangeEffectMode() {
	fxhl->SetState();
	ChangeEffects();
}

void EventHandler::ChangeEffects(bool stop) {
	bool haveMon = false, haveAmb = false, haveHap = false, haveGrid = false;
	if (!stop && fxhl->stateEffects /*&& noLightFX*/) {
		for (auto it = conf->activeProfile->lightsets.begin(); it != conf->activeProfile->lightsets.end(); it++) {
			if (it->events.size() && !sysmon) {
				haveMon = true;
				sysmon = new SysMonHelper();
			}
			if (it->ambients.size() && !capt) {
				haveAmb = true;
				capt = new CaptureHelper(true);
			}
			if (it->haptics.size() && !audio) {
				haveHap = true;
				audio = new WSAudioIn();
			}
			if (it->effect.trigger) {
				haveGrid = true;
				if (!grid)
					grid = new GridHelper();
				else
					((GridHelper*)grid)->RestartWatch();
			}
		}
	}
	if (!haveMon && sysmon) {	// System monitoring
		delete (SysMonHelper*)sysmon; sysmon = NULL;
	}
	if (!haveAmb && capt) {		// Ambient
			delete (CaptureHelper*)capt; capt = NULL;
		}
	if (!haveHap && audio) {	// Haptics
			delete (WSAudioIn*)audio; audio = NULL;
		}
	if (!haveGrid && grid) {
		delete (GridHelper*)grid; grid = NULL;
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

void EventHandler::CheckProfileChange() {

	DWORD prcId;
	DWORD cbNeeded;

	GetWindowThreadProcessId(GetForegroundWindow(), &prcId);
	string procName = GetProcessName(prcId);

	profile* newProf;

	if (procName.size()) {
		DebugPrint("Foreground switched to " + procName + "\n");

		if (conf->noDesktop && conf->IsActiveOnly() && (procName == "ShellExperienceHost.exe" || procName == "explorer.exe" || procName == "SearchApp.exe"
#ifdef _DEBUG
			|| procName == "devenv.exe"
#endif
			)) {
			DebugPrint("Forbidden app name\n");
			return;
		}

		if ((newProf = conf->FindProfileByApp(procName, true)) &&
			(conf->IsPriorityProfile(newProf) || !conf->IsPriorityProfile())) {
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
	}
#ifdef _DEBUG
	else {
		DebugPrint("Incorrect active app name\n");
	}
#endif
	DebugPrint("TaskScan initiated.\n");
	profile* finalP = conf->FindDefaultProfile();
	if (EnumProcesses(aProcesses, maxProcess << 2, &cbNeeded)) {
		while (cbNeeded >> 2 == maxProcess) {
			maxProcess = maxProcess << 1;
			delete[] aProcesses;
			aProcesses = new DWORD[maxProcess];
			EnumProcesses(aProcesses, maxProcess << 2 , &cbNeeded);
		}

		for (UINT i = 0; i < cbNeeded >> 2; i++) {
			if (aProcesses[i] && (newProf = conf->FindProfileByApp(GetProcessName(aProcesses[i])))) {
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
		kEvent = SetWindowsHookExW(WH_KEYBOARD_LL, KeyProc, NULL, 0);
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

// Create/destroy callback - switch profile if new/closed process in app list
static VOID CALLBACK CCreateProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {

	string szProcessName;
	DWORD prcId;
	profile* prof = NULL;

	GetWindowThreadProcessId(hwnd, &prcId);

	if (!GetParent(hwnd) && (szProcessName = eve->GetProcessName(prcId)).size() &&
		(prof = conf->FindProfileByApp(szProcessName))) {
		//DebugPrint(("C/D: " + to_string(dwEvent) + " - " + szProcessName + "\n").c_str());
		if (dwEvent == EVENT_OBJECT_DESTROY) {
			if (prof->id == conf->activeProfile->id) {
				DebugPrint("C/D: Active profile app closed, delay activated.\n");
				// Wait for termination
				HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, prcId);
				WaitForSingleObject(hProcess, 1000);
				CloseHandle(hProcess);
				DebugPrint("C/D: Delay done.\n");
				eve->CheckProfileChange();
			}
		} else 
			eve->CheckProfileChange();
	}
}

// Foreground app process change callback
static VOID CALLBACK CForegroundProc(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	eve->CheckProfileChange();
}

LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	LRESULT res = CallNextHookEx(NULL, nCode, wParam, lParam);

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

	return res;
}
