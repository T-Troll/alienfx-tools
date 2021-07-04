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
	struct com_apiv1_apiv2 {
		byte reset[2] = {0x02 ,0x07};
		byte loop[2] = {0x02, 0x04};
		byte color[2] = {0x02, 0x03};
		byte update[2] = {0x02, 0x05};
		byte status[2] = {0x02 ,0x06};
	} COMMV2;

	struct com_apiv3 {
		byte reset[7] = {0x00, 0x03 ,0x21 ,0x00 ,0x01 ,0xff ,0xff};
		byte colorSel[6] = {0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01};
		byte colorSet[8] = {0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa};
		byte update[7] = {0x00, 0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0xff};
		byte setPower[7] = {0x00, 0x03 ,0x22 ,0x00, 0x04, 0x00, 0x5b};
	} COMMV3;

#ifdef API_V4
	struct com_apiv4 {
		byte reset[3] = { 0xcc, 0xcc, 0x96};
		byte colorSel[19] = {0xcc, 0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
		byte colorSet[11] = {0xcc, 0xcc, 0x8c, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff};
		byte loop[4] = {0xcc, 0xcc, 0x8c, 0x13};
		byte update[5] = {0xcc, 0xcc, 0x8b, 0x01, 0x59};
		//byte setPower[7] = {0x00, 0x03 ,0x22 ,0x00 ,0x04 ,0x00 ,0x5b};
		//byte status;
	} COMMV4;
#endif

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
#ifdef _DEBUG
								wchar_t buff[2048];
								swprintf_s(buff, 2047, L"Scan: VID: %#x, PID: %#x, Version: %d, Length: %d\n",
										   attributes->VendorID, attributes->ProductID, attributes->VersionNumber, attributes->Size);
								OutputDebugString(buff);
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
				CloseHandle(tdevHandle);
			}
		}
		return pids;
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
#ifdef API_V4
							// Is it Darfon? Then set 64 bytes length.
							if (vid == vids[1]) {
								length = API_L_V4;
								version = API_V4;
							}
							else
#endif
								if (attributes->VersionNumber > 511) {
									length = API_L_V3;
									version = API_V3;
								}
								else {
									length = attributes->Size;
									if (attributes->ProductID > 0x529)
										version = API_V25;
									else
										version = API_V2;
								}
							this->vid = attributes->VendorID;
							pid = attributes->ProductID;
							flag = true;
#ifdef _DEBUG
							wchar_t buff[2048];
							swprintf_s(buff, 2047, L"Init: VID: %#x, PID: %#x, Version: %d, Length: %d\n",
									   attributes->VendorID, attributes->ProductID, attributes->VersionNumber, attributes->Size);
							OutputDebugString(buff);
							cout << buff;
#endif
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
#ifdef API_V4
		case API_L_V4:
			memcpy(buffer, COMMV4.loop, sizeof(COMMV4.loop));
			HidD_SetOutputReport(devHandle, buffer, length);
			break;
