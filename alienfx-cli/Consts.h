#pragma once

struct ARG {
	int num;
	string str;
};

struct COMMAND {
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
	{"set-all","\tr,g,b[,br] - set all device lights",3},
	{"set-one","\tpid,light,r,g,b[,br] - set one light",5},
	{"set-zone","zone,r,g,b[,br] - set zone lights",4},
	{"set-action","pid,light,action,r,g,b[,br,[action,r,g,b,br]] - set light and enable it's action",6},
	{"set-zone-action","zone,action,r,g,b[,br,[action,r,g,b,br]] - set all zone lights and enable it's action",5},
	{"set-power","light,r,g,b,r2,g2,b2 - set power button colors (low-level only)",7},
	{"set-tempo","tempo - set tempo for actions and pause",1},
	{"set-dev","\tpid - set active device for low-level",1},
	{"set-dim","\tbr - set active device dimming level",1},
	{"set-global","mode,r,g,b,r,g,b - set global effect mode (v5 devices only)",7},
	{"low-level","switch to low-level SDK"},
	{"high-level","switch to high-level SDK (Alienware LightFX)"},
	{"probe","\t[ald[,lights][,devID[,lightID]] - probe lights to set names"},
	{"status","\tshows devices and lights id's, names and statuses"},
	{"lightson","turn all current device lights on"},
	{"lightsoff","turn all current device lights off"},
	{"reset","\treset current device state"},
	{"loop","\trepeat commands from start, until user press CTRL+c"}
};