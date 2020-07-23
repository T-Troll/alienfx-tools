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
	unsigned devid;
	unsigned lightid;
	Colorcode colorfrom;
	Colorcode colorto;
	unsigned char lowcut;
	unsigned char hicut;
	std::vector<unsigned char> map;
};

class ConfigHandler
{
private:
	HKEY   hKey1, hKey2;
public:
	DWORD numbars;
	DWORD res;
	DWORD inpType;
	std::vector<mapping> mappings;

	ConfigHandler();
	~ConfigHandler();
	int Load();
	int Save();
};

