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

struct mapping {
	unsigned devid = 0;
	unsigned lightid = 0;
	std::vector<unsigned char> map;
};

class ConfigHandler
{
private:
	HKEY   hKey1 = NULL, hKey2 = NULL;
public:
	DWORD mode = 0;
	DWORD divider = 16;
	DWORD shift = 40;
	DWORD gammaCorrection = 1;
	DWORD stateOn = 1;
	DWORD offPowerButton = 0;
	std::vector<mapping> mappings;

	ConfigHandler();
	~ConfigHandler();
	int Load();
	int Save();
	static bool sortMappings(mapping i, mapping j);
};

