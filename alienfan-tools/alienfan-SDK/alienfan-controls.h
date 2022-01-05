#pragma once

#define NUM_DEVICES 6

namespace AlienFan_SDK {
	static const ALIENFAN_CONTROL dev_controls[2]{
		{   0x14,   3, // PowerID
			0x14,   5, // RPM
			0x14,   6, // Percent
			0x14, 0xc, // Boost
			0x15,   2, // Set boost
			0x14,   4, // Temp
			0x14, 0xb, // Get Power
			0x15,   1, // Set Power
			0x13,   4  // GPU power
		},
		{
			0x10,   3, // PowerID
			0x10,   5, // RPM
			0x10,   6, // Percent
			0x10, 0xc, // Boost
			0x11,   2, // Set boost
			0x10,   4, // Temp
			0x10, 0xb, // Get Power
			0x11,   1, // Set Power
			0,   0  // GPU power
		}
	};

	//static const char temp_names[6][24] = {
	static vector<string> temp_names{
			"CPU Internal Thermistor",
			"GPU Internal Thermistor",
			"Motherboard Thermistor",
			"CPU External Thermistor",
			"Memory Thermistor",
			"GPU External Thermistor"
	};

	static const ALIENFAN_COMMAND_CONTROL dev_c_controls[1]{
		{
			0x8ab, // unlock
			"\\_SB.PCI0.MMRB", // read RAM
			"\\_SB.PCI0.MMWB", // write RAM
			6,
			2,
		    { "\\_SB.PCI0.LPCB.EC0.CPUT",
			"\\_SB.PCI0.LPCB.EC0.GPUT",
			"\\_SB.PCI0.LPCB.EC0.PCHT",
			"\\_SB.PCI0.LPCB.EC0.CVRT",
			"\\_SB.PCI0.LPCB.EC0.MEMT",
			"\\_SB.PCI0.LPCB.EC0.GVRT"
	        },
		    {0x902, 0x929}
		}
	};

	static const ALIENFAN_DEVICE devs[NUM_DEVICES]{
		{ // Alienware m15/m17
			"\\_SB.AMW1.WMAX", // main command
			//-1, // Error code
			105, // max. boost
			true, // command controlled
			0, // controlID
			0x14,   2, // Probe command
		}, 
		{ // Dell G15
			"\\_SB.AMW3.WMAX", // main command
			//0, // Error code
			150, // Max. boost
			true, // command controlled
			0, // controlID
			0x14,   2, // Probe command
		},
		{ // Dell G5 SE
			"\\_SB.AMWW.WMAX", // main command
			//0, // Error code
			100, // Max. boost
			true, // command controlled
			0, // controlID
			0x14,   2, // Probe command
		},
		{ // Aurora R7
			"\\_SB.AMW1.WMAX", // main command
			//-1, // Error code
			100, // Max. boost
			true, // command controlled
			1, // controlID
			0x10,   2, // Probe command
		},
		{ // Area 51R4
			"\\_SB.WMI2.WMAX", // main command
			//-1, // Error code
			100, // Max. boost
			true, // command controlled
			1, // controlID
			0x10,   2, // Probe command
		},
		{ // Alienware 13R2, 15R2
			"\\_SB.AMW0.WMBC", // main command
			//0, // Error code
			99, // Max. boost
			false, // command controlled
			0, // controlID
			0x14,   5, // Probe command
		}
	};
}
