
#include <iostream>
#include <iomanip>
#include <WTypesbase.h>
#include <string>
#include <SetupAPI.h>
#include "alienfx-controls.h"

extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

using namespace std;

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
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {0};

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
		std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new char[dwRequiredSize]);
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL))
		{
			std::wstring devicePath = deviceInterfaceDetailData->DevicePath;
			tdevHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

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
								case 0xcc: apiver = "RGB, APIv5"; break;
								default: supported = false; apiver = "Unknown.";
								}
							} break;
							case 8:
								apiver =  "APIv1";
								break;
							case 9:
								apiver =  "APIv2";
								break;
							case 12:
								apiver = "APIv3";
								break;supported = true;
							case 34:
								apiver = "APIv4";
								break;
							case 65:
								switch (i) {
								case 2:
									apiver =  "APIv6";
									break;
								case 3:
									apiver =  "APIv7";
									break;
								}
								break;
							default: supported = false; apiver = "Unknown";
							}

							if (show_all || supported) {

								cout.fill('0');
								cout << hex << "===== New device VID_" << setw(4) << attributes->VendorID << ", PID_" << setw(4) << attributes->ProductID << " =====" << endl;

								cout << dec << "Version " << attributes->VersionNumber << ", Blocksize " << attributes->Size << endl;

								cout << dec << "Report Lengths: Output " << caps.OutputReportByteLength
									<< ", Input " << caps.InputReportByteLength
									<< ", Feature " << caps.FeatureReportByteLength
									<< endl;
								cout << hex << "Usage ID " << caps.Usage << ", Usage Page " << caps.UsagePage;
								cout << dec << ", Output caps " << caps.NumberOutputButtonCaps << ", Index " << caps.NumberOutputDataIndices << endl;

								cout << "+++++ Detected as: ";

								switch (i) {
								case 0: cout << "Alienware, "; break;
								case 1: cout << "DARFON, "; break;
								case 2: cout << "Microchip, "; break;
								case 3: cout << "Primax, "; break;
								}

								cout << apiver << " +++++" << endl;
							}
						}
					}
				}
			}
			CloseHandle(tdevHandle);
		}
	}
}