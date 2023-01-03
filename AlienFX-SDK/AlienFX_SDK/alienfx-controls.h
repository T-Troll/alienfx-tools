#pragma once

#define NUM_VIDS 5

namespace AlienFX_SDK {

	//This is VIDs for different devices: Alienware (common), Darfon (RGB keyboards), Microship (monitors), Primax (mouses), Chicony (external keyboards)
	//enum Afx_VIDs {
	//	Aleinware = 0x187c,
	//	Darfon = 0x0d62,
	//	Microchip = 0x0424,
	//	Primax = 0x461,
	//	Chicony = 0x4f2
	//};

	//static struct COMMV1 { // old devices
		const byte COMMV1_color[1]{ 0x03 };
		const byte COMMV1_loop[1]{ 0x04 };
		const byte COMMV1_update[1]{ 0x05 };
		const byte COMMV1_status[1]{ 0x06 };
		const byte COMMV1_reset[2]{0x07, 0x04};
		// [2] - 0x1 - power & indi, 0x2 - sleep, 0x3 - off, 0x4 - on
		const byte COMMV1_saveGroup[1]{0x08};
		// save group codes for saveGroup[2]:
		// 0x1 - lights
		// 0x2 - ac charge (color, inverse mask after with index 2!) (morph ac-0, 0-ac)
		// 0x5 - ac morph (ac-0)
		// 0x6 - ac morph (ac-batt, batt-ac)
		// 0x7 - batt critical (color, inverse mask after with index 2!) (morph batt-0, 0-batt)
		// 0x8 - batt critical (morph batt-0)
		// 0x9 - batt down (pulse batt-0)
		// 0x2 0x0 - end storage block
		const byte COMMV1_save[1]{0x09};
		const byte COMMV1_setTempo[1]{ 0x0e };
		const byte COMMV1_apply[2]{0x1d, 0x03};
	//} COMMV1;

	//static struct COMMV4 { // common tron
		const byte COMMV4_control[6]{ 0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0xff };
		// [4] - control type (1..7), 1 - start new, 2 - finish and save, 3 - finish and play, 4 - remove, 5 - play, 6 - set default, 7 - set startup
		// [5-6] - control ID 0xffff - common, 8 - startup, 61 - light
		const byte COMMV4_colorSel[5]{0x03 ,0x23 ,0x01 ,0x00 ,0x01};
		// [3] - 1 - loop, 0 - once
		// [5] - count of lights need to be set,
		// [6-33] - LightID (index, not mask) - it can be COUNT of them.
		const byte COMMV4_colorSet[7]{0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa};
		// [3] - action type ( 0 - light, 1 - pulse, 2 - morph)
		// [4] - how long phase keeps
		// [5] - mode (action type) - 0xd0 - light, 0xdc - pulse, 0xcf - morph, 0xe8 - power morph, 0x82 - spectrum, 0xac - rainbow
		// [7] - tempo (0xfa - steady)
		// [8-10]    - rgb
		// Then circle [11 - 17, 18 - 24, 25 - 31]
		// It can be up to 3 colorSet for one colorSel.
		const byte COMMV4_setPower[2]{0x03 ,0x22};
		// Better use control with [2] = 22
		// [6] - type
		const byte COMMV4_turnOn[2]{0x03, 0x26};
		// [4] - brightness (0..100),
		// [5] - lights count
		// [6-33] - light IDs (like colorSel)
		const byte COMMV4_setOneColor[2]{ 0x03, 0x27 };
		// [3-5] - rgb
		// [7] - count
		// [8-33] - IDs
	//} COMMV4;
		static byte v4OpCodes[]{ 0xd0, 0xdc, 0xcf, 0xdc, 0x82, 0xac, 0xe8 };

	//static struct COMMV5 { // notebook rgb keyboards
		// Start command block
		const byte COMMV5_reset[1]{0x94};
		const byte COMMV5_status[1]{0x93};
		const byte COMMV5_colorSet[2]{0x8c, 0x02};
		const byte COMMV5_loop[2]{0x8c, 0x13};
		const byte COMMV5_update[3]{0x8b, 0x01, 0xff}; // fe, 59
		const byte COMMV5_turnOnInit[55]
		{0x79,0x7b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x87,0xff,0xff,0xff,0x00,0xff,
			0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x77};
		const byte COMMV5_turnOnInit2[2]{0x79,0x88};
		const byte COMMV5_turnOnSet[3]{0x83,0x38,0x9c};
		const byte COMMV5_setEffect[8]{0x80,0x02,0x07,0x00,0x00,0x01,0x01,0x01};// , 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x05
		// [2],[3]=type, [9]=?, [10..12]=RGB1, [13..15]=RGB2, [16]=?
		// 0 - color, 1 - reset, 2 - Breathing, 3 - Single-color Wave, 4 - dual color wave, 5-7 - off?, 8 - pulse, 9 - mix pulse (2 colors), a - night rider, b - lazer
		// Seems like row masks: 8c 01 XX - 01, 02, 05, 08, 09, 0e
		// And other masks: 8c XX - 05, 06, 07 (3 in each)
		// first 3 rows bitmask map
		//byte colorSel5[64] = {0xcc,0x8c,05,00,01,01,01,01,01,01,01,01,01,01,01,01,
		//	                    01,  01,01,01,00,00,00,00,01,01,01,01,01,01,01,01,
		//	                    01,  01,01,01,01,01,00,01,00,00,00,00,01,00,01,01,
		//	                    01,  01,01,01,01,01,01,01,01,01,01,01,00,00,00,01};
		//// second 4 rows bitmask map
		//byte colorSel6[60] = {0xcc,0x8c,06,00,00,01,01,01,01,01,01,01,01,01,01,01,
		//	                    01,  01,01,00,00,00,00,00,00,01,01,01,01,01,01,01,
		//	                    01,  01,01,01,01,00,01,00,00,00,00,00,01,01,00,01,
		//	                    01,  01,00,00,00,00,01,01,01,01,01,01};
		//// special row bitmask map
		//byte colorSel7[20] = {0xcc,0x8c,07,00,00,00,00,00,00,00,00,00,00,00,00,00,
		//	                    00,  01,01,01};
		//// Unclear, effects?
		//byte colorSel[18] = //{0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0xff, 0x00, 0x00, 0xff,
		//                    // 0x00, 0x00, 0x01};
		//					//{0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0xf0, 0xf0, 0x00,
		//					// 0xf0, 0xf0, 0x01};
		//					{0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x00, 0x00,
		//					 0xff, 0x00, 0x01};
	//} COMMV5;

