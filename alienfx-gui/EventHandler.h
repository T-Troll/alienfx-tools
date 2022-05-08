#pragma once
#include "ConfigHandler.h"
#include "CaptureHelper.h"
#include "WSAudioIn.h"
#include "alienfan-SDK.h"
#include "MonHelper.h"

class EventHandler
{
private:
	HANDLE dwHandle = 0;

	HWINEVENTHOOK hEvent = 0, cEvent = 0;

	int effMode = 3;

	void StartEvents();
	void StopEvents();

public:
	void ChangePowerState();
	void ChangeScreenState(DWORD state);
	void SwitchActiveProfile(profile* newID);
	void StartProfiles();
	void StopProfiles();
	//void ToggleEvents();
	void ChangeEffectMode();
	void StopEffects();
	void StartEffects();

	void StartFanMon(AlienFan_SDK::Control* acpi);
	void StopFanMon();

	profile *ScanTaskList();

	void CheckProfileWindow(HWND hwnd);

	EventHandler(ConfigHandler*, FXHelper*);
	~EventHandler();

	FXHelper* fxh = NULL;
	ConfigHandler* conf = NULL;
	MonHelper *mon = NULL;
	CaptureHelper *capt = NULL;
	WSAudioIn *audio = NULL;

	HANDLE stopEvents = NULL;

	mutex modifyProfile;

	long maxNet = 1;
};
