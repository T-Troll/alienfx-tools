#pragma once
#include <vector>
#include <wtypesbase.h>
#include "AlienFX_SDK.h"

#define byte BYTE

#define NUMBARS 20
#define NUMPTS 2048

#define MAP_GAUGE 1
#define MAP_PFM   2

struct freq_map {
	AlienFX_SDK::Colorcode colorfrom{0};
	AlienFX_SDK::Colorcode colorto{0};
	byte lowcut{0};
	byte hicut{255};
	vector<byte> freqID;
};

struct haptics_map {
	DWORD devid = 0;
	DWORD lightid = 0;
	byte flags = 0;
	vector<freq_map> freqs;
};

class ConfigHaptics
{
private:
	HKEY hMainKey, hMappingKey;
	void GetReg(char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(char *text, DWORD value);
public:
	DWORD inpType = 0;
	DWORD showAxis = 1;

	std::vector<haptics_map> haptics;

	ConfigHaptics();
	~ConfigHaptics();

	haptics_map *FindHapMapping(DWORD devid, DWORD lid);

	void Load();
	void Save();
};

