#pragma once
#include "ConfigHandler.h"
#include "FXHelper.h"
class EventHandler
{
private:
	ConfigHandler* conf = NULL;

public:
	void ChangePowerState();
	void ChangeScreenState(DWORD state);
	void StartEvents();
	void StopEvents();

	EventHandler(ConfigHandler* config, FXHelper* fx);
	~EventHandler();

	bool stop = false;
	FXHelper* fxh = NULL;

	long maxNet = 1;
};

