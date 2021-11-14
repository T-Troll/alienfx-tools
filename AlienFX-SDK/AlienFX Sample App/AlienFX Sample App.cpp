// AlienFX Sample App.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include "AlienFX_SDK.h"
#include <iostream>

using namespace std;

int main()
{
	AlienFX_SDK::Functions afx_dev;
	AlienFX_SDK::Mappings afx_map;
	vector<pair<DWORD,DWORD>> devs = afx_map.AlienFXEnumDevices();
	cout << devs.size() << " device(s) detected." << endl;
	//for (int i = 0; i < devs.size(); i++) {
		int isInit = afx_dev.AlienFXInitialize(0x0461, 0x4EC0);
		cout << hex << "VID: 0x" << afx_dev.GetVid() << ", PID: 0x" << afx_dev.GetPID() << ", API v" << afx_dev.GetVersion() << endl;
		if (isInit != -1) {
			cout << "API v" << afx_dev.GetVersion() << endl;
			//int ret = afx_dev.AlienfxGetDeviceStatus();
			//cout << hex << "Status result " << ret << endl;
			//afx_dev.Reset();
			//afx_dev.AlienfxGetDeviceStatus();
			cout << "Let's try to set some colors..." << endl;
			int res = afx_dev.SetColor(1, 0, 0, 255);
			cout << "SetColor result: " << res << endl;
			res = afx_dev.SetColor(2, 0, 255, 255);
			cout << "SetColor 2 result: " << res << endl;
			res = afx_dev.UpdateColors();
			cin.get();
			//afx_dev.AlienfxGetDeviceStatus();
			//afx_dev.UpdateColors();
			//afx_dev.AlienfxGetDeviceStatus();
			//cout << "Let's try play with effects." << endl;
			//AlienFX_SDK::afx_act act1 = {0,0,0,255,0,0},
			//	act2 = {0,0,0,0,255,0};
			////afx_dev.SetGlobalEffects(NULL, 0x0e, 6, act1, act2); // Spectrum
			////cout << "Is it breath now? How?" << endl;
			////cin.get();
			////afx_dev.SetGlobalEffects(NULL, 0x1, 0, act2, act1); // Static
			////cout << "Is it rainbow now? How?" << endl; 
			////cin.get();
			////afx_dev.SetGlobalEffects(NULL, 0x2, 7, act1, act1); // Breathing
			////cout << "Should stop effects to color. Is it?" << endl;
			//cout << "Predefined colors will be (Red, Green), default one from AWCC set" << endl;
			//for (int efftype = 0; efftype < 0x10; efftype++)
			//{
			//	//for (int efflen = 0; efflen < 0x10; efflen++) {
			//	int efflen = 5;
			//	cout << "Set effect to (" << efftype << "," << efflen << "):" << endl;
			//	afx_dev.SetGlobalEffects(efftype, efflen, act2, act1);
			//	cin.get();
			//}
			//cout << "Now let's check different length for Breathing:" << endl;
			//for (int efftype = 0; efftype < 0x10; efftype++) {
			//	cout << "Set effect to (2," << efftype << "):" << endl;
			//	afx_dev.SetGlobalEffects(2, efftype, act2, act1);
			//	cin.get();
			//}
			//int ret = afx_dev.AlienfxGetDeviceStatus();
			//cout << hex << "Status result " << ret << endl;
			//cout << "Ok, how about multi colors?" << endl;
			//afx_dev.SetColor(23, 255, 0, 0);
			//afx_dev.SetColor(24, 0, 255, 0);
			//afx_dev.SetColor(25, 0, 0, 255);
			//afx_dev.UpdateColors();
			//cin.get();
			//cout << "That's all for now, exiting." << endl;
			//return 0;
			//AlienFX_SDK::Functions::SetAction(2, AlienFX_SDK::Action::AlienFX_Pulse, 7, 0x64, 25, 134, 245);
			//AlienFX_SDK::Functions::SetAction(2, AlienFX_SDK::Action::AlienFX_Color, 0, 0xfa, 255, 255, 255);
			//std::cout << "Initial state: " << std::dec << (int) AlienFX_SDK::Functions::AlienfxGetDeviceStatus() << std::endl;
			//AlienFX_SDK::Functions::Reset(false);
			//std::cout << "Reset state: " << std::dec << (int)AlienFX_SDK::Functions::AlienfxGetDeviceStatus() << std::endl;
			//AlienFX_SDK::Functions::SetColor(2, 255, 0, 0); //r
			//std::cout << "ColorSet state: " << std::dec << (int)AlienFX_SDK::Functions::AlienfxGetDeviceStatus() << std::endl;
			//AlienFX_SDK::Functions::SetAction(3, AlienFX_SDK::Action::AlienFX_A_Pulse, 7, 0x64, 25, 134, 245);
			//std::cout << "ActionSet state: " << std::dec << (int)AlienFX_SDK::Functions::AlienfxGetDeviceStatus() << std::endl;
			//AlienFX_SDK::Functions::SetPowerAction(1, 0, 0, 255, 255, 0, 0);
			//std::cout << "PowerAction state: " << std::dec << (int)AlienFX_SDK::Functions::AlienfxGetDeviceStatus() << std::endl;
			//AlienFX_SDK::Functions::SetColor(AlienFX_SDK::Index::AlienFX_leftMiddleZone, 0, 0, 255); //b
			//AlienFX_SDK::Functions::SetColor(AlienFX_SDK::Index::AlienFX_rightMiddleZone, 0, 255, 0); // g - right
			//afx_dev.SetColor(4, 0, 255, 0); // right middle
			//std::cout << "BeforeUpdate state: " << std::dec << (int)AlienFX_SDK::Functions::AlienfxGetDeviceStatus() << std::endl;
			//afx_dev.UpdateColors();
			//std::cout << "Update state:" << std::dec << (int)AlienFX_SDK::Functions::AlienfxGetDeviceStatus();
			/*for (int i = 0; i < 16; i++) { // let's test all lights...
				std::cout << "Set zone " << std::dec << i << std::endl;
				AlienFX_SDK::Functions::SetColor(i, 0, 255, 0);
				AlienFX_SDK::Functions::UpdateColors();
				Sleep(200);
				std::cin.get();
			}*/
			afx_dev.AlienFXClose();
		} else
			cout << "Can't init device!" << endl;
	//}
	//std::cin.get();
	return 0;
}

