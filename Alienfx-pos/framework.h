// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
//#include <stdlib.h>
//#include <malloc.h>
//#include <memory.h>
//#include <tchar.h>
#include <vector>
#include <string>

using namespace std;

//#define byte BYTE
//
//struct lightgrid {
//	byte id;
//	byte x, y;
//	string name;
//	DWORD grid[21][8];
//};

//struct gridpos {
//	byte zone;
//	byte x;
//	byte y;
//};
//
//struct lightpos {
//	DWORD devID;
//	WORD lightID;
//	vector<gridpos> pos;
//};