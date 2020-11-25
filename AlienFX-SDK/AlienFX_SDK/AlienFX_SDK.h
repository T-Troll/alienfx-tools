#pragma once  
#include "stdafx.h"
#include <vector>
#include <string>

//#ifdef ALIENFX_EXPORTS  
//#define ALIENFX_API __declspec(dllexport)   
//#else  
//#define ALIENFX_API __declspec(dllimport)   
//#endif  

namespace AlienFX_SDK

{	
	struct mapping {
		unsigned devid = 0;
		unsigned lightid = 0;
		unsigned flags = 0;
		std::string name;
	};

	struct devmap {
		unsigned devid = 0;
		std::string name;
	};

	struct afx_act {
		BYTE type = 0;
		BYTE time = 0;
		BYTE tempo = 0;
		BYTE r = 0;
		BYTE g = 0;
		BYTE b = 0;
	};

	enum Index
	{
		AlienFX_leftZone = 1, // 2 for m15
		AlienFX_leftMiddleZone = 2, // 3 for m15
		AlienFX_rightZone = 3, // 5 for m15
		AlienFX_rightMiddleZone = 4, // same for m15
		AlienFX_Macro = 5,
		AlienFX_AlienFrontLogo = 6, // ?? for m15
		AlienFX_LeftPanelTop = 7,
		AlienFX_LeftPanelBottom = 8,
		AlienFX_RightPanelTop = 9,
		AlienFX_RightPanelBottom = 10,
		AlienFX_TouchPad = 11,
		AlienFX_AlienBackLogo = 12, // 0 for m15
		AlienFX_Power = 13 // 1 for m15
	};
	
	enum Action
	{
		AlienFX_A_Color = 0,
		AlienFX_A_Pulse = 1,
		AlienFX_A_Morph = 2,
		AlienFX_A_Breathing= 3,
		AlienFX_A_Spectrum = 4,
		AlienFX_A_Rainbow = 5,
		AlienFX_A_Power = 6,
		AlienFX_A_NoAction = 7
	};

	class Functions
	{
	public:

		//This is VID for all alienware laptops, use this while initializing, it might be different for external AW device like mouse/kb
		const static int vid = 0x187c;

		// Enum alienware devices
		static  std::vector<int> AlienFXEnumDevices(int vid);
		//returns PID
		static  int AlienFXInitialize(int vid);

		static  int AlienFXInitialize(int vid, int pid);

		//De-init
		static  bool AlienFXClose();

		// Switch to other AlienFX device
		static  bool AlienFXChangeDevice(int pid);

		//Enable/Disable all lights (or just prepare to set)
		static  bool Reset(int status);

		static  bool IsDeviceReady();

		static  bool SetColor(int index, int Red, int Green, int Blue);

		// Set multipy lights to the same color. This only works for new API devices, and emulated at old ones.
		// numLights - how many lights need to be set
		// lights - pointer to array of light IDs need to be set.
		static  bool SetMultiColor(int numLights, UCHAR* lights, int r, int g, int b);

		// Set color to action
		// action - action type (see enum above)
		// time - how much time to keep action (0-255)
		// tempo - how fast to do evolution (f.e. pulse - 0-255) 
		// It can possible to mix 2 actions in one (useful for morph), in this case use action2...Blue2
		static  bool SetAction(int index, std::vector<afx_act> act);
			//int action, int time, int tempo, int Red, int Green, int Blue, int action2 = AlienFX_A_NoAction, int time2 = 0, int tempo2=0, int Red2 = 0, int Green2 = 0, int Blue2 = 0);

		// Set action for Power button
		// For now, settings as a default of AWCC, but it possible to do it more complex
		static  bool SetPowerAction(int index, BYTE Red, BYTE Green, BYTE Blue, BYTE Red2, BYTE Green2, BYTE Blue2, bool force = false);

		// return current device state
		static  BYTE AlienfxGetDeviceStatus();

		// Apply changes and update colors
		static  bool UpdateColors();

		// load light names from registry
		static void LoadMappings();

		// save light names into registry
		static void SaveMappings();

		// get saved devices names
		static std::vector<devmap>* GetDevices();

		// get saved light names
		static std::vector <mapping>* GetMappings();

		// get saved light names
		static int GetFlags(int devid, int lightid);

		// get saved light names
		static void SetFlags(int devid, int lightid, int flags);

		// get PID in use
		static int GetPID();

		// get version for current device
		static int GetVersion();
	};

}
