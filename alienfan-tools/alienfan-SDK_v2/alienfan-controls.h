#pragma once

//#define NUM_DEVICES 6

static const BSTR commandList[3] = {
	(BSTR)L"Thermal_Information", // 0x14
	(BSTR)L"Thermal_Control", // 0x15
	(BSTR)L"GameShiftStatus" // 0x25
};

namespace AlienFan_SDK {
	static const ALIENFAN_CONTROL dev_controls
		{
			0x0,   3, // PowerID
			0x0,   5, // RPM
			0x0,   6, // Percent
			0x0, 0xc, // Boost
			0x1,   2, // Set boost
			0x0,   4, // Temp
			0x0, 0xb, // Get Power
			0x1,   1, // Set Power
			//0x13,   4, // GPU power
			0x2,	2, // Get G-Mode
			0x2,	1, // Toggle G-Mode
		};

	static const string temp_names[2]{
			"CPU Internal Thermistor",
			"GPU Internal Thermistor"//,
			//"Motherboard Thermistor",
			//"CPU External Thermistor",
			//"Memory Thermistor",
			//"GPU External Thermistor"
	};
}
