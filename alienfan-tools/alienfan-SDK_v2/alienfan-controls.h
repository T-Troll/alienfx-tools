#pragma once

namespace AlienFan_SDK {

	const BSTR commandList[] = {
		(BSTR)L"Thermal_Information",	// 0 0x14
		(BSTR)L"Thermal_Control",		// 1 0x15
		(BSTR)L"GameShiftStatus",		// 2 0x25
		//(BSTR)L"SystemInformation",		// 3 0x1A
		(BSTR)L"GetFanSensors",			// 3 0x13
		(BSTR)L"GetThermalInfo2",		// 4 0x10
		(BSTR)L"SetThermalControl2",	// 5 0x11
		(BSTR)L"TccControl",			// 6 0x1e
		(BSTR)L"MemoryOCControl"		// 7 0x17
	};

	const byte functionID[2][19] = {
		{ 0,0,0,0,0,0,1,1,2,2,0,3,0,6,6,6,6,7,7 },
		{ 5,5,5,5,5,5,6,6,2,2,5,4,5,6,6,6,6,7,7 }
		//{ 0,0,0,0,0,0,1,1,2,2,3,4,0,7,7,7,7,8,8 },
		//{ 5,5,5,5,5,5,6,6,2,2,3,4,5,7,7,7,7,8,8 }
	};

	const byte dev_controls[] = {
		3, // PowerID
		5, // RPM
		6, // Percent
		0xc, // Get Boost
		4, // Temp
		0xb, // Get Power
		2, // Set boost
		1, // Set Power
		2, // Get G-Mode
		1, // Toggle G-Mode
		2, // Get system ID
		2, // Get fan sensor ID
		9, // Get Max. RPM
		1, // Get max TCC
		2, // Get max offset
		3, // Get current offset
		4, // Set current offset
		2, // Get XMP
		3, // Set XMP
	};

	enum { // devcontrol names
		getPowerID = 0,
		getFanRPM = 1,
		getFanPercent = 2,
		getFanBoost = 3,
		getTemp = 4,
		getPowerMode = 5,
		setFanBoost = 6,
		setPowerMode = 7,
		getGMode = 8,
		setGMode = 9,
		getSysID = 10,
		getFanSensor = 11,
		getMaxRPM = 12,
		getMaxTCC = 13,
		getMaxOffset = 14,
		getCurrentOffset = 15,
		setOffset = 16,
		getXMP = 17,
		setXMP = 18
	};

	const char* temp_names[] = {
			"CPU Internal Thermistor",
			"GPU Internal Thermistor",
			"Motherboard Thermistor"//,
			//"CPU External Thermistor",
			//"Memory Thermistor",
			//"GPU External Thermistor"
	};
}
