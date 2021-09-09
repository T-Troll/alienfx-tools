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

	const static ALIENFAN_DEVICE devs[2] = {
		{ // m15/m17
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
		{ 0 }
	};

	class Control {
	private:
		HANDLE acc = NULL;
		short aDev = -1;
		bool activated = false;
		bool haveService = false;
		SC_HANDLE scManager = NULL;
		//int RunMainCommand(short com, byte sub, byte value1 = 0, byte value2 = 0);
	public:
		Control();
		~Control();
		void UnloadService();
		bool Probe();
		int GetFanRPM(int fanID);
		int GetFanValue(int fanID);
		bool SetFanValue(int fanID, byte value);
		int GetTempValue(int TempID);
		int Unlock();
		int SetPower(int level);
		int GetPower();
		int SetGPU(int power);
		HANDLE GetHandle();
		bool IsActivated();
		int HowManyFans();
		int HowManyPower();
		int HowManySensors();

		int RunMainCommand(short com, byte sub, byte value1 = 0, byte value2 = 0);
		int RunGPUCommand(short com, DWORD packed);

		vector<ALIENFAN_SEN_INFO> sensors;
		vector<USHORT> fans;
		vector<USHORT> powers;
		bool wrongEnvironment = false;
	};
}
