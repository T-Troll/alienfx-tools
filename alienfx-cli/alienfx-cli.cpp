#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string>
#include "LFXUtil.h"
#include "LFXDecl.h"
#include "AlienFX_SDK.h"
#include "Consts.h"

namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

using namespace std;

extern void CheckDevices(bool);

unsigned GetZoneCode(ARG name, int mode) {
	switch (mode) {
	case 1: return name.num | 0x10000;
	case 0:
		for (int i = 0; i < ARRAYSIZE(zonecodes); i++)
			if (name.str == zonecodes[i].name)
				return zonecodes[i].code;
	}
	return LFX_ALL;
}

unsigned GetActionCode(ARG name, int mode) {
	for (int i = 0; i < ARRAYSIZE(actioncodes); i++)
		if (name.str == actioncodes[i].name)
			return mode ? actioncodes[i].afx_code : actioncodes[i].dell_code;
	return mode ? AlienFX_SDK::Action::AlienFX_A_Color : LFX_ACTION_COLOR;
}

void SetBrighness(AlienFX_SDK::Colorcode *color) {
	color->r = ((unsigned) color->r * color->br) / 255;// >> 8;
	color->g = ((unsigned) color->g * color->br) / 255;// >> 8;
	color->b = ((unsigned) color->b * color->br) / 255;// >> 8;
}

void printUsage()
{
	printf("Usage: alienfx-cli [command=option,option,option] ... [loop]\nCommands:\tOptions:\n");
	for (int i = 0; i < ARRAYSIZE(commands); i++)
		printf("%s\t%s.\n", commands[i].name, commands[i].desc);
	printf("\nProbe argument (can be combined):\n\
\ta - All devices info\n\
\tl - Define number of lights\n\
\td - DeviceID and optionally LightID.\n\
Zones:\tleft, right, top, bottom, front, rear (high-level) or ID of the Group (low-level).\n\
Actions:color (disable action), pulse, morph (you need 2 colors),\n\
\t(only for low-level v4) breath, spectrum, rainbow (up to 9 colors each).");
}

int CheckCommand(string name, int args) {
	// find command id, return -1 if wrong.
	for (int i = 0; i < ARRAYSIZE(commands); i++) {
		if (name == commands[i].name) {
			if (commands[i].minArgs > args) {
				printf("%s: Incorrect number of arguments (at least %d needed)\n", commands[i].name, commands[i].minArgs);
				return -1;
			}
			else
				return i;
		}
	}
	return -2;
}

AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
AlienFX_SDK::Functions *cdev = NULL;

bool have_low = false, have_high = false;

void Update() {
	if (have_low) {
		for (int i = 0; i < afx_map->fxdevs.size(); i++)
			afx_map->fxdevs[i].dev->UpdateColors();
	}
	if (have_high) {
		lfxUtil.Update();
	}
}

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

