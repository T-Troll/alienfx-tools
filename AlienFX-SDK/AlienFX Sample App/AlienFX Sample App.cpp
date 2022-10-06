// This define disable ACPI-based lights (Aurora R6/R7). You will need to use AlienFan_SDK to control it.
#define NOACPILIGHTS
#include "AlienFX_SDK.h"
#include <iostream>

using namespace std;

int main()
{
	AlienFX_SDK::Mappings afx_map;
	// Loading stored devices and maps (optional, in case you need to know which lights every device has)
	afx_map.LoadMappings();
	for (auto it = afx_map.fxdevs.begin(); it != afx_map.fxdevs.end(); it++)
		cout << "Stored device " << it->name << ", " << it->lights.size() << " lights\n";
	// Now detect active devices...
	vector<AlienFX_SDK::Functions*> devs = afx_map.AlienFXEnumDevices();
	cout << devs.size() << " device(s) detected." << endl;
	// And try to set it lights...
	for (int i = 0; i < devs.size(); i++) {
		cout << hex << "VID: 0x" << devs[i]->GetVID() << ", PID: 0x" << devs[i]->GetPID() << ", API v" << devs[i]->GetVersion() << endl;
		cout << "Now trying light 2 to blue... ";
		devs[i]->SetColor(2, { 255 });
		devs[i]->UpdateColors();
		cin.get();
		cout << "Now trying light 3 to mixed... ";
		devs[i]->SetColor(3, { 255, 255 });
		devs[i]->UpdateColors();
		cin.get();
		delete devs[i];
	}
	return 0;
}

