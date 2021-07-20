#pragma once  
#include "stdafx.h"
#include <vector>
#include <string>

using namespace std;

namespace AlienFX_SDK

{	
	// Old alieware device statuses v1-v3
	#define ALIENFX_V2_RESET 0x06
	#define ALIENFX_V2_READY 0x10
	#define ALIENFX_V2_BUSY 0x11
	#define ALIENFX_V2_UNKNOWN 0x12
	// new statuses for apiv4 - 33 = ok, 36 = wait for update, 35 = wait for color, 34 - busy processing power update
	#define ALIENFX_V4_READY 33
	#define ALIENFX_V4_BUSY 34
	#define ALIENFX_V4_WAITCOLOR 35
	#define ALIENFX_V4_WAITUPDATE 36
    #define ALIENFX_V4_WASON 38
	// apiv5
    #define ALIENFX_V5_STARTCOMMAND 0x8c
    #define ALIENFX_V5_INCOMMAND 0xcc

	// Length by API version:
	#define API_L_V5 64
	#define API_L_V4 34
	#define API_L_V3 12
    #define API_L_V2 9
	#define API_L_V1 8

	// Mapping flags:
    #define ALIENFX_FLAG_POWER 1
    #define ALIENFX_FLAG_INACTIVE 2

	// delay for power button update
    #define POWER_DELAY 450

	// Maximal buffer size across all device types
    #define MAX_BUFFERSIZE 64

	struct mapping {
		DWORD devid = 0;
		DWORD lightid = 0;
		DWORD flags = 0;
		std::string name;
	};

	struct devmap {
		DWORD devid = 0;
		std::string name;
	};

	struct group {
		DWORD gid = 0;
		std::string name;
		vector<mapping*> lights;
	};

	struct afx_act {
		BYTE type = 0;
		BYTE time = 0;
		BYTE tempo = 0;
		BYTE r = 0;
		BYTE g = 0;
		BYTE b = 0;
	};

	/*enum Index
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
	};*/
	/*
	int leftZone = 0x100;// 0x8;
	int leftMiddleZone = 0x8;// 0x4;
	int rightZone = 0x2;// 0x1;
	int rightMiddleZone = 0x4;// 0x2;
	int Macro = 0x1;// 0;
	int AlienFrontLogo = 0x20;// 0x40;
	int LeftPanelTop = 0x40;// 0x1000;
	int LeftPanelBottom = 0x280;// 0x400;
	int RightPanelTop = 0xf7810;// 0x2000;
	int RightPanelBottom = 0x800;
	int AlienBackLogo = 0x20;
	int Power = 13;
	int TouchPad = 0x80;
	*/
	
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
	const static DWORD vids[2] = {0x187c, 0x0d62};

	/* ????, left, leftmidlde, rightMiddle, right, backLogo, frontLogo, leftTop, rightTop, rightBottom, Power, touchPad */ 
	//const static int masks[][13] = {{  0x100, 0x8, 0x4, 0x1, 0x2,    0, 0x40,0x1000, 0x400,0x2000,  0x800,   0x80, 0x20},
	//						        {  0x100, 0x8, 0x4, 0x2, 0x1, 0x20, 0x40, 0x200, 0x080, 0x400,  0x680,    0xf,  0x0},
	//						        {  0x100, 0x8, 0x4, 0x2, 0x1, 0x20, 0x40,  0x80, 0x400, 0x800, 0x1000, 0x2000,  0x0}};
		    //mask12_4[] = {  0x100, 0x8, 0x4, 0x2, 0x1, 0x20, 0x40, 0x200, 0x080, 0x280,    0xf,    0x0},
		    //mask12_8[] = {  0x100, 0x8, 0x4, 0x2, 0x1, 0x20, 0x40,  0x80, 0x400, 0x800, 0x1000, 0x2000};
	                       //0      1    2    3    4    5     6       7       8      9     10      11     12

	class Functions
	{
	private:

		HANDLE devHandle = NULL;
		//bool inSet = false;
		ULONGLONG lastPowerCall = 0;

		int vid = -1;
		int pid = -1;
		int length = -1;
		int chain = 1;
		//int version = -1;

		// support function for mask-based devices (v1-v3)
		void SetMaskAndColor(int index, byte* buffer, byte r1, byte g1, byte b1, byte r2 = 0, byte g2 = 0, byte b2 = 0);

	public:

		bool inSet = false;
		// Initialize device
		// Returns PID of device used. if pid argument is -1, first device found into the system will be used.
		int AlienFXInitialize(int vid, int pid = -1);

		//De-init
		bool AlienFXClose();

		// Switch to other AlienFX device
		bool AlienFXChangeDevice(int vid, int pid);

		//Enable/Disable all lights (or just prepare to set)
		bool Reset(int status);

		void Loop();

		BYTE IsDeviceReady();

		bool SetColor(unsigned index, byte r, byte g, byte b);

		// Set multipy lights to the same color. This only works for new API devices, and emulated at old ones.
		// numLights - how many lights need to be set
		// lights - pointer to array of light IDs need to be set.
		bool SetMultiLights(int numLights, UCHAR* lights, int r, int g, int b);

		// Set multipy lights to different color.
		// size - how many lights
		// lights - pointer to array of light IDs need to be set (should be "size")
		// act - array of light colors set (should be "size)
		bool SetMultiColor(int size, UCHAR* lights, std::vector<vector<afx_act>> act);

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

		// Hardware enable/disable lights
		// newState - on/off
		// mappings - needed to keep some lights on
		// power - if true, power and indicator lights will be set on/off too
		bool ToggleState(BYTE brightness, vector <mapping>* mappings, bool power);

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
		vector <group> groups;
	public:

		~Mappings();

		// Enum alienware devices
		vector<pair<DWORD,DWORD>> AlienFXEnumDevices();

		// load light names from registry
		void LoadMappings();

		// save light names into registry
		void SaveMappings();

		// get saved devices names
		vector<devmap>* GetDevices();

		// get saved light names
		vector <mapping>* GetMappings();

		// get defined groups
		vector <group>* GetGroups();

		// find mapping by dev/light it...
		mapping* GetMappingById(int devID, int LightID);

		// find mapping by dev/light it...
		group* GetGroupById(int gid);

		// add new light name into the list
		void AddMapping(int devID, int lightID, char* name, int flags);

		void AddGroup(int gID, char* name, int lightNum, DWORD* lightlist);

		// get saved light names
		int GetFlags(int devid, int lightid);

		// get saved light names
		void SetFlags(int devid, int lightid, int flags);
	};

}
