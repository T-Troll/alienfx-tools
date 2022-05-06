#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string>
#include "LFXUtil.h"
#include "LFXDecl.h"
#include "AlienFX_SDK.h"

namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

using namespace std;

void CheckDevices(bool);

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
	printf("Usage: alienfx-cli [command=option,option,option] ... [command=option,option,option] [loop]\n\
Commands:\tOptions:\n\
set-all\t\tr,g,b[,br] - set all device lights.\n\
set-one\t\tpid,light,r,g,b[,br] - set one light.\n\
set-zone\tzone,r,g,b[,br] - set one zone lights.\n\
set-action\tpid,light,action,r,g,b[,br,[action,r,g,b,br]] - set light and enable it's action.\n\
set-zone-action\tzone,action,r,g,b[,br,[action,r,g,b,br]] - set all zone lights and enable it's action.\n\
set-power\tlight,r,g,b,r2,g2,b2 - set power button colors (low-level only).\n\
set-tempo\ttempo - set light action tempo (in milliseconds).\n\
set-dev\t\tpid - set active device for low-level.\n\
set-dim\t\tbr - set active device dim level.\n\
low-level\tswitch to low-level SDK (USB driver).\n\
high-level\tswitch to high-level SDK (Alienware LightFX).\n\
status\t\tshows devices and lights id's, names and statuses.\n\
probe\t\t[ald[,lights][,devID[,lightID]] - probe lights across devices\n\
lightson\tturn all current device lights on.\n\
lightsoff\tturn all current device lights off.\n\
reset\t\treset device state.\n\
loop\t\trepeat all commands endlessly, until user press CTRL+c. Should be the last command.\n\n\
For probe, a for all devices info, l for define number of lights, d for deviceID and (optionally)\n\
\tlightid. Can be combined.\n\
Zones:\tleft, right, top, bottom, front, rear (high-level) or ID of the Group (low-level).\n\
Actions:color (disable action), pulse, morph (you need 2 colors),\n\
\t(only for low-level v4) breath, spectrum, rainbow (up to 9 colors each).");
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
	if (minArgs > nargs) {
		printf("%s: Incorrect arguments (should be %d)\n", cName.c_str(), minArgs);
		return false;
	}
	return true;
}

