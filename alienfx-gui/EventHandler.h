#pragma once
#include "ConfigHandler.h"
#include "FXHelper.h"
class EventHandler
{

public:
	void ChangePowerState();
	void ChangeScreenState(DWORD state);
	profile* FindProfile(int id);
	void SwitchActiveProfile(int newID);
	void StartEvents();
	void StopEvents();
	void StartProfiles();
	void StopProfiles();
	void ToggleEvents();

	EventHandler(ConfigHandler* config, FXHelper* fx);
	~EventHandler();

	bool stop = false;
	bool stopProf = false;
	FXHelper* fxh = NULL;
	ConfigHandler* conf = NULL;

	long maxNet = 1;
};

