// alienfx-probe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"
#include "../alienfx-cli/LFXUtil.h"

using namespace std;
namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

int main(int argc, char* argv[])
{
	int numlights = 8;
	cout << "alienfx-probe v1.1.7" << endl;
	cout << "For each light please enter LightFX SDK light ID or light name if ID is not available" << endl
		<< "Tested light become green, and turned off after testing." << endl
		<< "Just press Enter if no visible light at this ID to skip it." << endl; 
	cout << "Probing low-level access... ";
	vector<int> pids;
	AlienFX_SDK::Functions* afx_dev = new AlienFX_SDK::Functions();
	//afx_dev->LoadMappings();
	pids = afx_dev->AlienFXEnumDevices(afx_dev->vid);
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
		}
		else {
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
			cout << "Probing device PID 0x..." << std::hex << pids[cdev];
			int isInit = afx_dev->AlienFXChangeDevice(pids[cdev]);
			if (isInit != -1)
			{
				cout << " Connected." << endl;
				char name[256], *outName;
				int count;
				for (count = 0; count < 5 && !afx_dev->IsDeviceReady(); count++)
					Sleep(20);
				afx_dev->Reset(false);
				cout << "Enter device name or id: ";
				std::cin.getline(name, 255);
				if (isdigit(name[0]) && res == (-1)) {
					outName = lfxUtil.GetDevInfo(atoi(name))->desc;
				}
				else {
					outName = name;
				}
				cout << "Final name is " << outName << endl;
				AlienFX_SDK::devmap devs;
				devs.devid = pids[cdev];
				devs.name = outName;
				afx_dev->GetDevices()->push_back(devs);
				// How many lights to check?
				if (argc > 1) // we have number of lights...
					numlights = atoi(argv[1]);
				// Let's probe low-level lights....
				for (int i = 0; i < numlights; i++) {
					//int j = 0;
					cout << "Testing light #" << i << "(enter name or ID, ENTER for skip): ";
					afx_dev->SetColor(i, 0, 255, 0);
					afx_dev->UpdateColors();
					Sleep(100);
					std::cin.getline(name, 255);
					if (name[0] != 0) {
						//not skipped
						if (isdigit(name[0]) && res == (-1)) {
							outName = lfxUtil.GetLightInfo(0, atoi(name))->desc;
						}
						else {
							outName = name;
						}
						cout << "Final name is " << outName << ", ";
						// Store value...
						AlienFX_SDK::mapping map;
						map.devid = pids[cdev];
						map.lightid = i;
						map.name = std::string(outName);
						afx_dev->GetMappings()->push_back(map);
						afx_dev->SaveMappings();
					}
					else {
						cout << "Skipped. ";
					}
					afx_dev->SetColor(i, 0, 0, 0);
					afx_dev->UpdateColors();
					afx_dev->Reset(false);
					Sleep(100);
				}
				// now store config...
				afx_dev->SaveMappings();
			}
			else {
				cerr << " Device didn't answer!" << endl;
			}
		}
		if (res == (-1))
			lfxUtil.Release();
		afx_dev->AlienFXClose();
	}
	else {
		cout << "AlienFX devices not present, please check device manage!" << endl;
	}
	delete afx_dev;
	return 0;
}

