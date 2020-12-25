#pragma once
#include <vector>
#include <string>
#include <Windows.h>
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"


struct ColorComp
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char brightness;
};

union Colorcode
{
	struct ColorComp cs;
	unsigned int ci = 0;
};

union FlagSet {
	struct {
		BYTE flags;
		BYTE cut;
		BYTE proc;
		BYTE reserved;
	} b;
	unsigned s = 0;
};

struct event {
	FlagSet fs;
	unsigned source = 0;
	std::vector<AlienFX_SDK::afx_act> map;
};

struct lightset {
	unsigned devid = 0;
	unsigned lightid = 0;
	event	 eve[4];
};

struct profile {
	unsigned id = 0;
	unsigned flags = 0;
	std::string triggerapp;
	std::string name;
	std::vector<lightset> lightsets;
};

class ConfigHandler
{
private:
	HKEY hKey1 = NULL, hKey2 = NULL, hKey3 = NULL, hKey4 = NULL;
	bool conf_loaded = false;
public:
	DWORD startWindows = 0;
	DWORD startMinimized = 0;
	DWORD autoRefresh = 0;
	DWORD lightsOn = 1;
	DWORD offWithScreen = 0;
	DWORD dimmed = 0;
	DWORD dimmedBatt = 0;
	DWORD dimmingPower = 92;
	DWORD enableMon = 1;
	DWORD enableProf = 0;
	DWORD monState = 1;
	DWORD offPowerButton = 0;
	DWORD activeProfile = 0;
	DWORD defaultProfile = 0;
	DWORD awcc_disable = 1;
	DWORD block_power = 0;
	unsigned stateDimmed = 0, stateOn = 1;
	DWORD gammaCorrection = 1;
	Colorcode testColor;
	COLORREF customColors[16];
	std::vector<lightset> active_set;
	std::vector<profile> profiles;

	ConfigHandler();
	~ConfigHandler();
	int Load();
	int Save();
	static bool sortMappings(lightset i, lightset j);
	void updateProfileByID(int id, std::string name, std::string app, DWORD flags);
};

