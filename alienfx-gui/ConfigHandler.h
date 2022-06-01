#pragma once
#include <vector>
#include <string>
#include "AlienFX_SDK.h"
#include "ConfigFan.h"
//#include "ConfigAmbient.h"
//#include "ConfigHaptics.h"

// Profile flags pattern
#define PROF_DEFAULT  0x1
#define PROF_PRIORITY 0x2
#define PROF_DIMMED   0x4
#define PROF_ACTIVE   0x8
#define PROF_FANS     0x10
#define PROF_GLOBAL_EFFECTS 0x20

#define LEVENT_COLOR 0x1
#define LEVENT_POWER 0x2
#define LEVENT_PERF  0x4
#define LEVENT_ACT   0x8

struct freq_map {
	AlienFX_SDK::Colorcode colorfrom{ 0 };
	AlienFX_SDK::Colorcode colorto{ 0 };
	byte lowcut{ 0 };
	byte hicut{ 255 };
	vector<byte> freqID;
};

union FlagSet {
	struct {
		BYTE flags;
		BYTE cut;
		BYTE proc;
		BYTE reserved;
	};
	DWORD s = 0;
};

struct event {
	bool state = false;
	BYTE source = 0;
	BYTE cut = 0;
	BYTE mode = 0;
	AlienFX_SDK::afx_act from;
	AlienFX_SDK::afx_act to;
	double coeff;
};

struct old_event {
	bool active = false;
	BYTE source = 0;
	BYTE cut = 0;
	BYTE proc = 0;
	vector<AlienFX_SDK::afx_act> map;
};

//struct posgrid {
//	int x, y, index;
//};

struct groupset {
	int group = 0;
	vector<AlienFX_SDK::afx_act> color;
	event events[3];
	vector<byte> ambients;
	vector<freq_map> haptics;
	AlienFX_SDK::lightgrid lightMap{ 0 };
	bool fromColor = false;
	bool gradient = false;
	byte gauge = 0;
};

struct lightset {
	union {
		struct {
			WORD pid;
			WORD vid;
		};
		DWORD devid = 0;
	};
	unsigned lightid = 0;
	BYTE     flags = 0;
	old_event	 eve[4];
};

struct profile {
	unsigned id = 0;
	WORD flags = 0;
	WORD effmode = 0;
	vector<string> triggerapp;
	string name;
	vector<groupset> lightsets;
	fan_profile fansets;
	AlienFX_SDK::Colorcode effColor1, effColor2;
	byte globalEffect = 0,
         globalDelay = 5;
	bool ignoreDimming;
};

class ConfigHandler
{
private:
	HKEY hKeyMain = NULL, hKeyZones = NULL, hKeyProfiles = NULL;
	void GetReg(char *, DWORD *, DWORD def = 0);
	void SetReg(char *text, DWORD value);
	void updateProfileByID(unsigned id, std::string name, std::string app, DWORD flags, DWORD *eff);
	void updateProfileFansByID(unsigned id, unsigned senID, fan_block* temp, DWORD flags);
	//AlienFX_SDK::group* FindCreateGroup(int did, int lid);
	groupset* FindCreateGroupSet(int profID, int groupID);
public:
	DWORD startWindows = 0;
	DWORD startMinimized = 0;
	DWORD updateCheck = 1;
	DWORD lightsOn = 1;
	DWORD dimmed = 0;
	DWORD offWithScreen = 0;
	DWORD dimmedBatt = 1;
	DWORD dimPowerButton = 0;
	DWORD dimmingPower = 92;
	DWORD enableProf = 0;
	DWORD offPowerButton = 0;
	DWORD offOnBattery = 0;
	DWORD awcc_disable = 0;
	DWORD esif_temp = 0;
	DWORD gammaCorrection = 1;
	DWORD fanControl = 0;
	DWORD enableMon = 1;
	DWORD noDesktop = 0;
	COLORREF customColors[16]{ 0 };

	// States
	bool stateDimmed = false, stateOn = true, statePower = true, dimmedScreen = false, stateScreen = true;
	DWORD monDelay = 200;
	bool block_power = 0;
	bool wasAWCC = false;
	AlienFX_SDK::Colorcode testColor{0,255};
	bool haveOldConfig = false;

	// Ambient...
	DWORD amb_mode = 0;
	DWORD amb_shift = 40;
	DWORD amb_grid = MAKELPARAM(4,3);

	// Haptics...
	DWORD hap_inpType = 0;

	// final states
	byte finalBrightness = 255;
	byte finalPBState = false;

	// local flags...
	bool haveV5 = false;

	// 3rd-party config blocks
	ConfigFan *fan_conf = NULL;

	vector<groupset>* active_set;
	vector<profile*> profiles;
	profile* activeProfile = NULL;
	profile* foregroundProfile = NULL;

	// Grid-related
	AlienFX_SDK::lightgrid* mainGrid = NULL;
	pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*>* colorGrid = NULL;
	int gridTabSel = 0;

	// mapping block from SDK
	AlienFX_SDK::Mappings afx_dev;

	NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA), 0, 0, NIF_ICON | NIF_MESSAGE | NIF_TIP, WM_APP + 1};

	ConfigHandler();
	~ConfigHandler();
	void Load();
	void Save();
	void SortGroupGauge(groupset* map);
	profile* FindProfile(int id);
	profile* FindDefaultProfile();
	profile* FindProfileByApp(std::string appName, bool active = false);
	bool IsPriorityProfile(profile* prof);
	bool SetStates();
	void SetToolTip();
	void SetIconState();
	bool IsDimmed();
	void SetDimmed();
	int  GetEffect();
};
