#pragma once

#include <string>
#include <vector>
#include <wtypes.h>

using namespace std;

#define byte BYTE

namespace AlienFan_SDK {

	struct ALIENFAN_SEN_INFO {
		SHORT senIndex = 0;
		string name;
		bool isFromAWC = false;
	};

	struct ALIENFAN_COMMAND {
		byte /*short*/ com;
		byte sub;
	};

	struct ALIENFAN_DEVICE {
		string mainCommand;
		//int  errorCode;
		byte maxBoost;
		bool commandControlled;
		short controlID;
		ALIENFAN_COMMAND probe;
	};

	struct ALIENFAN_CONTROL {
		ALIENFAN_COMMAND getPowerID;
		ALIENFAN_COMMAND getFanRPM;
		ALIENFAN_COMMAND getFanPercent;
		ALIENFAN_COMMAND getFanBoost;
		ALIENFAN_COMMAND setFanBoost;
		ALIENFAN_COMMAND getTemp;
		ALIENFAN_COMMAND getPower;
		ALIENFAN_COMMAND setPower;
		ALIENFAN_COMMAND setGPUPower;
	};

	struct ALIENFAN_COMMAND_CONTROL {
		short unlock;
		string readCom;
		string writeCom;
		byte numtemps;
		byte numfans;
		vector<string> getTemp;
		vector<short> fanID;
	};

	class Control {
	private:
		HANDLE acc = NULL;
		short aDev = -1;
		short cDev = -1;
		int systemID = 0;
		bool activated = false;
		SC_HANDLE scManager = NULL;

		int ReadRamDirect(DWORD);
		int WriteRamDirect(DWORD, byte);

	public:
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

		// Get fan RPMs as a persent from 100%
		// Result: percent of the fan speed
		int GetFanPercent(int fanID);

		// Get boost value for the fan index fanID at fans[]. If force, raw value returned, otherwise coocked by boost
		// Result: Error or raw value if forced, otherwise coocked by boost.
		int GetFanValue(int fanID, bool force = false);

		// Set boost value for the fan index fanID at fans[]. If force, raw value set, otherwise coocked by boost.
		// Result: value or error
		int SetFanValue(int fanID, byte value, bool force = false);

		// Get temperature value for the sensor index TanID at sensors[]
		// Result: temperature value or error
		int GetTempValue(int TempID);

		// Unlock manual fan operations. The same as SetPower(0)
		// Result: raw value set or error
		int Unlock();

		// Set system power profile to power index at powers[]
		// Result: raw value set or error
		int SetPower(int level);

		// Get current system power value index at powers[]
		// Result: power value index in powers[]
		int GetPower();

		// Set system GPU limit level (0 - no limit, 3 - min. limit)
		// Result: success or error
		int SetGPU(int power);

		// Get low-level driver handle for direct operations
		// Result: handle to driver or NULL
		HANDLE GetHandle();

		// True if driver activated and ready, false if not
		bool IsActivated();

		// Return number of fans into fans[] detected at Probe()
		int HowManyFans();

		// Return number of power levels into powers[] detected at Probe()
		int HowManyPower();

		// Return number of temperature sensors into sensors[] detected at Probe()
		int HowManySensors();

		// Return internal module version
		int GetVersion();

		// Call ACPI system control method with given parameters - see ALIENFAN_DEVICE for details
		// Result: reply from the driver or error
		int RunMainCommand(ALIENFAN_COMMAND com, byte value1 = 0, byte value2 = 0);

		// Call ACPI GPU control method with given parameters - see ALIENFAN_DEVICE for details
		// Result: reply from the driver or error
		int RunGPUCommand(short com, DWORD packed);

		// Arrays of sensors, fans, max. boosts and power values detected at Probe()
		vector<ALIENFAN_SEN_INFO> sensors;
		vector<USHORT> fans;
		vector<byte> boosts;
		vector<byte> powers;

#ifdef _SERVICE_WAY_
		// true if driver connection fails, as well as start driver attempt fails. Indicates you have not enough rights or system not configured correctly.
		bool wrongEnvironment = true;
#endif
	};

	class Lights {
	private:
		Control *acpi = NULL;
		bool inCommand = false;
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

		// Return color subsystem avaliability
		bool IsActivated();
	};
}
