#pragma once

struct ARG {
	int num;
	string str;
};

struct COMMAND {
	const int id;
	const char* name,
		* desc;
	const byte minArgs;
};

struct ZONECODE {
	const char* name;
	unsigned code;
};

struct ACTIONCODE {
	const char* name;
	int afx_code;
	unsigned dell_code;
};

const ZONECODE zonecodes[]{
	{"left", LFX_ALL_LEFT },
	{"right", LFX_ALL_RIGHT},
	{"top", LFX_ALL_UPPER},
	{"bottom", LFX_ALL_LOWER},
	{"front", LFX_ALL_FRONT},
	{"rear", LFX_ALL_REAR}
};

const ACTIONCODE actioncodes[]{
	{"pulse", AlienFX_SDK::Action::AlienFX_A_Pulse, LFX_ACTION_PULSE },
	{"morph", AlienFX_SDK::Action::AlienFX_A_Morph, LFX_ACTION_MORPH },
	{"breath", AlienFX_SDK::Action::AlienFX_A_Breathing, LFX_ACTION_MORPH },
	{"spectrum", AlienFX_SDK::Action::AlienFX_A_Spectrum, LFX_ACTION_MORPH },
	{"rainbow", AlienFX_SDK::Action::AlienFX_A_Rainbow, LFX_ACTION_MORPH }
};

const COMMAND commands[]{
	{0,"set-all","\tr,g,b[,br] - set all lights",3},
	{1,"set-one","\tdev,light,r,g,b[,br] - set one light",5},
	{2,"set-zone","zone,r,g,b[,br] - set zone lights",4},
	{3,"set-action","dev,light,action,r,g,b[,br,[action,r,g,b,br]] - set light and enable it's action",6},
	{4,"set-zone-action","zone,action,r,g,b[,br,[action,r,g,b,br]] - set all zone lights and enable it's action",5},
	{5,"set-power","dev,light,r,g,b,r2,g2,b2 - set power button colors (low-level only)",8},
	{6,"set-tempo","tempo - set tempo for actions and pause",1},
	//{7,"set-dev","\tpid - set active device for low-level",1},
	{8,"set-dim","\tdev,br - set active device dimming level",2},
	{9,"set-global","mode,r,g,b,r,g,b - set global effect mode (v5 devices only)",7},
	{10,"low-level","switch to low-level SDK"},
	{11,"high-level","switch to high-level SDK (Alienware LightFX)"},
	{12,"probe","\t[ald[,lights][,devID[,lightID]] - probe lights to set names"},
	{13,"status","\tshows devices and lights id's, names and statuses"},
	{14,"lightson","turn all lights on"},
	{15,"lightsoff","turn all lights off"},
	{16,"reset","\treset current device state"},
	{17,"loop","\trepeat commands from start, until user press CTRL+c"}
};