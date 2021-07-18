#include <iostream>
#include <windows.h>

#include "stdafx.h"
#include "LFXUtil.h"
#include "LFXDecl.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

namespace 
{
	LFXUtil::LFXUtilC lfxUtil;
}

using namespace std;

pair<DWORD,DWORD>* LocateDev(vector<pair<DWORD,DWORD>> *devs, int pid)
{
	for (int i = 0; i < devs->size(); i++)
		if (devs->at(i).second == pid)
			return &devs->at(i);
	return NULL;
}

void printUsage() 
{
	cerr << "Usage: alienfx-cli [command=option,option,option] ... [command=option,option,option] [loop]" << endl
		<< "Commands:\tOptions:" << endl
		<< "set-all\t\tr,g,b[,br] - set all device lights." << endl
		<< "set-one\t\tpid,light,r,g,b[,br] - set one light." << endl
		<< "set-zone\tzone,r,g,b[,br] - set one zone lights." << endl
		<< "set-action\tpid,light,action,r,g,b[,br,[action,r,g,b,br]] - set light and enable it's action." << endl
		<< "set-zone-action\tzone,action,r,g,b[,br,r,g,b[,br]] - set all zone lights and enable it's action." << endl
		<< "set-power\tlight,r,g,b,r2,g2,b2 - set power button colors (low-level only)." << endl
		<< "set-tempo\ttempo - set light action tempo (in milliseconds)." << endl
		<< "set-dev\t\tpid - set active device for low-level." << endl
		<< "low-level\tswitch to low-level SDK (USB driver)." << endl
		<< "high-level\tswitch to high-level SDK (Alienware LightFX)." << endl
		<< "status\t\tshows devices and lights id's, names and statuses." << endl
		<< "lightson\tturn all current device lights on." << endl
		<< "lightsoff\tturn all current device lights off." << endl
		<< "update\t\tupdates light status (for looped commands or old devices)." << endl
		<< "reset\t\treset device state." << endl
		<< "loop\t\trepeat all commands endlessly, until user press ^c. Should be the last command." << endl << endl
		<< "Zones:\tleft, right, top, bottom, front, rear (high-level) or ID of the Group (low-level)." << endl
		<< "Actions:color (disable action), pulse, morph (you need 2 colors)," << endl
		<< "\t(only for low-level v4) breath, spectrum, rainbow (up to 9 colors each)." << endl;
}

