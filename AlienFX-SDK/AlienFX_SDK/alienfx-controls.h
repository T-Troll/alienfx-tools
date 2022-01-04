#pragma once

#define NUM_VIDS 4

namespace AlienFX_SDK {

	//This is VIDs for different devices: Alienware (common), Darfon (RGB keyboards), Microship (monitors), Primax (mouses)
	const static WORD vids[NUM_VIDS]{0x187c, 0x0d62, 0x0424, 0x0461};

	static struct COMMV1 {
		const byte reset[2]{0x07, 0x04};
		const byte loop[1]{0x04};
		const byte color[1]{0x03};
		const byte update[1]{0x05};
		const byte status[1]{0x06};
		const byte saveGroup[1]{0x08};
		const byte save[1]{0x09};
		const byte apply[2]{0x1d, 0x03};
		const byte setTempo[1]{0x0e};
		// save group codes saveGroup[2]:
		// 0x1 - lights
		// 0x2 - ac charge (color, inverse mask after with index 2!) (morph ac-0, 0-ac)
		// 0x5 - ac morph (ac-0)
		// 0x6 - ac morph (ac-batt, batt-ac)
		// 0x7 - batt critical (color, inverse mask after with index 2!) (morph batt-0, 0-batt)
		// 0x8 - batt critical (morph batt-0)
		// 0x9 - batt down (pulse batt-0)
		// 0x2 0x0 - end storage block
		// Reset 0x1 - power & indi, 0x2 - sleep, 0x3 - off, 0x4 - on
	} COMMV1;

	static struct COMMV4 {
		const byte reset[6]{0x03 ,0x21 ,0x00 ,0x01 ,0xff ,0xff};
		const byte colorSel[5]{0x03 ,0x23 ,0x01 ,0x00 ,0x01};
		// [5] - COUNT of lights need to be set, [6-33] - LightID (index, not mask) - it can be COUNT of them.
		const byte colorSet[7]{0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa};
		// [3] - action type ( 0 - light, 1 - pulse, 2 - morph)
		// [4] - how long phase keeps
		// [5] - mode (action type) - 0xd0 - light, 0xdc - pulse, 0xcf - morph, 0xe8 - power morph, 0x82 - spectrum, 0xac - rainbow
		// [7] - tempo (0xfa - steady)
		// [8-10]    - rgb
		// Then circle [11 - 17, 18 - 24, 25 - 31]
		// It can be up to 3 colorSet for one colorSel.
		const byte update[6]{0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0xff};
		//{0x00, 0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0x00};
		const byte setPower[6]{0x03 ,0x22 ,0x00, 0x04, 0x00, 0x5b};
		const byte prepareTurn[3]{0x03, 0x20, 0x2};
		const byte turnOn[2]{0x03, 0x26};
		// 4 = 0x64 - off, 0x41 - dim, 0 - on, 6 - number, 7...31 - IDs (like colorSel)
		// Uknown command codes : 0x20 0x2
	} COMMV4;

	static struct COMMV5 {
		// Start command block
		const byte reset[1]{0x94};
		const byte status[1]{0x93};
		const byte colorSet[2]{0x8c, 0x02};
		const byte loop[2]{0x8c, 0x13};
		const byte update[3]{0x8b, 0x01, 0xff}; // fe, 59
		const byte turnOnInit[55]
		{0x79,0x7b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x87,0xff,0xff,0xff,0x00,0xff,
			0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x77};
		const byte turnOnInit2[2]{0x79,0x88};
		const byte turnOnSet[3]{0x83,0x38,0x9c};
		const byte setEffect[8]{0x80,0x02,0x07,0x00,0x00,0x01,0x01,0x01};// , 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x05
		// [2],[3]=type, [9]=?, [10..12]=RGB1, [13..15]=RGB2, [16]=?
		// 0 - color, 1 - reset, 2 - Breathing, 3 - Single-color Wave, 4 - dual color wave, 5-7 - off?, 8 - pulse, 9 - mix pulse (2 colors), a - night rider, b - lazer
		// Seems like row masks: 8c 01 XX - 01, 02, 05, 08, 09, 0e
		// And other masks: 8c XX - 05, 06, 07 (3 in each)
		// first 3 rows bitmask map
		//byte colorSel5[64] = {0xcc,0x8c,05,00,01,01,01,01,01,01,01,01,01,01,01,01,
		//	                    01,  01,01,01,00,00,00,00,01,01,01,01,01,01,01,01,
		//	                    01,  01,01,01,01,01,00,01,00,00,00,00,01,00,01,01,
		//	                    01,  01,01,01,01,01,01,01,01,01,01,01,00,00,00,01};
		//// secnd 4 rows bitmask map
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
	} COMMV5;

	static struct COMMV6 {
		const byte colorSet[8]{0x92,0x37,0x0a,0x00,0x51,0x87,0xd0,0x04};
		//[9] - light mask, [10,11,12] - RGB, [13] - Brightness (0..64), [14] - Mask
		//[6] - command (87 - color, 88 - Pulse?, 8d - morph?), [8] - command type - 4 - color, 1 - morph?, 2 - pulse?
		// Pulse - [13,14,15] - rgb2 + 63 (WTF?)
		// Morph - [13,14,15] - rgb2, [16-19] - 00,02,14,7b ???
	} COMMV6;

	static struct COMMV7 {
		//const byte ack[2]{0xc0,0x02};
		//const byte colorSet[3]{0x40,0x1a,0x07};
		//[5] - light ID, [6,7,8] - RGB
		const byte update[8]{0x40,0x60,0x07,0x00,0xc0,0x4e,0x00,0x01};
		//[8] = 1 - update finish, [9] = 1 - update color (after set)
		const byte status[5]{0x40,0x03,0x01,0x00,0x01};
		const byte control[5]{0x40,0x10,0x0c,0x00,0x01};// , 0x64, 0x00, 0x2a, 0xaa, 0xff};
		//[5] - effect mode, [6] - brightness, [7] - lightID, [8..10] - rgb1, [11..13] - rgb2...
	} COMMV7;

}