// alienfx-probe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "AlienFX_SDK.h"
#include "LFXUtil.h"

void CheckDevices(bool);

using namespace std;
namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

int main(int argc, char* argv[])
{
	int numlights = 23;
	bool show_all = argc > 1 && string(argv[1]) == "-a";
	cout << "alienfx-probe v5.3.2" << endl;
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

		AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
		AlienFX_SDK::Functions* afx_dev = new AlienFX_SDK::Functions();
		vector<pair<WORD, WORD>> pids = afx_map->AlienFXEnumDevices();

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
					if (argc > 1 && string(argv[1]) != "-a") // we have number of lights...
						numlights = atoi(argv[1]);
					else
						numlights = pids[cdev].first == 0x0d62 ? 0x88 : 23;
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
								{pids[cdev].first, pids[cdev].second, (WORD)i, 0, string(outName)});
							afx_map->GetMappings()->push_back(map);
							afx_map->SaveMappings();
						} else {
							cout << "Skipped. ";
						}
						afx_dev->SetColor(i, 0, 0, 255);
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

