#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#include "EventHandler.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

DWORD WINAPI CEventProc(LPVOID);

HANDLE dwHandle;
DWORD dwThreadID;

void EventHandler::ChangePowerState()
{
	SYSTEM_POWER_STATUS state;
	GetSystemPowerStatus(&state);
    if (state.ACLineStatus) {
        // AC line
        switch (state.BatteryFlag) {
        case 8: // charging
            fxh->SetMode(MODE_CHARGE);
            break;
        default:
            fxh->SetMode(MODE_AC);
            break;
        }
    }
    else {
        // Battery - check BatteryFlag for details
        switch (state.BatteryFlag) {
        case 1: // ok
            fxh->SetMode(MODE_BAT);
            break;
        case 2: case 4: // low/critical
            fxh->SetMode(MODE_LOW);
            break;
        }
    }
    fxh->Refresh();
}

void EventHandler::StartEvents()
{
    stop = false;
    // start threas with this as a param
    dwHandle = CreateThread(
        NULL,              // default security
        0,                 // default stack size
        CEventProc,        // name of the thread function
        this,
        0,                 // default startup flags
        &dwThreadID);
}

void EventHandler::StopEvents()
{
    stop = true;
    DWORD exitCode;
    GetExitCodeThread(dwHandle, &exitCode);
    while (exitCode == STILL_ACTIVE) {
        Sleep(50);
        GetExitCodeThread(dwHandle, &exitCode);
    }
    CloseHandle(dwHandle);
}

EventHandler::EventHandler(ConfigHandler* config, FXHelper* fx)
{
    conf = config;
    fxh = fx;
}

EventHandler::~EventHandler()
{
}

DWORD WINAPI CEventProc(LPVOID param)
{
    EventHandler* src = (EventHandler*)param;

    IWbemHiPerfEnum* pEnum = NULL;
    long lID;
    IWbemConfigureRefresher* pConfig;
    IWbemServices* pNameSpace;
    HRESULT hr;

    /*if (FAILED(hr = pConfig->AddEnum(
        pNameSpace,
        L"Win32_PerfFormattedData_PerfDisk_PhysicalDisk",
        0,
        NULL,
        &pEnum,
        &lID)))
    {
        pConfig->Release();
        return 1;
    }*/

    while (!src->stop) {
        // get indicators...
        /*if (FAILED(hr = pConfig->Refresh(0L)))
        {
            // can't refresh
        }*/
        // check events...
        Sleep(100);
    }
    //pConfig->Release();
    return 0;
}