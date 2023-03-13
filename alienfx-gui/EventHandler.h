#pragma once
#include "ConfigHandler.h"

class EventHandler
{
private:
	HWINEVENTHOOK hEvent = NULL, cEvent;
	HHOOK kEvent;
	DWORD maxProcess = 256;
	DWORD* aProcesses;

public:
	void* capt = NULL;
	void* grid = NULL;
	void* audio = NULL;
	void* sysmon = NULL;

	bool keyboardSwitchActive = false;

	mutex modifyProfile;

	void ChangePowerState();

	// Profiles
	void SwitchActiveProfile(profile* newID);
	void ToggleFans();
	void StartProfiles();
	void StopProfiles();
	string GetProcessName(DWORD proc);
	void CheckProfileChange();

	// Effects
	void ChangeEffectMode();
	void ChangeEffects(bool stop=false);

	EventHandler();
	~EventHandler();
};
