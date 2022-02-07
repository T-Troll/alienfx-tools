// alienfx-probe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
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
	char name[256]{0}, *outName;
	printf("alienfx-probe v5.4.6\n");
	printf("Checking USB light devices...\n");
	CheckDevices(show_all);
	printf("Do you want to set devices and lights names?");
	gets_s(name, 255);
	if (name[0] == 'y' || name[0] == 'Y') {
		printf("\nFor each light please enter LightFX SDK light ID or light name if ID is not available\n\
Tested light become green, and turned off after testing.\n\
Just press Enter if no visible light at this ID to skip it.\n");
		printf("Probing low-level access... ");

		AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
		AlienFX_SDK::Functions* afx_dev = new AlienFX_SDK::Functions();
		vector<pair<WORD, WORD>> pids = afx_map->AlienFXEnumDevices();

		if (pids.size() > 0) {
			printf("Found %d device(s)\n", (int)pids.size());
			printf("Probing Dell SDK... ");
			int res = lfxUtil.InitLFX();
			if (res != -1) {
				switch (res) {
				case 0: printf("Dell library DLL not found (no library?)!\n"); break;
				case 1: printf("Can't init Dell library!\n"); break;
				case 2: printf("No high-level devices found!\n"); break;
				default: printf("Dell library unknown error!\n"); break;
				}
				// No SDK detected
				printf("No LightFX SDK detected, you should provide names yourself!\n");
			} else {
				lfxUtil.FillInfo();
				printf("Found!\n\n");
				for (unsigned cdev = 0; cdev < lfxUtil.GetNumDev(); cdev++) {
					printf("Device #%d - %s\n", cdev, lfxUtil.GetDevInfo(cdev)->desc);
					for (UINT i = 0; i < lfxUtil.GetDevInfo(cdev)->lights; i++) {
						printf("\tLight #%d - %s\n", lfxUtil.GetLightInfo(cdev, i)->id, lfxUtil.GetLightInfo(cdev, i)->desc);
					}
				}
			}

			for (int cdev = 0; cdev < pids.size(); cdev++) {
				printf("Probing device VID_%04x, PID_%04x...", pids[cdev].first, pids[cdev].second);
				int isInit = afx_dev->AlienFXChangeDevice(pids[cdev].first, pids[cdev].second);
				if (isInit != -1) {
					printf(" Connected.\n");
					int count;
					afx_dev->Reset();
					for (count = 0; count < 5 && !afx_dev->IsDeviceReady(); count++)
						Sleep(20);
					printf("Enter device name or id: ");
					//scanf_s("%255s\n", name, 255);
					gets_s(name,255);
					if (isdigit(name[0]) && res == (-1)) {
						outName = lfxUtil.GetDevInfo(atoi(name))->desc;
					} else {
						outName = name;
					}
					printf("Final name is %s\n", outName);
					AlienFX_SDK::devmap devs{pids[cdev].first,pids[cdev].second,string(outName)};
					afx_map->GetDevices()->push_back(devs);
					// How many lights to check?
					if (argc > 1 && string(argv[1]) != "-a") // we have number of lights...
						numlights = atoi(argv[1]);
					else
						numlights = pids[cdev].first == 0x0d62 ? 0x88 : 23;
					// Let's probe low-level lights....
					for (int i = 0; i < numlights; i++) {
						//int j = 0;
						printf("Testing light #%d (enter name, ENTER for skip): ", i);
						afx_dev->SetColor(i, {0, 255, 0});
						afx_dev->UpdateColors();
						Sleep(100);
						//scanf_s("%255s\n", name, 255);
						gets_s(name, 255);
						if (name[0] != 0) {
							//not skipped
							//if (isdigit(name[0]) && res == (-1)) {
							//	outName = lfxUtil.GetLightInfo(0, atoi(name))->desc;
							//}
							//else {
							outName = name;
							//}
							printf("Final name is %s, ", outName);
							// Store value...
							AlienFX_SDK::mapping* map = new AlienFX_SDK::mapping(
								{pids[cdev].first, pids[cdev].second, (WORD)i, 0, string(outName)});
							afx_map->GetMappings()->push_back(map);
							afx_map->SaveMappings();
						} else {
							printf("Skipped, ");
						}
						afx_dev->SetColor(i, {0, 0, 255});
						afx_dev->UpdateColors();
						//afx_dev->Reset();
						Sleep(100);
					}
					// now store config...
					afx_map->SaveMappings();
				} else {
					printf(" Device not initialized!\n");
				}
			}
			if (res == (-1))
				lfxUtil.Release();
			afx_dev->AlienFXClose();
		} else {
			printf("AlienFX devices not present, please check device manage!\n");
		}
		delete afx_dev;
	}
	return 0;
}

