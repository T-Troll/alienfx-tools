#pragma once
#include "ConfigHandler.h"
#include "CaptureHelper.h"
#include "GridHelper.h"
#include "WSAudioIn.h"
#include "alienfan-SDK.h"
#include "MonHelper.h"

class EventHandler
{
private:
	HANDLE dwHandle = 0;

	HWINEVENTHOOK hEvent, cEvent = 0;
	HHOOK kEvent;

	int effMode = 0;

public:
	void ChangePowerState();
	void ChangeScreenState(DWORD state);
	void SwitchActiveProfile(profile* newID);

	void StartProfiles();
	void StopProfiles();

	void ChangeEffectMode();
	void StopEffects();
	void StartEffects();

	void StartEvents();
	void StopEvents();

	void StartFanMon();
	void StopFanMon();

	profile *ScanTaskList();

	void CheckProfileWindow(HWND hwnd);

	EventHandler();
	~EventHandler();

	MonHelper *mon = NULL;
	CaptureHelper *capt = NULL;
	GridHelper *grid = NULL;
	WSAudioIn *audio = NULL;

	HANDLE stopEvents = NULL;

	mutex modifyProfile;

	//long maxNet = 1;
};