int main(int argc, char* argv[])
{
	bool low_level = true;
	UINT sleepy = 0;
	AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
	AlienFX_SDK::Functions* afx_dev = new AlienFX_SDK::Functions();
	cerr << "alienfx-cli v3.1.0" << endl;
	if (argc < 2) 
	{
		printUsage();
		return 1;
	}

	int res = -1; 
	vector<pair<DWORD,DWORD>> devs = afx_map->AlienFXEnumDevices();
	int isInit = afx_dev->AlienFXInitialize(AlienFX_SDK::vids[0]);
	//std::cout << "PID: " << std::hex << isInit << std::endl;
	if (isInit != -1)
	{
		for (int rcount = 0; rcount < 10 && !afx_dev->IsDeviceReady(); rcount++)
			Sleep(20);
		if (!afx_dev->IsDeviceReady()) {
			afx_dev->Reset(true);
		}
		afx_map->LoadMappings();
	}
	else {
		cerr << "No low-level device found!" << endl;
		return 1;
	}
	const char* command = argv[1];
	for (int cc = 1; cc < argc; cc++) {
		if (low_level && cc > 1) {
			//cerr << "Sleep " << sleepy << endl;
			Sleep(sleepy);
		}
		else
			Sleep(100);
		string arg = string(argv[cc]);
		size_t vid = arg.find_first_of('=');
		string command = arg.substr(0, vid);
		string values;
		vector<string> args;
		if (vid != string::npos) {
			size_t vpos = 0;
			values = arg.substr(vid + 1, arg.size());
			while (vpos < values.size()) {
				size_t tvpos = values.find(',', vpos);
				args.push_back(values.substr(vpos, tvpos-vpos));
				vpos = tvpos == string::npos ? values.size() : tvpos+1;
			}
		}
		//cerr << "Executing " << command << " with " << values << endl;
		if (command == "low-level") {
			low_level = true;
			continue;
		}
		if (command == "high-level") {
			if (res == (-1))
				res = lfxUtil.InitLFX();
				if (res != -1) {
					switch (res) {
					case 0: cerr << "Dell library DLL not found (no library?)!" << endl; break;
					case 1: cerr << "Can't init Dell library!" << endl; break;
					case 2: cerr << "No high-level devices found!" << endl; break;
					default: cerr << "Dell library unknown error!" << endl; break;
					}
					low_level = true;
				}
				else
					low_level = false;
			continue;
		}
		if (command == "loop") {
			cc = 1;
			if (low_level)
				afx_dev->UpdateColors();
			else
				lfxUtil.Update();
			continue;
		}
		if (command == "status") {
			if (low_level) {
				// vector<int> devs = afx_dev->AlienFXEnumDevices(afx_dev->vid);
				for (int i = 0; i < devs.size(); i++) {
					cout << "Device VID#" << devs[i].first << ", PID#" << devs[i].second;
					int dn;
					for (dn = 0; dn < afx_map->GetDevices()->size(); dn++) {
						if (devs[i].second == afx_map->GetDevices()->at(i).devid) {
							cout << " - " << afx_map->GetDevices()->at(i).name;
							if (devs[i].second == afx_dev->GetPID()) {
								cout << " (Active, V" << afx_dev->GetVersion() << ")";
							}
							break;
						}
					}
					cout << endl;
					for (int k = 0; k < afx_map->GetMappings()->size(); k++) {
						if (devs[i].second == afx_map->GetMappings()->at(k).devid) {
							cout << "  Light ID#" << afx_map->GetMappings()->at(k).lightid
								<< " - " << afx_map->GetMappings()->at(k).name;
							if (afx_map->GetMappings()->at(k).flags)
								cout << " (Power button)";
							cout << endl;
						}
					}
				}
				// now groups...
				for (int i = 0; i < afx_map->GetGroups()->size(); i++)
					cout << "Group #" << (afx_map->GetGroups()->at(i).gid & 0xffff)
					<< " - " << afx_map->GetGroups()->at(i).name
					<< " (" << afx_map->GetGroups()->at(i).lights.size() << " lights)" << endl;
			}
			else {
				lfxUtil.GetStatus();
			}
			continue;
		}
		if (command == "set-tempo") {
			if (args.size() < 1) {
				cerr << "set-tempo: Incorrect argument" << endl;
				continue;
			}
			if (low_level) {
				sleepy = atoi(args.at(0).c_str());
			}
			else {
				unsigned tempo = atoi(args.at(0).c_str());
				lfxUtil.SetTempo(tempo);
				lfxUtil.Update();
			}
			continue;
		}
		if (command == "set-dev") {
			if (args.size() < 1) {
				cerr << "set-dev: Incorrect argument" << endl;
				continue;
			}
			if (low_level) {
				int newDev = atoi(args.at(0).c_str());
				if (newDev == afx_dev->GetPID())
					continue;
				for (int i = 0; i < devs.size(); i++)
					if (devs[i].second == newDev) {
						afx_dev->UpdateColors();
						afx_dev->AlienFXClose();
						isInit = afx_dev->AlienFXChangeDevice(devs[i].first, devs[i].second);
						if (isInit) {
							isInit = newDev;
						} else { 
							cerr << "Can't init device ID#" << newDev << ", exiting!" << endl;
							return 1;
						}
						break;
					}
			}
			continue;
		}
		if (command == "reset") {
			if (low_level) {
				afx_dev->Reset(true);
			}
			else
				lfxUtil.Reset();
			continue;
		}
		if (command == "update") {
			if (low_level)
				afx_dev->UpdateColors();
			else
				lfxUtil.Update();
			continue;
		}
		if (command == "lightson") {
			if (low_level)
				afx_dev->ToggleState(true, afx_map->GetMappings(), false);
			continue;
		}
		if (command == "lightsoff") {
			if (low_level)
				afx_dev->ToggleState(false, afx_map->GetMappings(), false);
			continue;
		}
		if (command == "set-all") {
			if (args.size() < 3) {
				cerr << "set-all: Incorrect arguments" << endl;
				continue;
			}
			unsigned zoneCode = LFX_ALL;
			static ColorU color;
			color.cs.red = atoi(args.at(0).c_str());
			color.cs.green = atoi(args.at(1).c_str());
			color.cs.blue = atoi(args.at(2).c_str());
			color.cs.brightness = args.size() > 3 ? atoi(args.at(3).c_str()) : 255;
			if (low_level) {
				color.cs.red = (color.cs.red * color.cs.brightness) >> 8;
				color.cs.green = (color.cs.green * color.cs.brightness) >> 8;
				color.cs.blue = (color.cs.blue * color.cs.brightness) >> 8;
				vector<UCHAR> lights;
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					if (afx_map->GetMappings()->at(i).devid == isInit &&
						!afx_map->GetMappings()->at(i).flags)
						lights.push_back((UCHAR)afx_map->GetMappings()->at(i).lightid);
						//afx_dev->SetColor(afx_map->GetMappings()->at(i).lightid,
						//	color.cs.red, color.cs.green, color.cs.blue);
				}
				afx_dev->SetMultiLights((int)lights.size(), lights.data(), color.cs.red, color.cs.green, color.cs.blue);
				afx_dev->UpdateColors();
			}
			else {
				lfxUtil.SetLFXColor(zoneCode, color.ci);
				lfxUtil.Update();
			}
			continue;
		}
		if (command == "set-one") {
			if (args.size() < 5) {
				cerr << "set-one: Incorrect arguments" << endl;
				continue;
			}
			static ColorU color;
			int devid = atoi(args.at(0).c_str());
			color.cs.red = atoi(args.at(4).c_str());
			color.cs.green = atoi(args.at(3).c_str());
			color.cs.blue = atoi(args.at(2).c_str());
			color.cs.brightness = args.size() > 5 ? atoi(args.at(5).c_str()) : 255;
			if (low_level) {
				color.cs.red = (color.cs.red * color.cs.brightness) >> 8;
				color.cs.green = (color.cs.green * color.cs.brightness) >> 8;
				color.cs.blue = (color.cs.blue * color.cs.brightness) >> 8;
				if (devid != 0 && devid != afx_dev->GetPID()) {
					afx_dev->UpdateColors();
					afx_dev->AlienFXClose();
					pair<DWORD, DWORD>* dev = LocateDev(&devs, devid);
					afx_dev->AlienFXChangeDevice(dev->first, dev->second);
				}
				afx_dev->SetColor(atoi(args.at(1).c_str()),
					color.cs.blue, color.cs.green, color.cs.red);
			}
			else {
				lfxUtil.SetOneLFXColor(devid, atoi(args.at(1).c_str()), &color.ci);
				lfxUtil.Update();
			}
			continue;
		}
		if (command == "set-zone") {
			if (args.size() < 4) {
				cerr << "set-zone: Incorrect arguments" << endl;
				continue;
			}
			unsigned zoneCode = LFX_ALL;
			if (args.at(0) == "left") {
				zoneCode = LFX_ALL_LEFT;
			}
			if (args.at(0) == "right") {
				zoneCode = LFX_ALL_RIGHT;
			}
			if (args.at(0) == "top") {
				zoneCode = LFX_ALL_UPPER;
			}
			if (args.at(0) == "bottom") {
				zoneCode = LFX_ALL_LOWER;
			}
			if (args.at(0) == "front") {
				zoneCode = LFX_ALL_FRONT;
			}
			if (args.at(0) == "rear") {
				zoneCode = LFX_ALL_REAR;
			}
			static ColorU color;
			color.cs.red = atoi(args.at(1).c_str());
			color.cs.green = atoi(args.at(2).c_str());
			color.cs.blue = atoi(args.at(3).c_str());
			color.cs.brightness = args.size() > 4 ? atoi(args.at(4).c_str()) : 255;
			if (low_level) {
				zoneCode = atoi(args.at(0).c_str()) | 0x10000;
				color.cs.red = (color.cs.red * color.cs.brightness) >> 8;
				color.cs.green = (color.cs.green * color.cs.brightness) >> 8;
				color.cs.blue = (color.cs.blue * color.cs.brightness) >> 8;
				AlienFX_SDK::group* grp = afx_map->GetGroupById(zoneCode);
				if (grp) {
					vector<UCHAR> lights;
					for (int i = 0; i < grp->lights.size(); i++)
						if (grp->lights[i]->devid == afx_dev->GetPID())
							lights.push_back((UCHAR)afx_map->GetMappings()->at(i).lightid);
							//afx_dev->SetColor(grp->lights[i]->lightid, color.cs.red, color.cs.green, color.cs.blue);
					afx_dev->SetMultiLights((int)lights.size(), lights.data(), color.cs.red, color.cs.green, color.cs.blue);
					afx_dev->UpdateColors();
				}
			}
			else {
				lfxUtil.SetLFXColor(zoneCode, color.ci);
				lfxUtil.Update();
			}
			continue;
		}
		if (command == "set-power") {
			if (args.size() != 7) {
				cerr << "set-power: Incorrect arguments" << endl;
				continue;
			}
			static ColorU color, color2;
			color.cs.red = atoi(args.at(1).c_str());
			color.cs.green = atoi(args.at(2).c_str());
			color.cs.blue = atoi(args.at(3).c_str());
			//color.cs.brightness = 255;
			color2.cs.red = atoi(args.at(4).c_str());
			color2.cs.green = atoi(args.at(5).c_str());
			color2.cs.blue = atoi(args.at(6).c_str());
			//color2.cs.brightness = 255;
			if (low_level) {
				afx_dev->SetPowerAction(atoi(args.at(0).c_str()),
					color.cs.red, color.cs.green, color.cs.blue,
					color2.cs.red, color2.cs.green, color2.cs.blue, true);
			}
			else {
				cerr << "High-level API doesn't support set-power!" << endl;
			}
			continue;
		}
		if (command == "set-action") {
			if (args.size() < 6) {
				cerr << "set-action: Incorrect arguments" << endl;
				continue;
			}
			unsigned actionCode = LFX_ACTION_COLOR;
			std::vector<AlienFX_SDK::afx_act> act;
			std::vector<ColorU> clrs;
			int argPos = 2;
			int devid = atoi(args.at(0).c_str());
			while (argPos < args.size()) {
				AlienFX_SDK::afx_act c_act;
				ColorU c;
				if (args.at(argPos) == "pulse") {
					actionCode = LFX_ACTION_PULSE;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Pulse;
				}
				else if (args.at(argPos) == "morph") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Morph;
				}
				else if (args.at(argPos) == "breath") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Breathing;
				}
				else if (args.at(argPos) == "spectrum") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Spectrum;
				}
				else if (args.at(argPos) == "rainbow") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Rainbow;
				}
				c.cs.blue = c_act.r = atoi(args.at(argPos+1).c_str());
				c.cs.green = c_act.g = atoi(args.at(argPos + 2).c_str());
				c.cs.red = c_act.b = atoi(args.at(argPos + 3).c_str());
				c.cs.brightness = argPos + 4 < args.size() ? atoi(args.at(argPos + 4).c_str()) : 255;
				c_act.tempo = 7;
				c_act.time = sleepy;
				c_act.r = (c_act.r * c.cs.brightness) >> 8;
				c_act.g = (c_act.g * c.cs.brightness) >> 8;
				c_act.b = (c_act.b * c.cs.brightness) >> 8;
				act.push_back(c_act);
				clrs.push_back(c);
				argPos += 5;
			}
			if (low_level) {
				if (devid != 0 && devid != afx_dev->GetPID()) {
					afx_dev->UpdateColors();
					afx_dev->AlienFXClose();
					pair<DWORD, DWORD>* dev = LocateDev(&devs, devid);
					afx_dev->AlienFXChangeDevice(dev->first, dev->second);
				}
				afx_dev->SetAction(atoi(args.at(1).c_str()), act);
			}
			else {
				if (clrs.size() < 2) {
					ColorU c;
					clrs.push_back(c);
				}
				lfxUtil.SetLFXAction(actionCode, devid, atoi(args.at(1).c_str()), &clrs[0].ci, &clrs[1].ci);
				lfxUtil.Update();
			}
			continue;
		}
		if (command == "set-zone-action") {
			if (args.size() < 5) {
				cerr << "set-zone-action: Incorrect arguments" << endl;
				continue;
			}
			unsigned zoneCode = LFX_ALL;
			if (args.at(0) == "left") {
				zoneCode = LFX_ALL_LEFT;
			}
			if (args.at(0) == "right") {
				zoneCode = LFX_ALL_RIGHT;
			}
			if (args.at(0) == "top") {
				zoneCode = LFX_ALL_UPPER;
			}
			if (args.at(0) == "bottom") {
				zoneCode = LFX_ALL_LOWER;
			}
			if (args.at(0) == "front") {
				zoneCode = LFX_ALL_FRONT;
			}
			if (args.at(0) == "rear") {
				zoneCode = LFX_ALL_REAR;
			}
			unsigned actionCode = LFX_ACTION_COLOR;
			std::vector<AlienFX_SDK::afx_act> act;
			std::vector<ColorU> clrs;
			int argPos = 2;
			while (argPos < args.size()) {
				AlienFX_SDK::afx_act c_act;
				ColorU c;
				if (args.at(argPos) == "pulse") {
					actionCode = LFX_ACTION_PULSE;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Pulse;
				}
				else if (args.at(argPos) == "morph") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Morph;
				}
				else if (args.at(argPos) == "breath") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Breathing;
				}
				else if (args.at(argPos) == "spectrum") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Spectrum;
				}
				else if (args.at(argPos) == "rainbow") {
					actionCode = LFX_ACTION_MORPH;
					c_act.type = AlienFX_SDK::Action::AlienFX_A_Rainbow;
				}
				c.cs.blue = c_act.r = atoi(args.at(argPos+1).c_str());
				c.cs.green = c_act.g = atoi(args.at(argPos + 2).c_str());
				c.cs.red = c_act.b = atoi(args.at(argPos + 3).c_str());
				c.cs.brightness = argPos + 4 < args.size() ? atoi(args.at(argPos + 4).c_str()) : 255;
				c_act.tempo = 7;
				c_act.time = sleepy;
				c_act.r = (c_act.r * c.cs.brightness) >> 8;
				c_act.g = (c_act.g * c.cs.brightness) >> 8;
				c_act.b = (c_act.b * c.cs.brightness) >> 8;
				act.push_back(c_act);
				clrs.push_back(c);
				argPos += 5;
			}
			if (low_level) {
					AlienFX_SDK::group* grp = afx_map->GetGroupById(zoneCode);
					if (grp) {
						for (int i = 0; i < grp->lights.size(); i++)
							if (grp->lights[i]->devid == afx_dev->GetPID())
								afx_dev->SetAction(grp->lights[i]->lightid, act);
					}
			}
			else {
				lfxUtil.SetLFXZoneAction(actionCode, zoneCode, clrs[0].ci, clrs[1].ci);
				lfxUtil.Update();
			}
			continue;
		}
		cerr << "Unknown command: " << command << endl;
	}
	cout << "Done." << endl;
	if (isInit != -1)
		afx_dev->UpdateColors();
		afx_dev->AlienFXClose();
	if (res != -1) 
		lfxUtil.Release();
	delete afx_dev;
    return 1;
}