int main(int argc, char* argv[])
{
	int devType = -1;
	UINT sleepy = 0;

	printf("alienfx-cli v5.9.4.3\n");
	if (argc < 2)
	{
		printUsage();
		return 1;
	}

	afx_map->LoadMappings();
	afx_map->AlienFXAssignDevices();

	if (have_high = (lfxUtil.InitLFX() == -1)) {
		printf("Dell API ready, ");
		devType = 0;
	} else
		printf("Dell API not found, ");

	if (have_low = (afx_map->fxdevs.size() > 0)) {
		cdev = afx_map->fxdevs[0].dev;
		for (int rcount = 0; rcount < 10 && !cdev->IsDeviceReady(); rcount++)
			Sleep(20);
		if (!cdev->IsDeviceReady()) {
			cdev->Reset();
		}
		printf("Low-level device ready.\n");
		devType = 1;
	}
	else {
		printf("Low-level not found.\n");
	}

	if (devType == -1) {
		printf("Both low-level and high-level devices not found, exiting!\n");
		goto deinit;
	}

	for (int cc = 1; cc < argc; cc++) {

		string arg = string(argv[cc]);
		size_t vid = arg.find_first_of('=');
		string command = arg.substr(0, vid);
		vector<ARG> args;
		while (vid != string::npos) {
			size_t argPos = arg.find(',', vid + 1);
			if (argPos == string::npos)
				// last one...
				args.push_back({ 0, arg.substr(vid + 1, arg.length() - vid - 1) });
			else
				args.push_back({ 0, arg.substr(vid + 1, argPos - vid - 1) });
			args.back().num = atoi(args.back().str.c_str());
			vid = argPos;
		}
		//printf("Executing " command " with " values);
		switch (CheckCommand(command, (int)args.size())) {
		case 0: {
			static AlienFX_SDK::Colorcode color{
					(byte)args[2].num,
					(byte)args[1].num,
					(byte)args[0].num,
					(byte)(args.size() > 3 ? args[3].num : 255) };
			if (devType) {
				SetBrighness(&color);
				vector<byte> lights;
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					AlienFX_SDK::mapping* lgh = afx_map->GetMappings()->at(i);
					if (lgh->devid == cdev->GetPID() &&
						!(lgh->flags & ALIENFX_FLAG_POWER))
						lights.push_back((byte)lgh->lightid);
				}
				cdev->SetMultiLights(&lights, color);
			}
			else {
				lfxUtil.SetLFXColor(LFX_ALL, color.ci);
			}
			Update();
		} break;
		case 1: {
			AlienFX_SDK::Colorcode color{ (byte)args[4].num,
				(byte)args[3].num,
				(byte)args[2].num,
				(byte)(args.size() > 5 ? args[5].num : 255) };
			if (devType) {
				SetBrighness(&color);
				if (args[0].num) {
					FindDevice(args[0].num);
				}
				cdev->SetColor(args[1].num, color);
			}
			else {
				lfxUtil.SetOneLFXColor(args[0].num, args[1].num, (unsigned*)&color.ci);
			}
			Update();
		} break;
		case 2: {
			AlienFX_SDK::Colorcode color{ (byte)args[3].num, (byte)args[2].num, (byte)args[1].num,
										(byte)(args.size() > 4 ? args[4].num : 255) };
			unsigned zoneCode = GetZoneCode(args[0], devType);
			if (devType) {
				SetBrighness(&color);
				AlienFX_SDK::group* grp = afx_map->GetGroupById(zoneCode);
				if (grp) {
					for (int j = 0; j < afx_map->fxdevs.size(); j++) {
						vector<UCHAR> lights;
						for (int i = 0; i < grp->lights.size(); i++) {
							if (grp->lights[i].first == afx_map->fxdevs[j].dev->GetPID())
								lights.push_back((UCHAR)grp->lights[i].second);
						}
						afx_map->fxdevs[j].dev->SetMultiLights(&lights, color);
					}
				}
			}
			else {
				lfxUtil.SetLFXColor(zoneCode, color.ci);
			}
			Update();
		} break;
		case 3: {
			unsigned actionCode = LFX_ACTION_COLOR;
			vector<AlienFX_SDK::Colorcode> clrs;
			AlienFX_SDK::act_block act{ (byte)args[1].num };
			for (int argPos = 2; argPos + 4 < args.size(); argPos += 5) {
				actionCode = GetActionCode(args[argPos], devType);
				AlienFX_SDK::Colorcode c{ (byte)args[argPos + 1].num, (byte)args[argPos + 2].num, (byte)args[argPos + 3].num,
										(byte)(argPos + 4 < args.size() ? args[argPos + 4].num : 255) };
				if (devType) {
					SetBrighness(&c);
					act.act.push_back(AlienFX_SDK::afx_act({ (BYTE)actionCode, (BYTE)sleepy, 7, (BYTE)c.b, (BYTE)c.g, (BYTE)c.r }));
				}
				else
					clrs.push_back(c);
				//argPos += 5;
			}
			if (devType) {
				if (args[0].num) FindDevice(args[0].num);
				if (act.act.size() < 2) {
					act.act.push_back(AlienFX_SDK::afx_act({ (BYTE)actionCode, (BYTE)sleepy, 7, 0, 0, 0 }));
				}
				cdev->SetAction(&act);
			}
			else {
				if (clrs.size() < 2) {
					clrs.push_back({ 0 });
				}
				lfxUtil.SetLFXAction(actionCode, args[0].num, args[1].num, (unsigned*)&clrs[0].ci, (unsigned*)&clrs[1].ci);
			}
			Update();
		} break;
		case 4: {
			unsigned zoneCode = GetZoneCode(args[0], devType);
			unsigned actionCode = GetActionCode(args[1], devType);
			AlienFX_SDK::act_block act;
			vector<AlienFX_SDK::Colorcode> clrs;
			//int argPos = 1;
			for (int argPos = 1; argPos + 4 < args.size(); argPos += 5) {
				AlienFX_SDK::Colorcode c{ (byte)args[argPos + 3].num, (byte)args[argPos + 2].num, (byte)args[argPos + 1].num,
										(byte)(argPos + 4 < args.size() ? args[argPos + 4].num : 255) };
				if (devType) {
					SetBrighness(&c);
					act.act.push_back(AlienFX_SDK::afx_act({ (BYTE)GetActionCode(args[argPos], devType), (BYTE)sleepy, 7, (BYTE)c.r, (BYTE)c.g, (BYTE)c.b }));
				}
				else
					clrs.push_back(c);
				//argPos += 5;
			}
			if (devType) {
				AlienFX_SDK::group* grp = afx_map->GetGroupById(zoneCode);
				if (act.act.size() < 2) {
					act.act.push_back(AlienFX_SDK::afx_act({ (BYTE)actionCode, (BYTE)sleepy, 7, 0, 0, 0 }));
				}
				for (int j = 0; j < afx_map->fxdevs.size(); j++) {
					for (int i = 0; i < (grp ? grp->lights.size() : afx_map->GetMappings()->size()); i++) {
						if (grp ? grp->lights[i].first : (*afx_map->GetMappings())[i]->devid
							== afx_map->fxdevs[j].dev->GetPID()) {
							act.index = (byte)(grp ? grp->lights[i].second : (*afx_map->GetMappings())[i]->lightid);
							afx_map->fxdevs[j].dev->SetAction(&act);
						}
					}
				}
			}
			else {
				if (clrs.size() < 2)
					clrs.push_back({ 0 });
				lfxUtil.SetLFXZoneAction(actionCode, zoneCode, clrs[0].ci, clrs[1].ci);
			}
		} break;
		case 5:
			if (devType) {
				vector<AlienFX_SDK::act_block> act{ {(byte)args[0].num,
								  {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
								  (byte)args[1].num, (byte)args[2].num, (byte)args[3].num},
								  {AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
								  (byte)args[4].num, (byte)args[5].num, (byte)args[6].num}}} };
				cdev->SetPowerAction(&act);
			}
			break;
		case 6:
			sleepy = args[0].num;
			if (!devType) {
				lfxUtil.SetTempo(args[0].num);
				Update();
			}
			break;
		case 7:
			// set-dev
			FindDevice(args[0].num);
			printf("Device #%d selected\n", cdev->GetPID());
			break;
		case 8:
			// set-dim
			if (devType)
				cdev->ToggleState(args[0].num, afx_map->GetMappings(), false);
			break;
		case 9:
			// set-global
			if (devType)
				cdev->SetGlobalEffects(args[0].num, sleepy,
					{ 0,0,0, (byte)args[1].num, (byte)args[2].num, (byte)args[3].num },
					{ 0,0,0, (byte)args[4].num, (byte)args[5].num, (byte)args[6].num });
			break;
		case 10:
			// low-level
			printf("Low-level device ");
			if (have_low) {
				devType = 1;
				printf("selected\n");
			}
			else
				printf("not found!\n");
			break;
		case 11:
			// high-level
			printf("High-level device ");
			if (have_high) {
				devType = 0;
				printf("selected\n");
			}
			else
				printf("not found!\n");
			break;
		case 13:
			if (devType) {
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
			}
			else {
				lfxUtil.GetStatus();
			}
			break;
		case 12: {
			int numlights = -1, devID = -1, lightID = -1;
			bool show_all = false;
			if (args.size()) {
				int pos = 1;
				show_all = args[0].str.find('a') != string::npos;
				if (args[0].str.find('l') != string::npos && args.size() > 1) {
					numlights = args[1].num;
					pos++;
				}
				if (args[0].str.find('d') != string::npos && args.size() > pos) {
					devID = args[pos].num;
					if (args.size() > pos + 1)
						lightID = args[pos + 1].num;
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
		} break;
		case 14:
			if (devType)
				cdev->ToggleState(255, afx_map->GetMappings(), false);
			break;
		case 15:
			if (devType)
				cdev->ToggleState(0, afx_map->GetMappings(), false);
			break;
		case 16:
			if (devType)
				cdev->Reset();
			else
				lfxUtil.Reset();
			break;
		case 17:
			cc = 1;
			Update();
			break;
		case -2:
			printf("Unknown command %s\n", command.c_str());
			break;
		}
	}
	printf("Done.");

deinit:
	if (have_high)
		lfxUtil.Release();
	delete afx_map;
    return 0;
}

