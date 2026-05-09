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
	// Now detect active devices...
	afx_map.AlienFXEnumDevices();
	cout << afx_map.fxdevs.size() << " device(s) found." << endl;
	// And try to set it lights...
	for (int i = 0; i < afx_map.fxdevs.size(); i++) {
		cout << hex << "VID: 0x" << afx_map.fxdevs[i].vid << ", PID: 0x" << afx_map.fxdevs[i].pid << ", " << 
			(afx_map.fxdevs[i].present ? 
				"API v" + to_string(afx_map.fxdevs[i].dev->version) : "Inactive") << ", " << afx_map.fxdevs[i].lights.size() << " lights" << endl;
		if (afx_map.fxdevs[i].present) {
			cout << "Now trying light 2 to blue... ";
			afx_map.fxdevs[i].dev->SetColor(2, { 255 });
			afx_map.fxdevs[i].dev->UpdateColors();
			cin.get();
			cout << "Now trying light 3 to mixed... ";
			afx_map.fxdevs[i].dev->SetColor(3, { 255, 255 });
			afx_map.fxdevs[i].dev->UpdateColors();
			cin.get();
		}
	}
	return 0;
}

