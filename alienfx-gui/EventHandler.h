#pragma once
#include "ConfigHandler.h"
#include "..\\alienfan-tools\alienfan-gui\MonHelper.h"
#include "FXHelper.h"
class EventHandler
{
private:
	HANDLE dwHandle = 0;

	HWINEVENTHOOK hEvent = 0, cEvent = 0;

	ConfigHelper* fan_conf = NULL;

public:
	void ChangePowerState();
	void ChangeScreenState(DWORD state);
	void SwitchActiveProfile(int newID);
	void StartEvents();
	void StopEvents();
	void StartProfiles();
	void StopProfiles();
	void ToggleEvents();
	void SetFanMon(MonHelper *fmon);

	EventHandler(ConfigHandler*, ConfigHelper*, FXHelper*);
	~EventHandler();

	FXHelper* fxh = NULL;
	ConfigHandler* conf = NULL;
	MonHelper *fan_mon = NULL;

	HANDLE stopEvents = NULL;

	mutex modifyProfile;

	long maxNet = 1;
};
