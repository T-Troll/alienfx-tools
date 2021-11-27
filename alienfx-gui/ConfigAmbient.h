#pragma once
#include <vector>
#include <wtypesbase.h>

struct zone {
	DWORD devid = 0;
	WORD lightid = 0;
	std::vector<byte> map;
};

class ConfigAmbient
{
private:
	HKEY hMainKey = NULL, hMappingKey = NULL;
	void GetReg(char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(char *text, DWORD value);
public:
	DWORD mode = 0;
	DWORD shift = 40;
	std::vector<zone> zones;

	HWND hDlg = NULL;

	ConfigAmbient();
	~ConfigAmbient();
	void Load();
	void Save();
};

