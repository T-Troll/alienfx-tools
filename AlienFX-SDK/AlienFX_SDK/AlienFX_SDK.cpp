#include "stdafx.h"
#include <Setupapi.h>
#include <codecvt>
#include <locale>
#include "AlienFX_SDK.h"  
#include <iostream>
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

#define ALIENFX_DEVICE_RESET 0x06
#define ALIENFX_READY 0x10
#define ALIENFX_BUSY 0x11
#define ALIENFX_UNKNOWN_COMMAND 0x12
// new statuses for m15 - 33 = ok, 36 = wait for update, 35 = wait for color, 34 - busy processing power update
#define ALIENFX_NEW_READY 33
#define ALIENFX_NEW_BUSY 34
#define ALIENFX_NEW_WAITCOLOR 35
#define ALIENFX_NEW_WAITUPDATE 36

// Length by API version:
#define API_V4 65
#define API_V3 34
#define API_V2 12
#define API_V1 8

#define POWER_DELAY 260

namespace AlienFX_SDK
{
	/*bool isInitialized = false;
	HANDLE devHandle;
	int length = 9;
	bool inSet = false;
	ULONGLONG lastPowerCall = GetTickCount64();

	// Name mappings for lights
	static std::vector <mapping> mappings;
	static std::vector <devmap> devices;

	static int pid = -1;
	static int version = -1;*/

