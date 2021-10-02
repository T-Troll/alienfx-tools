#pragma once
#include <vector>
#include <Windows.h>

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
	unsigned int ci;
};

struct haptics_map {
	unsigned devid;
	unsigned lightid;
	Colorcode colorfrom;
	Colorcode colorto;
	unsigned char lowcut;
	unsigned char hicut;
	unsigned flags;
	std::vector<unsigned char> map;
};

class ConfigHaptics
{
private:
	HKEY   hKey1, hKey2;
public:
	DWORD numbars = 20;
	DWORD numpts = 2048;
	DWORD inpType = 0;
	DWORD showAxis = 1;
	//DWORD stateOn = 1;
	//DWORD offPowerButton = 0;
	std::vector<haptics_map> mappings;

	ConfigHaptics();
	~ConfigHaptics();
	int Load();
	int Save();
	//static bool ConfigHaptics::sortMappings(mapping i, mapping j);
};

