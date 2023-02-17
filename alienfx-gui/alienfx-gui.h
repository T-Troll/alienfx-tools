#pragma once

#include <wtypes.h>
#include <CommCtrl.h>
#include <windowsx.h>
#include "resource.h"
#include "ConfigHandler.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(string(_x_).c_str());
#else
#define DebugPrint(_x_)
#endif

// defines and structures...
#define TAB_COLOR    0
#define TAB_EVENTS   1
#define TAB_AMBIENT  2
#define TAB_HAPTICS  3
#define TAB_GRID	 4
#define TAB_DEVICES  5

#define TAB_LIGHTS	 0
#define TAB_FANS     1
#define TAB_PROFILES 2
#define TAB_SETTINGS 3


typedef struct tag_dlghdr {
	HWND hwndControl;       // control dialog
	HWND hwndDisplay;   // current child dialog box
	DLGTEMPLATE** apRes; // Dialog templates
	DLGPROC* apProc; // dialog functions
} DLGHDR;

#define ind(_x, _y) (_y*conf->mainGrid->x+_x)

extern HINSTANCE hInst;
extern ConfigHandler* conf;
extern HWND sTip1, sTip2, sTip3;

extern int eItem;
extern groupset* mmap;



