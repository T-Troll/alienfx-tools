#pragma once
#include "ConfigHandler.h"
#include "CaptureHelper.h"
#include "GridHelper.h"
#include "WSAudioIn.h"
#include "MonHelper.h"

class EventHandler
{
private:
	HANDLE dwHandle = 0;

	HWINEVENTHOOK hEvent, cEvent = 0;
	HHOOK kEvent;

public:
	int effMode = 0;

	void ChangePowerState();
	void ChangeScreenState(DWORD state = 1);
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

	void ScanTaskList();

	void CheckProfileWindow(HWND hwnd);

	EventHandler();
	~EventHandler();

	CaptureHelper *capt = NULL;
	GridHelper *grid = NULL;
	WSAudioIn *audio = NULL;

	bool keyboardSwitchActive = false;

	HANDLE stopEvents = NULL;

	mutex modifyProfile;
};
