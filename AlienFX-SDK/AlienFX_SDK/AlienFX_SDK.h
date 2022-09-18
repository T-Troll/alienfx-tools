#pragma once
#include <wtypes.h>
#include <vector>
#include <string>
//#include "alienfx-controls.h"

#ifndef NOACPILIGHTS
#include "alienfan-SDK.h"
#endif

using namespace std;

namespace AlienFX_SDK {

	// Statuses for v1-v3
	#define ALIENFX_V2_RESET 0x06
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

	// API version:
    #define API_ACPI 0 //128
	#define API_V8 8 //65
    #define API_V7 7 //65
    #define API_V6 6 //65
	#define API_V5 5 //64
	#define API_V4 4 //34
	#define API_V3 3 //12
    #define API_V2 2 //9
	#define API_V1 1 //8

	// Mapping flags:
    #define ALIENFX_FLAG_POWER		1
    #define ALIENFX_FLAG_INDICATOR	2
	#define ALIENFX_FLAG_LOCK		4

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

	struct icommand {
		byte i, val;
	};

	struct mapping { // Light information block
		WORD lightid;
		WORD flags = 0;
		string name;
	};

	struct group { // Light group information block
		DWORD gid;
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

//#ifndef NOACPILIGHTS
		void* device = NULL;
		//AlienFan_SDK::Lights* device = NULL;
//#endif

		bool inSet = false;

		int vid = -1; // Device VID
		int pid = -1; // Device PID, -1 if not initialized
		int length = -1; // HID report length
		byte chain = 1; // seq. number for APIv1-v3
		int version = -1; // interface version
		byte reportID = 0; // HID ReportID (0 for auto)
		byte bright = 64; // Brightness for APIv4 and v6

		// support function for mask-based devices (v1-v3, v6)
		vector<icommand>* SetMaskAndColor(DWORD index, byte type, Colorcode c1, Colorcode c2 = { 0 }, byte tempo = 0, byte length = 0);

		// Support function to send data to USB device
		bool PrepareAndSend(const byte *command, byte size, vector<icommand> *mods = NULL);
		bool PrepareAndSend(const byte* command, byte size, vector<icommand> mods);

		// Support function to send whole power block for v1-v3
		bool SavePowerBlock(byte blID, act_block act, bool needSave, bool needInverse = false);

		// return current device state
		BYTE AlienfxGetDeviceStatus();

		// Next command delay for APIv1-v3
		BYTE AlienfxWaitForReady();

		// After-reset delay for APIv1-v3
		BYTE AlienfxWaitForBusy();

	public:

		// current power mode for APIv1-v3
		bool powerMode = true;

		~Functions();

		// Initialize device
		// Returns PID of device used.
		// If vid is 0, first device found into the system will be used, otherwise first device of this VID.
		// If pid is defined, device with vid/pid will be used, fist found otherwise.
		int AlienFXInitialize(int vidd = -1, int pidd = -1);

		// Check device and initialize path and vid/pid
		// in case vid/pid -1 or absent, any vid/pid acceptable
		// Returns PID of device if ok, -1 otherwise
		int AlienFXCheckDevice(string devPath, int vid = -1, int pid = -1);

#ifndef NOACPILIGHTS
		// Another init function, for Aurora ACPI init.
		// acc is a handle to low-level ACPI driver (hwacc.sys) interface - see alienfan project.
		int AlienFXInitialize(AlienFan_SDK::Control* acc);
#endif

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
		bool SetMultiColor(vector<byte> *lights, Colorcode c);

		// Set multiply lights to different color.
		// act - pointer to vector of light control blocks
		// store - need to save settings into device memory (v1-v4)
		bool SetMultiAction(vector<act_block> *act, bool store = false);

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

		// Global (whole device) effect control for APIv5, v8
		// effType - effect type
		// mode - effect mode (off, steady, keypress, etc)
		// tempo - effect tempo
		// act1 - first effect color
		// act2 - second effect color (not for all effects)
		bool SetGlobalEffects(byte effType, byte mode, byte tempo, afx_act act1, afx_act act2);

		// Apply changes and update colors
		bool UpdateColors();

		// get PID for current device
		int GetPID();

		// get VID for current device
		int GetVID();

		// get API version for current device
		int GetVersion();

		// get data length for current device
		int GetLength();

		// check global effects availability
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

	struct enum_device {
		WORD vid, pid;
		string path;
	};

	class Mappings {
	private:
		vector <group> groups; // Defined light groups
		vector <lightgrid> grids; // Grid zones info

	public:

		vector<afx_device> fxdevs; // main devices/mappings array
		int activeLights = 0; // total active lights into the system

		~Mappings();

		// Enumerate all alienware devices into the system
		vector<Functions*> AlienFXEnumDevices();

		// Load device data and assign it to structure, as well as init devices and set brightness
		// acc - link to AlienFan_SDK::Control object
		void AlienFXAssignDevices(void* acc = NULL, byte brightness=255, byte power=false);

		// load light names from registry
		void LoadMappings();

		// save light names into registry
		void SaveMappings();

		// get saved light names
		vector <mapping>* GetMappings(WORD pid, WORD vid);

		// get defined groups
		vector <group>* GetGroups();

		// get defined grids
		vector <lightgrid>* GetGrids() { return &grids; };

		// get grid object by it's ID
		lightgrid* GetGridByID(byte id);

		// get device structure by PID/VID (low/high WORD)
		afx_device* GetDeviceById(WORD pid, WORD vid);

		// get or add device structure by PID/VID
		afx_device* AddDeviceById(WORD pid, WORD vid);

		// find light mapping by PID (or PID/VID) and light ID
		mapping* GetMappingByDev(afx_device* dev, WORD LightID);

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
