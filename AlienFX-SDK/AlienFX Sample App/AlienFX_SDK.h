#pragma once  
#include "stdafx.h"

#ifdef ALIENFX_EXPORTS  
#define ALIENFX_API __declspec(dllexport)   
#else  
#define ALIENFX_API __declspec(dllimport)   
#endif  

namespace AlienFX_SDK

{	//This is VID for all alienware laptops, use this while initializing, it might be different for external AW device like mouse/kb
	int vid = 0x187c;

	enum Index
	{
		AlienFX_leftZone = 1,
		AlienFX_leftMiddleZone = 2,
		AlienFX_rightZone = 3,
		AlienFX_rightMiddleZone = 4,
		AlienFX_Macro = 5,
		AlienFX_AlienFrontLogo = 6,
		AlienFX_LeftPanelTop = 7,
		AlienFX_LeftPanelBottom = 8,
		AlienFX_RightPanelTop = 9,
		AlienFX_RightPanelBottom = 10,
		AlienFX_Power = 11,
		AlienFX_AlienBackLogo = 12,
		AlienFX_TouchPad = 13,
	};


	class Functions
	{
	public:
		//returns PID
		static ALIENFX_API int AlienFXInitialize(int vid);

		static ALIENFX_API  bool AlienFXInitialize(int vid, int pid);

		//De-init
		static ALIENFX_API  bool AlienFXClose();

		//Enable/Disable all lights
		static ALIENFX_API bool Reset(bool status);

		static ALIENFX_API bool IsDeviceReady();

		static ALIENFX_API bool UpdateColors();

		static ALIENFX_API bool SetColor(int index, int Red, int Green, int Blue);
	};



}
