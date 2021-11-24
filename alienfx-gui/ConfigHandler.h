#pragma once
#include <vector>
#include <string>
#include "AlienFX_SDK.h"
#include "..\alienfan-tools\alienfan-gui\ConfigHelper.h"
#include "ConfigAmbient.h"
#include "ConfigHaptics.h"

// Profile flags pattern
#define PROF_DEFAULT  0x1
#define PROF_PRIORITY 0x2
#define PROF_DIMMED   0x4
#define PROF_ACTIVE   0x8
#define PROF_FANS     0x10

union FlagSet {
	struct {
		BYTE flags;
		BYTE cut;
		BYTE proc;
		BYTE reserved;
	} b;
	DWORD s = 0;
};

struct event {
	FlagSet fs;
	BYTE source = 0;
	std::vector<AlienFX_SDK::afx_act> map;
};

struct lightset {
	union {
		struct {
			WORD pid;
			WORD vid;
		};
		DWORD devid = 0;
	};
	//unsigned devid = 0;
	unsigned lightid = 0;
	event	 eve[4];
};

struct profile {
	unsigned id = 0;
	WORD flags = 0;
	WORD effmode = 0;
	vector<string> triggerapp;
	string name;
	vector<lightset> lightsets;
	fan_profile fansets;
	bool ignoreDimming;
};

class ConfigHandler
{
private:
	HKEY hKey1 = NULL, 
		//hKey2 = NULL, 
		hKey3 = NULL, 
		hKey4 = NULL;
	bool conf_loaded = false;
	void GetReg(char *, DWORD *, DWORD def = 0);
	void SetReg(char *text, DWORD value);
	void updateProfileByID(unsigned id, std::string name, std::string app, DWORD flags);
	void updateProfileFansByID(unsigned id, unsigned senID, fan_block* temp, DWORD flags);
public:
	DWORD startWindows = 0;
	DWORD startMinimized = 0;
	//DWORD autoRefresh = 0;
	DWORD lightsOn = 1;
	DWORD dimmed = 0;
	DWORD offWithScreen = 0;
	DWORD dimmedBatt = 1;
	DWORD dimPowerButton = 0;
	DWORD dimmingPower = 92;
	DWORD enableProf = 0;
	DWORD offPowerButton = 0;
	DWORD activeProfile = -1;
	DWORD defaultProfile = 0;
	DWORD foregroundProfile = -1;
	DWORD awcc_disable = 0;
	DWORD esif_temp = 0;
	DWORD block_power = 0;
	DWORD gammaCorrection = 1;
	bool stateDimmed = false, stateOn = true, statePower = true, dimmedScreen = false, stateScreen = true;
	DWORD monDelay = 200;
	bool wasAWCC = false;
	Colorcode testColor, effColor1, effColor2;
	COLORREF customColors[16]{0};
	DWORD globalEffect = 0;
	DWORD globalDelay = 127;
	DWORD fanControl = 0;
	DWORD enableMon = 1;
	DWORD noDesktop = 0;

	// final states
	byte finalBrightness = 255;
	byte finalPBState = false;

	// local flags...
	bool haveV5 = false;

	// 3rd-party config blocks
	ConfigHelper *fan_conf = NULL;
	ConfigAmbient *amb_conf = NULL;
	ConfigHaptics *hap_conf = NULL;

	std::vector<lightset>* active_set;
	std::vector<profile> profiles;

	NOTIFYICONDATA niData{0};

	ConfigHandler();
	~ConfigHandler();
	int Load();
	int Save();
	//static bool sortMappings(lightset i, lightset j);
	profile* FindProfile(int id);
	profile* FindProfileByApp(std::string appName, bool active = false);
	bool IsPriorityProfile(int id);
	void SetStates();
	void SetIconState();
	bool IsDimmed();
	void SetDimmed();
	int  GetEffect();
	void SetEffect(int);
};
