#pragma once
#include <vector>
#include <wtypesbase.h>
#include "AlienFX_SDK.h"

#define byte BYTE

#define NUMBARS 20
#define NUMPTS 2048

//union Colorcode
//{
//	struct {
//		byte b;
//		byte g;
//		byte r;
//		byte br;
//	};
//	unsigned int ci;
//};

struct haptics_map {
	DWORD devid;
	WORD lightid;
	AlienFX_SDK::Colorcode colorfrom;
	AlienFX_SDK::Colorcode colorto;
	byte lowcut;
	byte hicut;
	byte flags;
	std::vector<unsigned char> map;
};

class ConfigHaptics
{
private:
	HKEY hMainKey, hMappingKey;
	void GetReg(char *name, DWORD *value, DWORD defValue = 0);
	void SetReg(char *text, DWORD value);
public:
	//const DWORD numbars = 20;
	//const DWORD numpts = 2048;
	DWORD inpType = 0;
	DWORD showAxis = 1;

	HWND dlg = NULL;

	std::vector<haptics_map> haptics;

	ConfigHaptics();
	~ConfigHaptics();

	void Load();
	void Save();
};

