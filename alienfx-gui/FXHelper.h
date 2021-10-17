#pragma once
#include <queue>
#include "ConfigHandler.h"
//#include "toolkit.h"
#include <mutex>

// Power modes: AC = 0, Battery = 1, Charge = 2, Low Battery = 4
#define MODE_AC		0
#define MODE_BAT	1
#define MODE_CHARGE	2
#define MODE_LOW	4

struct LightQueryElement {
	int did = 0;
	int lid = 0;
	DWORD flags = 0;
	bool update = false;
	byte actsize;
	AlienFX_SDK::afx_act actions[10];
};

struct deviceQuery {
	int devID = 0;
	vector<pair<int, vector<AlienFX_SDK::afx_act>>> dev_query;
};

class FXHelper
{
private:
	int activeMode = -1;
	long lCPU = 0, lRAM = 0, lHDD = 0, lGPU = 0, lNET = 0, lTemp = 0, lBatt = 100, lFan = 0;
	bool blinkStage = false;
	HANDLE updateThread = NULL;
	bool updateLock = false;
	ConfigHandler* config;
public:

	AlienFX_SDK::Mappings afx_dev;
	std::vector<AlienFX_SDK::Functions * > devs;
	HANDLE stopQuery = NULL;
	HANDLE haveNewElement = NULL;
	deque<LightQueryElement> lightQuery;
	mutex modifyQuery;
	bool unblockUpdates = true;

	FXHelper() {
		config = NULL;
	};
	FXHelper(ConfigHandler* conf) {
		config = conf;
		afx_dev.LoadMappings();
		FillDevs(config->stateOn, config->offPowerButton);
	};
	~FXHelper() {
		if (devs.size() > 0) {
			for (int i = 0; i < devs.size(); i++)
				devs[i]->AlienFXClose();
			devs.clear();
		}
	};

	AlienFX_SDK::Functions* LocateDev(int pid) {
		for (int i = 0; i < devs.size(); i++)
			if (devs[i]->GetPID() == pid)
				return devs[i];
		return nullptr;
	};

	size_t FillDevs(bool state, bool power) {
		vector<pair<DWORD, DWORD>> devList = afx_dev.AlienFXEnumDevices();
		config->haveV5 = false;

		if (devs.size() > 0) {
			for (int i = 0; i < devs.size(); i++) {
				devs[i]->AlienFXClose();
				delete devs[i];
			}
			devs.clear();
		}
		for (int i = 0; i < devList.size(); i++) {
			AlienFX_SDK::Functions* dev = new AlienFX_SDK::Functions();
			int pid = dev->AlienFXInitialize(devList[i].first, devList[i].second);
			if (pid != -1) {
				devs.push_back(dev);
				dev->ToggleState(state?255:0, afx_dev.GetMappings(), power);
				if (dev->GetVersion() == 5) config->haveV5 = true;
			} else
				delete dev;
		}
		return devs.size();
	};

	size_t FillAllDevs(bool state, bool power, HANDLE acc);

	void Start();
	void Stop();
	int Refresh(bool force = false);
	bool RefreshOne(lightset* map, bool force = false, bool update = false);
	bool SetMode(int mode);
	void TestLight(int did, int id);
	void ResetPower(int did);
	void QueryUpdate(int did = -1, bool force = false);
	void SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, long cFan, bool force = false);
	bool SetLight(int did, int id, std::vector<AlienFX_SDK::afx_act> actions, bool force = false);
	void RefreshState(bool force = false);
	void RefreshMon();
	int RefreshAmbient(UCHAR *img);
	void RefreshHaptics(int *freq);
	void Flush();
	void ChangeState();
	void UpdateGlobalEffect(AlienFX_SDK::Functions* dev = NULL);
	void UnblockUpdates(bool newState, bool lock = false);

	ConfigHandler* GetConfig() { return config; };
};
