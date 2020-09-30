
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
        COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage",
        COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Disk Time";
    ULONG SAMPLE_INTERVAL_MS = 1000;

    HQUERY hQuery = NULL;
    HLOG hLog = NULL;
    PDH_STATUS pdhStatus;
    DWORD dwLogType = PDH_LOG_TYPE_CSV;
    HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hGPUCounter;

    MEMORYSTATUSEX memStat;
    memStat.dwLength = sizeof(MEMORYSTATUSEX);

    PDH_FMT_COUNTERVALUE_ITEM netArray[20] = { 0 };
    DWORD maxnet = 1;

    PDH_FMT_COUNTERVALUE_ITEM gpuArray[500] = { 0 };

    // Open a query object.
    pdhStatus = PdhOpenQuery(NULL, 0, &hQuery);

    if (pdhStatus != ERROR_SUCCESS)
    {
        //wprintf(L"PdhOpenQuery failed with 0x%x\n", pdhStatus);
        goto cleanup;
    }

    // Add one counter that will provide the data.
    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_CPU,
        0,
        &hCPUCounter);

    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_HDD,
        0,
        &hHDDCounter);

    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_NET,
        0,
        &hNETCounter);

    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_GPU,
        0,
        &hGPUCounter);

    while (!src->stop) {
        // get indicators...
        PdhCollectQueryData(hQuery);
        PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal, cNETVal, cGPUVal;
        DWORD cType = 0, netbSize = 20 * sizeof(PDH_FMT_COUNTERVALUE_ITEM), netCount = 0,
            gpubSize = 500 * sizeof(PDH_FMT_COUNTERVALUE_ITEM), gpuCount = 0;
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

        pdhStatus = PdhGetFormattedCounterArray(
            hNETCounter,
            PDH_FMT_LONG,
            &netbSize,
            &netCount,
            netArray
        );

        if (pdhStatus == PDH_INVALID_DATA) {
            netCount = 0;
        }

        pdhStatus = PdhGetFormattedCounterArray(
            hGPUCounter,
            PDH_FMT_LONG,
            &gpubSize,
            &gpuCount,
            gpuArray
        );

        if (pdhStatus == PDH_INVALID_DATA) {
            gpuCount = 0;
        }

        GlobalMemoryStatusEx(&memStat);

        // Normilizing net values...
        DWORD totalNet = 0;
        for (int i = 0; i < netCount; i++) {
            totalNet += netArray[i].FmtValue.longValue;
        }

        if (maxnet < totalNet) maxnet = totalNet;
        //if (maxnet / 4 > totalNet) maxnet /= 2; TODO: think about decay!
        totalNet = (totalNet * 100) / maxnet;

        // Getting maximum GPU load value...
        DWORD maxGPU = 0;
        for (int i = 0; i < gpuCount && gpuArray[i].szName != NULL; i++) {
            if (maxGPU < gpuArray[i].FmtValue.longValue) 
                maxGPU = gpuArray[i].FmtValue.longValue;
        }

        char buff[2048];
        sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d\n", cCPUVal.longValue, memStat.dwMemoryLoad, cHDDVal.longValue, totalNet, maxGPU);
        OutputDebugString(buff);

        src->fxh->SetCounterColor(cCPUVal.longValue, memStat.dwMemoryLoad, maxGPU, totalNet, cHDDVal.longValue);
        // check events...
        Sleep(200);
    }

cleanup:

    if (hQuery)
        PdhCloseQuery(hQuery);

    return 0;
}