int main(int argc, char* argv[])
{
	int devType = -1; bool have_low = false, have_high = false;
	UINT sleepy = 0;

	printf("alienfx-cli v5.9.0\n");
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
		printf("Low-level device ready\n");
		have_low = true;
		devType = 1;
	}
	else {
		printf("No low-level devices found, trying high-level...\n");
	}

	switch (lfxUtil.InitLFX()) {
	case -1: printf("Dell API ready\n");
		have_high = true;
		if (!have_low) devType = 0;
		break;
	case 0: printf("Dell library DLL not found!\n"); break;
	case 1: printf("Can't access Dell library!\n"); break;
	case 2: printf("No high-level devices found!\n"); break;
	default: printf("Dell library unknown error!\n"); break;
	}

	if (devType == -1) {
		printf("Both low-level and high-level devices not found, exiting!\n");
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
				args.push_back(values.substr(vpos, tvpos - vpos));
				vpos = tvpos == string::npos ? values.size() : tvpos + 1;
			}
		}
		//printf("Executing " command " with " values);
		if (command == "low-level") {
			printf("Low-level device ");
			if (have_low) {
				devType = 1;
				printf("selected\n");
			}
			else
				printf("not found!\n");
			continue;
		}
		if (command == "high-level") {
			printf("High-level device ");
			if (have_high) {
				devType = 0;
				printf("selected\n");
			}
			else
				printf("not found!\n");
			continue;
		}
		if (command == "loop") {
			cc = 1;
			goto update;
		}
		if (command == "status") {
			switch (devType) {
			case 1:
				for (int i = 0; i < afx_map->fxdevs.size(); i++) {
					printf("Device #%d - %s, ", i, (afx_map->fxdevs[i].desc ? afx_map->fxdevs[i].desc->name.c_str() : "No name"));
					string typeName = "Unknown";
					switch (afx_map->fxdevs[i].dev->GetVersion()) {
					case 0: typeName = "Desktop"; break;
					case 1: case 2: case 3: case 4: typeName = "Notebook"; break;
					case 5: typeName = "Keyboard"; break;
					case 6: typeName = "Display"; break;
					case 7: typeName = "Mouse"; break;
					}

					printf("%s, VID#%d, PID#%d, APIv%d, %d lights %s\n", typeName.c_str(),
						afx_map->fxdevs[i].dev->GetVid(), afx_map->fxdevs[i].dev->GetPID(), afx_map->fxdevs[i].dev->GetVersion(),
						(int)afx_map->fxdevs[i].lights.size(),
						(cdev->GetPID() == afx_map->fxdevs[i].dev->GetPID()) ? "(Active)" : ""
					);

					for (int k = 0; k < afx_map->fxdevs[i].lights.size(); k++) {
						printf("  Light ID#%d - %s%s%s\n", afx_map->fxdevs[i].lights[k]->lightid,
							afx_map->fxdevs[i].lights[k]->name.c_str(),
							(afx_map->fxdevs[i].lights[k]->flags & ALIENFX_FLAG_POWER) ? " (Power button)" : "",
							(afx_map->fxdevs[i].lights[k]->flags & ALIENFX_FLAG_INDICATOR) ? " (Indicator)" : "");
					}
				}
				// now groups...
				if (afx_map->GetGroups()->size() > 0)
					printf("%d groups:\n", (int)afx_map->GetGroups()->size());
				for (int i = 0; i < afx_map->GetGroups()->size(); i++)
					printf("  Group #%d - %s (%d lights)\n", (afx_map->GetGroups()->at(i).gid & 0xffff),
						afx_map->GetGroups()->at(i).name.c_str(),
						(int)afx_map->GetGroups()->at(i).lights.size());
				break;
			case 0:
				lfxUtil.GetStatus(); break;
			}
			continue;
		}
		if (command == "probe") {
			int numlights = -1, devID = -1, lightID = -1;
			bool show_all = false;
			if (args.size()) {
				int pos = 1;
				show_all = args[0].find('a') != string::npos;
				if (args[0].find('l') != string::npos && args.size() > 1) {
					numlights = atoi(args[1].c_str());
					pos++;
				}
				if (args[0].find('d') != string::npos && args.size() > pos) {
					devID = atoi(args[pos].c_str());
					if (args.size() > pos+1)
						lightID = atoi(args[pos+1].c_str());
				}
			}
			char name[256]{ 0 };
			CheckDevices(show_all);
			printf("Do you want to set devices and lights names?");
			gets_s(name, 255);
			if (name[0] == 'y' || name[0] == 'Y') {
				printf("\nFor each light please enter LightFX SDK light ID or light name if ID is not available\n\
Tested light become green, and turned off after testing.\n\
Just press Enter if no visible light at this ID to skip it.\n");
				printf("Probing low-level access... ");

				//AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
				AlienFX_SDK::Functions* afx_dev = new AlienFX_SDK::Functions();
				vector<pair<WORD, WORD>> pids = afx_map->AlienFXEnumDevices();

				if (pids.size() > 0) {
					printf("Found %d device(s)\n", (int)pids.size());
					if (have_high) {
						lfxUtil.FillInfo();
						lfxUtil.GetStatus();
					}

					for (int cdev = 0; cdev < pids.size(); cdev++)
						if (devID == -1 || devID == pids[cdev].second) {
							printf("Probing device VID_%04x, PID_%04x...", pids[cdev].first, pids[cdev].second);
							int isInit = afx_dev->AlienFXChangeDevice(pids[cdev].first, pids[cdev].second);
							if (isInit != -1) {
								printf(" Connected.\n");
								int count;
								afx_dev->Reset();
								for (count = 0; count < 5 && !afx_dev->IsDeviceReady(); count++)
									Sleep(20);
								AlienFX_SDK::devmap* devs = afx_map->GetDeviceById(afx_dev->GetPID(), afx_dev->GetVid());
								if (devs)
									printf("Old device name is %s, ", devs->name.c_str());
								else {
									devs = new AlienFX_SDK::devmap{ pids[cdev].first,pids[cdev].second,"" };
									afx_map->GetDevices()->push_back(*devs);
									devs = &afx_map->GetDevices()->back();
								}
								printf("Enter device name or LightFX id: ");
								gets_s(name, 255);
								devs->name = isdigit(name[0]) && have_high ? lfxUtil.GetDevInfo(atoi(name))->desc
									: devs && name[0] == 0 ? devs->name : name;
								printf("Final device name is %s\n", devs->name.c_str());
								// How many lights to check?
								int fnumlights = numlights == -1 ? pids[cdev].first == 0x0d62 ? 0x88 : 23 : numlights;
								for (int i = 0; i < fnumlights; i++)
									if (lightID == -1 || lightID == i) {
										printf("Testing light #%d", i);
										AlienFX_SDK::mapping* lmap = afx_map->GetMappingById(afx_dev->GetPID(), i);
										if (lmap) {
											printf(", old name %s ", lmap->name.c_str());
										}
										printf("(ENTER for skip): ");
										afx_dev->SetColor(i, { 0, 255, 0 });
										afx_dev->UpdateColors();
										Sleep(100);
										gets_s(name, 255);
										if (name[0] != 0) {
											//not skipped
											if (lmap) {
												lmap->name = name;
											}
											else {
												lmap = new AlienFX_SDK::mapping({ pids[cdev].first, pids[cdev].second, (WORD)i, 0, name });
												afx_map->GetMappings()->push_back(lmap);
												lmap = afx_map->GetMappings()->back();
											}
											afx_map->SaveMappings();
											//}
											printf("Final name is %s, ", lmap->name.c_str());
										}
										else {
											printf("Skipped, ");
										}
										afx_dev->SetColor(i, { 0, 0, 255 });
										afx_dev->UpdateColors();
										//afx_dev->Reset();
										Sleep(100);
									}
								// now store config...
								afx_map->SaveMappings();
							}
							else {
								printf(" Device not initialized!\n");
							}
						}
					afx_dev->AlienFXClose();
					delete afx_dev;
				}
			}
			else {
				printf("AlienFX devices not present, please check device manage!\n");
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
			printf("Device #%d selected\n", cdev->GetPID());
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
				vector<byte> lights;
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					AlienFX_SDK::mapping *lgh = afx_map->GetMappings()->at(i);
					if (lgh->devid == cdev->GetPID() &&
						!(lgh->flags & ALIENFX_FLAG_POWER))
						lights.push_back((byte) lgh->lightid);
				}
				cdev->SetMultiLights(&lights, color);
			} break;
			case 0:
				lfxUtil.SetLFXColor(LFX_ALL, color.ci);
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
				AlienFX_SDK::group* grp = afx_map->GetGroupById(0x10000 + zoneCode);
				if (grp) {
					for (int j = 0; j < afx_map->fxdevs.size(); j++) {
						vector<UCHAR> lights;
						for (int i = 0; i < grp->lights.size(); i++) {
							if (grp->lights[i]->devid == afx_map->fxdevs[j].dev->GetPID())
								lights.push_back((UCHAR) grp->lights[i]->lightid);
						}
						afx_map->fxdevs[j].dev->SetMultiLights(&lights, color);
					}
				}
			} break;
			case 0:
			{
				lfxUtil.SetLFXColor(zoneCode, color.ci);
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
				AlienFX_SDK::group* grp = afx_map->GetGroupById(0x10000 + zoneCode);
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
				}
			} break;
			case 0:
				if (clrs.size() < 2)
					clrs.push_back({0});
				lfxUtil.SetLFXZoneAction(actionCode, zoneCode, clrs[0].ci, clrs[1].ci);
				break;
			}
			//goto update;
		} else
			printf("Unknown command %s\n", command.c_str());
update:
		if (have_low) {
			for (int i = 0; i < afx_map->fxdevs.size(); i++)
				afx_map->fxdevs[i].dev->UpdateColors();
		}
		if (have_high) {
			lfxUtil.Update();
		}
	}
	printf("Done.");

deinit:
	if (have_high)
		lfxUtil.Release();
	delete afx_map;
    return 0;
}