	std::vector<int> Functions::AlienFXEnumDevices(int vid)
	{
		std::vector<int> pids;
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

						if (vid == 0 || attributes->VendorID == vid)
						{
#ifdef _DEBUG
							wchar_t buff[2048];
							swprintf_s(buff, 2047, L"VID: %#x, PID: %#x, Version: %d, Length: %d\n",
								attributes->VendorID, attributes->ProductID, attributes->VersionNumber, attributes->Size);
							OutputDebugString(buff);
							//std::cout << std::hex << "VID:" << attributes->VendorID << ",PID:" << attributes->ProductID 
							//	<< ",Ver:" << attributes->VersionNumber << ",Len:" << attributes->Size << std::endl;
#endif
							pids.push_back(attributes->ProductID);
						}
					}
				}
				CloseHandle(tdevHandle);
			}
		}
		return pids;
	}

	//Use this method to scan for devices that uses same vid
	int Functions::AlienFXInitialize(int vid)
	{
		pid = -1;
		GUID guid;
		bool flag = false;

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

						if (attributes->VendorID == vid)
						{
							// Is it Darfon? Then set 64 bytes length.
							if (vid == AlienFX_SDK::Functions::vid2)
								length = API_V4;
							else
								// I use Version to detect is it old device or new, i have version = 0 for old, and version = 512 for new
								if (attributes->VersionNumber > 511)
									length = API_V3;
								else
									length = attributes->Size;
							pid = attributes->ProductID;
							version = attributes->VersionNumber;
							flag = true;
						}
					}


				}
			}
		}
		//OutputDebugString(flag);
		return pid;
	}

	//Use this method for general devices
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

						if (((attributes->VendorID == vid) && (attributes->ProductID == pidd)))
						{
							// Check API version...
							// Is it Darfon? Then set 64 bytes length.
							if (vid == AlienFX_SDK::Functions::vid2)
								length = API_V4;
							else
								if (attributes->VersionNumber > 511)
									length = API_V3;
								else
									length = attributes->Size;
							version = attributes->VersionNumber;
							pid = pidd;
							flag = true;
						}
					}

				}
			}
		}
		//OutputDebugString(flag);
		return pid;
	}

	void Functions::Loop()
	{
		size_t BytesWritten;
		byte BufferN[] = { 0x00, 0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x02 ,0x04 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,0x00 ,0x00 ,0x00 };
		switch (length) {
		case API_V3: {
			// m15 require Input report as a confirmation, not output. 
			// WARNING!!! In latest firmware, this can provide up to 10sec(!) slowdown, so i disable status read. It works without it as well.
			// DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, BufferN, length, (DWORD*)&BytesWritten, NULL);
			// std::cout << "Status: 0x" << std::hex << (int) BufferN[2] << std::endl;
		} break;
		case API_V2: case API_V1: {
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		}
	}

	byte AlienfxWaitForReady();
	byte AlienfxWaitForBusy();

	bool Functions::Reset(int status)
	{
		size_t BytesWritten;
		bool result = false;
		// m15/m17 use 34 bytes (ID (always 0) + 33 bytes payload) report.
		byte BufferN[] = { 0x00, 0x03 ,0x21 ,0x00 ,0x01 ,0xff ,0xff ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x02 ,0x07 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00 };

		switch (length) {
		case API_V3: {
			result = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferN, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
		} break;
		case API_V2: case API_V1: {
			if (status)
				BufferO[2] = 0x04;
			else
				BufferO[2] = 0x03;
			AlienfxWaitForBusy();
			result = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			AlienfxWaitForReady();
		} break;
		}
		inSet = true;
		//std::cout << "Reset!" << std::endl;
		return result;
	}

	bool Functions::UpdateColors()
	{
		size_t BytesWritten;
		bool res = false;
		// API v4 command
		byte Buffer4[] = { 0x00, 0xcc, 0x8c, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		// As well, 34 byte report for m15 - first byte ID, then command
		byte BufferN[] = { 0x00, 0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0xff ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x02 ,0x05 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,0x00 ,0x00 ,0x00 };
		switch (length) {
		case API_V4: {
			if (inSet) {
				res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer4, API_V4, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();
			}
		}
		case API_V3: {
			if (inSet) {
				res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferN, API_V3, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();
			}
		} break;
		case API_V2: case API_V1: {
			res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		}
		//std::cout << "Update!" << std::endl;
		inSet = false;
		return res;
	}

	/*
	int leftZone = 0x100;// 0x8;
	int leftMiddleZone = 0x8;// 0x4;
	int rightZone = 0x2;// 0x1;
	int rightMiddleZone = 0x4;// 0x2;
	int Macro = 0x1;// 0;
	int AlienFrontLogo = 0x20;// 0x40;
	int LeftPanelTop = 0x40;// 0x1000;
	int LeftPanelBottom = 0x280;// 0x400;
	int RightPanelTop = 0xf7810;// 0x2000;
	int RightPanelBottom = 0x800;
	int AlienBackLogo = 0x20;
	int Power = 13;
	int TouchPad = 0x80;
	*/

	int mask8[] = { 13, 0x8, 0x4, 0x1, 0x2, 0, 0x20, 0x40, 0x1000, 0x400, 0x2000, 0x800, 0x20, 13, 0x80 },
		mask12[] = { 0x40, 0x100, 0x8, 0x4, 0x2, 0x1, 0x20, 0x40, 0x280, 0xf7810, 0x800, 0x20, 13, 0x80 };

	bool Functions::SetColor(int index, int r, int g, int b)
	{
		size_t BytesWritten;
		bool val = false;
		// API v4 command - 11,12,13 and 14,15,16 is RGB
		byte Buffer41[] = { 0x00, 0xcc, 0x8c, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		// API v4 command - 4 is index
		byte Buffer42[] = { 0x00, 0xcc, 0x8c, 0x02, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		// Buffer[8,9,10] = rgb
		byte BufferN[] = { 0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00 ,0x00 ,0x00 };
		/// But we need to issue 2 commands - light_select and color_set.... this for light_select
		/// Buffer2[5] - Count of lights need to be set
		/// Buffer2[6-33] - LightID (index, not mask) - it can be COUNT of them.
		byte Buffer2[] = { 0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		//byte BufferO2[] = { 0x02 ,0x08 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00 ,0x00 ,0x00 };
		if (!inSet)
			Reset(1);
		switch (length) {
		case API_V4: {
			Buffer41[11] = Buffer41[14] = r;
			Buffer41[12] = Buffer41[15] = g;
			Buffer41[13] = Buffer41[16] = b;
			Buffer42[4] = index;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer41, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer42, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		case API_V3: {
			Buffer2[6] = index;
			BufferN[8] = r;
			BufferN[9] = g;
			BufferN[10] = b;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer2, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferN, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		case API_V2: {
			int location = mask12[index];
			BufferO[0] = 0x02;
			BufferO[1] = 0x03;
			BufferO[2] = index;
			BufferO[3] = (location & 0xFF0000) >> 16;
			BufferO[4] = (location & 0x00FF00) >> 8;
			BufferO[5] = (location & 0x0000FF);
			BufferO[6] = (r & 0xf0) | ((g & 0xf0) >> 4); // 4-bit color!
			BufferO[7] = b;

			if (index == 0)
			{
				BufferO[1] = 0x01;
				BufferO[2] = (byte)index;
				BufferO[3] = 00;
				BufferO[4] = 01;
				BufferO[5] = 00;
			}

			val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		case API_V1: {
			int location = mask8[index];
			BufferO[0] = 0x02;
			BufferO[1] = 0x03;
			BufferO[2] = index;
			BufferO[3] = (location & 0xFF0000) >> 16;
			BufferO[4] = (location & 0x00FF00) >> 8;
			BufferO[5] = (location & 0x0000FF);
			BufferO[6] = r;
			BufferO[7] = g;
			BufferO[8] = b;

			if (index == 5)
			{
				BufferO[1] = 0x83;
				BufferO[2] = (byte)index;
				BufferO[3] = 00;
				BufferO[4] = 00;
				BufferO[5] = 00;
			}

			if (index == 11)
			{
				BufferO[1] = 0x01;
				BufferO[2] = (byte)index;
				BufferO[3] = 00;
				BufferO[4] = 01;
				BufferO[5] = 00;
			}

			val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		} break;
		}
		Loop();
		return val;
	}

	bool Functions::SetMultiColor(int numLights, UCHAR* lights, int r, int g, int b)
	{
		size_t BytesWritten; bool val = false;
		byte Buffer[] = { 0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		/// But we need to issue 2 commands - light_select and color_set.... this for light_select
		/// Buffer2[5] - Count of lights need to be set
		/// Buffer2[6-33] - LightID (index, not mask) - it can be COUNT of them.
		byte Buffer2[] = { 0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		switch (length) {
		case API_V3: {
			Buffer2[5] = numLights;
			for (int nc = 0; nc < numLights; nc++)
				Buffer2[6 + nc] = lights[nc];
			Buffer[8] = r;
			Buffer[9] = g;
			Buffer[10] = b;
			if (!inSet) Reset(false);
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer2, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
		} break;
		case API_V2: case API_V1: {
			for (int nc = 0; nc < numLights; nc++)
				val = SetColor(lights[nc], r, g, b);
		} break;
		}
		return val;
	}

	bool Functions::SetAction(int index, std::vector<afx_act> act)
		//int action, int time, int tempo, int Red, int Green, int Blue, int action2, int time2, int tempo2, int Red2, int Green2, int Blue2)
	{
		size_t BytesWritten;
		// Buffer[3], [11] - action type ( 0 - light, 1 - pulse, 2 - morph)
		// Buffer[4], [12] - how long phase keeps
		// Buffer[5], [13] - mode (action type) - 0xd0 - light, 0xdc - pulse, 0xcf - morph, 0xe8 - power morph, 0x82 - spectrum, 0xac - rainbow
		// Buffer[7], [15] - tempo (0xfa - steady)
		// Buffer[8-10]    - rgb
		// 00 03 24 02 0b b7 00 64
		// 00 03 24 02 05 dc 00 64 - pulse, but mode 2
		// 00 03 24 02 02 82 00 0f x3rgb(!) x3(!) - last one only 1 color.
		// 00 03 24 02 01 ac 00 0f x3rgb x3
		byte Buffer[] = { 0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0x32 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x64 , 0x00
		, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00, 0x00 };
		byte Buffer2[] = { 0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		switch (length) {
		case API_V3: { // only supported at new devices
			if (!inSet) Reset(false);
			int bPos = 3, res = 0;
			if (act.size() > 0) {
				Buffer2[6] = index;
				DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer2, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();				
				for (int ca = 0; ca < act.size(); ca++) {
					// 3 actions per record..
					Buffer[bPos] = act[ca].type < AlienFX_A_Breathing ? act[ca].type : AlienFX_A_Morph;
					Buffer[bPos + 1] = act[ca].time;
					Buffer[bPos + 3] = 0;
					Buffer[bPos + 4] = act[ca].tempo;
					Buffer[bPos + 5] = act[ca].r;
					Buffer[bPos + 6] = act[ca].g;
					Buffer[bPos + 7] = act[ca].b;
					switch (act[ca].type) {
					case AlienFX_A_Color: Buffer[bPos + 2] = 0xd0; Buffer[bPos + 4] = 0xfa; break;
					case AlienFX_A_Pulse: Buffer[bPos + 2] = 0xdc; break;
					case AlienFX_A_Morph: Buffer[bPos + 2] = 0xcf; break;
					case AlienFX_A_Breathing: Buffer[bPos + 2] = 0xdc; break;
					case AlienFX_A_Spectrum: Buffer[bPos + 2] = 0x82; break;
					case AlienFX_A_Rainbow: Buffer[bPos + 2] = 0xac; break;
					case AlienFX_A_Power: Buffer[bPos + 2] = 0xe8; break;
					default: Buffer[bPos + 2] = 0xd0; Buffer[bPos + 4] = 0xfa; Buffer[bPos] = AlienFX_A_Color;
					}
					bPos += 8;
					if (bPos == 27) {
						res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
						Loop();
						// clean buffer....
						ZeroMemory(Buffer + 3, 31);
						bPos = 3;
					}
				}
				if (bPos != 3) {
					res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
					Loop();
				}
			}
			return res;
		} break;
		case API_V2: {
			SetColor(index, act[0].r, act[0].g, act[0].b);
			// But here trick for Pulse!
			switch (act[0].type) {
			case 1: SetColor(index, act[0].r, act[0].g, act[0].b); break;
			}
		} break;
		case API_V1: {
			// can't set action for old, just use color. 
			SetColor(index, act[0].r, act[0].g, act[0].b);
		} break;
		}
		return false;
	}

	bool Functions::SetPowerAction(int index, BYTE Red, BYTE Green, BYTE Blue, BYTE Red2, BYTE Green2, BYTE Blue2, bool force)
	{
		size_t BytesWritten;
		byte Buffer[] = { 0x00, 0x03 ,0x22 ,0x00 ,0x04 ,0x00 ,0x5b ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
		, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		switch (length) {
		case API_V3: { // only supported at new devices
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
					Sleep(lastPowerCall + (ULONGLONG)POWER_DELAY - cPowerCall);
				}
				else {
#ifdef _DEBUG
					OutputDebugString(TEXT("Power update skipped!\n"));
#endif
					return false;
				}
			// Need to flush query...
			/*if (inSet)*/ UpdateColors();
			if (AlienfxGetDeviceStatus() != ALIENFX_NEW_READY) {
#ifdef _DEBUG
				if (force)
					OutputDebugString(TEXT("Forced power update - device still not ready\n"));
				else
					OutputDebugString(TEXT("Power update - device still not ready\n"));
#endif
				return false;
			}
			inSet = true;
			// Now set....
			for (BYTE cid = 0x5b; cid < 0x61; cid++) {
				// Init query...
				Buffer[6] = cid;
				Buffer[4] = 4;
				DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();
				Buffer[4] = 1;
				DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();
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
				Buffer[4] = 2;
				DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();
			}
			// Now (default) color set, if needed...
			/*Buffer[2] = 0x21; Buffer[4] = 4; Buffer[6] = 0x61;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			Buffer[4] = 1;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			// TODO: color set here...
			Buffer[4] = 2;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();*/
			Buffer[4] = 6;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			lastPowerCall = GetTickCount64();
			Reset(false);
		} break;
		case API_V2: case API_V1: {
			// can't set action for old, just use color
			SetColor(index, Red, Green, Blue);
		} break;
		}
		return 0;
	}
	int ReadStatus;

	BYTE Functions::AlienfxGetDeviceStatus()
	{
		if (pid == -1) return 0xff;
		size_t BytesWritten;
		byte ret = 0;
		byte BufferN[] = { 0x00, 0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte Buffer[] = { 0x02 ,0x06 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00 };
		switch (length) {
		case API_V3: {
			DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, BufferN, length, (DWORD*)&BytesWritten, NULL);
			ret = BufferN[2];
		} break;
		case API_V2: case API_V1: {
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);

			Buffer[0] = 0x01;
			DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, Buffer, length, (DWORD*)&BytesWritten, NULL);

			if (Buffer[0] == 0x01)
				ret = 0x06;
			else ret = Buffer[0];
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
		for (int i = 0; i < 10 && (status != ALIENFX_READY && status != ALIENFX_NEW_READY); i++)
		{
			if (status == ALIENFX_DEVICE_RESET)
				return status;
			Sleep(5);
			status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		}
		return status;
	}

	BYTE Functions::AlienfxWaitForBusy()
	{

		byte status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		for (int i = 0; i < 10 && status != ALIENFX_BUSY && status != ALIENFX_NEW_BUSY; i++)
		{
			if (status == ALIENFX_DEVICE_RESET)
				return status;
			Sleep(5);
			status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		}
		return status;
	}

	bool Functions::IsDeviceReady()
	{
		int status;
		switch (length) {
		case API_V3: {
			status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
			return status == ALIENFX_NEW_READY;
		} break;
		case API_V2: case API_V1: {
			status = AlienfxWaitForBusy();

			if (status == ALIENFX_DEVICE_RESET)
			{
				Sleep(1000);

				return false;
				//AlienfxReinit();

			}
			else if (status != ALIENFX_BUSY)
			{
				Sleep(50);
			}
			Reset(0x04);

			status = AlienfxWaitForReady();
			if (status == ALIENFX_DEVICE_RESET)
			{
				Sleep(1000);
				//AlienfxReinit();

				return false;
			}
			else if (status != ALIENFX_READY)
			{
				if (status == ALIENFX_BUSY)
				{
					Reset(0x04);

					status = AlienfxWaitForReady();
					if (status == ALIENFX_DEVICE_RESET)
					{
						Sleep(1000);
						//AlienfxReinit();
						return false;
					}
				}
				else
				{
					Sleep(50);

					return false;
				}
			}
		} break;
		}
		return true;
	}

	bool Functions::AlienFXClose()
	{
		bool result = false;
		if (devHandle != NULL)
		{
			mappings.clear();
			devices.clear();
			result = CloseHandle(devHandle);
		}
		return result;
	}

	bool Functions::AlienFXChangeDevice(int npid)
	{
		int res;
		if (pid != (-1) && devHandle != NULL)
				CloseHandle(devHandle);
		res = AlienFXInitialize(vid, npid);
		if (res != (-1)) {
			pid = npid;
			Reset(false);
			return true;
		}
		return false;
	}

	void Functions::AddMapping(int devID, int lightID, char* name, int flags) {
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

	void Functions::LoadMappings() {
		DWORD  dwDisposition;
		HKEY   hKey1;

		devices.clear();
		mappings.clear();

		RegCreateKeyEx(HKEY_CURRENT_USER,
			TEXT("SOFTWARE\\Alienfx_SDK"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,//KEY_WRITE,
			NULL,
			&hKey1,
			&dwDisposition);
		//int size = 4;

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
				int lID = 0, dID = 0;
				unsigned ret2 = sscanf_s((char*)name, "%d-%d", &dID, &lID);
				if (ret2 == 2) {
					// light name
					AddMapping(dID, lID, inarray, 0);
				}
				else {
					// device flags?
					ret2 = sscanf_s((char*)name, "Flags%d-%d", &dID, &lID);
					if (ret2 == 2) {
						AddMapping(dID, lID, NULL, *(unsigned *)inarray);
					}
					else {
						// device name?
						ret2 = sscanf_s((char*)name, "Dev#%d", &dev.devid);
						if (ret2 == 1) {
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

	void Functions::SaveMappings() {
		DWORD  dwDisposition;
		HKEY   hKey1;
		size_t numdevs = devices.size();
		size_t numlights = mappings.size();
		if (numdevs == 0) return;
		RegCreateKeyEx(HKEY_CURRENT_USER,
			TEXT("SOFTWARE"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,//KEY_WRITE,
			NULL,
			&hKey1,
			&dwDisposition);
		RegDeleteTreeA(hKey1, "Alienfx_SDK");
		RegCloseKey(hKey1);
		RegCreateKeyEx(HKEY_CURRENT_USER,
			TEXT("SOFTWARE\\Alienfx_SDK"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,//KEY_WRITE,
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
		RegCloseKey(hKey1);
	}

	std::vector<mapping>* Functions::GetMappings()
	{
		return &mappings;
	}

	int Functions::GetFlags(int devid, int lightid)
	{
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i].devid == devid && mappings[i].lightid == lightid)
				return mappings[i].flags;
		return -1;
	}

	void Functions::SetFlags(int devid, int lightid, int flags)
	{
		for (int i = 0; i < mappings.size(); i++)
			if (mappings[i].devid == devid && mappings[i].lightid == lightid) {
				mappings[i].flags = flags;
				return;
			}
	}

	std::vector<devmap>* Functions::GetDevices()
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
		case API_V4: return 4; break;
		case API_V3: return 3; break;
		case API_V2: return 2; break;
		case API_V1: return 1; break;
			default: return -1;
		}
		return length;
	}

}


