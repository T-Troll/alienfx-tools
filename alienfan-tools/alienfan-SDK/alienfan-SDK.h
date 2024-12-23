#pragma once

#include <string>
#include <vector>
#include <wtypes.h>

using namespace std;

//#define byte BYTE
#define ALIENFAN_SDK_V1

namespace AlienFan_SDK {

	struct ALIENFAN_SEN_INFO {
		union {
			struct {
				byte index;
				byte type;
			};
			WORD sid; // LOBYTE - index, HIBYTE - type: 0 = ESIF, 1 = AWCC, 2 - Disk, 3 - AMD, 4 = OHM
		};
		string name;
	};

	struct ALIENFAN_FAN_INFO {
		byte id, type;
	};

	struct ALIENFAN_COMMAND {
		byte /*short*/ com;
		byte sub;
	};

	struct ALIENFAN_DEVICE {
		string mainCommand;
		string gpuCommand;
		//bool commandControlled;
		short delta;
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
		ALIENFAN_COMMAND getGMode;
		ALIENFAN_COMMAND setGMode;
		ALIENFAN_COMMAND getSystemID;
		ALIENFAN_COMMAND getFanType;
		ALIENFAN_COMMAND getMaxRPM;
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
		short aDev = -1;
		bool activated = false;
		DWORD systemID = 0;
		bool lastMode = 0;
#ifdef _SERVICE_WAY_
		SC_HANDLE scManager = NULL;
#endif
		//int ReadRamDirect(DWORD);
		//int WriteRamDirect(DWORD, byte);

	public:
		Control();
		~Control();

		bool isTcc = false, isXMP = false;
		byte maxTCC = 100, maxOffset = 16;

#ifdef _SERVICE_WAY_
		// Stop and unload service if driver loaded from service
		void UnloadService();
#endif
		// Probe hardware, sensors, fans, power modes and fill structures.
		// Result: true - compatible hardware found, false - not found.
		bool Probe(bool diskSensors);

		// Get RPM for the fan index fanID at fans[]
		// Result: fan RPM
		int GetFanRPM(int fanID);

		// Get fan RPMs as a percent of RPM
		// Result: percent of the fan speed
		int GetFanPercent(int fanID);

		// Get boost value for the fan index fanID at fans[]. If force, raw value returned, otherwise cooked by boost
		// Result: Error or raw value if forced, otherwise cooked by boost.
		int GetFanBoost(int fanID);

		// Set boost value for the fan index fanID at fans[]. If force, raw value set, otherwise cooked by boost.
		// Result: value or error
		int SetFanBoost(int fanID, byte value);

		// Get temperature value for the sensor index TanID at sensors[]
		// Result: temperature value or error
		int GetTempValue(int TempID);

		// Unlock manual fan operations. The same as SetPower(0)
		// Result: raw value set or error
		int Unlock();

		// Set system power profile to power index at powers[]
		// Result: raw value set or error
		int SetPower(byte level);

		// Get current system power value index at powers[]
		// Result: power value index in powers[]
		int GetPower(bool raw = false);

		// Set system GPU limit level (0 - no limit, 3 - min. limit)
		// Result: success or error
		int SetGPU(int power);

		// Toggle G-mode on some systems
		int SetGMode(bool state);

		// Check G-mode state
		int GetGMode();

		// Max. RPM for fan
		int GetMaxRPM(int fanID);

		// Return current device ID
		inline DWORD GetSystemID() { return systemID; };

		int GetCharge();

		int SetCharge(byte val);

		// Fake OC
		int GetTCC() { return -1; };
		int SetTCC(byte temp) { return -1; };
		int GetXMP() { return -1; };
		int SetXMP(byte xmp) { return -1; };

		// Get low-level driver handle for direct operations
		// Result: handle to driver or NULL
		//HANDLE GetHandle();

		// Call ACPI system control method with given parameters - see ALIENFAN_DEVICE for details
		// Result: reply from the driver or error
		int RunMainCommand(ALIENFAN_COMMAND com, byte value1 = 0, byte value2 = 0);

		// Call ACPI GPU control method with given parameters - see ALIENFAN_DEVICE for details
		// Result: reply from the driver or error
		int RunGPUCommand(short com, DWORD packed);

		// Arrays of sensors, fans, max. boosts and power values detected at Probe()
		vector<ALIENFAN_SEN_INFO> sensors;
		vector<ALIENFAN_FAN_INFO> fans;
		vector<byte> powers;
		HANDLE acc = NULL;

		bool isAlienware = false, isSupported = false, isGmode = false, isCharge = false;

#ifdef _SERVICE_WAY_
		// true if driver connection fails, as well as start driver attempt fails. Indicates you have not enough rights or system not configured correctly.
		bool wrongEnvironment = true;
#endif
	};

	class Lights {
	private:
		Control *acpi = NULL;
		bool inCommand = false;
	public:
		Lights(Control *ac);

		// Resets light subsystem
		bool Reset();

		// Prepare for operations
		bool Prepare();

		// Update lights state (end operation)
		bool Update();

		// Set color of lights mask defined by id to RGB
		bool SetColor(byte mask, byte r, byte g, byte b, bool save = false);

		// Set light system mode (brightness, ???)
		bool SetBrightness(byte mode);

		//// Return color subsystem availability
		//bool IsActivated();
		bool isActivated = false;
	};
}
