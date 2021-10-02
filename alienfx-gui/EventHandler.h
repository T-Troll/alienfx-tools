#pragma once
#include "ConfigHandler.h"
#include "CaptureHelper.h"
#include "WSAudioIn.h"
#include "..\\alienfan-tools\alienfan-gui\MonHelper.h"
#include "FXHelper.h"
class EventHandler
{
private:
	HANDLE dwHandle = 0;

	HWINEVENTHOOK hEvent = 0, cEvent = 0;

	void StartEvents();
	void StopEvents();

public:
	void ChangePowerState();
	void ChangeScreenState(DWORD state);
	void SwitchActiveProfile(int newID);
	void StartProfiles();
	void StopProfiles();
	void ToggleEvents();
	void ChangeEffectMode(int);
	void StopEffects();
	void StartEffects(bool force = false);

	EventHandler(ConfigHandler*, MonHelper*, FXHelper*);
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
