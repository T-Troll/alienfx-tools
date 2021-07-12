#include "stdafx.h"
#include <Setupapi.h>
//#include <codecvt>
//#include <locale>
#include "AlienFX_SDK.h"  
#include <iostream>
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace AlienFX_SDK
{
	static struct COMMV1 {
		const byte reset[2] = {0x02 ,0x07};
		const byte loop[2] = {0x02, 0x04};
		const byte color[2] = {0x02, 0x03};
		const byte update[2] = {0x02, 0x05};
		const byte setMorph[2] = {0x02, 0x01};
		//const byte setBatt[2] = {0x2, 0x2};
		//const byte confirmBatt[6] = {0x2, 0x3, 0x2, 0xf, 0x7e, 0xff};
		//const byte confirmBattUpdate[6] = {0x2, 0x3, 0x9, 0xf, 0x78, 0x10};
		//const byte setPower[2] = {0x02, 0x09};
		//const byte updatePower[6] = {0x02, 0x0e, 0x00, 0xc8, 0x03, 0xe8};
		//const byte updatePowerEnd[3] = {0x02, 0x1d, 0x82};
		const byte status[2] = {0x02 ,0x06};
		// Unknown: 0x2, 0x8, 0xXX - seems like group, before and after each color set until update; 0x2, 0x9; 0x2, 0x2 (power?)
	} COMMV1;

	static struct COMMV4 {
		const byte reset[7] = {0x00, 0x03 ,0x21 ,0x00 ,0x01 ,0xff ,0xff};
		const byte colorSel[6] = {0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01};
		const byte colorSet[8] = {0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa};
		const byte update[7] = {0x00, 0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0x00};
			//{0x00, 0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0xff};
		const byte setPower[7] = {0x00, 0x03 ,0x22 ,0x00, 0x04, 0x00, 0x5b};
		const byte prepareTurn[4] = {0x00, 0x03, 0x20, 0x2};
		const byte turnOn[3] = {0x00, 0x03, 0x26};
		// 4 = 0x64 - off, 0x41 - dim, 0 - on, 6 - number, 7...31 - IDs (like colorSel)
	} COMMV4;

	static struct COMMV5 {
		// Start command block
		const byte reset[2] = { 0xcc, 0x94 };
		// request status - doesn/t used
		byte status[2] = {0xcc, 0x93};
		// first 3 rows bitmask map
		byte colorSel5[64] = {0xcc,0x8c,05,00,01,01,01,01,01,01,01,01,01,01,01,01,
			                    01,  01,01,01,00,00,00,00,01,01,01,01,01,01,01,01,
			                    01,  01,01,01,01,01,00,01,00,00,00,00,01,00,01,01,
			                    01,  01,01,01,01,01,01,01,01,01,01,01,00,00,00,01};
		//// secnd 4 rows bitmask map
		byte colorSel6[60] = {0xcc,0x8c,06,00,00,01,01,01,01,01,01,01,01,01,01,01,
			                    01,  01,01,00,00,00,00,00,00,01,01,01,01,01,01,01,
			                    01,  01,01,01,01,00,01,00,00,00,00,00,01,01,00,01,
			                    01,  01,00,00,00,00,01,01,01,01,01,01};
		//// special row bitmask map
		byte colorSel7[20] = {0xcc,0x8c,07,00,00,00,00,00,00,00,00,00,00,00,00,00,
			                    00,  01,01,01};
		//// Unclear, effects?
		//byte colorSel[18] = //{0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0xff, 0x00, 0x00, 0xff,
		//                    // 0x00, 0x00, 0x01};
		//					//{0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0xf0, 0xf0, 0x00, 
		//					// 0xf0, 0xf0, 0x01};
		//					{0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x00, 0x00, 
		//					 0xff, 0x00, 0x01};
		const byte colorSet[4] = {0xcc, 0x8c, 0x02, 0x00 };
		const byte loop[3] = {0xcc, 0x8c, 0x13};
		const byte update[4] = {0xcc, 0x8b, 0x01, 0xff}; // fe, 59
		const byte turnOnInit[56] = 
		                     {0xcc,0x79,0x7b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			                  0xff,0xff,0xff,0xff,0xff,0xff,0x7c,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			                  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x87,0xff,0xff,0xff,0x00,0xff,
			                  0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x77};
		const byte turnOnInit2[3] = {0xcc,0x79,0x88};
		const byte turnOnSet[4] = {0xcc,0x83,0x38,0x9c};
	} COMMV5;

	void Functions::SetMaskAndColor(int index, byte* buffer, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2) {
		int mask = index < 13 ? masks[GetVersion()-1][index] : 0;;
		buffer[2] = (byte)index+1;
		buffer[3] = (mask & 0xFF0000) >> 16;
		buffer[4] = (mask & 0x00FF00) >> 8;
		buffer[5] = (mask & 0x0000FF);
		switch (length) {
		case API_L_V1: case API_L_V3:
			buffer[6] = r1;
			buffer[7] = g1;
			buffer[8] = b1;
			buffer[9] = r2;
			buffer[10] = g2;
			buffer[11] = b2;
			break;
		case API_L_V2: 
			buffer[6] = (r1 & 0xf0) | ((g1 & 0xf0) >> 4); // 4-bit color!
			buffer[7] = (b1 & 0xf0) | ((r2 & 0xf0) >> 4);
			buffer[8] = (g2 & 0xf0) | ((b2 & 0xf0) >> 4); // 4-bit color!
			//buffer[9] = b2 & 0xf0;
			break;
		}

		return;
	}

	//Use this method for general devices pid = -1 for full scan
	int Functions::AlienFXInitialize(int vid, int pidd)
	{
		GUID guid;
		bool flag = false;
		pid = -1;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE)
		{
			//std::cout << "Couldn't get guid";
			return pid;
		}
		unsigned int dw = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

		unsigned int lastError = 0;
		while (!flag)
		{
			deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData))
			{
				lastError = GetLastError();
				return pid;
			}
			dw++;
			DWORD dwRequiredSize = 0;
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL))
			{
				//std::cout << "Getting the needed buffer size failed";
				return pid;
			}
			//std::cout << "Required size is " << dwRequiredSize << std::endl;
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			{
				//std::cout << "Last error is not ERROR_INSUFFICIENT_BUFFER";
				return pid;
			}
			std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new char[dwRequiredSize]);
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL))
			{
				std::wstring devicePath = deviceInterfaceDetailData->DevicePath;
				//OutputDebugString(devicePath.c_str());
				devHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				if (devHandle != INVALID_HANDLE_VALUE)
				{
					std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
					attributes->Size = sizeof(HIDD_ATTRIBUTES);
					if (HidD_GetAttributes(devHandle, attributes.get()))
					{

						if (((attributes->VendorID == vid) && (pidd == -1 || attributes->ProductID == pidd)))
						{
							// Check API version...
							PHIDP_PREPARSED_DATA prep_caps;
							HIDP_CAPS caps;
							HidD_GetPreparsedData(devHandle, &prep_caps);
							HidP_GetCaps(prep_caps, &caps);
							HidD_FreePreparsedData(prep_caps);

							if (caps.OutputReportByteLength || caps.Usage == 0xcc) {
#ifdef _DEBUG
								cout << dec << "Output Report Length " << caps.OutputReportByteLength
									<< ", Input Report Length " << caps.InputReportByteLength
									<< ", Feature Report Length " << caps.FeatureReportByteLength
									<< endl;
								cout << hex << "Usage ID " << caps.Usage << ", Usage Page " << caps.UsagePage << endl;
								cout << dec << "Output caps " << caps.NumberOutputButtonCaps << ", Index " << caps.NumberOutputDataIndices << endl;
#endif
								// Yes, now so easy...
								switch (caps.OutputReportByteLength) {
								case 0: length = caps.FeatureReportByteLength;
									break;
								//case 9: length = caps.OutputReportByteLength; //length = attributes->Size;
								//	if (attributes->ProductID > 0x529)
								//		version = API_V25;
								//	else
								//		version = API_V2;
								//	break;
								default: length = caps.OutputReportByteLength;
								}

								this->vid = attributes->VendorID;
								pid = attributes->ProductID;
								flag = true;
#ifdef _DEBUG
								wchar_t buff[2048];
								swprintf_s(buff, 2047, L"Init: VID: %#x, PID: %#x, Version: %d, Length: %d\n",
										   attributes->VendorID, attributes->ProductID, attributes->VersionNumber, length);
								OutputDebugString(buff);
								cout << "Attributes - length: " << attributes->Size << ", version: " << attributes->VersionNumber << endl;
								wprintf(L"Path: %s\n%s", devicePath.c_str(), buff);
#endif
							}
						}
					}

				}
			}
		}
		return pid;
	}

	void Functions::Loop()
	{
		//size_t BytesWritten;
		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE];
		ZeroMemory(buffer, length);
		switch (length) {
		case API_L_V5:
			memcpy(buffer, COMMV5.loop, sizeof(COMMV5.loop));
			HidD_SetFeature(devHandle, buffer, length);
			break;
		//case API_L_V4: {
			// m15 require Input report as a confirmation, not output. 
			// WARNING!!! In latest firmware, this can provide up to 10sec(!) slowdown, so i disable status read. It works without it as well.
			// DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, BufferN, length, (DWORD*)&BytesWritten, NULL);
			// std::cout << "Status: 0x" << std::hex << (int) BufferN[2] << std::endl;
		//} break;
		case API_L_V2: case API_L_V1: {
			memcpy(buffer, COMMV1.loop, sizeof(COMMV1.loop));
			HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		}
	}

	bool Functions::Reset(int status)
	{
		//size_t BytesWritten;
		bool result = false;

		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE];
		ZeroMemory(buffer, length);
		switch (length) {
		case API_L_V5: {
			// DEBUG!
			memcpy(buffer, COMMV5.reset, sizeof(COMMV5.reset));
			//result = DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			result = HidD_SetFeature(devHandle, buffer, length);
		} break;
		case API_L_V4: {
			memcpy(buffer, COMMV4.reset, sizeof(COMMV4.reset));
			result = HidD_SetOutputReport(devHandle, buffer, length);
			//result = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, NULL, NULL);
			//cout << "IO result is " << GetLastError() << endl;
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1: {
			memcpy(buffer, COMMV1.reset, sizeof(COMMV1.reset));
			if (status)
				buffer[2] = 0x04;
			else
				buffer[2] = 0x03;
			// ???? Do i need it?
			AlienfxWaitForBusy();
			result = HidD_SetOutputReport(devHandle, buffer, length);
			//result = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			AlienfxWaitForReady();
		} break;
		}
		inSet = true;
		//std::cout << "Reset!" << std::endl;
		return result;
	}

	bool Functions::UpdateColors()
	{
		//size_t BytesWritten;
		bool res = false;
		if (inSet) {
			//byte* buffer = new byte[length];
			byte buffer[MAX_BUFFERSIZE] = {0};
			switch (length) {
			case API_L_V5:
			{
				memcpy(buffer, COMMV5.update, sizeof(COMMV5.update));
				res = HidD_SetFeature(devHandle, buffer, length);
				//return DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*) &BytesWritten, NULL);
			} break;
			case API_L_V4:
			{
				memcpy(buffer, COMMV4.update, sizeof(COMMV4.update));
				res = HidD_SetOutputReport(devHandle, buffer, length);
				//return DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*) &BytesWritten, NULL);
			} break;
			case API_L_V3: case API_L_V2: case API_L_V1:
			{
				//if (wasPowerButton) {
				//	wasPowerButton = false;
				//	//ZeroMemory(buffer, length);
				//	memcpy(buffer, COMMV1.confirmBattUpdate, sizeof(COMMV1.confirmBattUpdate));
				//	HidD_SetOutputReport(devHandle, buffer, length);
				//	Loop();
				//	ZeroMemory(buffer, length);
				//	memcpy(buffer, COMMV1.updatePowerEnd, sizeof(COMMV1.updatePowerEnd));
				//	HidD_SetOutputReport(devHandle, buffer, length);
				//	ZeroMemory(buffer, length);
				//}
				memcpy(buffer, COMMV1.update, sizeof(COMMV1.update));
				res = HidD_SetOutputReport(devHandle, buffer, length);
				//return DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*) &BytesWritten, NULL);
			} break;
			default: return false;
			}
			//std::cout << "Update!" << std::endl;
			inSet = false;
			Sleep(5); // Fix for ultra-fast updates, or next command will fail sometimes.
			return res;
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, NULL, NULL);
				//HidD_SetOutputReport(devHandle, buffer, length);
		}
		return false;
	}

	bool Functions::SetColor(unsigned index, byte r, byte g, byte b)
	{
		//size_t BytesWritten;
		bool val = false;
		// API v4 command - 11,12,13 and 14,15,16 is RGB
		// API v4 command - 4 is index, 5,6,7 is RGB, then circle (8,9,10,11 etc)
		/// API v3 - Buffer[8,9,10] = rgb
		/// But we need to issue 2 commands - light_select and color_set.... this for light_select
		/// Buffer2[5] - Count of lights need to be set
		/// Buffer2[6-33] - LightID (index, not mask) - it can be COUNT of them.
		if (!inSet)
			Reset(1);
		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (length) {
		case API_L_V5: {
			//memcpy(buffer, COMMV5.colorSel5, sizeof(COMMV5.colorSel5));
			//HidD_SetFeature(devHandle, buffer, length);
			//ZeroMemory(buffer, length);
			//memcpy(buffer, COMMV5.colorSel6, sizeof(COMMV5.colorSel6));
			//HidD_SetFeature(devHandle, buffer, length);
			//ZeroMemory(buffer, length);
			//memcpy(buffer, COMMV5.colorSel7, sizeof(COMMV5.colorSel7));
			//HidD_SetFeature(devHandle, buffer, length);
			//ZeroMemory(buffer, length);
			//memcpy(buffer, COMMV5.colorSel, sizeof(COMMV5.colorSel));
			//HidD_SetFeature(devHandle, buffer, length);
//			//DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
			buffer[4] = index + 1;
			buffer[5] = r;
			buffer[6] = g;
			buffer[7] = b;
			int ret = HidD_SetFeature(devHandle, buffer, length);
#ifdef _DEBUG
			//cout << "Color Set IO result is ";
			//switch (GetLastError()) {
			//case 0: cout << "Ok."; break;
			//case 1: cout << "Incorrect function"; break;
			//case ERROR_INSUFFICIENT_BUFFER: cout << "Buffer too small!"; break;
			//case ERROR_MORE_DATA: cout << "More data remains!"; break;
			//default: cout << "Unknown (" << GetLastError() << ")";
			//}
			//cout <<  endl;
			//cout << "Data: " << buffer[0] << "," << buffer[1] << "," << buffer[2] << endl;
#endif
			Loop();
			return ret;
			//return DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		case API_L_V4: {
			memcpy(buffer, COMMV4.colorSel, sizeof(COMMV4.colorSel));
			buffer[6] = index;
			HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, NULL, NULL);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));
			buffer[8] = r;
			buffer[9] = g;
			buffer[10] = b;
		} break;
		case API_L_V3: case API_L_V2:
		{
			memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
			SetMaskAndColor(index, buffer, r, g, b);

			//if (index == 0)
			//{
			//	buffer[1] = 0x02;
			//}

		} break;
		case API_L_V1: {
			memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
			SetMaskAndColor(index, buffer, r, g, b);

			if (index == 5)
			{
				buffer[1] = 0x83;
			}

			if (index == 0)
			{
				buffer[1] = 0x01;
			}

		} break;
		default: return false;
		}
		val = HidD_SetOutputReport(devHandle, buffer, length);
		//val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, NULL, NULL);
		Loop();
		return val;
	}

	bool Functions::SetMultiColor(int numLights, UCHAR* lights, int r, int g, int b)
	{
		//size_t BytesWritten; 
		bool val = false;
		//byte* buffer = new byte[length];
		switch (length) {
		case API_L_V5:
		{
			// up to 15 lights in set, many set blocks possible
			if (!inSet) Reset(false);
			byte buffer[MAX_BUFFERSIZE] = {0};
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
			int bPos = 4;
			for (int nc = 0; nc < numLights; nc++) {
				if (bPos < length) {
					buffer[bPos] = lights[nc] + 1;
					buffer[bPos + 1] = r;
					buffer[bPos + 2] = g;
					buffer[bPos + 3] = b;
					bPos += 4;
				} else {
					// Send command and clear buffer...
					val = HidD_SetFeature(devHandle, buffer, length);
					ZeroMemory(buffer, length);
					memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
					bPos = 4;
				}
			}
			if (bPos > 4)
				val = HidD_SetFeature(devHandle, buffer, length);
			Loop();
		} break;
		case API_L_V4: {
			/// We need to issue 2 commands - light_select and color_set.... this for light_select
			/// Buffer[5] - Count of lights need to be set
			/// Buffer[6-33] - LightID (index, not mask).
			if (!inSet) Reset(false);
			byte buffer[MAX_BUFFERSIZE] = {0};
			memcpy(buffer, COMMV4.colorSel, sizeof(COMMV4.colorSel));
			buffer[5] = numLights;
			for (int nc = 0; nc < numLights; nc++)
				buffer[6 + nc] = lights[nc];
			val = HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));
			buffer[8] = r;
			buffer[9] = g;
			buffer[10] = b;
			val = HidD_SetOutputReport(devHandle, buffer, length);
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		default: {
			for (int nc = 0; nc < numLights; nc++)
				val = SetColor(lights[nc], r, g, b);
		}
		}
		return val;
	}

	bool Functions::SetAction(int index, std::vector<afx_act> act)
		//int action, int time, int tempo, int Red, int Green, int Blue, int action2, int time2, int tempo2, int Red2, int Green2, int Blue2)
	{
		//size_t BytesWritten;
		bool res = false;
		// Buffer[3], [11] - action type ( 0 - light, 1 - pulse, 2 - morph)
		// Buffer[4], [12] - how long phase keeps
		// Buffer[5], [13] - mode (action type) - 0xd0 - light, 0xdc - pulse, 0xcf - morph, 0xe8 - power morph, 0x82 - spectrum, 0xac - rainbow
		// Buffer[7], [15] - tempo (0xfa - steady)
		// Buffer[8-10]    - rgb
		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE] = {0};
		if (act.size() > 0) {
			switch (length) {
			case API_L_V4:
			{ // only supported at new devices
				if (!inSet) Reset(false);
				int bPos = 3;
				memcpy(buffer, COMMV4.colorSel, sizeof(COMMV4.colorSel));
				buffer[6] = index;
				HidD_SetOutputReport(devHandle, buffer, length);
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);			
				for (int ca = 0; ca < act.size(); ca++) {
					ZeroMemory(buffer, length);
					memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));
					// 3 actions per record..
					buffer[bPos] = act[ca].type < AlienFX_A_Breathing ? act[ca].type : AlienFX_A_Morph;
					buffer[bPos + 1] = act[ca].time;
					buffer[bPos + 3] = 0;
					buffer[bPos + 4] = act[ca].tempo;
					buffer[bPos + 5] = act[ca].r;
					buffer[bPos + 6] = act[ca].g;
					buffer[bPos + 7] = act[ca].b;
					switch (act[ca].type) {
					case AlienFX_A_Color: buffer[bPos + 2] = 0xd0; buffer[bPos + 4] = 0xfa; break;
					case AlienFX_A_Pulse: buffer[bPos + 2] = 0xdc; break;
					case AlienFX_A_Morph: buffer[bPos + 2] = 0xcf; break;
					case AlienFX_A_Breathing: buffer[bPos + 2] = 0xdc; break;
					case AlienFX_A_Spectrum: buffer[bPos + 2] = 0x82; break;
					case AlienFX_A_Rainbow: buffer[bPos + 2] = 0xac; break;
					case AlienFX_A_Power: buffer[bPos + 2] = 0xe8; break;
					default: buffer[bPos + 2] = 0xd0; buffer[bPos + 4] = 0xfa; buffer[bPos] = AlienFX_A_Color;
					}
					bPos += 8;
					if (bPos == 27) {
						res = HidD_SetOutputReport(devHandle, buffer, length);
						//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
						bPos = 3;
					}
				}
				if (bPos != 3) {
					res = HidD_SetOutputReport(devHandle, buffer, length);
					//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				}
				return res;
			} break;
			case API_L_V3: case API_L_V2:
			{
				if (!inSet) Reset(1);
				for (size_t ca = 0; ca < act.size(); ca++) {
					switch (act[ca].type) {
					case AlienFX_A_Color: res = SetColor(index, act[ca].r, act[ca].g, act[ca].b); ca = act.size();  break;
					case AlienFX_A_Pulse:
						res = SetColor(index, act[ca].r, act[ca].g, act[ca].b);
						break;
					case AlienFX_A_Morph:
					{
						memcpy(buffer, COMMV1.setMorph, sizeof(COMMV1.setMorph));
						if (ca < act.size() - 1) {
							SetMaskAndColor(index, buffer, act[ca].r, act[ca].g, act[ca].b, act[ca + 1].r, act[ca + 1].g, act[ca + 1].b);
							HidD_SetOutputReport(devHandle, buffer, length);
							Loop();
						} else {
							SetMaskAndColor(index, buffer, act[ca].r, act[ca].g, act[ca].b, act[0].r, act[0].g, act[0].b);
							HidD_SetOutputReport(devHandle, buffer, length);
							Loop();
						}
					} break;
					default: res = SetColor(index, act[0].r, act[0].g, act[0].b);
					}
				}
			} break;
			case API_L_V1:
			{
				// can't set action for old, just use color. 
				res = SetColor(index, act[0].r, act[0].g, act[0].b);
			} break;
			}
		}
		return res;
	}

	bool Functions::SetPowerAction(int index, BYTE Red, BYTE Green, BYTE Blue, BYTE Red2, BYTE Green2, BYTE Blue2, bool force)
	{
		//size_t BytesWritten;

		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (length) {
		case API_L_V4: { // only supported at new devices
			// this function can be called not early then 250ms after last call!
			ULONGLONG cPowerCall = GetTickCount64();
#ifdef _DEBUG
			if (force)
				OutputDebugString(TEXT("Forced power button update!\n"));
#endif
			if (cPowerCall - lastPowerCall < POWER_DELAY)
				if (force) {
#ifdef _DEBUG
					OutputDebugString(TEXT("Forced power button update waiting...\n"));
#endif
					Sleep(lastPowerCall + POWER_DELAY - cPowerCall);
				}
				else {
#ifdef _DEBUG
					OutputDebugString(TEXT("Power button update skipped!\n"));
#endif
					return false;
				}
			if (!IsDeviceReady()) {
#ifdef _DEBUG
				if (force)
					OutputDebugString(TEXT("Forced power update - device still not ready\n"));
				else
					OutputDebugString(TEXT("Power update - device still not ready\n"));
#endif
				return false;
			}
			// Need to flush query...
			if (inSet) UpdateColors();
			inSet = true;
			// Now set....
			memcpy(buffer, COMMV4.setPower, sizeof(COMMV4.setPower));
			for (BYTE cid = 0x5b; cid < 0x61; cid++) {
				// Init query...
				buffer[6] = cid;
				buffer[4] = 4;
				HidD_SetOutputReport(devHandle, buffer, length);
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				buffer[4] = 1;
				HidD_SetOutputReport(devHandle, buffer, length);
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				// Now set color by type...
				std::vector<afx_act> act;
				switch (cid) {
				case 0x5b: // Alarm
					act.push_back(afx_act{ AlienFX_A_Power, 3, 0x64, Red, Green, Blue });
					act.push_back(afx_act{ AlienFX_A_Power, 3, 0x64, 0, 0, 0 });
					SetAction(index, act);
					break;
				case 0x5c: // AC power
					act.push_back(afx_act{ AlienFX_A_Color, 0, 0, Red, Green, Blue });
					SetAction(index, act);
					break;
				case 0x5d: // Charge
					act.push_back(afx_act{ AlienFX_A_Power, 3, 0x64, Red, Green, Blue });
					act.push_back(afx_act{ AlienFX_A_Power, 3, 0x64, Red2, Green2, Blue2 });
					SetAction(index, act);
					break;
				case 0x5e: // Low batt
					act.push_back(afx_act{ AlienFX_A_Power, 3, 0x64, Red2, Green2, Blue2 });
					act.push_back(afx_act{ AlienFX_A_Power, 3, 0x64, 0, 0, 0 });
					SetAction(index, act);
					break;
				case 0x5f: // Batt power
					act.push_back(afx_act{ AlienFX_A_Color, 0, 0, Red2, Green2, Blue2 });
					SetAction(index, act);
					break;
				case 0x60: // System sleep?
					act.push_back(afx_act{ AlienFX_A_Color, 0, 0, Red2, Green2, Blue2 });
					SetAction(index, act);
				}
				// And finish
				buffer[4] = 2;
				HidD_SetOutputReport(devHandle, buffer, length);
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			}
			// Now (default) color set, if needed...
			/*Buffer[2] = 0x21; Buffer[4] = 4; Buffer[6] = 0x61;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			Buffer[4] = 1;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			// Default color set here...
			for (int i = 0; i < mappings.size(); i++) {
				if (mappings[i].devid == pid && !mappings[i].flags)
					SetColor(mappings[i].lightid, 0, 0, 0);
			}
			Buffer[4] = 2;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();*/
			// Close set
			//buffer[4] = 6;
			//HidD_SetOutputReport(devHandle, buffer, length);
			//memcpy(buffer, COMMV4.update, sizeof(COMMV4.update));
			//buffer[4] = 4; buffer[6] = 0x61;
			//HidD_SetOutputReport(devHandle, buffer, length);
			//buffer[4] = 1;
			//HidD_SetOutputReport(devHandle, buffer, length);
			//buffer[4] = 2;
			//HidD_SetOutputReport(devHandle, buffer, length);
			//buffer[4] = 6;
			//HidD_SetOutputReport(devHandle, buffer, length);
			UpdateColors();
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			lastPowerCall = GetTickCount64();
			Sleep(20);
		} break;
		//case API_L_V3: case API_L_V2:
		//{
		//	if (!inSet) Reset(1);
		//	// 08 02
		//	memcpy(buffer, COMMV1.setMorph, sizeof(COMMV1.setMorph));
		//	SetMaskAndColor(index, buffer, Red, Green, Blue);
		//	// neeed to set colors 6 times - 1-2 and 2-1
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	SetMaskAndColor(index, buffer, 0, 0, 0, Red, Green, Blue);
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.confirmBatt, sizeof(COMMV1.confirmBatt));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.setPower, sizeof(COMMV1.setPower));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Reset(true);
		//	// 08 05
		//	memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
		//	SetMaskAndColor(index, buffer, Red, Green, Blue);
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.setPower, sizeof(COMMV1.setPower));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Reset(true);
		//	// 08 06
		//	memcpy(buffer, COMMV1.setMorph, sizeof(COMMV1.setMorph));
		//	SetMaskAndColor(index, buffer, Red, Green, Blue, Red2, Green2, Blue2);
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	SetMaskAndColor(index, buffer, Red2, Green2, Blue2, Red, Green, Blue);
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.setPower, sizeof(COMMV1.setPower));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Reset(true);
		//	// 08 07
		//	SetMaskAndColor(index, buffer, Red2, Green2, Blue2);
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	SetMaskAndColor(index, buffer, 0, 0, 0, Red2, Green2, Blue2);
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.confirmBatt, sizeof(COMMV1.confirmBatt));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.setPower, sizeof(COMMV1.setPower));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Reset(true);
		//	// 08 08
		//	memcpy(buffer, COMMV1.color, sizeof(COMMV1.color));
		//	SetMaskAndColor(index, buffer, Red2, Green2, Blue2);
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.setPower, sizeof(COMMV1.setPower));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Reset(true);
		//	// 08 09
		//	memcpy(buffer, COMMV1.setBatt, sizeof(COMMV1.setBatt));
		//	SetMaskAndColor(index, buffer, Red2, Green2, Blue2);
		//	Loop();
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.setPower, sizeof(COMMV1.setPower));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	Reset(true);
		//	ZeroMemory(buffer, length);
		//	memcpy(buffer, COMMV1.updatePower, sizeof(COMMV1.updatePower));
		//	HidD_SetOutputReport(devHandle, buffer, length);
		//	wasPowerButton = true;
		//	//ZeroMemory(buffer, length);
		//	//memcpy(buffer, COMMV1.updatePowerEnd, sizeof(COMMV1.updatePowerEnd));
		//	//HidD_SetOutputReport(devHandle, buffer, length);
		//} break;
		default:
			// can't set action for old, just use color
			SetColor(index, Red, Green, Blue);
		}
		return true;
	}

	bool Functions::ToggleState(bool newState, vector<mapping>* mappings, bool power) {

		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (length) {
		case API_L_V5:
		{
			if (inSet) UpdateColors();
			Reset(true);
			memcpy(buffer, COMMV5.turnOnInit, sizeof(COMMV5.turnOnInit));
			HidD_SetFeature(devHandle, buffer, length);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV5.turnOnInit2, sizeof(COMMV5.turnOnInit2));
			HidD_SetFeature(devHandle, buffer, length);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV5.turnOnSet, sizeof(COMMV5.turnOnSet));
			if (newState)
				buffer[4] = 0xfe;
			return HidD_SetFeature(devHandle, buffer, length);
			//memcpy(buffer, COMMV5.colorSel5, sizeof(COMMV5.colorSel5));
			//if (!newState)
			//	ZeroMemory(buffer + 3, sizeof(COMMV5.colorSel5) - 3);
			//HidD_SetFeature(devHandle, buffer, length);
			//
			//ZeroMemory(buffer, length);
			//memcpy(buffer, COMMV5.colorSel6, sizeof(COMMV5.colorSel6));
			//if (!newState)
			//	ZeroMemory(buffer + 3, sizeof(COMMV5.colorSel6) - 3);
			//HidD_SetFeature(devHandle, buffer, length);

			//ZeroMemory(buffer, length);
			//memcpy(buffer, COMMV5.colorSel7, sizeof(COMMV5.colorSel7));
			//if (!newState)
			//	ZeroMemory(buffer + 3, sizeof(COMMV5.colorSel7) - 3);
			//HidD_SetFeature(devHandle, buffer, length);

			//ZeroMemory(buffer, length);
			//memcpy(buffer, COMMV5.colorSet, sizeof(COMMV5.colorSet));
			//HidD_SetFeature(devHandle, buffer, length);
			//Loop();
			//UpdateColors();
		} break;
		case API_L_V4:
		{
			if (inSet) UpdateColors();
			memcpy(buffer, COMMV4.prepareTurn, sizeof(COMMV4.prepareTurn));
			HidD_SetOutputReport(devHandle, buffer, length);
			//AlienfxGetDeviceStatus();
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV4.turnOn, sizeof(COMMV4.turnOn));
			if (!newState)
				buffer[3] = 0x64;
			byte pos = 6, pindex = 0;
			for (int i = 0; i < mappings->size(); i++) {
				mapping cur = mappings->at(i);
				if (cur.devid == pid)
					if (!cur.flags || power) {
						buffer[pos] = (byte)cur.lightid;
						pos++; pindex++;
					}
			}
			buffer[5] = pindex;
			return HidD_SetOutputReport(devHandle, buffer, length);
			//AlienfxGetDeviceStatus();
		} break;
		case API_L_V3: case API_L_V1: case API_L_V2: return Reset(newState); break;
		}
		return false;
	}

	BYTE Functions::AlienfxGetDeviceStatus()
	{
		if (pid == -1) return 0;
		//size_t BytesWritten;
		byte ret = 0;
		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE] = {0};
		switch (length) {
		case API_L_V5:
		{
			memcpy(buffer, COMMV5.status, sizeof(COMMV5.status));
			HidD_SetOutputReport(devHandle, buffer, length);
			buffer[0] = 0xcc;
			if (HidD_GetFeature(devHandle, buffer, length)) //DeviceIoControl(devHandle, IOCTL_HID_GET_FEATURE, 0, 0, buffer, length, (DWORD*) &BytesWritten, NULL))
				ret = buffer[2];
#ifdef _DEBUG
			//cout << "IO result is ";
			//switch (GetLastError()) {
			//case 0: cout << "Ok."; break;
			//case 1: cout << "Incorrect function"; break;
			//case ERROR_INSUFFICIENT_BUFFER: cout << "Buffer too small!"; break;
			//case ERROR_MORE_DATA: cout << "More data remains!"; break;
			//default: cout << "Unknown (" << GetLastError() << ")";
			//}
			//cout <<  endl;
			//cout << "Data: " << buffer[0] << "," << buffer[1] << "," << buffer[2] << endl;
#endif
		} break;
		case API_L_V4: {
			if (HidD_GetInputReport(devHandle, buffer, length))
				//DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, NULL, NULL))
				ret = buffer[2];
		} break;
		case API_L_V3: case API_L_V2: case API_L_V1: {
			memcpy(buffer, COMMV1.status, sizeof(COMMV1.status));
			HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);

			buffer[0] = 0x01;
			HidD_GetInputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, buffer, length, (DWORD*)&BytesWritten, NULL);

			if (buffer[0] == 0x01)
				ret = 0x06;
			else ret = buffer[0];
		} break;
		}
