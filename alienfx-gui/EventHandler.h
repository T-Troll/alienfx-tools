#pragma once
#include <pdh.h>
#include "ConfigHandler.h"
#include "CaptureHelper.h"
#include "GridHelper.h"
#include "WSAudioIn.h"
#include "MonHelper.h"

class EventHandler
{
private:
	HWINEVENTHOOK hEvent, cEvent = 0;
	HHOOK kEvent;
	DWORD maxProcess = 256;
	DWORD* aProcesses;

	LPCTSTR COUNTER_PATH_CPU = "\\Processor Information(_Total)\\% Processor Time",
		COUNTER_PATH_NET = "\\Network Interface(*)\\Bytes Total/sec",
		COUNTER_PATH_NETMAX = "\\Network Interface(*)\\Current BandWidth",
		COUNTER_PATH_GPU = "\\GPU Engine(*)\\Utilization Percentage",
		COUNTER_PATH_HOT = "\\Thermal Zone Information(*)\\Temperature",
		COUNTER_PATH_HOT2 = "\\EsifDeviceInformation(*)\\Temperature",
		COUNTER_PATH_PWR = "\\EsifDeviceInformation(*)\\RAPL Power",
		COUNTER_PATH_HDD = "\\PhysicalDisk(_Total)\\% Idle Time";

	ThreadHelper* eventProc = NULL;

public:
	int effMode = 0;

	CaptureHelper* capt = NULL;
	GridHelper* grid = NULL;
	WSAudioIn* audio = NULL;

	bool keyboardSwitchActive = false;

	//HANDLE stopEvents = NULL;

	mutex modifyProfile, monInUse;

	HQUERY hQuery = NULL;
	HCOUNTER hCPUCounter, hHDDCounter, hNETCounter, hNETMAXCounter, hGPUCounter, hTempCounter, hTempCounter2, hPwrCounter;

	EventData cData;

	void ChangePowerState();
	void ChangeScreenState(DWORD state = 1);
	void SwitchActiveProfile(profile* newID);

	void StartProfiles();
	void StopProfiles();

	void ChangeEffectMode();
	void StopEffects();
	void StartEffects();

	void StartEvents();
	void StopEvents();

	void StartFanMon();
	void StopFanMon();

	void ScanTaskList();

	void CheckProfileWindow();

	EventHandler();
	~EventHandler();
};
