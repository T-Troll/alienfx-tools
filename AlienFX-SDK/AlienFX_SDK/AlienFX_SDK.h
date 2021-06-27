#pragma once  
#include "stdafx.h"
#include <vector>
#include <string>

using namespace std;

namespace AlienFX_SDK

{	
	// Old alieware device statuses
	#define ALIENFX_DEVICE_RESET 0x06
	#define ALIENFX_READY 0x10
	#define ALIENFX_BUSY 0x11
	#define ALIENFX_UNKNOWN_COMMAND 0x12
	// new statuses for apiv3 - 33 = ok, 36 = wait for update, 35 = wait for color, 34 - busy processing power update
	#define ALIENFX_NEW_READY 33
	#define ALIENFX_NEW_BUSY 34
	#define ALIENFX_NEW_WAITCOLOR 35
	#define ALIENFX_NEW_WAITUPDATE 36

	// Length by API version:
	#define API_V4 65
	#define API_V3 34
	#define API_V2 12
	#define API_V1 8

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

	//This is VID for all alienware laptops, use this while initializing, it might be different for external AW device like mouse/kb
	const static int vid = 0x187c;
	const static int vid2 = 0x0d62; // DARFON per-key RGB keyboard - m1X R2, R3. 

	class Functions
	{
	private:

		bool isInitialized = false;
		HANDLE devHandle = NULL;
		int length = 9;
		bool inSet = false;
		ULONGLONG lastPowerCall = 0;

		int pid = -1;
		int version = -1;

	public:

		//returns PID
		int AlienFXInitialize(int vid);

		int AlienFXInitialize(int vid, int pid);

		//De-init
		bool AlienFXClose();

		// Switch to other AlienFX device
		bool AlienFXChangeDevice(int pid);

		//Enable/Disable all lights (or just prepare to set)
		bool Reset(int status);

		void Loop();

		bool IsDeviceReady();

		bool SetColor(int index, int Red, int Green, int Blue);

		// Set multipy lights to the same color. This only works for new API devices, and emulated at old ones.
		// numLights - how many lights need to be set
		// lights - pointer to array of light IDs need to be set.
		bool SetMultiColor(int numLights, UCHAR* lights, int r, int g, int b);

		// Set color to action
		// action - action type (see enum above)
		// time - how much time to keep action (0-255)
		// tempo - how fast to do evolution (f.e. pulse - 0-255) 
		// It can possible to mix 2 actions in one (useful for morph), in this case use action2...Blue2
		bool SetAction(int index, std::vector<afx_act> act);
			//int action, int time, int tempo, int Red, int Green, int Blue, int action2 = AlienFX_A_NoAction, int time2 = 0, int tempo2=0, int Red2 = 0, int Green2 = 0, int Blue2 = 0);

		// Set action for Power button
		// For now, settings as a default of AWCC, but it possible to do it more complex
		bool SetPowerAction(int index, BYTE Red, BYTE Green, BYTE Blue, BYTE Red2, BYTE Green2, BYTE Blue2, bool force = false);

		// return current device state
		BYTE AlienfxGetDeviceStatus();

		BYTE AlienfxWaitForReady();

		BYTE AlienfxWaitForBusy();

		// Apply changes and update colors
		bool UpdateColors();

		// get PID in use
		 int GetPID();

		// get version for current device
		 int GetVersion();
	};

	class Mappings {
	private:
		// Name mappings for lights
		vector <mapping> mappings;
		vector <devmap> devices;
	public:

		~Mappings();

		// Enum alienware devices
		vector<int> AlienFXEnumDevices(int vid);

		// load light names from registry
		void LoadMappings();

		// save light names into registry
		void SaveMappings();

		// get saved devices names
		vector<devmap>* GetDevices();

		// get saved light names
		vector <mapping>* GetMappings();

		// find mapping by dev/light it...
		//mapping* GetMappingById(int devID, int LightID);

		// add new light name into the list
		void AddMapping(int devID, int lightID, char* name, int flags);

		// get saved light names
		int GetFlags(int devid, int lightid);

		// get saved light names
		void SetFlags(int devid, int lightid, int flags);
	};

}
