#pragma once
#include <mutex>
#include "SysMonHelper.h"
#include "CaptureHelper.h"
#include "GridHelper.h"
#include "WSAudioIn.h"

class EventHandler
{
private:
	HWINEVENTHOOK hEvent = NULL, cEvent;
	HHOOK kEvent;
	DWORD maxProcess = 256;
	DWORD* aProcesses;
	string GetProcessName(DWORD proc);

public:
	CaptureHelper* capt = NULL;
	GridHelper* grid = NULL;
	WSAudioIn* audio = NULL;
	SysMonHelper* sysmon = NULL;

	bool keyboardSwitchActive = false;

	mutex modifyProfile;

	void ChangePowerState();

	// Profiles
	void SwitchActiveProfile(profile* newID);
	void StartProfiles();
	void StopProfiles();
	void ScanTaskList();
	void CheckProfileWindow();

	// Effects
	void ChangeEffectMode();
	void StopEffects();
	void StartEffects();

	EventHandler();
	~EventHandler();
};
