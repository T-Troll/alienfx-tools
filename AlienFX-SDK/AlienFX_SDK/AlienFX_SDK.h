#pragma once
#include <wtypes.h>
#include <vector>
#include <string>

#ifndef NOACPILIGHTS
#include "alienfan-SDK.h"
#endif

using namespace std;

namespace AlienFX_SDK {

	// Statuses for v1-v3
	#define ALIENFX_V2_READY 0x10
	#define ALIENFX_V2_BUSY 0x11
	#define ALIENFX_V2_UNKNOWN 0x12
	// Statuses for apiv4
	#define ALIENFX_V4_READY 33
	#define ALIENFX_V4_BUSY 34
	#define ALIENFX_V4_WAITCOLOR 35
	#define ALIENFX_V4_WAITUPDATE 36
    #define ALIENFX_V4_WASON 38
	// Statuses for apiv5
    #define ALIENFX_V5_STARTCOMMAND 0x8c
    #define ALIENFX_V5_WAITUPDATE 0x80
    #define ALIENFX_V5_INCOMMAND 0xcc

	// Mapping flags:
    #define ALIENFX_FLAG_POWER		1 // This is power button
    #define ALIENFX_FLAG_INDICATOR	2 // This is indicator light (keep at lights off)

	// Maximal buffer size across all device types
    #define MAX_BUFFERSIZE 193

	union Afx_colorcode // Atomic color structure
	{
		struct {
			byte b, g, r;
			byte br; // Brightness
		};
		DWORD ci;
	};

	struct Afx_icommand {
		int i;
		vector<byte> vval;
	};

	struct Afx_light { // Light information block
		byte lightid;
		union {
			struct {
				WORD flags;
				WORD scancode;
			};
			DWORD data;
		};
		string name;
	};

	union Afx_groupLight {
		struct {
			WORD did, lid;
		};
		DWORD lgh;
	};

	struct Afx_group { // Light group information block
		DWORD gid;
		string name;
		vector<Afx_groupLight> lights;
		//bool have_power = false;
	};

	struct Afx_grid {
		byte id;
		byte x, y;
		string name;
		Afx_groupLight *grid;
	};

	struct Afx_action { // atomic light action phase
		BYTE type; // one of Action values - action type
		BYTE time; // How long this phase stay
		BYTE tempo; // How fast it should transform
		BYTE r, g, b; // phase color
	};

	struct Afx_lightblock { // light action block
		byte index;
		vector<Afx_action> act;
	};

	enum Afx_Version {
		API_ACPI = 0, //128
		API_V9 = 9, //193
		API_V8 = 8, //65
		API_V7 = 7, //65
		API_V6 = 6, //65
		API_V5 = 5, //64
		API_V4 = 4, //34
		API_V3 = 3, //12
		API_V2 = 2, //9
		API_UNKNOWN = -1
	};

	enum Action	{
		AlienFX_A_Color = 0,
		AlienFX_A_Pulse = 1,
		AlienFX_A_Morph = 2,
		AlienFX_A_Breathing= 3,
		AlienFX_A_Spectrum = 4,
		AlienFX_A_Rainbow = 5,
		AlienFX_A_Power = 6
	};

	class Functions
	{
	private:

		HANDLE devHandle = NULL; // USB device handle, NULL if not initialized
		void* ACPIdevice = NULL; // ACPI device object pointer

		bool inSet = false;

		int length = -1; // HID report length
		byte chain = 1; // seq. number for APIv1-v3

		// support function for mask-based devices (v1-v3, v6)
		vector<Afx_icommand>* SetMaskAndColor(vector<Afx_icommand>* mods, Afx_lightblock* act, bool needInverse = false, DWORD index = 0);

		// Support function to send data to USB device
		bool PrepareAndSend(const byte* command, vector<Afx_icommand> mods);
		bool PrepareAndSend(const byte* command, vector<Afx_icommand> *mods = NULL);

		// Add new light effect block for v8
		inline void AddV8DataBlock(byte bPos, vector<Afx_icommand>* mods, Afx_lightblock* act);

		// Add new color block for v5
		inline void AddV5DataBlock(byte bPos, vector<Afx_icommand>* mods, byte index, Afx_action* act);

		// Support function to send whole power block for v1-v3
		void SavePowerBlock(byte blID, Afx_lightblock* act, bool needSave, bool needSecondary = false, bool needInverse = false);

		// Support function for APIv4 action set
		bool SetV4Action(Afx_lightblock* act);

		// return current device state
		BYTE GetDeviceStatus();

		// Next command delay for APIv1-v3
		BYTE WaitForReady();

		// After-reset delay for APIv1-v3
		BYTE WaitForBusy();

	public:
		WORD vid = 0; // Device VID
		WORD pid = 0; // Device PID
		int version = API_UNKNOWN; // interface version, will stay at API_UNKNOWN if not initialized
		byte bright = 64; // Last brightness set for device
		string description; // device description

		~Functions();

		// Initialize device
		// If vid is 0 or absent, first device found into the system will be used, otherwise device with this VID only.
		// If pid is 0 or absent, first device with any pid and given vid will be used, otherwise vid/pid pair.
		// Returns true if device found and initialized.
		bool AlienFXInitialize(WORD vidd = 0, WORD pidd = 0);

