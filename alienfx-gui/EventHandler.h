#pragma once
#include "ConfigHandler.h"

class EventHandler
{
private:
	HWINEVENTHOOK hEvent = NULL, cEvent;
	HHOOK kEvent, ackEvent = NULL, acmEvent;
	DWORD maxProcess = 1024;
	DWORD* aProcesses = NULL;
	DWORD currentFreq = 0;

public:
	void* capt = NULL;
	void* grid = NULL;
	void* audio = NULL;
	void* sysmon = NULL;

	bool keyboardSwitchActive = false;

	HANDLE wasAction, acThread, acStop;

	// Power and display
	void ChangePowerState();
	void SetDisplayFreq(int freq);

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
