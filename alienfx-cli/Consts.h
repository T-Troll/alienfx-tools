#pragma once

struct ARG {
	string str;
	int num;
};

struct COMMAND {
	const int id;
	const char* name, *desc;
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

enum COMMANDS {
	setall = 0,
	setone = 1,
	setzone = 2,
	setaction = 3,
	setzoneact = 4,
	setpower = 5,
	settempo = 6,
	setdim = 8,
	setglobal = 9,
	lowlevel = 10,
	highlevel = 11,
	probe = 12,
	status = 13,
	loop = 14
};

const COMMAND commands[]{
	{COMMANDS::setall,"setall","\tr,g,b - set all lights",3},
	{COMMANDS::setone,"setone","\tdev,light,r,g,b - set one light",5},
	{COMMANDS::setzone,"setzone","\tzone,r,g,b - set zone lights",4},
	{COMMANDS::setaction,"setaction","dev,light,action,r,g,b[,action,r,g,b] - set light and enable it's action",6},
	{COMMANDS::setzoneact,"setzoneaction","zone,action,r,g,b[,action,r,g,b] - set all zone lights and enable it's action",5},
	{COMMANDS::setpower,"setpower","dev,light,r,g,b,r2,g2,b2 - set power button colors (low-level only)",8},
	{COMMANDS::settempo,"settempo","tempo[,length] - set tempo and effect length for actions",1},
	{COMMANDS::setdim,"setdim","\t[dev,]br - set brightness level",1},
	{COMMANDS::setglobal,"setglobal","dev,type,mode[,r,g,b[,r,g,b]] - set global effect (v5, v8 devices)",3},
	{COMMANDS::lowlevel,"lowlevel","switch to low-level SDK (USB)"},
	{COMMANDS::highlevel,"highlevel","switch to high-level SDK (LightFX)"},
	{COMMANDS::probe,"probe","\t[l][d][,lights][,devID[,lightID]] - probe lights and set names"},
	{COMMANDS::status,"status","\tshows devices, lights and zones id's and names"},
	{COMMANDS::loop,"loop","\trepeat commands from start, until user press CTRL+c"}
};