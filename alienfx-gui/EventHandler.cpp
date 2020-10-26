
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
    fxh->RefreshState();
}

void EventHandler::ChangeScreenState(DWORD state)
{
    if (conf->offWithScreen) {
        conf->stateOn = state;
        if (state) {
            fxh->Refresh(true);
            StartEvents();
        }
        else
            StopEvents();
    }
}

void EventHandler::StartEvents()
{
    fxh->RefreshState();
    if (!dwHandle && conf->enableMon && conf->stateOn) {
        // start threas with this as a param
#ifdef _DEBUG
        OutputDebugString("Event thread start.\n");
#endif
        this->stop = false;
        dwHandle = CreateThread(
            NULL,              // default security
            0,                 // default stack size
            CEventProc,        // name of the thread function
            this,
            0,                 // default startup flags
            &dwThreadID);
    }
}

void EventHandler::StopEvents()
{
    DWORD exitCode;
    if (dwHandle) {
#ifdef _DEBUG
        OutputDebugString("Event thread stop.\n");
#endif
        this->stop = true; 
        GetExitCodeThread(dwHandle, &exitCode);
        while (exitCode == STILL_ACTIVE) {
            Sleep(50);
            GetExitCodeThread(dwHandle, &exitCode);
        }
        CloseHandle(dwHandle);
        dwHandle = 0;
    }
    fxh->Refresh(true);
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
        //COUNTER_PATH_RAM = "\\Memory\\Avaliable MBytes",
        COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage",
        COUNTER_PATH_HOT = "\\Thermal Zone Information(*)\\Temperature",
        COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Disk Time";

    HQUERY hQuery = NULL;
    HLOG hLog = NULL;
    PDH_STATUS pdhStatus;
    DWORD dwLogType = PDH_LOG_TYPE_CSV;
    HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hGPUCounter, hTempCounter;

    MEMORYSTATUSEX memStat;
    memStat.dwLength = sizeof(MEMORYSTATUSEX);

    SYSTEM_POWER_STATUS state;

    PDH_FMT_COUNTERVALUE_ITEM netArray[20] = { 0 };
    long maxnet = 1;

    //PDH_FMT_COUNTERVALUE_ITEM gpuArray[300] = { 0 };
    long maxgpuarray = 300;
    PDH_FMT_COUNTERVALUE_ITEM* gpuArray = new PDH_FMT_COUNTERVALUE_ITEM[maxgpuarray];

    PDH_FMT_COUNTERVALUE_ITEM tempArray[20] = { 0 };

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
    pdhStatus = PdhAddCounter(hQuery,
        COUNTER_PATH_HOT,
        0,
        &hTempCounter);

    while (!src->stop) {
        // wait a little...
        Sleep(200);
        // get indicators...
        PdhCollectQueryData(hQuery);
        PDH_FMT_COUNTERVALUE cCPUVal, cHDDVal;
        DWORD cType = 0, netbSize = 20 * sizeof(PDH_FMT_COUNTERVALUE_ITEM), netCount = 0,
            gpubSize = maxgpuarray * sizeof(PDH_FMT_COUNTERVALUE_ITEM), gpuCount = 0,
            tempbSize = 20 * sizeof(PDH_FMT_COUNTERVALUE_ITEM), tempCount = 0;
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

        if (pdhStatus != ERROR_SUCCESS) {
            netCount = 0;
        }

        //memset(gpuArray, 0, gpubSize-1);

        pdhStatus = PdhGetFormattedCounterArray(
            hGPUCounter,
            PDH_FMT_LONG,
            &gpubSize,
            &gpuCount,
            gpuArray
        );

        if (pdhStatus != ERROR_SUCCESS) {
            maxgpuarray = gpubSize / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
            delete gpuArray;
            gpuArray = new PDH_FMT_COUNTERVALUE_ITEM[maxgpuarray];
            gpuCount = 0;
        }

        pdhStatus = PdhGetFormattedCounterArray(
            hTempCounter,
            PDH_FMT_LONG,
            &tempbSize,
            &tempCount,
            tempArray
        );

        if (pdhStatus != ERROR_SUCCESS) {
            tempCount = 0;
        }

        GlobalMemoryStatusEx(&memStat);
        GetSystemPowerStatus(&state);

        // Normilizing net values...
        long totalNet = 0;
        for (unsigned i = 0; i < netCount; i++) {
            totalNet += netArray[i].FmtValue.longValue;
        }

        if (maxnet < totalNet) maxnet = totalNet;
        //if (maxnet / 4 > totalNet) maxnet /= 2; TODO: think about decay!
        totalNet = (totalNet * 100) / maxnet;

        // Getting maximum GPU load value...
        long maxGPU = 0;
        for (unsigned i = 0; i < gpuCount && gpuArray[i].szName != NULL; i++) {
            if (maxGPU < gpuArray[i].FmtValue.longValue) 
                maxGPU = gpuArray[i].FmtValue.longValue;
        }

        // Getting maximum temp...
        long maxTemp = 0;
        for (unsigned i = 0; i < tempCount; i++) {
            if (maxTemp < tempArray[i].FmtValue.longValue)
                maxTemp = tempArray[i].FmtValue.longValue;
        }

        if (state.BatteryLifePercent > 100) state.BatteryLifePercent = 100;

        src->fxh->SetCounterColor(cCPUVal.longValue, memStat.dwMemoryLoad, maxGPU, totalNet, cHDDVal.longValue, maxTemp, state.BatteryLifePercent);
        
    }

cleanup:

    if (hQuery)
        PdhCloseQuery(hQuery);

    return 0;
}