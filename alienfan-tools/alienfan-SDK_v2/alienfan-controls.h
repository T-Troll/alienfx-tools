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

	static vector<string> temp_names{
			"CPU Internal Thermistor",
			"GPU Internal Thermistor"//,
			//"Motherboard Thermistor",
			//"CPU External Thermistor",
			//"Memory Thermistor",
			//"GPU External Thermistor"
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

	//static const ALIENFAN_DEVICE devs[NUM_DEVICES]{
	//	{ // Alienware m15/m17
	//		"\\_SB.AMW1.WMAX", // main command
	//		"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
	//		true, // command controlled
	//		0, // controlID
	//		0x14,   2, // Probe command
	//	},
	//	{ // Dell G15
	//		"\\_SB.AMW3.WMAX", // main command
	//		"\\_SB.PCI0.GPP0.PEGP.GPS", // GPU command
	//		true, // command controlled
	//		0, // controlID
	//		0x14,   2, // Probe command
	//	},
	//	{ // Dell G5 SE and Alienware m15R6
	//		"\\_SB.AMWW.WMAX", // main command
	//		"\\_SB.PC00.PEG1.PEGP.GPS", // GPU command
	//		true, // command controlled
	//		0, // controlID
	//		0x14,   2, // Probe command
	//	},
	//	{ // Aurora R7
	//		"\\_SB.AMW1.WMAX", // main command
	//		"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
	//		true, // command controlled
	//		1, // controlID
	//		0x14,   2, // Probe command
	//	},
	//	{ // Area 51R4
	//		"\\_SB.WMI2.WMAX", // main command
	//		"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
	//		true, // command controlled
	//		1, // controlID
	//		0x14,   2, // Probe command
	//	},
	//	{ // Alienware 13R2, 15R2
	//		"\\_SB.AMW0.WMBC", // main command
	//		"\\_SB.PCI0.PEG0.PEGP.GPS", // GPU command
	//		false, // command controlled
	//		0, // controlID
	//		0x14,   5, // Probe command
	//	}
	//};
}
