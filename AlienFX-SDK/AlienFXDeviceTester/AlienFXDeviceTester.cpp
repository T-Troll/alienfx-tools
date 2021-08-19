// AlienFXDeviceTester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iomanip>
#include <wtypes.h>
#include <WTypesbase.h>
#include <SetupAPI.h>
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

using namespace std;

int main()
{
	GUID guid;
	bool flag = false;
	HANDLE tdevHandle;
	//This is VID for all alienware laptops, use this while initializing, it might be different for external AW device like mouse/kb
	const static DWORD vids[2] = {0x187c, 0x0d62};

	HidD_GetHidGuid(&guid);
	HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return 0;
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
					for (unsigned i = 0; i < sizeof(vids)/sizeof(DWORD); i++) {
						if (attributes->VendorID == vids[i]) {

							PHIDP_PREPARSED_DATA prep_caps;
							HIDP_CAPS caps;
							HidD_GetPreparsedData(tdevHandle, &prep_caps);
							HidP_GetCaps(prep_caps, &caps);
							HidD_FreePreparsedData(prep_caps);

							cout.fill('0');
							cout << hex << "===== New device VID" << setw(4) << attributes->VendorID << ", PID" << setw(4) << attributes->ProductID << " =====" << endl;

							cout << dec << "Version " << attributes->VersionNumber << ", Blocksize " << attributes->Size << endl;

							cout << dec << "Output Report Length " << caps.OutputReportByteLength
								<< ", Input Report Length " << caps.InputReportByteLength
								<< ", Feature Report Length " << caps.FeatureReportByteLength
								<< endl;
							cout << hex << "Usage ID " << caps.Usage << ", Usage Page " << caps.UsagePage << endl;
							cout << dec << "Output caps " << caps.NumberOutputButtonCaps << ", Index " << caps.NumberOutputDataIndices << endl;

							//if (caps.OutputReportByteLength || caps.Usage == 0xcc) {
								cout << "+++++ Detected as: ";

								switch (i) {
								case 0: cout << "Alienware, "; break;
								case 1: cout << "DARFON, "; break;
								}

								switch (caps.OutputReportByteLength) {
								case 0:
								{
									switch (caps.Usage) {
									case 0xcc: cout << "RGB, APIv5"; break;
									default: cout << "Unknown.";
									}
								} break;
								case 9:
									if (attributes->VersionNumber > 511)
										cout << "APIv2";
									else
										cout << "APIv1";
									break;
								case 12:
									cout << "APIv3";
									break;
								case 34:
									cout << "APIv4";
									break;
								default: "Unknown.";
								}
								cout << " +++++" << endl;
							//}
						}
					}
				}
			}
			CloseHandle(tdevHandle);
		}
	}
}
