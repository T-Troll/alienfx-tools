// alienfx-probe.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"
#include "../alienfx-cli/LFXUtil.h"
//#include "ConfigHandler.h"

using namespace std;
namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

int main()
{
	cout << "Probing low-level access... ";
	int isInit = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
	std::cout << "PID: " << std::hex << isInit << std::endl;
	if (isInit != -1)
	{
		//AlienFX_SDK::Functions::LoadMappings();
		// now check about Dell SDK...
		cout << "Probing Dell SDK... ";
		int res = lfxUtil.InitLFX();
		if (res != -1) {
			switch (res) {
			case 0: cerr << "Can't load DLL library!" << endl; break;
			case 1: cerr << "Can't init library!" << endl; break;
			case 2: cerr << "No devices found!" << endl; break;
			default: cerr << "Unknown error!" << endl; break;
			}
			// No SDK detected
			cout << "No LightFX SDK detected, you should provide names yourself!" << endl;
		}
		else {
			lfxUtil.FillInfo();
			cout << "Lights found for " << lfxUtil.GetDevInfo(0)->desc << ":" << endl;
			for (UINT i = 0; i < lfxUtil.GetDevInfo(0)->lights; i++) {
				cout << "Light #" << lfxUtil.GetLightInfo(0, i)->id
					<< " - " << lfxUtil.GetLightInfo(0, i)->desc << endl;
			}
		}
		bool result = AlienFX_SDK::Functions::Reset(false);
		if (!result) {
			std::cout << "Reset faled with " << std::hex << GetLastError() << std::endl;
			return 1;
		}
		result = AlienFX_SDK::Functions::IsDeviceReady();
		// Let's probe low-level lights....
		cout << "For each light please enter LightFX SDK light ID or light name if ID is not available" << endl
		     << "Tested light become green, and change color to blue after testing." << endl 
			 << "Just press Enter if no visible light at this ID to skip it." << endl;
		for (int i = 0; i < 16; i++) {
			//int j = 0;
			cout << "Testing light #" << i;
			/*for (j = 0; j < AlienFX_SDK::Functions::GetMappings()->size() &&
				(AlienFX_SDK::Functions::GetMappings()->at(j).devid != isInit ||
				AlienFX_SDK::Functions::GetMappings()->at(j).lightid != i); j++);
			if (j != AlienFX_SDK::Functions::GetMappings()->size())
				cout << " (Old value - " << AlienFX_SDK::Functions::GetMappings()->at(j).name << ")";*/
			cout << ": ";
			char name[256], *outName;
			AlienFX_SDK::Functions::SetColor(i, 0, 255, 0);
			AlienFX_SDK::Functions::UpdateColors();
			Sleep(100);
			std::cin.getline(name, 255);
			if (name[0] != 0) {
				//not skipped
				//cout << "Ok, " << name << endl;
				if (isdigit(name[0]) && res == (-1)) {
					outName = lfxUtil.GetLightInfo(0, atoi(name))->desc;
				}
				else {
					outName = name;
				}
				cout << "Final name is " << outName << ", ";
				// Store value...
				AlienFX_SDK::mapping map;
				map.devid = isInit;
				map.lightid = i;
				map.name = std::string(outName);
				AlienFX_SDK::Functions::GetMappings()->push_back(map);
			}
			else {
				//outName = name;
				cout << "Skipped. ";
			}
			AlienFX_SDK::Functions::SetColor(i, 0, 0, 255);
			AlienFX_SDK::Functions::UpdateColors();
			Sleep(100);
		}
		lfxUtil.Release();
		// now store config...

		AlienFX_SDK::Functions::SaveMappings();
	}
	else {
		cerr << "AlienFX device not found, please check device manager";
	}
	AlienFX_SDK::Functions::AlienFXClose();
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
