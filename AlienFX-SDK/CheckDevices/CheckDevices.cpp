
#include <stdio.h>
#include <iomanip>
#include <WTypesbase.h>
#include <string>
#include <SetupAPI.h>
#include "..\AlienFX_SDK\alienfx-controls.h"

extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

using namespace std;

//#define NUM_VIDS 4
//const static WORD vids[NUM_VIDS]{0x187c, 0x0d62, 0x0424, 0x0461};

void CheckDevices(bool show_all) {

	GUID guid;
	bool flag = false;
	HANDLE tdevHandle;

	HidD_GetHidGuid(&guid);
	HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return;
	}
	unsigned int dw = 0;
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData{0};

	while (!flag)
	{
		deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData))
		{
			flag = true;
			continue;
		}
		dw++;
		DWORD dwRequiredSize = 0;
		if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL))
		{
			continue;
		}
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			continue;
		}
		std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA_W> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA_W*)new char[dwRequiredSize]);
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
		if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL))
		{
			wstring devicePath = deviceInterfaceDetailData->DevicePath;
			tdevHandle = CreateFileW(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

			if (tdevHandle != INVALID_HANDLE_VALUE)
			{
				std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
				attributes->Size = sizeof(HIDD_ATTRIBUTES);
				if (HidD_GetAttributes(tdevHandle, attributes.get()))
				{
					for (unsigned i = 0; i < NUM_VIDS; i++) {
						if (attributes->VendorID == AlienFX_SDK::vids[i]) {

							PHIDP_PREPARSED_DATA prep_caps;
							HIDP_CAPS caps;
							HidD_GetPreparsedData(tdevHandle, &prep_caps);
							HidP_GetCaps(prep_caps, &caps);
							HidD_FreePreparsedData(prep_caps);

							string apiver;
							bool supported = true;

							switch (caps.OutputReportByteLength) {
							case 0:
							{
								switch (caps.Usage) {
								case 0xcc: apiver = "RGB keyboard, APIv5"; break;
								default: supported = false; apiver = "Unknown.";
								}
							} break;
							case 8:
								apiver =  "Notebook, APIv1";
								break;
							case 9:
								apiver =  "Notebook, APIv2";
								break;
							case 12:
								apiver = "Notebook, APIv3";
								break;supported = true;
							case 34:
								apiver = "Notebook/Desktop, APIv4";
								break;
							case 65:
								switch (i) {
								case 2:
									apiver =  "Monitor, APIv6";
									break;
								case 3:
									apiver =  "Mouse, APIv7";
									break;
								case 4:
									apiver =  "Keyboard, APIv8";
									break;
								}
								break;
							default: supported = false; apiver = "Unknown";
							}

							if (show_all || supported) {

								printf("===== Device VID_%04x, PID_%04x =====\n", attributes->VendorID, attributes->ProductID);
								printf("Version %d, block size %d\n", attributes->VersionNumber, attributes->Size);

								printf("Report Lengths: Output %d, Input %d, Feature %d\n", caps.OutputReportByteLength,
									caps.InputReportByteLength,
									caps.FeatureReportByteLength);
								printf("Usage ID %#x, Usage page %#x, Output caps %d, Index %d\n", caps.Usage, caps.UsagePage,
								    caps.NumberOutputButtonCaps, caps.NumberOutputDataIndices);

								printf("+++++ Detected as: ");

								switch (i) {
								case 0: printf("Alienware,"); break;
								case 1: printf("DARFON,"); break;
								case 2: printf("Microchip,"); break;
								case 3: printf("Primax,"); break;
								case 4: printf("Chicony,"); break;
								}

								printf(" %s +++++\n", apiver.c_str());
							}
						}
					}
				}
			}
			CloseHandle(tdevHandle);
		}
	}
}