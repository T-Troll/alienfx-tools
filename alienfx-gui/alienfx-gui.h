#pragma once

//#include "toolkit.h"
//#include <windowsx.h>
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

// defines and structures...
#define C_PAGES 9

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


