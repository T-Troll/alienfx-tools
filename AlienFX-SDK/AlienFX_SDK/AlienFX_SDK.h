#pragma once
#include <wtypesbase.h>
#include <vector>
#include <string>
#include "alienfx-controls.h"

using namespace std;

#define byte BYTE

namespace AlienFX_SDK {

	// Old alienware device statuses v1-v3
	#define ALIENFX_V2_RESET 0x06
	#define ALIENFX_V2_READY 0x10
	#define ALIENFX_V2_BUSY 0x11
	#define ALIENFX_V2_UNKNOWN 0x12
	// new statuses for apiv4
	#define ALIENFX_V4_READY 33
	#define ALIENFX_V4_BUSY 34
	#define ALIENFX_V4_WAITCOLOR 35
	#define ALIENFX_V4_WAITUPDATE 36
    #define ALIENFX_V4_WASON 38
	// new statuses for apiv5
    #define ALIENFX_V5_STARTCOMMAND 0x8c
    #define ALIENFX_V5_WAITUPDATE 0x80
    #define ALIENFX_V5_INCOMMAND 0xcc

	// API version (was length):
    #define API_L_ACPI 0 //128
	#define API_L_V9 9 //65
    #define API_L_V8 8 //65
    #define API_L_V7 7 //65
    #define API_L_V6 6 //65
	#define API_L_V5 5 //64
	#define API_L_V4 4 //34
	#define API_L_V3 3 //12
    #define API_L_V2 2 //9
	#define API_L_V1 1 //8

	// Mapping flags:
    #define ALIENFX_FLAG_POWER 1
    #define ALIENFX_FLAG_INDICATOR 2

	// Maximal buffer size across all device types
    #define MAX_BUFFERSIZE 65

	union Colorcode // Atomic color structure
	{
		struct {
			byte b;
			byte g;
			byte r;
			byte br;
		};
		DWORD ci;
	};

	struct mapping { // Light information block
		//WORD vid = 0;
		//WORD devid;// = 0;
		WORD lightid;// = 0;
		WORD flags = 0;
		string name;
	};

	struct group { // Light group information block
		DWORD gid;// = 0;
		string name;
		vector<DWORD> lights;
		bool have_power = false;
	};

	struct lightgrid {
		byte id;
		byte x, y;
		string name;
		DWORD *grid;
	};

	struct afx_act { // atomic light action phase
		BYTE type; // one of Action values - action type
		BYTE time; // How long this phase stay
		BYTE tempo; // How fast it should transform
		BYTE r; // phase color
		BYTE g;
		BYTE b;
	};

	struct act_block { // light action block
		byte index;
		vector<afx_act> act;
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
	private:

		HANDLE devHandle = NULL;
		bool inSet = false;

		int vid = -1; // Device VID, can be zero!
		int pid = -1; // Device PID, -1 if not initialized
		int length = -1; // HID report length
		byte chain = 1; // seq. number for APIv1-v3
		byte version = -1; // interface version
		byte reportID = 0; // HID ReportID (0 if auto)
		byte bright = 64; // Brightness for APIv6-v7

		// support function for mask-based devices (v1-v3)
		vector<pair<byte, byte>> *SetMaskAndColor(DWORD index, byte type, Colorcode c1, Colorcode c2 = {0});

#ifndef NOACPILIGHTS
		// Support functions for ACPI calls (v0)
		bool SetAcpiColor(byte mask, Colorcode c);
#endif

		// Support function to send data to USB device
		bool PrepareAndSend(const byte *command, byte size, vector<pair<byte, byte>> mods);
		bool PrepareAndSend(const byte *command, byte size, vector<pair<byte, byte>> *mods = NULL);

		// Support function to send whole power block for v1-v3
		bool SavePowerBlock(byte blID, act_block act, bool needSave, bool needInverse = false);

	public:

		bool powerMode = true; // current power mode for APIv1-v3

		// Initialize device
		// Returns PID of device used.
		// If vid is 0, first device found into the system will be used, otherwise first device of this type.
		// If pid is defined, device with vid/pid will be used.
		int AlienFXInitialize(int vid, int pid = -1);

		// Another init function, for Aurora ACPI init.
		// acc is a handle to low-level ACPI driver (hwacc.sys) interface - see alienfan project.
		int AlienFXInitialize(HANDLE acc);

		//De-init
		bool AlienFXClose();

		// Switch to other AlienFX device
		bool AlienFXChangeDevice(int vid, int pid, HANDLE acc = NULL);

