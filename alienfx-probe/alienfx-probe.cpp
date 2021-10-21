// alienfx-probe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iomanip>
#include "AlienFX_SDK.h"
#include "LFXUtil.h"
#include <SetupAPI.h>

extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

using namespace std;
namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

void CheckDevices(bool show_all) {
	GUID guid;
	bool flag = false;
	HANDLE tdevHandle;
	//This is VID for all alienware laptops, use this while initializing, it might be different for external AW device like mouse/kb
	const static DWORD vids[2] = {0x187c, 0x0d62};

	HidD_GetHidGuid(&guid);
	HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return;
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

							string apiver;
							bool supported = false;

							switch (caps.OutputReportByteLength) {
							case 0:
							{
								switch (caps.Usage) {
								case 0xcc: supported = true; apiver = "RGB, APIv5"; break;
								default: apiver = "Unknown.";
								}
							} break;
							case 8:
								supported = true; apiver =  "APIv1";
								break;
							case 9:
								if (attributes->VersionNumber > 511)
									apiver =  "APIv2";
								else
									apiver =  "APIv1";
								supported = true; 
								break;
							case 12:
								supported = true; apiver = "APIv3";
								break;
							case 34:
								supported = true; apiver = "APIv4";
								break;
							default: apiver = "Unknown";
							}

							if (show_all || supported) {

								cout.fill('0');
								cout << hex << "===== New device VID" << setw(4) << attributes->VendorID << ", PID" << setw(4) << attributes->ProductID << " =====" << endl;

								cout << dec << "Version " << attributes->VersionNumber << ", Blocksize " << attributes->Size << endl;

								cout << dec << "Output Report Length " << caps.OutputReportByteLength
									<< ", Input Report Length " << caps.InputReportByteLength
									<< ", Feature Report Length " << caps.FeatureReportByteLength
									<< endl;
								cout << hex << "Usage ID " << caps.Usage << ", Usage Page " << caps.UsagePage << endl;
								cout << dec << "Output caps " << caps.NumberOutputButtonCaps << ", Index " << caps.NumberOutputDataIndices << endl;

								cout << "+++++ Detected as: ";

								switch (i) {
								case 0: cout << "Alienware, "; break;
								case 1: cout << "DARFON, "; break;
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

int main(int argc, char* argv[])
{
	int numlights = 23;
	bool show_all = argc > 1 && string(argv[1]) == "-a";
	cout << "alienfx-probe v5.0.6" << endl;
	cout << "Checking USB light devices..." << endl;
	CheckDevices(show_all);
	cout << "Do you want to set devices and lights names?";
	char answer;
	cin >> answer;
	if (answer == 'y' || answer == 'Y') {
		cout << endl << "For each light please enter LightFX SDK light ID or light name if ID is not available" << endl
			<< "Tested light become green, and turned off after testing." << endl
			<< "Just press Enter if no visible light at this ID to skip it." << endl;
		cout << "Probing low-level access... ";
		vector<pair<DWORD, DWORD>> pids;
		AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
		AlienFX_SDK::Functions* afx_dev = new AlienFX_SDK::Functions();
		//afx_dev->LoadMappings();
		pids = afx_map->AlienFXEnumDevices();
		if (pids.size() > 0) {
			cout << "Found " << pids.size() << " device(s)" << endl;
			cout << "Probing Dell SDK... ";
			int res = lfxUtil.InitLFX();
			if (res != -1) {
				switch (res) {
				case 0: cerr << "Dell library DLL not found (no library?)!" << endl; break;
				case 1: cerr << "Can't init Dell library!" << endl; break;
				case 2: cerr << "No high-level devices found!" << endl; break;
				default: cerr << "Dell library unknown error!" << endl; break;
				}
				// No SDK detected
				cout << "No LightFX SDK detected, you should provide names yourself!" << endl;
			} else {
				lfxUtil.FillInfo();
				cout << "Found!" << endl << endl;
				for (unsigned cdev = 0; cdev < lfxUtil.GetNumDev(); cdev++) {
					cout << "Device #" << cdev << " (" << lfxUtil.GetDevInfo(cdev)->desc << "):" << endl;
					for (UINT i = 0; i < lfxUtil.GetDevInfo(cdev)->lights; i++) {
						cout << "\tLight #" << lfxUtil.GetLightInfo(cdev, i)->id
							<< " - " << lfxUtil.GetLightInfo(cdev, i)->desc << endl;
					}
				}
			}

			for (int cdev = 0; cdev < pids.size(); cdev++) {
				cout << "Probing device VID 0x" << std::hex << pids[cdev].first << ", PID 0x" << std::hex << pids[cdev].second;
				int isInit = afx_dev->AlienFXChangeDevice(pids[cdev].first, pids[cdev].second);
				if (isInit != -1) {
					cout << " Connected." << endl;
					char name[256], * outName;
					int count;
					afx_dev->Reset();
					for (count = 0; count < 5 && !afx_dev->IsDeviceReady(); count++)
						Sleep(20);
					cout << "Enter device name or id: ";
					std::cin.getline(name, 255);
					if (isdigit(name[0]) && res == (-1)) {
						outName = lfxUtil.GetDevInfo(atoi(name))->desc;
					} else {
						outName = name;
					}
					cout << "Final name is " << outName << endl;
					AlienFX_SDK::devmap devs;
					devs.devid = pids[cdev].second;
					devs.name = outName;
					afx_map->GetDevices()->push_back(devs);
					// How many lights to check?
					if (argc > 1) // we have number of lights...
						numlights = atoi(argv[1]);
					else
						if (pids[cdev].first == AlienFX_SDK::vids[1])
							// RGB keyboard;
							numlights = 0x88;
						else
							numlights = 23;
					// Let's probe low-level lights....
					for (int i = 0; i < numlights; i++) {
						//int j = 0;
						cout << "Testing light #" << i << "(enter name, ENTER for skip): ";
						afx_dev->SetColor(i, 0, 255, 0);
						afx_dev->UpdateColors();
						Sleep(100);
						std::cin.getline(name, 255);
						if (name[0] != 0) {
							//not skipped
							//if (isdigit(name[0]) && res == (-1)) {
							//	outName = lfxUtil.GetLightInfo(0, atoi(name))->desc;
							//}
							//else {
							outName = name;
							//}
							cout << "Final name is " << outName << ", ";
							// Store value...
							AlienFX_SDK::mapping* map = new AlienFX_SDK::mapping(
								{pids[cdev].first, pids[cdev].second, (DWORD)i, 0, string(outName)});
							afx_map->GetMappings()->push_back(map);
							afx_map->SaveMappings();
						} else {
							cout << "Skipped. ";
						}
						afx_dev->SetColor(i, 0, 0, 0);
						afx_dev->UpdateColors();
						//afx_dev->Reset();
						Sleep(100);
					}
					// now store config...
					afx_map->SaveMappings();
				} else {
					cerr << " Device didn't answer!" << endl;
				}
			}
			if (res == (-1))
				lfxUtil.Release();
			afx_dev->AlienFXClose();
		} else {
			cout << "AlienFX devices not present, please check device manage!" << endl;
		}
		delete afx_dev;
	}
	return 0;
}

