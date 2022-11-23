#pragma once


namespace AlienFan_SDK {

	static const BSTR commandList[7]{
		(BSTR)L"Thermal_Information",	// 0x14
		(BSTR)L"Thermal_Control",		// 0x15
		(BSTR)L"GameShiftStatus",		// 0x25
		(BSTR)L"SystemInformation",		// 0x1A
		(BSTR)L"GetFanSensors",			// 0x13
		(BSTR)L"GetThermalInfo2",		// 0x10
		(BSTR)L"SetThermalControl2"		// 0x11
	};

	//static const BSTR colorList[2]{
	//	(BSTR)L"Set24BitsLEDColor",		// 0x12
	//	(BSTR)L"LEDBrightness"			// 0x03
	//};

	static const byte functionID[2][12]{
		{ 0,0,0,0,0,0,1,1,2,2,3,4 },
		{ 5,5,5,5,5,5,6,6,2,2,3,4 }
	};

	static const byte dev_controls[12]{
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
		2  // Get fan sensor ID
	};

	static const char* temp_names[2]{
			"CPU Internal Thermistor",
			"GPU Internal Thermistor"//,
			//"Motherboard Thermistor",
			//"CPU External Thermistor",
			//"Memory Thermistor",
			//"GPU External Thermistor"
	};
}