		// Prepare to set lights
		bool Reset();

		// Command separator for some APIs
		void Loop();

		// false - not ready, true - ready, 0xff - stalled
		BYTE IsDeviceReady();

		// basic color set with ID index for current device. loop - does it need loop command after?
		bool SetColor(unsigned index, Colorcode c, bool loop = true);

		// Set multiply lights to the same color. This only works for new API devices, and emulated for old ones.
		// lights - pointer to vector of light IDs need to be set.
		// c - color to set (brightness ignored)
		bool SetMultiLights(vector<byte> *lights, Colorcode c);

		// Set multiply lights to different color.
		// act - pointer to vector of light control blocks
		// store - need to save settings into device memory (v1-v4)
		bool SetMultiColor(vector<act_block> *act, bool store = false);

		// Set color to action
		// act - pointer to light control block
		bool SetAction(act_block *act);

		// Set action for Power button and store other light colors as default
		// act - pointer to vector of light control blocks
		bool SetPowerAction(vector<act_block> *act);

		// Hardware enable/disable/dim lights
		// brightness - desired brightness (0 - off, 255 - full)
		// mappings - needed to enable some lights for v1-v4 and for software emulation
		// power - if true, power and indicator lights will be set too
		bool ToggleState(BYTE brightness, vector <mapping>* mappings, bool power);

		// Global (whole device) effect control for APIv5, v8, v9
		// effType - effect type
		// mode - effect mode (off, steady, keypress, etc)
		// tempo - effect tempo
		// act1 - first effect color
		// act2 - second effect color (not for all effects)
		bool SetGlobalEffects(byte effType, byte mode, byte tempo, afx_act act1, afx_act act2);

		// return current device state
		BYTE AlienfxGetDeviceStatus();

		// Next command delay for APIv1-v3
		BYTE AlienfxWaitForReady();

		// After-reset delay for APIv1-v3
		BYTE AlienfxWaitForBusy();

		// Apply changes and update colors
		bool UpdateColors();

		// get PID for current device
		int GetPID();

		// get VID for current device
		int GetVid();

		// get API version for current device
		int GetVersion();

		// check global effects avaliability
		bool IsHaveGlobal();
	};

	// Single device data - device pointer, description pointer, lights
	struct afx_device {
		WORD vid, pid;
		Functions* dev = NULL;
		string name;
		Colorcode white = { 255,255,255 };
		vector <mapping> lights;
	};

	class Mappings {
	private:
		//vector <mapping*> mappings; // Lights data for all devices
		//vector <devmap> devices; // Device data found/present in system
		vector <group> groups; // Defined light groups
		vector <lightgrid> grids; // Grid zones info

	public:

		vector<afx_device> fxdevs;
		int activeLights = 0;

		~Mappings();

		// Enumerate all alienware devices into the system
		vector<pair<WORD,WORD>> AlienFXEnumDevices();

		// Load device data and assign it to structure, as well as init devices and set brightness
		void AlienFXAssignDevices(HANDLE acc = NULL, byte brightness=255, byte power=false);

		// load light names from registry
		void LoadMappings();

		// save light names into registry
		void SaveMappings();

		// get saved devices names
		//vector<devmap>* GetDevices();

		// get saved light names
		vector <mapping>* GetMappings(DWORD devID);

		// get defined groups
		vector <group>* GetGroups();

		// get defined grids
		vector <lightgrid>* GetGrids() { return &grids; };

		// get grid object by it's ID
		lightgrid* GetGridByID(byte id);

		// get device structure by PID/VID (low/high WORD)
		afx_device* GetDeviceById(DWORD devID);

		// get or add device structure by PID/VID (low/high WORD)
		afx_device* AddDeviceById(DWORD devID);

		// find light mapping by PID (or PID/VID) and light ID
		mapping* GetMappingById(afx_device* dev, WORD LightID);

		// find light group by it's ID
		group* GetGroupById(DWORD gid);

		// add new light name into the list field-by-field
		void AddMappingByDev(afx_device* dev, WORD lightID, const char* name, WORD flags);

		// add new light name into the list field-by-field
		void AddMapping(DWORD devID, WORD lightID, const char* name, WORD flags);

		// remove light mapping by id
		void RemoveMapping(afx_device* dev, WORD lightID);

		// get light flags (Power, indicator) by device pointer and light ID
		int GetFlags(afx_device* dev, WORD lightid);

		// get light flags (Power, indicator) by PID/VID and light ID
		int GetFlags(DWORD devID, WORD lightid);
	};

}
