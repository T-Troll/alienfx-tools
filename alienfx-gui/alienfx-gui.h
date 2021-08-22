#pragma once

#include "toolkit.h"

// defines and structures...
#define C_PAGES 7

typedef struct tag_dlghdr {
	HWND hwndTab;       // tab control
	HWND hwndDisplay;   // current child dialog box
	RECT rcDisplay;     // display rectangle for the tab control
	DLGTEMPLATE* apRes[C_PAGES];
} DLGHDR;


