#pragma once
#include "ConfigHandler.h"
#include "FXHelper.h"
class EventHandler
{
private:
	ConfigHandler* conf = NULL;
	FXHelper* fxh = NULL;
	//byte power_state = 255;
	//byte batt_state = 255;
public:
	void ChangePowerState();
	void StartEvents();
	void StopEvents();

	EventHandler(ConfigHandler* config, FXHelper* fx);
	~EventHandler();

	bool stop = false;
};

