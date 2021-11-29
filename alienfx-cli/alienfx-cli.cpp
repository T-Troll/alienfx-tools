#define WIN32_LEAN_AND_MEAN
#include <iostream>

#include "stdafx.h"
#include "LFXUtil.h"
#include "LFXDecl.h"
#include "AlienFX_SDK.h"

namespace 
{
	LFXUtil::LFXUtilC lfxUtil;
}

using namespace std;

unsigned GetZoneCode(string name, int mode) {
	switch (mode) {
	case 1: return atoi(name.c_str()) | 0x10000;
	case 0:
		if (name == "left") return LFX_ALL_LEFT;		
		if (name == "right") return LFX_ALL_RIGHT;
		if (name == "top") return LFX_ALL_UPPER;		
		if (name == "bottom") return LFX_ALL_LOWER;		
		if (name == "front") return LFX_ALL_FRONT;		
		if (name == "rear") return LFX_ALL_REAR;
	}
	return LFX_ALL;
}

unsigned GetActionCode(string name, int mode) {
	if (name == "pulse") {
		return mode ? AlienFX_SDK::Action::AlienFX_A_Pulse : LFX_ACTION_PULSE;
	}
	if (name == "morph") {
		return mode ? AlienFX_SDK::Action::AlienFX_A_Morph : LFX_ACTION_MORPH;
	}
	if (name == "breath") {
		return mode ? AlienFX_SDK::Action::AlienFX_A_Breathing : LFX_ACTION_MORPH;
	}
	else if (name == "spectrum") {
		return mode ? AlienFX_SDK::Action::AlienFX_A_Spectrum : LFX_ACTION_MORPH;
	}
	else if (name == "rainbow") {
		return mode ? AlienFX_SDK::Action::AlienFX_A_Rainbow : LFX_ACTION_MORPH;
	}
	return mode ? AlienFX_SDK::Action::AlienFX_A_Color : LFX_ACTION_COLOR;
}

void SetBrighness(AlienFX_SDK::Colorcode *color) {
	color->r = ((unsigned) color->r * color->br) / 255;// >> 8;
	color->g = ((unsigned) color->g * color->br) / 255;// >> 8;
	color->b = ((unsigned) color->b * color->br) / 255;// >> 8;
}

void printUsage() 
{
	cerr << "Usage: alienfx-cli [command=option,option,option] ... [command=option,option,option] [loop]" << endl
		<< "Commands:\tOptions:" << endl
		<< "set-all\t\tr,g,b[,br] - set all device lights." << endl
		<< "set-one\t\tpid,light,r,g,b[,br] - set one light." << endl
		<< "set-zone\tzone,r,g,b[,br] - set one zone lights." << endl
		<< "set-action\tpid,light,action,r,g,b[,br,[action,r,g,b,br]] - set light and enable it's action." << endl
		<< "set-zone-action\tzone,action,r,g,b[,br,[action,r,g,b,br]] - set all zone lights and enable it's action." << endl
		<< "set-power\tlight,r,g,b,r2,g2,b2 - set power button colors (low-level only)." << endl
		<< "set-tempo\ttempo - set light action tempo (in milliseconds)." << endl
		<< "set-dev\t\tpid - set active device for low-level." << endl
		<< "set-dim\t\tbr - set active device dim level." << endl
		<< "low-level\tswitch to low-level SDK (USB driver)." << endl
		<< "high-level\tswitch to high-level SDK (Alienware LightFX)." << endl
		<< "status\t\tshows devices and lights id's, names and statuses." << endl
		<< "lightson\tturn all current device lights on." << endl
		<< "lightsoff\tturn all current device lights off." << endl
		//<< "update\t\tupdates light status (for looped commands or old devices)." << endl
		<< "reset\t\treset device state." << endl
		<< "loop\t\trepeat all commands endlessly, until user press ^c. Should be the last command." << endl << endl
		<< "Zones:\tleft, right, top, bottom, front, rear (high-level) or ID of the Group (low-level)." << endl
		<< "Actions:color (disable action), pulse, morph (you need 2 colors)," << endl
		<< "\t(only for low-level v4) breath, spectrum, rainbow (up to 9 colors each)." << endl;
}

AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
AlienFX_SDK::Functions *cdev = NULL;

void FindDevice(int devID) {
	if (devID != cdev->GetPID()) {
		cdev->UpdateColors();
		for (int i = 0; i < afx_map->fxdevs.size(); i++)
			if (afx_map->fxdevs[i].dev->GetPID() == devID) {
				cdev = afx_map->fxdevs[i].dev;
				return;
			}
	}
}

bool CheckArgs(string cName, int minArgs, size_t nargs) {
	if (minArgs < nargs) {
		cerr << cName << ": Incorrect arguments (should be " << minArgs << ")" << endl;
		return false;
	}
	return true;
}

int main(int argc, char* argv[])
{
	int devType = -1; bool have_low = false, have_high = false;
	UINT sleepy = 0;

	cerr << "alienfx-cli v5.3.3.1" << endl;
	if (argc < 2) 
	{
		printUsage();
		return 1;
	}

	afx_map->LoadMappings();
	afx_map->AlienFXAssignDevices();

	if (afx_map->fxdevs.size() > 0)
	{
		cdev = afx_map->fxdevs[0].dev;
		for (int rcount = 0; rcount < 10 && !cdev->IsDeviceReady(); rcount++)
			Sleep(20);
		if (!cdev->IsDeviceReady()) {
			cdev->Reset();
		}
		cout << "Low-level device ready" << endl;
		have_low = true;
		devType = 1;
	}
	else {
		cerr << "No low-level devices found, trying high-level..." << endl;
	}

	switch (lfxUtil.InitLFX()) {
	case -1: cout << "Dell API ready" << endl;
		have_high = true;
		if (!have_low) devType = 0;
		break;
	case 0: cerr << "Dell library DLL not found (no library?)!" << endl; break;
	case 1: cerr << "Can't init Dell library!" << endl; break;
	case 2: cerr << "No high-level devices found!" << endl; break;
	default: cerr << "Dell library unknown error!" << endl; break;
	}

	if (devType == -1) {
		cout << "Both low-levl and high-level devices not found, exiting!" << endl;
		goto deinit;
	}

	for (int cc = 1; cc < argc; cc++) {

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
			cout << "Low-level device ";
			if (have_low) {
				devType = 1;
				cout << "selected" << endl;
			} else
				cout << "not found!" << endl;
			continue;
		}
		if (command == "high-level") {
			cout << "High-level device ";
			if (have_high) {
				devType = 0;
				cout << "selected" << endl;
			} else
				cout << "not found!" << endl;
			continue;
		}
		if (command == "loop") {
			cc = 1;
			//switch (devType) {
			//case 1:
			//	for (int i = 0; i < afx_map->fxdevs.size(); i++)
			//		afx_map->fxdevs[i].dev->UpdateColors(); 
			//	break;
			//case 0:
			//	lfxUtil.Update(); break;
			//}
			goto update;
		}
		if (command == "status") {
			switch (devType) {
			case 1:
				for (int i = 0; i < afx_map->fxdevs.size(); i++) {
					cout << "Device ";
					if (afx_map->fxdevs[i].desc)
						cout << afx_map->fxdevs[i].desc->name;
					else
						cout << "No name";
					unsigned devtype = afx_map->fxdevs[i].dev->GetType();
					string typeName = "Unknown";
					switch (devtype) {
					case 0: typeName = "Notebook"; break;
					case 1: typeName = "Keyboard"; break;
					case 2: typeName = "Monitor"; break;
					case 3: typeName = "Mouse"; break;
					}
					cout << ", " << typeName << ", VID#" << afx_map->fxdevs[i].dev->GetVid()
						<< ", PID#" << afx_map->fxdevs[i].dev->GetPID()
						<< ", V" << afx_map->fxdevs[i].dev->GetVersion();

					if (cdev->GetPID() == afx_map->fxdevs[i].dev->GetPID())
						cout << " (Active)";
					cout << endl;
					for (int k = 0; k < afx_map->fxdevs[i].lights.size(); k++) {
							cout << "  Light ID#" << afx_map->fxdevs[i].lights[k]->lightid
								<< " - " << afx_map->fxdevs[i].lights[k]->name;
							if (afx_map->fxdevs[i].lights[k]->flags & ALIENFX_FLAG_POWER)
								cout << " (Power button)";
							if (afx_map->fxdevs[i].lights[k]->flags & ALIENFX_FLAG_INDICATOR)
								cout << " (Indicator)";
							cout << endl;
					}
				}
				// now groups...
				for (int i = 0; i < afx_map->GetGroups()->size(); i++)
					cout << "  Group #" << (afx_map->GetGroups()->at(i).gid & 0xffff)
					<< " - " << afx_map->GetGroups()->at(i).name
					<< " (" << afx_map->GetGroups()->at(i).lights.size() << " lights)" << endl;
				break;
			case 0:
				lfxUtil.GetStatus(); break;
			}
			continue;
		}
		if (command == "set-tempo" && CheckArgs(command, 1, args.size())) {
			sleepy = atoi(args.at(0).c_str());
			switch (devType) {
			case 0:
			{
				lfxUtil.SetTempo(sleepy);
				goto update;
				//lfxUtil.Update();
			} break;
			}
			continue;
		}
		if (command == "set-dev" && CheckArgs(command, 1, args.size())) {
			FindDevice(atoi(args.at(0).c_str()));
			cout << "Device #" << cdev->GetPID() << " selected" << endl;
			continue;
		}
		if (command == "reset") {
			switch (devType) {
			case 1:
				cdev->Reset(); break;
			case 0:
				lfxUtil.Reset(); break;
			}
			continue;
		}
		//if (command == "update") {
		//	if (have_low)
		//		for (int i = 0; i < afx_map->fxdevs.size(); i++)
		//			afx_map->fxdevs[i].dev->UpdateColors(); 
		//	/*if (have_high)
		//		lfxUtil.Update(); break;*/
		//	continue;
		//}
		if (command == "set-dim" && CheckArgs(command, 1, args.size())) {
			byte dim = atoi(args.at(0).c_str());
			if (devType)
				cdev->ToggleState(dim, afx_map->GetMappings(), false);
			continue;
		}
		if (command == "lightson") {
			if (devType)
				cdev->ToggleState(255, afx_map->GetMappings(), false);
			continue;
		}
		if (command == "lightsoff") {
			if (devType)
				cdev->ToggleState(0, afx_map->GetMappings(), false);
			continue;
		}
		if (command == "set-all" && CheckArgs(command, 3, args.size())) {

			static AlienFX_SDK::Colorcode color{
			        (byte) atoi(args.at(2).c_str()),
					(byte) atoi(args.at(1).c_str()),
					(byte) atoi(args.at(0).c_str()),
					(byte) (args.size() > 3 ? atoi(args.at(3).c_str()) : 255)};
			switch (devType) {
			case 1:
			{
				SetBrighness(&color);
				//for (int j = 0; j < afx_map->fxdevs.size(); j++) {
				//	vector<byte> lights;
				//	for (int i = 0; i < afx_map->fxdevs[j].lights.size(); i++) {
				//		lights.push_back((byte)afx_map->fxdevs[j].lights[i]->lightid);
				//	}
				//	afx_map->fxdevs[j].dev->SetMultiLights(&lights, color);
				//	//afx_map->fxdevs[j].dev->UpdateColors();
				//}
				vector<byte> lights;
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					AlienFX_SDK::mapping *lgh = afx_map->GetMappings()->at(i);
					if (lgh->devid == cdev->GetPID() &&
						!(lgh->flags & ALIENFX_FLAG_POWER))
						lights.push_back((byte) lgh->lightid);
				}
				cdev->SetMultiLights(&lights, color);
				//cdev->UpdateColors();
			} break;
			case 0:
				lfxUtil.SetLFXColor(LFX_ALL, color.ci);
				//lfxUtil.Update();
				break;
			}
			goto update;
		}
		if (command == "set-one" && CheckArgs(command, 5, args.size())) {
			int devid = atoi(args.at(0).c_str());
			AlienFX_SDK::Colorcode color{(byte)atoi(args.at(4).c_str()),
				(byte)atoi(args.at(3).c_str()),
				(byte)atoi(args.at(2).c_str()),
				(byte)(args.size() > 5 ? atoi(args.at(5).c_str()) : 255)};
			switch (devType) {
			case 1:
			{
				SetBrighness(&color);
				if (devid) {
					FindDevice(devid);
				}
				cdev->SetColor(atoi(args.at(1).c_str()), color);
			} break;
			case 0:
				lfxUtil.SetOneLFXColor(devid, atoi(args.at(1).c_str()), (unsigned *)&color.ci);
				//lfxUtil.Update();
				break;
			}
			goto update;
		}
		if (command == "set-zone" && CheckArgs(command, 4, args.size())) {
			AlienFX_SDK::Colorcode color{(byte)atoi(args.at(3).c_str()),
				(byte)atoi(args.at(2).c_str()),
				(byte)atoi(args.at(1).c_str()),
				(byte)(args.size() > 4 ? atoi(args.at(4).c_str()) : 255)};
			unsigned zoneCode = GetZoneCode(args[0], devType);
			switch (devType) {
			case 1:
			{
				SetBrighness(&color);
				AlienFX_SDK::group* grp = afx_map->GetGroupById(zoneCode);
				if (grp) {
					for (int j = 0; j < afx_map->fxdevs.size(); j++) {
						vector<UCHAR> lights;
						for (int i = 0; i < grp->lights.size(); i++) {
							if (grp->lights[i]->devid == afx_map->fxdevs[j].dev->GetPID())
								lights.push_back((UCHAR) grp->lights[i]->lightid);
						}
						afx_map->fxdevs[j].dev->SetMultiLights(&lights, color);
						//afx_map->fxdevs[j].dev->UpdateColors();
					}
				}
			} break;
			case 0:
			{
				lfxUtil.SetLFXColor(zoneCode, color.ci);
				//lfxUtil.Update();
			} break;
			}
			goto update;
		}
		if (command == "set-power" && CheckArgs(command, 7, args.size())) {
			if (devType) {
				vector<AlienFX_SDK::act_block> act{{(byte) atoi(args.at(0).c_str()),
								  {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
								  (byte) atoi(args.at(1).c_str()),
								  (byte) atoi(args.at(2).c_str()),
								  (byte) atoi(args.at(3).c_str())},
								  {AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
								  (byte) atoi(args.at(4).c_str()),
								  (byte) atoi(args.at(5).c_str()),
								  (byte) atoi(args.at(6).c_str())}}}};
				cdev->SetPowerAction(&act);
			}
			continue;
		}
		if (command == "set-action" && CheckArgs(command, 6, args.size())) {
			unsigned actionCode = LFX_ACTION_COLOR;
			vector<AlienFX_SDK::Colorcode> clrs;
			int argPos = 2;
			int devid = atoi(args.at(0).c_str()),
				lightid = atoi(args.at(1).c_str());
			AlienFX_SDK::act_block act{(byte)lightid};
			while (argPos + 4 < args.size()) {
				AlienFX_SDK::Colorcode c;
				actionCode = GetActionCode(args[argPos], devType);
				c.b = atoi(args.at(argPos+1).c_str());
				c.g = atoi(args.at(argPos + 2).c_str());
				c.r = atoi(args.at(argPos + 3).c_str());
				c.br = argPos + 4 < args.size() ? atoi(args.at(argPos + 4).c_str()) : 255;
				if (devType) {
					SetBrighness(&c);
					act.act.push_back(AlienFX_SDK::afx_act({(BYTE) actionCode, (BYTE) sleepy, 7, (BYTE) c.b, (BYTE) c.g, (BYTE) c.r}));
				} else
					clrs.push_back(c);
				argPos += 5;
			}
			switch (devType) {
			case 1:
				if (devid) FindDevice(devid);
				if (act.act.size() < 2) {
					act.act.push_back(AlienFX_SDK::afx_act({(BYTE) actionCode, (BYTE) sleepy, 7, 0, 0, 0}));
				}
				cdev->SetAction(&act);
				break;
			case 0:
				if (clrs.size() < 2) {
					clrs.push_back({0});
				}
				lfxUtil.SetLFXAction(actionCode, devid, lightid, (unsigned*)&clrs[0].ci, (unsigned*)&clrs[1].ci);
				//lfxUtil.Update();
				break;
			}
			goto update;
		}
		if (command == "set-zone-action" && CheckArgs(command, 5, args.size())) {
			unsigned zoneCode = GetZoneCode(args[0], devType);
			unsigned actionCode = LFX_ACTION_COLOR;
			AlienFX_SDK::act_block act;
			vector<AlienFX_SDK::Colorcode> clrs;
			int argPos = 1;
			while (argPos + 4 < args.size()) {
				AlienFX_SDK::Colorcode c;
				actionCode = GetActionCode(args[argPos], devType);
				c.r = atoi(args.at(argPos+1).c_str());
				c.g = atoi(args.at(argPos+2).c_str());
				c.b = atoi(args.at(argPos+3).c_str());
				c.br = argPos + 4 < args.size() ? atoi(args.at(argPos + 4).c_str()) : 255;
				if (devType) {
					SetBrighness(&c);
					act.act.push_back(AlienFX_SDK::afx_act({(BYTE) actionCode, (BYTE) sleepy, 7, (BYTE) c.r, (BYTE) c.g, (BYTE) c.b}));
				} else
					clrs.push_back(c);
				argPos += 5;
			}
			switch (devType) {
			case 1:
			{
				AlienFX_SDK::group* grp = afx_map->GetGroupById(zoneCode);
				if (act.act.size() < 2) {
					act.act.push_back(AlienFX_SDK::afx_act({(BYTE) actionCode, (BYTE) sleepy, 7, 0, 0, 0}));
				}
				for (int j = 0; j < afx_map->fxdevs.size(); j++) {
					for (int i = 0; i < (grp ? grp->lights.size() : afx_map->GetMappings()->size()); i++) {
						if (grp ? grp->lights[i]->devid : (*afx_map->GetMappings())[i]->devid
							== afx_map->fxdevs[j].dev->GetPID()) {
							act.index = (byte) (grp ? grp->lights[i]->lightid :  (*afx_map->GetMappings())[i]->lightid);
							afx_map->fxdevs[j].dev->SetAction(&act);
						}
					}
					//afx_map->fxdevs[j].dev->UpdateColors();
				}
			} break;
			case 0:
				if (clrs.size() < 2)
					clrs.push_back({0});
				lfxUtil.SetLFXZoneAction(actionCode, zoneCode, clrs[0].ci, clrs[1].ci);
				//lfxUtil.Update();
				break;
			}
			//goto update;
		} else
			cerr << "Unknown command: " << command << endl;
update:
		if (have_low) {
			for (int i = 0; i < afx_map->fxdevs.size(); i++)
				afx_map->fxdevs[i].dev->UpdateColors(); 
		}
		if (have_high) {
			lfxUtil.Update();
		}
	}
	cout << "Done." << endl;

deinit:
	if (have_high) 
		lfxUtil.Release();
	delete afx_map;
    return 0;
}

