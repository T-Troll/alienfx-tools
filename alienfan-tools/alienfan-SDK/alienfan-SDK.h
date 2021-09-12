#pragma once

#include <windows.h>
#include <string>
#include <vector>

using namespace std;

#define byte BYTE

namespace AlienFan_SDK {

	struct ALIENFAN_SEN_INFO {
		SHORT senIndex = 0;
		string name;
		bool isFromAWC = false;
	};

	struct ALIENFAN_COMMAND {
		short com;
		byte sub;
	};

	struct ALIENFAN_DEVICE {
		char mainCommand[15];
		ALIENFAN_COMMAND getFanID;
		ALIENFAN_COMMAND getPowerID;
		ALIENFAN_COMMAND getZoneSensorID;
		ALIENFAN_COMMAND getFanRPM;
		ALIENFAN_COMMAND getFanBoost;
		ALIENFAN_COMMAND setFanBoost;
		ALIENFAN_COMMAND getTemp;
		ALIENFAN_COMMAND getPower;
		ALIENFAN_COMMAND setPower;
		ALIENFAN_COMMAND setGPUPower;
	};

	static ALIENFAN_DEVICE devs[2] = {
		{ // m15/m17
			"\\_SB.AMW1.WMAX", // main command
			0x13,   1, // FanID
			0x14,   3, // PowerID
			0x13,   2, // ZoneSensor ID
			0x14,   5, // RPM
			0x14, 0xc, // Boost
			0x15,   2, // Set boost
			0x14,   4, // Temp
			0x14, 0xb, // Get Power (value, not index!)
			0x15,   1, // Set Power
			0x13,   4  // GPU power
		}, 
		{ "", 0 }
	};

	class Control {
	private:
		HANDLE acc = NULL;
		short aDev = -1;
		bool activated = false;
		//bool haveService = false;
		SC_HANDLE scManager = NULL;
	public:
		Control();
		~Control();

		// Stop and unload service if driver loaded from service
		void UnloadService();

		// Probe hardware, sensors, fans, power modes and fill structures.
		// Result: true - compatible hardware found, false - not found.
		bool Probe();

		// Get RPM for the fan index fanID at fans[]
		int GetFanRPM(int fanID);

		// Get boost value for the fan index fanID at fans[]
		int GetFanValue(int fanID);

		// Set boost value for the fan index fanID at fans[] (0..100)
		bool SetFanValue(int fanID, byte value);

		// Get temperature value for the sensor index TanID at sensors[]
		int GetTempValue(int TempID);

		// Unlock manual fan operations. The same as SetPower(0)
		int Unlock();

		// Set system power profile to power index at powers[]
		int SetPower(int level);

		// Get current system power value index at powers[]
		int GetPower();

		// Set system GPU limit level (0 - no limit, 3 - min. limit)
		int SetGPU(int power);

		// Get low-level driver handle for direct operations
		HANDLE GetHandle();

		// True if driver activated and ready, false if not
		bool IsActivated();

		// Return number of fans into fans[] detected at Probe()
		int HowManyFans();

		// Return number of power levels into powers[] detected at Probe()
		int HowManyPower();

		// Return number of temperature sensors into sensors[] detected at Probe()
		int HowManySensors();

		// Call ACPI system control method with given parameters - see ALIENFAN_DEVICE for details
		int RunMainCommand(short com, byte sub, byte value1 = 0, byte value2 = 0);

		// Call ACPI GPU control method with given parameters - see ALIENFAN_DEVICE for details
		int RunGPUCommand(short com, DWORD packed);

		// Arrays of sensors, fans and power values detected at Probe()
		vector<ALIENFAN_SEN_INFO> sensors;
		vector<USHORT> fans;
		vector<USHORT> powers;

		// true if driver connection fails, as well as start driver attempt fails. Indicates you have not enough rights or system not configured correctly.
		bool wrongEnvironment = false;
	};
}
