#pragma once

#include <wtypes.h>
#include <CommCtrl.h>
#include "resource.h"
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "AlienFX_SDK.h"
#include "EventHandler.h"
#include "CaptureHelper.h"
#include "../alienfan-tools/alienfan-SDK/alienfan-SDK.h"
#include "../alienfan-tools/alienfan-gui/MonHelper.h"
#include "../alienfan-tools/alienfan-gui/ConfigHelper.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)  
#endif

// defines and structures...
#define C_PAGES      9

#define TAB_COLOR    0
#define TAB_EVENTS   1
#define TAB_AMBIENT  2
#define TAB_HAPTICS  3
#define TAB_GROUPS   4
#define TAB_PROFILES 5
#define TAB_DEVICES  6
#define TAB_FANS     7
#define TAB_SETTINGS 8

typedef struct tag_dlghdr {
	HWND hwndTab;       // tab control
	HWND hwndDisplay;   // current child dialog box
	RECT rcDisplay;     // display rectangle for the tab control
	DLGTEMPLATE* apRes[C_PAGES];
} DLGHDR;

extern HINSTANCE hInst;

extern FXHelper* fxhl;
extern ConfigHandler* conf;
extern EventHandler* eve;
extern HWND sTip, lTip;
extern AlienFan_SDK::Control* acpi;
extern MonHelper* mon;

extern HWND sTip, lTip;


