#pragma once
#include <vector>
#include <Windows.h>


struct mapping {
	unsigned devid = 0;
	unsigned lightid = 0;
	std::vector<unsigned char> map;
};

class ConfigAmbient
{
private:
	HKEY   hKey1 = NULL, hKey2 = NULL;
	void GetReg(char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(char *text, DWORD value);
public:
	DWORD mode = 0;
	//DWORD divider = 16;
	DWORD shift = 40;
	DWORD gammaCorrection = 1;
	DWORD stateOn = 1;
	DWORD offPowerButton = 0;
	std::vector<mapping> mappings;

	HWND hDlg = NULL;

	ConfigAmbient();
	~ConfigAmbient();
	int Load();
	int Save();
	//static bool sortMappings(mapping i, mapping j);
};

