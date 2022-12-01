#pragma once
#include <vector>
#include <string>
#include <random>
#include "AlienFX_SDK.h"
#include "ConfigFan.h"

// Profile flags
#define PROF_DEFAULT		0x1
#define PROF_PRIORITY		0x2
#define PROF_DIMMED			0x4
#define PROF_ACTIVE			0x8
#define PROF_FANS			0x10
#define PROF_GLOBAL_EFFECTS 0x20

// Profile power triggers
#define PROF_TRIGGER_AC		 0x1
#define PROF_TRIGGER_BATTERY 0x2

// event types
#define LEVENT_COLOR	0x1
#define LEVENT_POWER	0x2
#define LEVENT_PERF		0x4
#define LEVENT_ACT		0x8

// Monitoring types
#define MON_TYPE_POWER	0
#define MON_TYPE_PERF	1
#define MON_TYPE_IND	2

// Gauge flags
#define GAUGE_GRADIENT	0x1
#define GAUGE_REVERSE	0x2

// Grid effect flags
#define GE_FLAG_CIRCLE	0x1
#define GE_FLAG_RANDOM	0x2
#define GE_FLAG_PHASE	0x4
#define GE_FLAG_BACK	0x8

struct freq_map {
	AlienFX_SDK::Afx_colorcode colorfrom;
	AlienFX_SDK::Afx_colorcode colorto;
	byte hicut, lowcut, freqsize;
	vector<byte> freqID;
};

struct event {
	byte state, source, cut, mode;
	AlienFX_SDK::Afx_action from, to;
	double coeff;
};

struct zonelight {
	DWORD light;
	byte x, y;
};

struct zonemap {
	DWORD gID;
	DWORD gridID;
	byte xMax=0, yMax=0, gMinX=255, gMaxX=0, gMinY=255, gMaxY=0;
	vector<zonelight> lightMap;
};

struct gridClr {
	AlienFX_SDK::Afx_action* first;
	AlienFX_SDK::Afx_action* last;
};

struct grideffect {
	// static info
	byte trigger = 0;
	byte type = 0;
	byte speed = 80;
	byte padding = 0;
	byte width = 1;
	WORD flags;
	vector<AlienFX_SDK::Afx_colorcode> effectColors;
	//AlienFX_SDK::Afx_colorcode from;
	//AlienFX_SDK::Afx_colorcode to;
};

struct grideffop {
	// operational info
	bool passive = true;
	int gridX, gridY, 
		//phase = -1, 
		oldphase=-1, 
		size;
	UINT start_tact;
};

struct groupset {
	int group = 0;
	vector<AlienFX_SDK::Afx_action> color;
	vector<event> events;
	vector<byte> ambients;
	vector<freq_map> haptics;
	grideffect effect;
	grideffop  gridop;
	bool fromColor = false;
	WORD flags = 0;
	byte gauge = 0;
};

struct deviceeffect {
	WORD vid, pid;
	AlienFX_SDK::Afx_colorcode effColor1, effColor2;
	byte globalEffect = 0,
		globalDelay = 5,
		globalMode = 1;
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
	vector<deviceeffect> effects;
	bool ignoreDimming;
};

class ConfigHandler
{
private:
	HKEY hKeyMain = NULL, hKeyZones = NULL, hKeyProfiles = NULL;
	void GetReg(char *, DWORD *, DWORD def = 0);
	void SetReg(char *text, DWORD value);
	DWORD GetRegData(HKEY key, int vindex, char* name, byte** data);
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
	DWORD showGridNames = 0;
	DWORD keyShortcuts = 1;
	COLORREF customColors[16]{ 0 };

	// States
	bool stateDimmed = false, stateOn = true, statePower = true, dimmedScreen = false, stateScreen = true;
	bool lightsNoDelay = true;
	bool block_power = 0;
	bool wasAWCC = false;
	AlienFX_SDK::Afx_colorcode testColor{0,255};
	//bool haveOldConfig = false;

	// Ambient...
	DWORD amb_mode = 0;
	DWORD amb_shift = 40;
	DWORD amb_grid = MAKELPARAM(4,3);

	// Haptics...
	DWORD hap_inpType = 0;

	// final states
	byte finalBrightness = 255;
	byte finalPBState = false;

	ConfigFan *fan_conf = NULL;

	// Profiles and zones
	vector<groupset>* active_set;
	vector<profile*> profiles;
	vector<zonemap> zoneMaps;
	profile* activeProfile = NULL;

	// Random
	mt19937 rnd;

	// Grid-related
	AlienFX_SDK::Afx_grid* mainGrid = NULL;
	gridClr* colorGrid = NULL;
	int gridTabSel = 0;

	// mapping block from SDK
	AlienFX_SDK::Mappings afx_dev;

	NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA), 0, 0, NIF_ICON | NIF_MESSAGE | NIF_TIP, WM_APP + 1};

	ConfigHandler();
	~ConfigHandler();
	void Load();
	bool SamePower(WORD flags, bool anyFit = false);
	void Save();
	//void SortAllGauge();
	zonemap* FindZoneMap(int gid);
	zonemap* SortGroupGauge(int gid);
	//vector<deviceeffect>::iterator FindDevEffect(profile* prof, AlienFX_SDK::afx_device* dev, int type);
	profile* FindProfile(int id);
	profile* FindDefaultProfile();
	profile* FindProfileByApp(std::string appName, bool active = false);
	bool IsPriorityProfile(profile* prof);
	bool SetStates();
	//void SetToolTip();
	void SetIconState();
	bool IsDimmed();
	void SetDimmed();
	int GetEffect();
};
