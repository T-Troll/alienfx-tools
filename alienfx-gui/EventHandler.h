#pragma once
#include "ConfigHandler.h"

class EventHandler
{
private:
	HWINEVENTHOOK hEvent = NULL, cEvent;
	HHOOK kEvent, ackEvent = NULL, acmEvent;
	DWORD maxProcess = 1024;
	DWORD* aProcesses = NULL;

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
	void ToggleProfiles();
	void CheckProfileChange();

	// Effects
	void ChangeEffectMode();
	void ChangeEffects(bool stop = false);

	EventHandler();
	~EventHandler();
};
