#pragma once
#include <vector>
#include <string>
#include <random>
#include "AlienFX_SDK.h"
#include "resource.h"
#include <ThreadHelper.h>
#include <map>

// Profile flags
#define PROF_DEFAULT		0x1
#define PROF_PRIORITY		0x2
#define PROF_DIMMED			0x4
#define PROF_ACTIVE			0x8
#define PROF_FANS			0x10
#define PROF_GLOBAL_EFFECTS 0x20
#define PROF_RUN_SCRIPT		0x40

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
#define GE_FLAG_RPOS	0x10

struct freq_map {
	AlienFX_SDK::Afx_colorcode colorfrom;
	AlienFX_SDK::Afx_colorcode colorto;
	byte hicut, lowcut, freqsize;
	vector<byte> freqID;
};

struct event {
	byte state, source, cut, mode;
	AlienFX_SDK::Afx_action from, to;
};

struct zonelight {
	DWORD light;
	byte x, y;
};

struct zonemap {
	//DWORD gID;
	byte gridID;
	byte xMax = 0, yMax = 0,
		scaleX = 1, scaleY = 1,
		gMinX = 255, gMaxX = 0, gMinY = 255, gMaxY = 0;
	vector<zonelight> lightMap;
	bool havePower = false;
};

struct gridClr {
	AlienFX_SDK::Afx_colorcode first;
	AlienFX_SDK::Afx_colorcode last;
};

struct grideffect {
	// static info
	byte trigger = 0;
	byte type = 0;
	byte speed = 80;
	byte numclr = 0;
	byte width = 1;
	WORD flags;
	vector<AlienFX_SDK::Afx_colorcode> effectColors;
};

struct starmap {
	DWORD lightID = 0;
	int colorIndex = 0;
	int count = 0, maxCount = 0;
};

struct grideffop {
	// operational info
	bool passive = true;
	//void* capt = NULL; // capture object if present in operation
	int gridX, gridY,
		oldphase,
		size,
		effsize,
		lmp,
		current_tact;
	vector<starmap> stars;
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
	WORD gaugeflags = 0;
	byte gauge = 0;
};

struct deviceeffect {
	WORD vid, pid;
	AlienFX_SDK::Afx_colorcode effColor1, effColor2;
	byte globalEffect=0, globalDelay=5, globalMode = 1, colorMode = 1;
	DWORD reserved;
};

struct profile {
	unsigned id = 0;
	string name;
	union {
		struct {
			WORD flags;
			WORD effmode;
		};
		DWORD gflags = 0;
	};
	union {
		struct {
			WORD triggerFlags;
			WORD triggerkey;
		};
		DWORD triggers = 0;
	};
	vector<string> triggerapp;
	vector<groupset> lightsets;
	void *fansets = NULL;
	vector<deviceeffect> effects;
	string script = "";
};

union ambgrid {
	struct {
		WORD x, y;
	};
	DWORD ag;
};

class ConfigHandler
{
private:
	HKEY hKeyMain = NULL, hKeyZones = NULL, hKeyProfiles = NULL;
	void GetReg(char *, DWORD *, DWORD def = 0);
	void SetReg(char *text, DWORD value);
	//DWORD GetRegData(HKEY key, int vindex, char* name, byte** data);
	groupset* FindCreateGroupSet(int profID, int groupID);
	profile* FindCreateProfile(unsigned id);
	uniform_int_distribution<WORD> rclr = uniform_int_distribution<WORD>(0x20, 0xff);
	CustomMutex zoneUpdate;
public:
	DWORD startWindows;
	DWORD startMinimized;
	DWORD updateCheck;
	DWORD lightsOn;
	DWORD dimmed;
	DWORD offWithScreen;
	DWORD dimPowerButton;
	DWORD dimmingPower;
	DWORD fullPower;
	DWORD enableProfSwitch;
	DWORD offPowerButton;
	DWORD awcc_disable;
	DWORD esif_temp;
	DWORD gammaCorrection;
	DWORD fanControl;
	DWORD enableEffects;
	DWORD noDesktop;
	DWORD showGridNames;
	DWORD keyShortcuts;
	DWORD geTact;
	// Battery related
	DWORD offOnBattery;
	DWORD dimmedBatt;
	DWORD effectsOnBattery;
	DWORD fansOnBattery;

	COLORREF customColors[16]{ 0 };

	// States
	bool stateDimmed = false,
		 stateOn = true,
		 stateEffects = true,
		 statePower = true,
		 wasAWCC = false;
	AlienFX_SDK::Afx_colorcode testColor{0,255};
	CustomMutex modifyProfile;

	// Ambient...
	DWORD amb_mode;
	DWORD amb_calc;
	DWORD amb_shift;
	ambgrid amb_grid;

	// Haptics...
	DWORD hap_inpType;

	// Profiles and zones
	vector<profile*> profiles;
	map<DWORD, zonemap> zoneMaps;
	profile* activeProfile = NULL;

	// Random
	mt19937 rnd;

	// Grid-related
	AlienFX_SDK::Afx_grid* mainGrid = NULL;
	int gridTabSel = 0;

	// mapping block from FX SDK
	AlienFX_SDK::Mappings afx_dev;

	NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA), 0, 0, NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP, WM_APP + 1 };

	ConfigHandler();
	~ConfigHandler();
	void Load();
	bool SamePower(profile* cur, profile* prof = NULL);
	void Save();
	groupset* FindMapping(int mid, vector<groupset>* set = NULL);
	void SetRandomColor(AlienFX_SDK::Afx_colorcode* clr);
	zonemap* FindZoneMap(int gid, bool reset=false);
	profile* FindProfile(int id);
	profile* FindDefaultProfile();
	profile* FindProfileByApp(std::string appName, bool active = false);
	AlienFX_SDK::Afx_group* FindCreateGroup(int groupID);
	bool IsPriorityProfile(profile* prof);
	//bool IsActiveOnly(profile* prof);
	void SetIconState();
};