		// Check device and initialize data
		// vid/pid the same as above
		// Returns true if device found and initialized.
		bool AlienFXProbeDevice(void* hDevInfo, void* devData, WORD vid = 0, WORD pid = 0);

#ifndef NOACPILIGHTS
		// Initialize Aurora ACPI lights if present.
		// acc is a pointer to active AlienFan control object (see AlienFan_SDK for reference)
		bool AlienFXInitialize(AlienFan_SDK::Control* acc);
#endif

		// Prepare to set lights
		bool Reset();

		// false - not ready, true - ready, 0xff - stalled
		BYTE IsDeviceReady();

		// Basic color set for current device.
		// index - light ID
		// c - color action structure
		// It's a synonym of SetAction, but with one color action
		bool SetColor(byte index, Afx_action c);

		// Set multiply lights to the same color. This only works for some API devices, and emulated for other ones.
		// lights - pointer to vector of light IDs need to be set.
		// c - color to set
		bool SetMultiColor(vector<byte> *lights, Afx_action c);

		// Set multiply lights to different color.
		// act - pointer to vector of light control blocks (each define one light)
		// store - need to save settings into device memory (v1-v4)
		bool SetMultiAction(vector<Afx_lightblock> *act, bool store = false);

		// Set color to action
		// index - light ID
		// act - pointer to light actions vector
		bool SetAction(Afx_lightblock* act);

		// Set action for Power button and store other light colors as default
		// act - pointer to vector of light control blocks
		bool SetPowerAction(vector<Afx_lightblock> *act, bool save = false);

		// Hardware brightness for device, if supported
		// brightness - desired brightness (0 - off, 255 - full)
		// mappings - mappings list for v4 brightness set (it require light IDs list)
		// power - if true, power and indicator lights will be set too
		bool SetBrightness(BYTE brightness, vector <Afx_light>* mappings, bool power);

		// Global (whole device) effect control for APIv5, v8
		// effType - effect type
		// mode - effect mode (off, steady, keypress, etc)
		// nc - number of colors (3 - spectrum)
		// tempo - effect tempo
		// act1 - first effect color
		// act2 - second effect color (not for all effects)
		bool SetGlobalEffects(byte effType, byte mode, byte nc, byte tempo, Afx_action act1, Afx_action act2);

		// Apply changes and update colors
		bool UpdateColors();

		//// get PID for current device
		//int GetPID();

		//// get VID for current device
		//int GetVID();

		//// get API version for current device
		//int GetVersion();

		// check global effects availability
		bool IsHaveGlobal();
	};

	struct Afx_device { // Single device data
		union {
			struct {
				WORD pid, vid;			// IDs
			};
			DWORD devID;
		};
		Functions* dev = NULL;  // device control object pointer
		string name;			// device name
		Afx_colorcode white = { 255,255,255 }; // white point
		vector <Afx_light> lights; // vector of lights defined
		int version = API_UNKNOWN; // API version used for this device
	};

	class Mappings {
	private:
		vector<Afx_group> groups; // Defined light groups
		vector<Afx_grid> grids; // Grid zones info

	public:

		vector<Afx_device> fxdevs; // main devices/mappings array
		unsigned activeLights = 0,  // total number of active lights into the system
				 activeDevices = 0; // total number of active devices

		~Mappings();

		// Enumerate all alienware devices into the system
		// acc - link to AlienFan_SDK::Control object for ACPI lights
		// returns vector of active device objects
		vector<Functions*> AlienFXEnumDevices(void* acc);

		// Apply device vector to fxdevs structure
		// activeOnly - clear inactive devices from list
		// devList - list of active devices
		void AlienFXApplyDevices(bool activeOnly, vector<Functions*> devList);

		// Load device data and assign it to structure, as well as init devices and set brightness
		// activeOnly - clear inactive devices from list
		// acc - pointer to AlienFan_SDK::Control object for ACPI lights
		void AlienFXAssignDevices(bool activeOnly = true, void* acc = NULL);

		// load light names from registry
		void LoadMappings();

		// save light names into registry
		void SaveMappings();

		// get saved light structure by device ID and light ID
		Afx_light* GetMappingByID(WORD pid, WORD vid);

		// get defined groups vector
		vector <Afx_group>* GetGroups();

		// get defined grids vector
		vector<Afx_grid>* GetGrids() { return &grids; };

		// get grid object by it's ID
		Afx_grid* GetGridByID(byte id);

		// get device structure by PID/VID.
		// VID can be zero for any VID
		Afx_device* GetDeviceById(WORD pid, WORD vid = 0);

		// get or add device structure by PID/VID
		// VID can be zero for any VID
		Afx_device* AddDeviceById(WORD pid, WORD vid);

		// find light mapping into device structure by light ID
		Afx_light* GetMappingByDev(Afx_device* dev, WORD LightID);

		// find light group by it's ID
		Afx_group* GetGroupById(DWORD gid);

		// remove light mapping from device by id
		void RemoveMapping(Afx_device* dev, WORD lightID);

		// get light flags (Power, indicator, etc) from light structure
		int GetFlags(Afx_device* dev, WORD lightid);

		// get light flags (Power, indicator) by DevID (PID/VID)
		int GetFlags(DWORD devID, WORD lightid);
	};

}