	//static struct COMMV6 { // monitors
		//const byte systemReset[3]{0x93,0x37,0x0e};
		//[3] - some command, can be 06 and 0e
		const byte COMMV6_systemReset[4]{ 0x95,0,0,0 };
		//const byte colorReset[11]{ 0x92,0x37,0x07,0x00,0x51,0x84,0xd0,0x03,0x64,0x55,0x59 };
		const byte COMMV6_colorSet[8]{0x92,0x37,0x0a,0x00,0x51,0x87,0xd0,0x04};
		//[3] - command length (a - color, b - pulse, f - morph, 7 - timing),
		//[6] - command (87 - color, 88 - Pulse, 8c - morph/breath, 84 - timing?),
		//[8] - command type - 4 - color, 1 - morph, 2 - pulse, 3 - timing?
		//[9] - light mask,
		// 3,84 - [10] - Brightness, [11] - ???, [12] - checksum
		// 4,87 - [10,11,12] - RGB, [13] - Brightness (0..64), [14] - checksum
		// 2,88 - [10,11,12] - RGB, [13] - Brightness (0..64), [14] - Tempo?, [15] - checksum
		// 1,8c - [10,11,12] - RGB, [13,14,15] - RGB2, [16] - brightness, [17,18] - tempo, [19] - checksum
	//} COMMV6;

	//static struct COMMV7 { // mouses
		const byte COMMV7_update[8]{0x40,0x60,0x07,0x00,0xc0,0x4e,0x00,0x01};
		//[8] = 1 - update finish, [9] = 1 - update color (after set)
		const byte COMMV7_status[5]{0x40,0x03,0x01,0x00,0x01};
		const byte COMMV7_control[5]{0x40,0x10,0x0c,0x00,0x01};// , 0x64, 0x00, 0x2a, 0xaa, 0xff};
		//[5] - effect mode, [6] - brightness, [7] - lightID, [8..10] - rgb1, [11..13] - rgb2...
	//} COMMV7;
		static byte v7OpCodes[]{ 1,5,3,2,4,6,1 };

	//static struct COMMV8 { // external keyboards
		const byte COMMV8_effectReady[4]{0x5,0x1,0x51,0x00};
		// [2] - profile number
		const byte COMMV8_effectSet[14]{0x5,0x1,0x13,0x00,0xf0,0xf0,0x00,0x00,0x00,0x10,0x0a,0x00,0x01,0x01 };
		// [2] - profile number
		// [3] - effect type (0x13 - color reset, valid from 0 to it)
		// [4-6] - RGB1
		// [7-9] - RGB2
		// [10] - tempo
		// [11] - brightness?
		// [12] - ???
		// [13] - mode (1 - permanent, 2 - key press)
		// [14] - NumColors (0..3) into block?
		const byte COMMV8_readyToColor[4]{ 0xe,0x1,0x0,0x1 };
		// [2] - how much lights into next color block(s)
		// [3] - profile number
		// [4] - ???
		const byte COMMV8_colorSet[10]{ 0xe, 0x01,0x00, 0x01, 0x0, 0x81, 0x00, 0xa5, 0x00, 0x0a };// , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
		// [4] - packet number in group
		// [5] - light id
		// [6] - Effect type (80 - off, 81 - color, 82 - Pulse, 83 - morph, 87 - breath, 88 - spectrum (undocumented))
		// 84 - default blue?
		// [7] - Effect speed (tempo)
		// [9] - Effect length (time)
		// [10] - brightness?
		// [11-13] - RGB
		// [14-16] - RGB2
		// [18] - Number of RGB? (0,1,2)
		// [20-33],[35-48],[50-63] - same block [5..18] (so total 4 blocks per command)
		const byte COMMV8_setBrightness[4]{ 0x17,0x00,0x00,0x00 };
		// [1] - brightness (0..a)
	//} COMMV8;
}