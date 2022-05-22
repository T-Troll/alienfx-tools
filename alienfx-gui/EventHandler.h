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
	void ChangeEffectMode();
	void StopEffects();
	void StartEffects();

	void StartFanMon();
	void StopFanMon();

	profile *ScanTaskList();

	void CheckProfileWindow(HWND hwnd);

	EventHandler();
	~EventHandler();

	MonHelper *mon = NULL;
	CaptureHelper *capt = NULL;
	WSAudioIn *audio = NULL;

	HANDLE stopEvents = NULL;

	mutex modifyProfile;

	//long maxNet = 1;
};