#ifdef _DEBUG
		if (ret == 0) {
			OutputDebugString(TEXT("System hangs!\n"));
		}
#endif
		return ret;
	}

	BYTE Functions::AlienfxWaitForReady()
	{
		byte status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		for (int i = 0; i < 10 && (status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus()) != ALIENFX_V2_READY && status != ALIENFX_V4_READY; i++)
		{
			if (status == ALIENFX_V2_RESET)
				return status;
			Sleep(5);
		}
		return status;
	}

	BYTE Functions::AlienfxWaitForBusy()
	{

		byte status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		for (int i = 0; i < 10 && (status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus()) != ALIENFX_V2_BUSY && status != ALIENFX_V4_BUSY; i++)
		{
			if (status == ALIENFX_V2_RESET)
				return status;
			Sleep(5);
		}
		return status;
	}

	bool Functions::IsDeviceReady()
	{
		int status;
		switch (length) {
		case API_L_V5:
			status = AlienfxGetDeviceStatus();
			return status == ALIENFX_V5_STARTCOMMAND || status == ALIENFX_V5_INCOMMAND;
		case API_L_V4:
			status = AlienfxGetDeviceStatus();
			return status == 0 || status == ALIENFX_V4_READY || status == ALIENFX_V4_WAITUPDATE || status == ALIENFX_V4_WASON;
		case API_L_V2: case API_L_V1:
			switch (AlienfxGetDeviceStatus()) {
			case ALIENFX_V2_READY:
				return true;
			case ALIENFX_V2_BUSY:
				Reset(0x04);
				return AlienfxWaitForReady() == ALIENFX_V2_READY;
			case ALIENFX_V2_RESET:
				return AlienfxWaitForReady() == ALIENFX_V2_READY;
			}
			return false;
		}
		return true;
	}

	bool Functions::AlienFXClose()
	{
		bool result = false;
		if (devHandle != NULL)
		{
			result = CloseHandle(devHandle);
		}
		return result;
	}

	bool Functions::AlienFXChangeDevice(int nvid, int npid)
	{
		int res;
		if (pid != (-1) && devHandle != NULL)
				CloseHandle(devHandle);
		res = AlienFXInitialize(nvid, npid);
		if (res != (-1)) {
			pid = npid;
			Reset(false);
			return true;
		}
		return false;
	}

	Mappings::~Mappings () {
		mappings.clear();
		devices.clear();
	}

	vector<pair<DWORD,DWORD>> Mappings::AlienFXEnumDevices()
	{
		vector<pair<DWORD,DWORD>> pids;
		GUID guid;
		bool flag = false;
		HANDLE tdevHandle;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE)
		{
			//std::cout << "Couldn't get guid";
			return pids;
		}
		unsigned int dw = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

		unsigned int lastError = 0;
		while (!flag)
		{
			deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData))
			{
				lastError = GetLastError();
				flag = true;
				continue;
			}
			dw++;
			DWORD dwRequiredSize = 0;
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL))
			{
				//std::cout << "Getting the needed buffer size failed";
				//return pids;
				continue;
			}
			//std::cout << "Required size is " << dwRequiredSize << std::endl;
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			{
				//std::cout << "Last error is not ERROR_INSUFFICIENT_BUFFER";
				//return pid;
				continue;
			}
			std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new char[dwRequiredSize]);
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL))
			{
				std::wstring devicePath = deviceInterfaceDetailData->DevicePath;
				//OutputDebugString(devicePath.c_str());
				tdevHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				if (tdevHandle != INVALID_HANDLE_VALUE)
				{
					std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
					attributes->Size = sizeof(HIDD_ATTRIBUTES);
					if (HidD_GetAttributes(tdevHandle, attributes.get()))
					{
						for (unsigned i = 0; i < sizeof(vids)/4; i++) {
							if (attributes->VendorID == vids[i]) {

								PHIDP_PREPARSED_DATA prep_caps;
								HIDP_CAPS caps;
								HidD_GetPreparsedData(tdevHandle, &prep_caps);
								HidP_GetCaps(prep_caps, &caps);
								HidD_FreePreparsedData(prep_caps);

								if (caps.OutputReportByteLength || caps.Usage == 0xcc) {
#ifdef _DEBUG
									wchar_t buff[2048];
									swprintf_s(buff, 2047, L"Scan: VID: %#x, PID: %#x, Version: %d, Length: %d\n",
											   attributes->VendorID, attributes->ProductID, attributes->VersionNumber, attributes->Size);
									OutputDebugString(buff);
									wprintf(L"%s", buff);
#endif
									pair<DWORD, DWORD> dev;
									dev.first = attributes->VendorID;
									dev.second = attributes->ProductID;
									pids.push_back(dev);
									break;
								}
							}
						}
					}
				}
				CloseHandle(tdevHandle);
			}
		}
		return pids;
	}	

	void Mappings::AddMapping(int devID, int lightID, char* name, int flags) {
		mapping map;
		int i = 0;
		for (i = 0; i < mappings.size(); i++) {
			if (mappings[i].devid == devID && mappings[i].lightid == lightID) {
				if (name != NULL)
					mappings[i].name = name;
				if (flags != -1)
					mappings[i].flags = flags;
				break;
			}
		}
		if (i == mappings.size()) {
			// add new mapping
			map.devid = devID; map.lightid = lightID;
			if (name != NULL)
				map.name = name;
			if (flags != -1)
				map.flags = flags;
			mappings.push_back(map);
		}
	}

	void Mappings::LoadMappings() {
		DWORD  dwDisposition;
		HKEY   hKey1;

		devices.clear();
		mappings.clear();

		RegCreateKeyEx(HKEY_CURRENT_USER,
			TEXT("SOFTWARE\\Alienfx_SDK"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey1,
			&dwDisposition);

		unsigned vindex = 0; 
		mapping map; 
		devmap dev;
		char name[255], inarray[255];
		unsigned ret = 0;
		do {
			DWORD len = 255, lend = 255;
			ret = RegEnumValueA(
				hKey1,
				vindex,
				name,
				&len,
				NULL,
				NULL,
				(LPBYTE)inarray,
				&lend
			);
			// get id(s)...
			if (ret == ERROR_SUCCESS) {
				DWORD lID = 0, dID = 0;
				vindex++;
				if (sscanf_s((char*)name, "%d-%d", &dID, &lID) == 2) {
					// light name
					AddMapping(dID, lID, inarray, -1);
					continue;
				}
				if (sscanf_s((char*)name, "Flags%d-%d", &dID, &lID) == 2) {
					// light flags
					AddMapping(dID, lID, NULL, *(unsigned *)inarray);
					continue;
				}
				if (sscanf_s((char*)name, "Dev#%d", &dev.devid) == 1) {
					std::string devname(inarray);
					dev.name = devname;
					devices.push_back(dev);
					continue;
				}
			}
		} while (ret == ERROR_SUCCESS);
		RegCloseKey(hKey1);
	}

	void Mappings::SaveMappings() {
		DWORD  dwDisposition;
		HKEY   hKey1;
		size_t numdevs = devices.size();
		size_t numlights = mappings.size();
		if (numdevs == 0) return;
		
		RegCreateKeyEx(HKEY_CURRENT_USER,
			TEXT("SOFTWARE\\Alienfx_SDK"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey1,
			&dwDisposition);
		char name[1024];

		for (int i = 0; i < numdevs; i++) {
			//preparing name
			sprintf_s(name, 255, "Dev#%d", devices[i].devid);

			RegSetValueExA(
				hKey1,
				name,
				0,
				REG_SZ,
				(BYTE*)devices[i].name.c_str(),
				(DWORD)devices[i].name.size()
			);
		}

		for (int i = 0; i < numlights; i++) {
			//preparing name
			sprintf_s((char*)name, 255, "%d-%d", mappings[i].devid, mappings[i].lightid);
			//sprintf_s((char*)name, 1024, "Light %d-%d-%s\n", mappings[i].devid, mappings[i].lightid, mappings[i].name.c_str());

			RegSetValueExA(
				hKey1,
				name,
				0,
				REG_SZ,
				(BYTE*)mappings[i].name.c_str(),
				(DWORD)mappings[i].name.size()
			);
			sprintf_s((char*)name, 255, "Flags%d-%d", mappings[i].devid, mappings[i].lightid);

			RegSetValueExA(
				hKey1,
				name,
				0,
				REG_DWORD,
				(BYTE*)&mappings[i].flags,
				4
			);
		}

		std::vector <mapping> oldMappings = mappings;
		LoadMappings();
		// remove non-existing mappings...
		for (int i = 0; i < mappings.size(); i++) {
			int j;
			for (j = 0; j < numlights && (mappings[i].devid != oldMappings[j].devid ||
				mappings[i].lightid != oldMappings[j].lightid); j++);
			if (j == numlights) { // no mapping found, delete...
				sprintf_s((char*)name, 255, "%d-%d", mappings[i].devid, mappings[i].lightid);
				RegDeleteValueA(hKey1, name);
				sprintf_s((char*)name, 255, "Flags%d-%d", mappings[i].devid, mappings[i].lightid);
				RegDeleteValueA(hKey1, name);
			}
		}

		mappings = oldMappings;

		RegCloseKey(hKey1);
	}

	std::vector<mapping>* Mappings::GetMappings()
	{
		return &mappings;
	}

	int Mappings::GetFlags(int devid, int lightid)
	{
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i].devid == devid && mappings[i].lightid == lightid)
				return mappings[i].flags;
		return -1;
	}

	void Mappings::SetFlags(int devid, int lightid, int flags)
	{
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i].devid == devid && mappings[i].lightid == lightid) {
				mappings[i].flags = flags;
				return;
			}
	}

	std::vector<devmap>* Mappings::GetDevices()
	{
		return &devices;
	}

	int Functions::GetPID()
	{
		return pid;
	}

	int Functions::GetVersion()
	{
		switch (length) {
		case API_L_V5: return 5; break;
		case API_L_V4: return 4; break;
		case API_L_V3: return 3; break;
		case API_L_V2: return 2; break;
		case API_L_V1: return 1; break;
		default: return 0;
		}
		return length;
	}

}


