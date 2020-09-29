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
	unsigned int ci = 0;
};

struct mapping {
	unsigned mode = 0;
	unsigned mode2 = 0;
	unsigned speed1 = 0, speed2 = 0;
	unsigned length1 = 0, length2 = 0;
	Colorcode c1, c2;
	void* lightset;
};

struct event {
	unsigned flags = 0;
	unsigned source = 0;
	mapping  map;
};

struct lightset {
	unsigned devid = 0;
	unsigned lightid = 0;
	event	 eve[4];
};

class ConfigHandler
{
private:
	HKEY   hKey1 = NULL, hKey2 = NULL, hKey3 = NULL;
public:
	DWORD startWindows = 0;
	DWORD startMinimized = 0;
	DWORD autoRefresh = 0;
	DWORD lightsOn = 1;
	DWORD dimmed = 0;
	DWORD dimmedBatt = 0;
	DWORD dimmingPower = 92;
	//DWORD shift = 40;
	std::vector<lightset> mappings;

	ConfigHandler();
	~ConfigHandler();
	int Load();
	int Save();
};

