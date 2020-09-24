#include "EventHandler.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

void EventHandler::ChangePowerState()
{
	SYSTEM_POWER_STATUS state;
	GetSystemPowerStatus(&state);
    for (int i = 0; i < conf->mappings.size(); i++) {
        for (int j = 0; j < conf->mappings[i].events.size(); j++) {
            event evt = conf->mappings[i].events[j];
            if (evt.type == 1) // power...
            if (state.ACLineStatus) {
                // AC line
                switch (state.BatteryFlag) {
                case 8: // charging
                    //fxh->SetLight(&conf->mappings[i], AlienFX_SDK::AlienFX_Morph, conf->mappings[i].events[j].color);
                    break;
                default:
                    fxh->UpdateLight(&conf->mappings[i]);
                    break;
                }
            }
            else {
                // Battery - check BatteryFlag for details
                switch (state.BatteryFlag) {
                case 1: // ok
                    break;
                case 2: case 4: // low/critical
                    break;
                }
            }
        }
    }

}

void EventHandler::StartEvents()
{
}

void EventHandler::StopEvents()
{
}

EventHandler::EventHandler(ConfigHandler* config, FXHelper* fx)
{
    conf = config;
    fxh = fx;
}

EventHandler::~EventHandler()
{
}
