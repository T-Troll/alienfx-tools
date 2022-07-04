#pragma once
#include <vector>
#include <string>
#include "AlienFX_SDK.h"
#include "ConfigFan.h"

// Profile flags
#define PROF_DEFAULT		0x1
#define PROF_PRIORITY		0x2
#define PROF_DIMMED			0x4
#define PROF_ACTIVE			0x8
#define PROF_FANS			0x10
#define PROF_GLOBAL_EFFECTS 0x20

#define PROF_TRIGGER_AC		 0x1
#define PROF_TRIGGER_BATTERY 0x2

#define LEVENT_COLOR	0x1
#define LEVENT_POWER	0x2
#define LEVENT_PERF		0x4
#define LEVENT_ACT		0x8

#define GAUGE_GRADIENT	0x1
#define GAUGE_REVERSE	0x2

#define GE_FLAG_CIRCLE	0x1
#define GE_FLAG_ZONE	0x2

struct freq_map {
	AlienFX_SDK::Colorcode colorfrom{ 0 };
	AlienFX_SDK::Colorcode colorto{ 0 };
	byte hicut{ 255 };
	byte lowcut{ 0 };
	byte freqsize{ 0 };
	vector<byte> freqID;
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

struct zonelight {
	DWORD light;
	byte x, y;
};

struct zonemap {
	DWORD gID;
	DWORD gridID;
	byte xMax, yMax, gMinX, gMaxX, gMinY, gMaxY;
	vector<zonelight> lightMap;
};

struct grideffect {
	// static info
	byte trigger = 0;
	byte type = 0;
	byte speed = 80;
	byte size = 0;
	byte width = 1;
	WORD flags;
	AlienFX_SDK::Colorcode from;
	AlienFX_SDK::Colorcode to;
};

struct grideffop {
	// operational info
	bool passive = true;
	byte gridX, gridY, phase, oldphase;
	UINT start_tact;
};

struct groupset {
	int group = 0;
	vector<AlienFX_SDK::afx_act> color;
	event events[3];
	vector<byte> ambients;
	vector<freq_map> haptics;
	grideffect effect;
	grideffop  gridop;
	bool fromColor = false;
	WORD flags = 0;
	byte gauge = 0;
};

struct profile {
	unsigned id = 0;
	WORD flags = 0;
	WORD effmode = 0;
	vector<string> triggerapp;
	string name;
	WORD triggerkey = 0;
	WORD triggerFlags;
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
	//void updateProfileByID(unsigned id, std::string name, std::string app, DWORD flags, DWORD tFlags, DWORD* eff);
	//void updateProfileFansByID(unsigned id, unsigned senID, fan_block* temp, DWORD flags);
	groupset* FindCreateGroupSet(int profID, int groupID);
	profile* FindCreateProfile(unsigned id);
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
	//bool haveOldConfig = false;
	bool showGridNames = false;

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
	vector<zonemap> zoneMaps;
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
	void SortAllGauge();
	zonemap* FindZoneMap(int gid);
	void SortGroupGauge(int gid);
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
