
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")

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
    
    LPCTSTR COUNTER_PATH_CPU = "\\Processor(_Total)\\% Processor Time",
        COUNTER_PATH_NET = "\\Network Interface(*)\\Bytes Total/sec",
        COUNTER_PATH_RAM = "\\Memory\\Avaliable MBytes",
        COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Disk Time";
    ULONG SAMPLE_INTERVAL_MS = 1000;

    HQUERY hQuery = NULL;
    HLOG hLog = NULL;
    PDH_STATUS pdhStatus;
    DWORD dwLogType = PDH_LOG_TYPE_CSV;
    HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hGPUCounter;

    MEMORYSTATUSEX memStat;

    memStat.dwLength = sizeof(MEMORYSTATUSEX);

    // Open a query object.
    pdhStatus = PdhOpenQuery(NULL, 0, &hQuery);

    if (pdhStatus != ERROR_SUCCESS)
    {
        wprintf(L"PdhOpenQuery failed with 0x%x\n", pdhStatus);
        goto cleanup;
    }

    // Add one counter that will provide the data.
    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_CPU,
        0,
        &hCPUCounter);

    if (pdhStatus != ERROR_SUCCESS)
    {
        wprintf(L"PdhAddCounter failed with 0x%x\n", pdhStatus);
        goto cleanup;
    }

    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_HDD,
        0,
        &hHDDCounter);

    if (pdhStatus != ERROR_SUCCESS)
    {
        wprintf(L"PdhAddCounter failed with 0x%x\n", pdhStatus);
        goto cleanup;
    }

    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_NET,
        0,
        &hNETCounter);

    if (pdhStatus != ERROR_SUCCESS)
    {
        wprintf(L"PdhAddCounter failed with 0x%x\n", pdhStatus);
        goto cleanup;
    }

    while (!src->stop) {
        // get indicators...
        PdhCollectQueryData(hQuery);
        PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal, cNETVal;
        DWORD cType = 0;
        pdhStatus = PdhGetFormattedCounterValue(
            hCPUCounter,
            PDH_FMT_LONG,
            &cType,
            &cCPUVal
        );
        pdhStatus = PdhGetFormattedCounterValue(
            hHDDCounter,
            PDH_FMT_LONG,
            &cType,
            &cHDDVal
        );

        pdhStatus = PdhGetFormattedCounterValue(
            hNETCounter,
            PDH_FMT_LONG,
            &cType,
            &cNETVal
        );

        if (pdhStatus == PDH_INVALID_DATA) {
            cNETVal.longValue = 0;
        }

        GlobalMemoryStatusEx(&memStat);

        //char buff[2048];
        //sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d\n", cCPUVal.longValue, memStat.dwMemoryLoad, cHDDVal.longValue, cNETVal.longValue);
        //OutputDebugString(buff);

        src->fxh->SetCounterColor(cCPUVal.longValue, memStat.dwMemoryLoad, 0, 0, cHDDVal.longValue);
        // check events...
        Sleep(200);

        // DEBUG!
        //src->stop = true;
    }

cleanup:

    if (hQuery)
        PdhCloseQuery(hQuery);

    return 0;
}