#endif
		//case API_L_V3: {
			// m15 require Input report as a confirmation, not output. 
			// WARNING!!! In latest firmware, this can provide up to 10sec(!) slowdown, so i disable status read. It works without it as well.
			// DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, BufferN, length, (DWORD*)&BytesWritten, NULL);
			// std::cout << "Status: 0x" << std::hex << (int) BufferN[2] << std::endl;
		//} break;
		case API_L_V2: case API_L_V1: {
			memcpy(buffer, COMMV2.loop, sizeof(COMMV2.loop));
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
#ifdef API_V4
		case API_L_V4: {
			memcpy(buffer, COMMV4.reset, sizeof(COMMV4.reset));
			//result = DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			result = HidD_SetFeature(devHandle, buffer, length);
			cout << "IO result is ";
			switch (GetLastError()) {
			case 0: cout << "Ok."; break;
			case 1: cout << "Incorrect function"; break;
			case ERROR_INSUFFICIENT_BUFFER: cout << "Buffer too small!"; break;
			case ERROR_MORE_DATA: cout << "More data remains!"; break;
			default: cout << "Unknown (" << GetLastError() << ")";
			}
			cout <<  endl;
		} break;
#endif
		case API_L_V3: {
			memcpy(buffer, COMMV3.reset, sizeof(COMMV3.reset));
			//result = HidD_SetOutputReport(devHandle, buffer, length);
			result = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, NULL, NULL);
			//cout << "IO result is " << GetLastError() << endl;
		} break;
		case API_L_V2: case API_L_V1: {
			memcpy(buffer, COMMV2.reset, sizeof(COMMV2.reset));
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
		//if (inSet) {
			//byte* buffer = new byte[length];
			byte buffer[MAX_BUFFERSIZE];
			ZeroMemory(buffer, length);
			switch (length) {
#ifdef API_V4
			case API_L_V4:
			{
				memcpy(buffer, COMMV4.update, sizeof(COMMV4.update));
				res = HidD_SetFeature(devHandle, buffer, length);
				//return DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*) &BytesWritten, NULL);
			}
#endif
			case API_L_V3:
			{
				memcpy(buffer, COMMV3.update, sizeof(COMMV3.update));
				res = HidD_SetOutputReport(devHandle, buffer, length);
				//return DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*) &BytesWritten, NULL);
			} break;
			case API_L_V2: case API_L_V1:
			{
				memcpy(buffer, COMMV2.update, sizeof(COMMV2.update));
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
		//}
		return false;
	}

	bool Functions::SetColor(unsigned index, byte r, byte g, byte b)
	{
		//size_t BytesWritten;
		bool val = false;
		// API v4 command - 11,12,13 and 14,15,16 is RGB
		// API v4 command - 4 is index
		/// API v3 - Buffer[8,9,10] = rgb
		/// But we need to issue 2 commands - light_select and color_set.... this for light_select
		/// Buffer2[5] - Count of lights need to be set
		/// Buffer2[6-33] - LightID (index, not mask) - it can be COUNT of them.
		if (!inSet)
			Reset(1);
		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE];
		ZeroMemory(buffer, length);
		switch (version) {
#ifdef API_V4
		case API_V4: {
			memcpy(buffer, COMMV4.colorSel, sizeof(COMMV4.colorSel));
			buffer[11] = buffer[14] = r;
			buffer[12] = buffer[15] = g;
			buffer[13] = buffer[16] = b;
			HidD_SetFeature(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV4.colorSet, sizeof(COMMV4.colorSet));
			buffer[4] = index;
			//HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			//ZeroMemory(buffer, length);
			//memcpy(buffer, COMMV4.colorAfterSet, sizeof(COMMV4.colorAfterSet));
			return HidD_SetOutputReport(devHandle, buffer, length);
			//return DeviceIoControl(devHandle, IOCTL_HID_SET_FEATURE, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
#endif
		case API_V3: {
			memcpy(buffer, COMMV3.colorSel, sizeof(COMMV3.colorSel));
			buffer[6] = index;
			HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, NULL, NULL);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV3.colorSet, sizeof(COMMV3.colorSet));
			buffer[8] = r;
			buffer[9] = g;
			buffer[10] = b;
		} break;
		case API_V25:
		{
			int location = index < 14 ? mask12[index] : 0;
			memcpy(buffer, COMMV2.color, sizeof(COMMV2.color));
			buffer[2] = (byte)index;
			buffer[3] = (location & 0xFF0000) >> 16;
			buffer[4] = (location & 0x00FF00) >> 8;
			buffer[5] = (location & 0x0000FF);
			buffer[6] = r;
			buffer[7] = g;
			buffer[8] = b;

		} break;
		case API_V2: {
			int location = index < 14 ? mask12[index] : 0;
			memcpy(buffer, COMMV2.color, sizeof(COMMV2.color));
			buffer[2] = (byte)index;
			buffer[3] = (location & 0xFF0000) >> 16;
			buffer[4] = (location & 0x00FF00) >> 8;
			buffer[5] = (location & 0x0000FF);
			buffer[6] = (r & 0xf0) | ((g & 0xf0) >> 4); // 4-bit color!
			buffer[7] = b;

			if (index == 0)
			{
				buffer[1] = 0x01;
				buffer[2] = (byte)index;
				buffer[3] = 00;
				buffer[4] = 01;
				buffer[5] = 00;
			}
		} break;
		case API_V1: {
			int location = index < 14 ?mask8[index] : 0;
			memcpy(buffer, COMMV2.color, sizeof(COMMV2.color));
			buffer[2] = index;
			buffer[3] = (location & 0xFF0000) >> 16;
			buffer[4] = (location & 0x00FF00) >> 8;
			buffer[5] = (location & 0x0000FF);
			buffer[6] = r;
			buffer[7] = g;
			buffer[8] = b;

			if (index == 5)
			{
				buffer[1] = 0x83;
				buffer[2] = (byte)index;
				buffer[3] = 00;
				buffer[4] = 00;
				buffer[5] = 00;
			}

			if (index == 11)
			{
				buffer[1] = 0x01;
				buffer[2] = (byte)index;
				buffer[3] = 00;
				buffer[4] = 01;
				buffer[5] = 00;
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
		/// We need to issue 2 commands - light_select and color_set.... this for light_select
		/// Buffer2[5] - Count of lights need to be set
		/// Buffer2[6-33] - LightID (index, not mask).
		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE];
		switch (length) {
		case API_L_V3: {
			if (!inSet) Reset(false);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV3.colorSel, sizeof(COMMV3.colorSel));
			buffer[5] = numLights;
			for (int nc = 0; nc < numLights; nc++)
				buffer[6 + nc] = lights[nc];
			HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV3.colorSet, sizeof(COMMV3.colorSet));
			buffer[8] = r;
			buffer[9] = g;
			buffer[10] = b;
			val = HidD_SetOutputReport(devHandle, buffer, length);
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		case API_L_V2: case API_L_V1: {
			for (int nc = 0; nc < numLights; nc++)
				val = SetColor(lights[nc], r, g, b);
		} break;
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
		byte buffer[MAX_BUFFERSIZE];
		switch (length) {
		case API_L_V3: { // only supported at new devices
			if (!inSet) Reset(false);
			int bPos = 3;
			if (act.size() > 0) {
				ZeroMemory(buffer, length);
				memcpy(buffer, COMMV3.colorSel, sizeof(COMMV3.colorSel));
				buffer[6] = index;
				HidD_SetOutputReport(devHandle, buffer, length);
				//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);			
				for (int ca = 0; ca < act.size(); ca++) {
					ZeroMemory(buffer, length);
					memcpy(buffer, COMMV3.colorSet, sizeof(COMMV3.colorSet));
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
			}
			return res;
		} break;
		case API_L_V2: {
			res = SetColor(index, act[0].r, act[0].g, act[0].b);
			// But here trick for Pulse!
			switch (act[0].type) {
			case 1: SetColor(index, act[0].r, act[0].g, act[0].b); break;
			}
		} break;
		case API_L_V1: {
			// can't set action for old, just use color. 
			res = SetColor(index, act[0].r, act[0].g, act[0].b);
		} break;
		}
		return res;
	}

	bool Functions::SetPowerAction(int index, BYTE Red, BYTE Green, BYTE Blue, BYTE Red2, BYTE Green2, BYTE Blue2, bool force)
	{
		//size_t BytesWritten;
		switch (length) {
		case API_L_V3: { // only supported at new devices
			// this function can be called not early then 250ms after last call!
			//ULONGLONG cPowerCall = GetTickCount64();
#ifdef _DEBUG
			if (force)
				OutputDebugString(TEXT("Forced power button update!\n"));
#endif
			//if (cPowerCall - lastPowerCall < POWER_DELAY)
			//	if (force) {
#ifdef _DEBUG
			//		OutputDebugString(TEXT("Forced power button update waiting...\n"));
#endif
			//		Sleep(lastPowerCall + POWER_DELAY - cPowerCall);
			//	}
			//	else {
#ifdef _DEBUG
			//		OutputDebugString(TEXT("Power button update skipped!\n"));
#endif
			//		return false;
			//	}
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
			//byte* buffer = new byte[length];
			byte buffer[MAX_BUFFERSIZE];
			ZeroMemory(buffer, length);
			memcpy(buffer, COMMV3.setPower, sizeof(COMMV3.setPower));
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
			buffer[4] = 6;
			HidD_SetOutputReport(devHandle, buffer, length);
			//DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			//lastPowerCall = GetTickCount64();
			Sleep(50);
			// Let's try to wait until system ready...
			if (force)
				for (UINT counter = 0; !IsDeviceReady() && counter < 10; counter++) Sleep(20);
			Reset(false);
		} break;
		case API_L_V2: case API_L_V1: {
			// can't set action for old, just use color
			SetColor(index, Red, Green, Blue);
		} break;
		}
		return true;
	}
	//int ReadStatus;

	BYTE Functions::AlienfxGetDeviceStatus()
	{
		if (pid == -1) return 0;
		//size_t BytesWritten;
		byte ret = 0;
		//byte* buffer = new byte[length];
		byte buffer[MAX_BUFFERSIZE];
		ZeroMemory(buffer, length);
		switch (length) {
#ifdef API_V4
		case API_L_V4:
		{
			buffer[0] = 0xcc;
			if (HidD_GetFeature(devHandle, buffer, length)) //DeviceIoControl(devHandle, IOCTL_HID_GET_FEATURE, 0, 0, buffer, length, (DWORD*) &BytesWritten, NULL))
				ret = buffer[2];
			cout << "IO result is ";
			switch (GetLastError()) {
			case 0: cout << "Ok."; break;
			case 1: cout << "Incorrect function"; break;
			case ERROR_INSUFFICIENT_BUFFER: cout << "Buffer too small!"; break;
			case ERROR_MORE_DATA: cout << "More data remains!"; break;
			default: cout << "Unknown (" << GetLastError() << ")";
			}
			cout <<  endl;
			cout << "Data: " << buffer[0] << "," << buffer[1] << "," << buffer[2] << endl;
		} break;
#endif
		case API_L_V3: {
			if (HidD_GetInputReport(devHandle, buffer, length))
				//DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, buffer, length, NULL, NULL))
				ret = buffer[2];
		} break;
		case API_L_V2: case API_L_V1: {
			memcpy(buffer, COMMV2.status, sizeof(COMMV2.status));
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
		for (int i = 0; i < 10 && (status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus()) != ALIENFX_V2_READY && status != ALIENFX_V3_READY; i++)
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
		for (int i = 0; i < 10 && (status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus()) != ALIENFX_V2_BUSY && status != ALIENFX_V3_BUSY; i++)
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
#ifdef API_V4
		case API_V4:
			status = AlienfxGetDeviceStatus();
			return status == 1 || status == 0x96;
#endif
		case API_L_V3:
			status = AlienfxGetDeviceStatus();
			return status == 0 || status == ALIENFX_V3_READY || status == ALIENFX_V3_WAITUPDATE;
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

	void Mappings::AddMapping(int devID, int lightID, char* name, int flags) {
		mapping map;
		int i = 0;
		for (i = 0; i < mappings.size(); i++) {
			if (mappings[i].devid == devID && mappings[i].lightid == lightID) {
				if (name != NULL)
					mappings[i].name = name;
				else
					mappings[i].flags = flags;
				break;
			}
		}
		if (i == mappings.size()) {
			// add new mapping
			map.devid = devID; map.lightid = lightID;
			if (name != NULL)
				map.name = name;
			else
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

		unsigned vindex = 0; mapping map; devmap dev;
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
				//unsigned ret2 = sscanf_s((char*)name, "%d-%d", &dID, &lID);
				if (sscanf_s((char*)name, "%d-%d", &dID, &lID) == 2) {
					// light name
					AddMapping(dID, lID, inarray, 0);
				}
				else {
					// device flags?
					//ret2 = sscanf_s((char*)name, "Flags%d-%d", &dID, &lID);
					if (sscanf_s((char*)name, "Flags%d-%d", &dID, &lID) == 2) {
						AddMapping(dID, lID, NULL, *(unsigned *)inarray);
					}
					else {
						// device name?
						//ret2 = sscanf_s((char*)name, "Dev#%d", &dev.devid);
						if (sscanf_s((char*)name, "Dev#%d", &dev.devid) == 1) {
							std::string devname(inarray);
							dev.name = devname;
							devices.push_back(dev);
						}
					}
				}
				vindex++;
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
		char name[256];

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

	/*mapping* Functions::GetMappingById(int devID, int lightID)
	{
		for (unsigned i = 0; i < mappings.size(); i++)
			if (mappings[i].devid == devID && mappings[i].lightid == lightID)
				return &mappings[i];
		return nullptr;
	}*/

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
#ifdef API_V4
		case API_L_V4: return 4; break;
#endif
		case API_L_V3: return 3; break;
		case API_L_V2: return 2; break;
		case API_L_V1: return 1; break;
			default: return -1;
		}
		return length;
	}

}


