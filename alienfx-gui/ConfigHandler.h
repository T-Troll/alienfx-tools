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
	unsigned mode = 0;
	unsigned mode2 = 0;
	unsigned speed1 = 0, speed2 = 0;
	unsigned length1 = 0, length2 = 0;
	Colorcode c1, c2;
	//std::vector<unsigned char> map; // events will be here
};

class ConfigHandler
{
private:
	HKEY   hKey1 = NULL, hKey2 = NULL, hKey3 = NULL;
public:
	//DWORD maxcolors = 12;
	//DWORD mode = 0;
	//DWORD divider = 16;
	//DWORD shift = 40;
	std::vector<mapping> mappings;

	ConfigHandler();
	~ConfigHandler();
	int Load();
	int Save();
};

