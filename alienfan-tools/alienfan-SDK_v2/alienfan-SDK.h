#pragma once

#include <string>
#include <vector>
#include <wbemidl.h>
#include <wtypes.h>

using namespace std;

#define byte BYTE

#define DEV_FLAG_AWCC		1
#define DEV_FLAG_INFO		2
#define DEV_FLAG_CONTROL	4
#define DEV_FLAG_GMODE		8
#define DEV_FLAG_ESIF		0x10

namespace AlienFan_SDK {

	struct ALIENFAN_SEN_INFO {
		SHORT senIndex = 0;
		string name;
		byte type = 0; // 0 = TZ (ESIF), 1 = AWCC, 2 = ECDV1, 3 = ECDV 2, 4 = OHM
		BSTR instance; // for ESIF/OHM sensors
	};

	struct ALIENFAN_COMMAND {
		byte com;
		byte sub;
	};

	//struct ALIENFAN_DEVICE {
	//	string mainCommand;
	//	string gpuCommand;
	//	bool commandControlled;
	//	short delta;
	//	ALIENFAN_COMMAND probe;
	//};

	struct ALIENFAN_CONTROL {
		ALIENFAN_COMMAND getPowerID;
		ALIENFAN_COMMAND getFanRPM;
		ALIENFAN_COMMAND getFanPercent;
		ALIENFAN_COMMAND getFanBoost;
		ALIENFAN_COMMAND setFanBoost;
		ALIENFAN_COMMAND getTemp;
		ALIENFAN_COMMAND getPower;
		ALIENFAN_COMMAND setPower;
		//ALIENFAN_COMMAND setGPUPower;
		ALIENFAN_COMMAND getGMode;
		ALIENFAN_COMMAND setGMode;
	};

	//struct ALIENFAN_COMMAND_CONTROL {
	//	short unlock;
	//	string readCom;
	//	string writeCom;
	//	byte numtemps;
	//	byte numfans;
	//	vector<string> getTemp;
	//	vector<short> fanID;
	//};

	class Control {
	private:
		VARIANT m_instancePath;
		byte devFlags = 0;
		//DWORD systemID = 0;
		//bool haveGmode = false;
		//int ReadRamDirect(DWORD);
		//int WriteRamDirect(DWORD, byte);

	public:
		//VARIANT m_instancePath;
		IWbemServices* m_WbemServices = NULL, *m_OHMService = NULL;
		Control();
		~Control();

#ifdef _SERVICE_WAY_
		// Stop and unload service if driver loaded from service
		void UnloadService();
#endif
		// Probe hardware, sensors, fans, power modes and fill structures.
		// Result: true - compatible hardware found, false - not found.
		bool Probe();

		// Get RPM for the fan index fanID at fans[]
		// Result: fan RPM
		int GetFanRPM(int fanID);

		// Get fan RPMs as a percent of RPM
		// Result: percent of the fan speed
		int GetFanPercent(int fanID);

		// Get boost value for the fan index fanID at fans[]. If force, raw value returned, otherwise cooked by boost
		// Result: Error or raw value if forced, otherwise cooked by boost.
		int GetFanBoost(int fanID, bool force = false);

		// Set boost value for the fan index fanID at fans[]. If force, raw value set, otherwise cooked by boost.
		// Result: value or error
		int SetFanBoost(int fanID, byte value, bool force = false);

		// Get temperature value for the sensor index TanID at sensors[]
		// Result: temperature value or error
		int GetTempValue(int TempID);

		// Unlock manual fan operations. The same as SetPower(0)
		// Result: raw value set or error
		int Unlock();

		// Set system power profile to power level (value from powers[])
		// Result: raw value set or error
		int SetPower(byte level);

		// Get current system power value index at powers[]
		// Result: power value index in powers[]
		int GetPower();

		// Set system GPU limit level (0 - no limit, 3 - min. limit)
		// Result: success or error
		int SetGPU(int power);

		// Toggle G-mode on some systems
		int SetGMode(bool state);

		// Check G-mode state
		int GetGMode();

		// Get low-level driver handle for direct operations
		// Result: handle to driver or NULL
		HANDLE GetHandle();

		// True if driver activated and ready, false if not
		//bool IsActivated();

		// Return number of fans into fans[] detected at Probe()
		int HowManyFans();

		// Return number of power levels into powers[] detected at Probe()
		int HowManyPower();

		// Return number of temperature sensors into sensors[] detected at Probe()
		int HowManySensors();

		// Return current device capability
		inline byte GetDeviceFlags() { return devFlags; };

		// Return current device ID
		//inline DWORD GetSystemID() { return systemID; };

		// Call custom Alienware method trough WMI
		int CallWMIMethod(ALIENFAN_COMMAND com, byte arg1 = 0, byte arg2 = 0);

		//// Call ACPI system control method with given parameters - see ALIENFAN_DEVICE for details
		//// Result: reply from the driver or error
		//int RunMainCommand(ALIENFAN_COMMAND com, byte value1 = 0, byte value2 = 0);

		//// Call ACPI GPU control method with given parameters - see ALIENFAN_DEVICE for details
		//// Result: reply from the driver or error
		//int RunGPUCommand(short com, DWORD packed);

		// Arrays of sensors, fans, max. boosts and power values detected at Probe()
		vector<ALIENFAN_SEN_INFO> sensors;
		vector<USHORT> fans;
		vector<byte> boosts;
		vector<WORD> maxrpm;
		vector<byte> powers;

		IWbemClassObject* m_AWCCGetObj = NULL;
	};

	class Lights {
	private:
		//Control *acpi = NULL;
		//bool inCommand = false;
		bool activated = false;
	public:
		Lights(Control *ac);

		// Resets light subsystem
		bool Reset();

		// Prepare for operations
		bool Prepare();

		// Update lights state (end operation)
		bool Update();

		// Set color of lights mask defined by id to RGB
		bool SetColor(byte id, byte r, byte g, byte b);

		// Set light system mode (brightness, ???)
		bool SetMode(byte mode, bool onoff);

		// Return color subsystem availability
		bool IsActivated();
	};
}
