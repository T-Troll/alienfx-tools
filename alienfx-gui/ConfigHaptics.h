#pragma once
#include <vector>
#include <Windows.h>

union Colorcode
{
	struct {
		byte b;
		byte g;
		byte r;
		byte br;
	};
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

	void GetReg(char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(char *text, DWORD value);
public:
	DWORD numbars = 20;
	DWORD numpts = 2048;
	DWORD inpType = 0;
	DWORD showAxis = 1;

	HWND dlg = NULL;

	std::vector<haptics_map> mappings;

	ConfigHaptics();
	~ConfigHaptics();

	int Load();
	int Save();
	//static bool ConfigHaptics::sortMappings(mapping i, mapping j);
};

