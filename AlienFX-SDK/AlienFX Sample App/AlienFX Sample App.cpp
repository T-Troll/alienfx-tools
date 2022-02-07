// AlienFX Sample App.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include "AlienFX_SDK.h"
#include <iostream>

using namespace std;

int main()
{
	AlienFX_SDK::Functions afx_dev;
	AlienFX_SDK::Mappings afx_map;
	int res;
	vector<pair<WORD,WORD>> devs = afx_map.AlienFXEnumDevices();
	cout << devs.size() << " device(s) detected." << endl;
	//for (int i = 0; i < devs.size(); i++) {
		//if (afx_dev.AlienFXInitialize(0x0461, 0x4EC0) != -1) { // mouse
		//if (afx_dev.AlienFXInitialize(0x0424, 0x2745) != -1) { // monitor
	    if (afx_dev.AlienFXInitialize(0x04f2) != -1) { // keyboard
			cout << hex << "VID: 0x" << afx_dev.GetVid() << ", PID: 0x" << afx_dev.GetPID() << ", API v" << afx_dev.GetVersion() << endl;
			cout << "Now try light 2 to blue... ";
			afx_dev.SetColor(2, {255});
			afx_dev.UpdateColors();
			cin.get();
			cout << "Now try light 3 to mixed... ";
			res = afx_dev.SetColor(3, {255, 255});
			res = afx_dev.UpdateColors();
			cin.get();
			/*cout << "Ok, now lights off...";
			afx_dev.ToggleState(0, NULL, false);
			cin.get();
			cout << "... and back on...";
			afx_dev.ToggleState(255, NULL, false);
			cin.get();*/
			afx_dev.AlienFXClose();
		} else
			cout << "Can't init device!" << endl;
	//}
	//std::cin.get();
	return 0;
}

