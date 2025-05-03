#pragma once

#define NUM_DEVICES 5

namespace AlienFan_SDK {
	static const ALIENFAN_CONTROL dev_controls
		{
			0x14,   3, // PowerID
			0x14,   5, // RPM
			0x14,   6, // Percent
			0x14, 0xc, // Boost
			0x15,   2, // Set boost
			0x14,   4, // Temp
			0x14, 0xb, // Get Power
			0x15,   1, // Set Power
			0x13,   4, // GPU power
			0x25,	2, // Get G-Mode
			0x25,	1, // Toggle G-Mode
			0x1a,	2, // Get system ID
			0x13,	2, // Get fan sensor ID
			0x14,	9  // Get Max. RPM
		};

	static const char* temp_names[3]{
			"CPU Internal Thermistor",
			"GPU Internal Thermistor",
			"Motherboard Thermistor"/*,
			"CPU External Thermistor",
			"Memory Thermistor",
			"GPU External Thermistor"*/
	};

	//static const ALIENFAN_COMMAND_CONTROL dev_c_controls
	//	{
	//		0x8ab, // unlock
	//		"\\_SB.PCI0.MMRB", // read RAM
	//		"\\_SB.PCI0.MMWB", // write RAM
	//		6,
	//		2,
	//	    { "\\_SB.PCI0.LPCB.EC0.CPUT",
	//		"\\_SB.PCI0.LPCB.EC0.GPUT",
	//		"\\_SB.PCI0.LPCB.EC0.PCHT",
	//		"\\_SB.PCI0.LPCB.EC0.CVRT",
	//		"\\_SB.PCI0.LPCB.EC0.MEMT",
	//		"\\_SB.PCI0.LPCB.EC0.GVRT"
	//        },
	//	    {0x902, 0x929}
	//	//}
	//};

	static const ALIENFAN_DEVICE devs[NUM_DEVICES]{
		{ // Alienware m15/m17
			"\\_SB.AMW1.WMAX", // main command
			"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
			//true, // command controlled
			0, // controlID
			0x14,   2, // Probe command
		},
		{ // Dell G15
			"\\_SB.AMW3.WMAX", // main command
			"\\_SB.PCI0.GPP0.PEGP.GPS", // GPU command
			//true, // command controlled
			0, // controlID
			0x14,   2, // Probe command
		},
		{ // Dell G5 SE and Alienware m15R6
			"\\_SB.AMWW.WMAX", // main command
			"\\_SB.PC00.PEG1.PEGP.GPS", // GPU command
			//true, // command controlled
			0, // controlID
			0x14,   2, // Probe command
		},
		{ // Aurora R7
			"\\_SB.AMW1.WMAX", // main command
			"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
			//true, // command controlled
			4, // controlID
			0x14,   2, // Probe command
		},
		{ // Area 51R4
			"\\_SB.WMI2.WMAX", // main command
			"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
			//true, // command controlled
			4, // controlID
			0x14,   2, // Probe command
		}//,
		//{ // Alienware 13R2, 15R2
		//	"\\_SB.AMW0.WMBC", // main command
		//	"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
		//	false, // command controlled
		//	0, // controlID
		//	0x14,   5, // Probe command
		//}
	};
}
