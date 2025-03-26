#pragma once
#include "ConfigHandler.h"

class EventHandler
{
private:
	HWINEVENTHOOK hEvent = NULL, cEvent;
	HHOOK kEvent, ackEvent = NULL, acmEvent;
	DWORD maxProcess = 256;
	DWORD* aProcesses = NULL;

public:
	void* capt = NULL;
	void* grid = NULL;
	void* audio = NULL;
	void* sysmon = NULL;

	bool keyboardSwitchActive = false;

	HANDLE wasAction, acThread, acStop;

	void ChangePowerState();

	// Profiles
	void SwitchActiveProfile(profile* newID, bool force = false);
	void ToggleFans();
	void StartProfiles();
	void StopProfiles();
	string GetProcessName(DWORD proc);
	void CheckProfileChange(bool isRun = true);

	// Effects
	void ChangeEffectMode(bool profile = false);
	void ChangeEffects(bool stop = false);

	// Timeout Action
	void ChangeAction(bool run = true);

	EventHandler();
	~EventHandler();
};
