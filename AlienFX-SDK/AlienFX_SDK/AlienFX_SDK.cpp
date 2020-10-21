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
#define ALIENFX_UNKOWN_COMMAND 0x12
// new statuses for m15 - 33 = ok, 36 = wait for update, 35 = wait for color, 34 - busy processing power update
#define ALIENFX_NEW_READY 33
#define ALIENFX_NEW_BUSY 34
#define ALIENFX_NEW_WAITCOLOR 35
#define ALIENFX_NEW_WAITUPDATE 36

namespace AlienFX_SDK
{
	bool isInitialized = false;
	HANDLE devHandle;
	int length = 9;
	bool inSet = false;
	ULONGLONG lastPowerCall = 0;

	// Name mappings for lights
	static std::vector <mapping> mappings;
	static std::vector <devmap> devices;

	static int pid = -1;
	static int version = -1;

	std::vector<int> Functions::AlienFXEnumDevices(int vid)
	{
		std::vector<int> pids;
		GUID guid;
		bool flag = false;

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
				devHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				if (devHandle != INVALID_HANDLE_VALUE)
				{
					std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
					attributes->Size = sizeof(HIDD_ATTRIBUTES);
					if (HidD_GetAttributes(devHandle, attributes.get()))
					{

						if (attributes->VendorID == vid)
						{
							pids.push_back(attributes->ProductID);
						}
					}
				}
				CloseHandle(devHandle);
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
							// I use it to detect is it old device or new, i have version = 0 for old, and version = 512 for new
							if (attributes->VersionNumber > 511)
								length = 34;
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
	bool Functions::AlienFXInitialize(int vid, int pidd)
	{
		GUID guid;
		bool flag = false;
		pid = pidd;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE)
		{
			//std::cout << "Couldn't get guid";
			return false;
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
				return false;
			}
			dw++;
			DWORD dwRequiredSize = 0;
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL))
			{
				//std::cout << "Getting the needed buffer size failed";
				return false;
			}
			//std::cout << "Required size is " << dwRequiredSize << std::endl;
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			{
				//std::cout << "Last error is not ERROR_INSUFFICIENT_BUFFER";
				return false;
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

						if (((attributes->VendorID == vid) && (attributes->ProductID == pid)))
						{
							// BUGFIX - length was not filled in this procedure
							// I use it to detect is it old device or new, i have version = 0 for old, and version = 512 for new
							if (attributes->VersionNumber > 511)
								length = 34;
							else
								length = attributes->Size;
							version = attributes->VersionNumber;
							flag = true;
						}
					}

				}
			}
		}
		//OutputDebugString(flag);
		return flag;
	}

	void Loop()
	{
		size_t BytesWritten;
		byte BufferN[] = { 0x00, 0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x02 ,0x04 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,0x00 ,0x00 ,0x00 };
		if (length == 34) {
			// m15 require Input report as a confirmation, not output. 
			DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, BufferN, length, (DWORD*)&BytesWritten, NULL);
			// std::cout << "Status: 0x" << std::hex << (int) BufferN[2] << std::endl;
		}
		else {
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		}
	}

	bool Functions::Reset(int status)
	{
		size_t BytesWritten;
		bool result;
		byte* Buffer = NULL;
		// m15/m17 use 34 bytes (ID (always 0) + 33 bytes payload) report.
		byte BufferN[] = { 0x00, 0x03 ,0x21 ,0x00 ,0x01 ,0xff ,0xff ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x02 ,0x07 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00 };

		if (length == 34) {
			inSet = true;
			result = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferN, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
		}
		else {
			if (status)
				BufferO[2] = 0x04;
			else
				BufferO[2] = 0x03;
			result = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		}
		//std::cout << "Reset!" << std::endl;
		return result;
	}

	bool Functions::UpdateColors()
	{
		size_t BytesWritten;
		bool res = false;
		// As well, 34 byte report for m15 - first byte ID, then command
		byte BufferN[] = { 0x00, 0x03 ,0x21 ,0x00 ,0x03 ,0x00 ,0xff ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x02 ,0x05 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,0x00 ,0x00 ,0x00 };
		if (length == 34) {
			if (inSet) {
				res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferN, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();
				inSet = false;
			}
		}
		else
			res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, BufferO, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		//std::cout << "Update!" << std::endl;
		return res;
	}

	int Power = 13;
	int Macro = 0;
	int leftZone = 0x8;
	int leftMiddleZone = 0x4;
	int rightZone = 0x1;
	int rightMiddleZone = 0x2;
	int AlienFrontLogo = 0x40;
	int AlienBackLogo = 0x20;
	int LeftPanelTop = 0x1000;
	int LeftPanelBottom = 0x400;
	int RightPanelTop = 0x2000;
	int RightPanelBottom = 0x800;
	int TouchPad = 0x80;


	bool Functions::SetColor(int index, int r, int g, int b)
	{
		int location;
		switch (index)
		{
		case 1: location = leftZone; break;
		case 2: location = leftMiddleZone; break;
		case 3: location = rightZone; break;
		case 4: location = rightMiddleZone; break;
		case 5: location = Macro; break;
		case 6: location = AlienFrontLogo; break;
		case 7: location = LeftPanelTop; break;
		case 8: location = LeftPanelBottom; break;
		case 9: location = RightPanelTop; break;
		case 10: location = RightPanelBottom; break;

		case 12: location = AlienBackLogo; break;
		case 11: location = Power; break;
		case 13: location = TouchPad; break;
		default: location = AlienFrontLogo; break;
		}
		size_t BytesWritten;
		byte* Buffer;
		// Buffer[8,9,10] = rgb
		byte BufferN[] = { 0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte BufferO[] = { 0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00 ,0x00 ,0x00 };
		/// But we need to issue 2 commands - light_select and color_set.... this for light_select
		/// Buffer2[5] - Count of lights need to be set
		/// Buffer2[6-33] - LightID (index, not mask) - it can be COUNT of them.
		byte Buffer2[] = { 0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		if (length == 34) {
			Buffer = BufferN;
			Buffer2[6] = index;
			Buffer[8] = r;
			Buffer[9] = g;
			Buffer[10] = b;
			if (!inSet) Reset(false);
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer2, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
		}
		else {
			Buffer = BufferO;
			Buffer[0] = 0x02;
			Buffer[1] = 0x03;
			Buffer[2] = index;
			Buffer[3] = (location & 0xFF0000) >> 16;
			Buffer[4] = (location & 0x00FF00) >> 8;
			Buffer[5] = (location & 0x0000FF);
			Buffer[6] = r;
			Buffer[7] = g;
			Buffer[8] = b;

			if (index == 5)
			{
				Buffer[1] = 0x83;
				Buffer[2] = (byte)index;
				Buffer[3] = 00;
				Buffer[4] = 00;
				Buffer[5] = 00;
			}

			if (index == 11)
			{
				Buffer[1] = 0x01;
				Buffer[2] = (byte)index;
				Buffer[3] = 00;
				Buffer[4] = 01;
				Buffer[5] = 00;
			}
		}
		bool val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		Loop();
		return val;
	}

	bool Functions::SetMultiColor(int numLights, UCHAR* lights, int r, int g, int b)
	{
		size_t BytesWritten;
		byte Buffer[] = { 0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0xfa ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		/// But we need to issue 2 commands - light_select and color_set.... this for light_select
		/// Buffer2[5] - Count of lights need to be set
		/// Buffer2[6-33] - LightID (index, not mask) - it can be COUNT of them.
		byte Buffer2[] = { 0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		if (length == 34) {
			Buffer2[5] = numLights;
			for (int nc = 0; nc < numLights; nc++)
				Buffer2[6 + nc] = lights[nc];
			Buffer[8] = r;
			Buffer[9] = g;
			Buffer[10] = b;
			if (!inSet) Reset(false);
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer2, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
		}
		else {
			bool ret = false;
			for (int nc = 0; nc < numLights; nc++)
				ret = SetColor(lights[nc], r, g, b);
			return ret;
		}
		bool val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
		Loop();
		return val;
	}

	bool Functions::SetAction(int index, int action, int time, int tempo, int Red, int Green, int Blue, int action2, int time2, int tempo2, int Red2, int Green2, int Blue2)
	{
		size_t BytesWritten;
		// Buffer[3], [11] - action type ( 0 - light, 1 - pulse, 2 - morph)
		// Buffer[4], [12] - how long phase keeps
		// Buffer[5], [13] - mode (action type) - 0xd0 - light, 0xdc - pulse, 0xcf - morph, 0xe8 - power morph
		// Buffer[7], [15] - tempo (0xfa - steady)
		byte Buffer[] = { 0x00, 0x03 ,0x24 ,0x00 ,0x07 ,0xd0 ,0x00 ,0x32 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x64 , 0x00
		, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte Buffer2[] = { 0x00, 0x03 ,0x23 ,0x01 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
				, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		if (length == 34) { // only supported at new devices
			Buffer[3] = action;
			Buffer[4] = time;
			Buffer[7] = tempo;
			Buffer[11] = action2;
			Buffer[12] = time2;
			Buffer[15] = tempo2;
			Buffer[8] = Red;
			Buffer[9] = Green;
			Buffer[10] = Blue;
			Buffer[16] = Red2;
			Buffer[17] = Green2;
			Buffer[18] = Blue2;
			Buffer2[6] = index;

			switch (action) {
			case 0: Buffer[5] = 0xd0; Buffer[7] = 0xfa; break;
			case 1: Buffer[5] = 0xdc; break;
			case 2: Buffer[5] = 0xcf; break;
			case 3: Buffer[5] = 0xe8; Buffer[3] = AlienFX_A_Morph; break;
			}
			switch (action2) {
			case 0: Buffer[13] = 0xd0; Buffer[11] = 0; Buffer[15] = 0xfa; break;
			case 1: Buffer[13] = 0xdc; break;
			case 2: Buffer[13] = 0xcf; break;
			case 3: Buffer[13] = 0xe8; Buffer[11] = AlienFX_A_Morph; break;
			case 4: // No action
				Buffer[11] = Buffer[12] = Buffer[13] = Buffer[14] = Buffer[15] = 0;
				break;
			}
			if (!inSet) Reset(false);
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer2, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			int res = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			return res;
		}
		else {
			// can't set action for old, just use color
			SetColor(index, Red, Green, Blue);
		}
		return false;
	}

	byte AlienfxWaitForReady();

	bool Functions::SetPowerAction(int index, int Red, int Green, int Blue, int Red2, int Green2, int Blue2)
	{
		size_t BytesWritten;
		byte Buffer[] = { 0x00, 0x03 ,0x22 ,0x00 ,0x04 ,0x00 ,0x5b ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
		, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		if (length == 34) { // only supported at new devices
			//OutputDebugString(L"Power button set initiated.\n");
			ULONGLONG cPowerCall = GetTickCount64();
			if (cPowerCall - lastPowerCall < 260)
				//Sleep(lastPowerCall + 260 - cPowerCall);
				return false;
			// Need to flush query...
			if (inSet) UpdateColors();
			if (AlienfxGetDeviceStatus() != ALIENFX_NEW_READY) {
				//OutputDebugString(L"Power set skipped.");
				return false;
			}
			// this function can be called not early then 250ms after last call!
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
				switch (cid) {
				case 0x5b: // Alarm
					SetAction(index, AlienFX_A_Power, 3, 0x64, Red, Green, Blue, AlienFX_A_Power, 3, 0x64, 0, 0, 0);
					break;
				case 0x5c: // AC power
					SetAction(index, AlienFX_A_Color, 0, 0, Red, Green, Blue);
					break;
				case 0x5d: // Charge
					SetAction(index, AlienFX_A_Power, 3, 0x64, Red, Green, Blue, AlienFX_A_Power, 3, 0x64, Red2, Green2, Blue2);
					break;
				case 0x5e: // Low batt
					SetAction(index, AlienFX_A_Power, 3, 0x64, Red2, Green2, Blue2, AlienFX_A_Power, 3, 0x64, 0, 0, 0);
					break;
				case 0x5f: // Batt power
					SetAction(index, AlienFX_A_Color, 0, 0, Red2, Green2, Blue2);
					break;
				case 0x60: // Unknown - No battery?
					SetAction(index, AlienFX_A_Color, 0, 0, Red2, Green2, Blue2);
					break;
				}
				// And finish
				Buffer[4] = 2;
				DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
				Loop();
				//AlienfxGetDeviceStatus();
			}
			// Now color set...
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
			Buffer[4] = 6; //Buffer[6] = 0x60;
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);
			Loop();
			lastPowerCall = GetTickCount64();
			//Sleep(250);
			Reset(false);
			//UpdateColors();
			//inSet = false;
		}
		else {
			// can't set action for old, just use color
			SetColor(index, Red, Green, Blue);
		}
		return 0;
	}
	int ReadStatus;

	BYTE Functions::AlienfxGetDeviceStatus()
	{
		size_t BytesWritten;
		byte ret = 0;
		byte BufferN[] = { 0x00, 0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 0x00, 0x00, 0x00, 0x00 , 0x00 , 0x00 , 0x00
			, 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00, 0x00, 0x00 };
		byte Buffer[] = { 0x02 ,0x06 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 };

		if (length == 34) {
			DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, 0, 0, BufferN, length, (DWORD*)&BytesWritten, NULL);
			ret = BufferN[2];
			//std::cout << "Status: " << std::hex << ret << std::endl;
		}
		else {
			DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*)&BytesWritten, NULL);

			Buffer[0] = 0x01;
			DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, Buffer, length, (DWORD*)&BytesWritten, NULL);

			if (Buffer[0] == 0x01)
				ret = 0x06;
			else ret = Buffer[0];
		}
#ifdef _DEBUG
		wchar_t buff[2048];
		if (ret == 0) {
			swprintf_s(buff, 2047, L"Status: %d\n", ret);
			OutputDebugString(buff);
		}
#endif
		return ret;
	}

	byte AlienfxWaitForReady()
	{
		byte status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		for (int i = 0; i < 5 && (status != ALIENFX_READY && status != ALIENFX_NEW_READY); i++)
		{
			if (status == ALIENFX_DEVICE_RESET)
				return status;
			Sleep(2);
			status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		}
		return status;
	}

	byte AlienfxWaitForBusy()
	{

		byte status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		for (int i = 0; i < 5 && status != ALIENFX_BUSY && status != ALIENFX_NEW_BUSY; i++)
		{
			if (status == ALIENFX_DEVICE_RESET)
				return status;
			Sleep(2);
			status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
		}
		return status;
	}

	bool Functions::IsDeviceReady()
	{
		int status;
		if (length == 34) {
			status = AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
			return status == ALIENFX_NEW_READY;
		} else {
			status = AlienfxWaitForBusy();

			if (status == ALIENFX_DEVICE_RESET)
			{
				Sleep(1000);

				return false;
				//AlienfxReinit();

			}
			else if (status != ALIENFX_BUSY && status != ALIENFX_NEW_WAITUPDATE)
			{
				Sleep(50);
			}
			Reset(0x04);

			status = AlienfxWaitForReady();
			if (status == 0x06)
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
		}
		return true;
	}

	bool Functions::AlienFXClose()
	{
		bool result = false;
		mappings.clear();
		if (devHandle != NULL)
		{
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
			pid = res;
			Reset(false);
			return true;
		}
		return false;
	}

	void AddMapping(int devID, int lightID, char* name, int flags) {
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
		int size = 4;

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
			sprintf_s((char*)name, 255, "Dev#%d", devices[i].devid);

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
		return version;
	}

	int GetError()
	{
		return GetLastError();
	}
}


