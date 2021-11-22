#pragma once  
#include <wtypesbase.h>
#include <vector>
#include <string>

using namespace std;

#define byte BYTE

namespace AlienFX_SDK {	

	// Old alieware device statuses v1-v3
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

	// Length by API version:
    #define API_L_ACPI 0 //128
    #define API_L_V7 7 //65
    #define API_L_V6 6 //65
	#define API_L_V5 5 //64
	#define API_L_V4 4 //34
	#define API_L_V3 3 //12
    #define API_L_V2 2 //9
	#define API_L_V1 1 //8

	// Mapping flags:
    #define ALIENFX_FLAG_POWER 1
    #define ALIENFX_FLAG_INACTIVE 2

	// Maximal buffer size across all device types
    #define MAX_BUFFERSIZE 65

	struct mapping {
		WORD vid = 0;
		WORD devid = 0;
		WORD lightid = 0;
		WORD flags = 0;
		string name;
	};

	struct devmap {
		WORD vid = 0;
		WORD devid = 0;
		string name;
	};

	struct group {
		DWORD gid = 0;
		string name;
		vector<mapping*> lights;
	};

	struct afx_act { // atomic light action phase
		BYTE type = 0; // one of Action values - action type
		BYTE time = 0; // How long this phase stay
		BYTE tempo = 0; // How fast it should transform
		BYTE r = 0; // phase color
		BYTE g = 0;
		BYTE b = 0;
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
		//void SetMaskAndColor(int index, byte* buffer, byte r1, byte g1, byte b1, byte r2 = 0, byte g2 = 0, byte b2 = 0);
		vector<pair<byte, byte>> *SetMaskAndColor(DWORD index, byte type, byte r1, byte g1, byte b1, byte r2 = 0, byte g2 = 0, byte b2 = 0);

		// Support functions for ACPI calls (v0)
		bool SetAcpiColor(byte mask, byte r, byte g, byte b);

		// Support function to send data to USB device
		bool PrepareAndSend(const byte *command, byte size, vector<pair<byte, byte>> mods);
		bool PrepareAndSend(const byte *command, byte size, vector<pair<byte, byte>> *mods = NULL);

		// Support function to send whole power block for v1-v3
		bool SavePowerBlock(byte blID, act_block act, bool needSave, bool needInverse = false);

	public:

		bool powerMode = true; // current power mode for APIv1-v3

		// Initialize device
		// Returns PID of device used. 
		// If vid is 0, first device found into the system will be used, othervise first device of this type.
		// If pid is defined, device with vid/pid will be used.
		int AlienFXInitialize(int vid, int pid = -1);

		// Another init function, for Aurora ACPI init.
		// acc is a handle to low-level ACPI driver (hwacc.sys) interface - see alienfan project.
		int AlienFXInitialize(HANDLE acc);

		//De-init
		bool AlienFXClose();

		// Switch to other AlienFX device
		bool AlienFXChangeDevice(int vid, int pid, HANDLE acc = NULL);

		//Enable/Disable all lights (or just prepare to set)
		bool Reset();

		void Loop();

		BYTE IsDeviceReady();

		bool SetColor(unsigned index, byte r, byte g, byte b, bool loop = true);

		// Set multipy lights to the same color. This only works for new API devices, and emulated at old ones.
		// numLights - how many lights need to be set
		// lights - pointer to array of light IDs need to be set.
		bool SetMultiLights(vector<byte> *lights, int r, int g, int b);

		// Set multipy lights to different color.
		// size - how many lights
		// lights - pointer to array of light IDs need to be set (should be "size")
		// act - array of light colors set (should be "size)
		// store - need to save solors into device memory (v1-v3)
		bool SetMultiColor(vector<act_block> *act, bool store = false);

		// Set color to action
		// action - action type (see enum above)
		// time - how much time to keep action (0-255)
		// tempo - how fast to do evolution (f.e. pulse - 0-255) 
		// It can possible to mix 2 actions in one (useful for morph), in this case use action2...Blue2
		bool SetAction(act_block *act);
			//int action, int time, int tempo, int Red, int Green, int Blue, int action2 = AlienFX_A_NoAction, int time2 = 0, int tempo2=0, int Red2 = 0, int Green2 = 0, int Blue2 = 0);

		// Set action for Power button
		// For now, settings as a default of AWCC, but it possible to do it more complex
		bool SetPowerAction(vector<act_block> *act);

		// Hardware enable/disable lights
		// newState - on/off
		// mappings - needed to keep some lights on
		// power - if true, power and indicator lights will be set on/off too
		bool ToggleState(BYTE brightness, vector <mapping*>* mappings, bool power);

		bool SetGlobalEffects(byte effType, byte tempo, afx_act act1, afx_act act2);

		// return current device state
		BYTE AlienfxGetDeviceStatus();

		BYTE AlienfxWaitForReady();

		BYTE AlienfxWaitForBusy();

		// Apply changes and update colors
		bool UpdateColors();

		// get PID in use
		int GetPID();

		// get VID in use
		int GetVid();

		// get version for current device
		int GetVersion();
	};

	struct afx_device {
		Functions *dev;
		devmap *desc;
		vector <mapping *> lights;
	};

	class Mappings {
	private:
		// Name mappings for lights
		vector <mapping*> mappings;
		vector <devmap> devices;
		vector <group> groups;

	public:

		vector<afx_device> fxdevs;

		~Mappings();

		// Enum alienware devices
		vector<pair<WORD,WORD>> AlienFXEnumDevices();

		// Load device data and assign it to structure
		void AlienFXAssignDevices(HANDLE acc = NULL, byte brightness=255, byte power=false);

		// load light names from registry
		void LoadMappings();

		// save light names into registry
		void SaveMappings();

		// get saved devices names
		vector<devmap>* GetDevices();

		// get saved light names
		vector <mapping*>* GetMappings();

		// get defined groups
		vector <group>* GetGroups();

		devmap* GetDeviceById(WORD devID, WORD vid = 0);

		// find mapping by dev/light it...
		mapping* GetMappingById(WORD devID, WORD LightID);

		// find mapping by dev/light it...
		group* GetGroupById(DWORD gid);

		// add new light name into the list field-by-field
		void AddMapping(WORD devID, WORD lightID, char* name, WORD flags);

		// Add new group into the list field-by-field
		//void AddGroup(DWORD gID, char* name, int lightNum, DWORD* lightlist);

		// get saved light names
		int GetFlags(WORD devid, WORD lightid);

		// get saved light names
		void SetFlags(WORD devid, WORD lightid, WORD flags);
	};

}
