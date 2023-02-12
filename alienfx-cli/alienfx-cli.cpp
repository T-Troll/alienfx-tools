#define WIN32_LEAN_AND_MEAN
#ifdef _DEBUG
#define NOACPILIGHTS
#endif
#include <stdio.h>
#include <string>
#include "LFXUtil.h"
#include "LFXDecl.h"
#include "AlienFX_SDK.h"
#include "Consts.h"
#include <SetupAPI.h>
#include <memory>
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

using namespace std;

AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
bool have_high = false;
byte globalBright = 255;
byte sleepy = 5, longer = 5;

string GetDeviceType(int version) {
	switch (version) {
	case 0: return "Desktop";
	//case 1: case 2: case 3: return "Notebook";
	case 4: return "Desktop/Notebook";
	case 5: case 8: return "Keyboard";
	case 6: return "Display";
	case 7: return "Mouse";
	default: return "Notebook";
	}
	//return "Unknown";
}

void CheckDevices() {

	for (auto i = afx_map->fxdevs.begin(); i != afx_map->fxdevs.end(); i++) {
		printf("===== Device VID_%04x, PID_%04x =====\n", i->vid, i->pid);
		//printf("Command size %d\n", i->dev->GetLength());
		printf("+++++ Detected as: ");
		switch (i->vid) {
		case 0x187c: printf("Alienware,"); break;
		case 0x0d62: printf("Darfon,"); break;
		case 0x0424: printf("Microchip,"); break;
		case 0x0461: printf("Primax,"); break;
		case 0x04f2: printf("Chicony,"); break;
		//default: printf("Unknown,");
		}
		//if (i->dev) {
			printf(" %s,", GetDeviceType(i->dev->GetVersion()).c_str());
			printf(" APIv%d +++++\n", i->dev->GetVersion() );
		//} else
		//	printf(" Inactive +++++\n");

	}
}

unsigned GetZoneCode(ARG name, int mode) {
	if (mode)
		return name.num | 0x10000;
	else
		for (int i = 0; i < ARRAYSIZE(zonecodes); i++)
			if (name.str == zonecodes[i].name)
				return zonecodes[i].code;
	return LFX_ALL;
}

unsigned GetActionCode(ARG name, int mode) {
	for (int i = 0; i < ARRAYSIZE(actioncodes); i++)
		if (name.str == actioncodes[i].name)
			return mode ? actioncodes[i].afx_code : actioncodes[i].dell_code;
	return mode ? AlienFX_SDK::Action::AlienFX_A_Color : LFX_ACTION_COLOR;
}

void SetBrighness(AlienFX_SDK::Afx_action *color) {
	color->r = ((unsigned) color->r * globalBright) / 255;// >> 8;
	color->g = ((unsigned) color->g * globalBright) / 255;// >> 8;
	color->b = ((unsigned) color->b * globalBright) / 255;// >> 8;
}

void printUsage()
{
	printf("Usage: alienfx-cli [command=option,option,option] ... [loop]\nCommands:\tOptions:\n");
	for (int i = 0; i < ARRAYSIZE(commands); i++)
		printf("%s\t%s.\n", commands[i].name, commands[i].desc);
	printf("\nProbe argument (can be combined):\n\
\tl - Define number of lights\n\
\td - DeviceID and optionally LightID.\n\
Zones:\tleft, right, top, bottom, front, rear, all (high-level) or ID of the Group (low-level).\n\
Actions:color (disable action), pulse, morph,\n\
\tbreath, spectrum, rainbow (low-level only).\n\
\tUp to 9 colors can be entered.");
}

int CheckCommand(string name, int args) {
	// find command id, return -1 if wrong.
	for (int i = 0; i < ARRAYSIZE(commands); i++) {
		if (name == commands[i].name) {
			if (commands[i].minArgs > args) {
				printf("%s: Incorrect arguments count (at least %d needed)\n", commands[i].name, commands[i].minArgs);
				return -1;
			}
			else
				return commands[i].id;
		}
	}
	return -2;
}

void Update() {
	for (int i = 0; i < afx_map->fxdevs.size(); i++)
		afx_map->fxdevs[i].dev->UpdateColors();
	if (have_high)
		lfxUtil.Update();
}

AlienFX_SDK::Afx_colorcode Act2Code(AlienFX_SDK::Afx_action* act) {
	return { act->b,act->g,act->r, 255 };
}

LFX_COLOR Act2Lfx(AlienFX_SDK::Afx_action* act) {
	return { act->r,act->g,act->b, 255 };
}

AlienFX_SDK::Afx_action ParseRGB(vector<ARG>* args, int from) {
	AlienFX_SDK::Afx_action cur{ 0 };
	if (args->size() > from + 2) {
		cur = { 0, sleepy, longer, (byte)args->at(from).num, (byte)args->at(from + 1).num, (byte)args->at(from+2).num };
		SetBrighness(&cur);
	}
	return cur;
}

vector<AlienFX_SDK::Afx_action>* ParseActions(vector<ARG>* args, int startPos) {
	vector<AlienFX_SDK::Afx_action>* actions = new vector<AlienFX_SDK::Afx_action>;
	for (int argPos = startPos; argPos + 3 < args->size(); argPos += 4) {
		actions->push_back(ParseRGB(args, argPos + 1));
		actions->back().type = GetActionCode(args->at(argPos), 1);
	}
	return actions;
}

int main(int argc, char* argv[])
{
	int devType = -1;

	printf("alienfx-cli v8.1.0\n");
	if (argc < 2)
	{
		printUsage();
		return 0;
	}

	afx_map->LoadMappings();
	afx_map->AlienFXAssignDevices();

	if (have_high = (lfxUtil.InitLFX() == -1)) {
		printf("Dell API ready");
		devType = 0;
	} else
		printf("Dell API not found");

	if (afx_map->activeDevices) {
		printf(", %d devices found.\n", afx_map->activeDevices);
		devType = 1;
	} else
		printf(", devices not found.\n");

	if (devType >= 0) {

		for (int cc = 1; cc < argc; cc++) {

			string arg = string(argv[cc]);
			size_t vid = arg.find_first_of('=');
			string command = arg.substr(0, vid);
			vector<ARG> args;
			while (vid != string::npos) {
				size_t argPos = arg.find(',', vid + 1);
				args.push_back({ arg.substr(vid + 1, (argPos == string::npos ? arg.length() : argPos) - vid - 1) });
				args.back().num = atoi(args.back().str.c_str());
				vid = argPos;
			}
			switch (CheckCommand(command, (int)args.size())) {
			case COMMANDS::setall: {
				//AlienFX_SDK::Afx_colorcode color = ParseRGB(&args, 0);
				if (devType) {
					for (auto cd = afx_map->fxdevs.begin(); cd != afx_map->fxdevs.end(); cd++) {
						vector<byte> lights;
						for (auto i = cd->lights.begin(); i != cd->lights.end(); i++) {
							if (!(i->flags & ALIENFX_FLAG_POWER))
								lights.push_back((byte)i->lightid);
						}
						cd->dev->SetMultiColor(&lights, ParseRGB(&args, 0));
					}
				}
				else {
					lfxUtil.SetLFXColor(LFX_ALL, Act2Code(&ParseRGB(&args, 0)).ci);
				}
				Update();
			} break;
			case COMMANDS::setone: {
				if (devType) {
					if (args[0].num < afx_map->fxdevs.size())
						afx_map->fxdevs[args[0].num].dev->SetAction(args[1].num, &vector<AlienFX_SDK::Afx_action>{ ParseRGB(&args, 2) });
				}
				else {
					lfxUtil.SetOneLFXColor(args[0].num, args[1].num, &Act2Lfx(&ParseRGB(&args, 2)));
				}
				Update();
			} break;
			case COMMANDS::setzone: {
				unsigned zoneCode = GetZoneCode(args[0], devType);
				if (devType) {
					AlienFX_SDK::Afx_group* grp = afx_map->GetGroupById(zoneCode);
					if (grp) {
						for (auto j = afx_map->fxdevs.begin(); j != afx_map->fxdevs.end(); j++) {
							vector<UCHAR> lights;
							for (auto i = grp->lights.begin(); i != grp->lights.end(); i++) {
								if (i->did == j->pid)
									lights.push_back((byte)i->lid);
							}
							j->dev->SetMultiColor(&lights, ParseRGB(&args, 1));
						}
					}
				}
				else {
					lfxUtil.SetLFXColor(zoneCode, Act2Code(&ParseRGB(&args, 1)).ci);
				}
				Update();
			} break;
			case COMMANDS::setaction: {
				unsigned actionCode = GetActionCode(args[2], devType);
				vector<AlienFX_SDK::Afx_action>* act = ParseActions(&args, 2);
				if (act->size() < 2) {
					act->push_back(AlienFX_SDK::Afx_action({ (BYTE)actionCode, (BYTE)sleepy, longer, 0, 0, 0 }));
				}
				if (devType) {
					if (args[0].num < afx_map->fxdevs.size())
						afx_map->fxdevs[args[0].num].dev->SetAction(args[1].num, act);
				}
				else {
					lfxUtil.SetLFXAction(actionCode, args[0].num, args[1].num,
						&Act2Lfx(&act->front()), &Act2Lfx(&act->back()));
				}
				delete act;
				Update();
			} break;
			case COMMANDS::setzoneact: {
				unsigned zoneCode = GetZoneCode(args[0], devType);
				unsigned actionCode = GetActionCode(args[1], devType);
				vector<AlienFX_SDK::Afx_action>* act = ParseActions(&args, 1);
				if (act->size() < 2) {
					act->push_back(AlienFX_SDK::Afx_action({ (BYTE)actionCode, sleepy, longer, 0, 0, 0 }));
				}
				if (devType) {
					AlienFX_SDK::Afx_group* grp = afx_map->GetGroupById(zoneCode);
					if (grp) {
						AlienFX_SDK::Afx_device* dev;
						for (auto i = grp->lights.begin(); i != grp->lights.end(); i++) {
							if (dev = afx_map->GetDeviceById(i->did)) {
								dev->dev->SetAction((byte)i->lid, act);
							}
						}
					}
				}
				else {
					lfxUtil.SetLFXZoneAction(actionCode, zoneCode, Act2Code(&act->front()).ci, Act2Code(&act->back()).ci);
				}
				delete act;
				Update();
			} break;
			case COMMANDS::setpower:
				if (devType) {
					vector<AlienFX_SDK::Afx_lightblock> act{ {(byte)args[1].num,
									  {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
									  (byte)args[2].num, (byte)args[3].num, (byte)args[4].num},
									  {AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
									  (byte)args[5].num, (byte)args[6].num, (byte)args[7].num}}} };
					afx_map->fxdevs[args[0].num].dev->SetPowerAction(&act);
				}
				break;
			case COMMANDS::settempo:
				sleepy = args[0].num;
				if (args.size() > 1)
					longer = args[1].num;
				if (!devType) {
					lfxUtil.SetTempo(sleepy);
					lfxUtil.Update();;
				}
				sleepy /= 50;
				break;
			case COMMANDS::setdim:
				// set-dim
				if (devType && args.size() > 1 && args[0].num < afx_map->fxdevs.size())
					afx_map->fxdevs[args[0].num].dev->ToggleState(args[1].num, &afx_map->fxdevs[args[0].num].lights, false);
				else
					globalBright = args.back().num;
				break;
			case COMMANDS::setglobal:
				// set-global
				if (devType && args[0].num < afx_map->fxdevs.size())
					afx_map->fxdevs[args[0].num].dev->SetGlobalEffects(args[1].num, args[2].num, sleepy,
						{ 0,0,0, (byte)args[3].num, (byte)args[4].num, (byte)args[5].num },
						{ 0,0,0, (byte)args[6].num, (byte)args[7].num, (byte)args[8].num });
				break;
			case COMMANDS::lowlevel:
				// low-level
				if (afx_map->fxdevs.size()) {
					devType = 1;
					printf("Device access selected\n");
				}
				break;
			case COMMANDS::highlevel:
				// high-level
				if (have_high) {
					devType = 0;
					printf("Dell API selected\n");
				}
				break;
			case COMMANDS::status:
				// status
				if (devType) {
					for (auto i = afx_map->fxdevs.begin(); i < afx_map->fxdevs.end(); i++) {
						printf("Device #%d - %s, %s, VID#%d, PID#%d, APIv%d, %d lights\n", (int)(i - afx_map->fxdevs.begin()), i->name.c_str(),
							GetDeviceType(i->dev->GetVersion()).c_str(), i->vid, i->pid, i->dev->GetVersion(), (int)i->lights.size());

						for (int k = 0; k < i->lights.size(); k++) {
							printf("  Light ID#%d - %s%s%s\n", i->lights[k].lightid,
								i->lights[k].name.c_str(),
								(i->lights[k].flags & ALIENFX_FLAG_POWER) ? " (Power button)" : "",
								(i->lights[k].flags & ALIENFX_FLAG_INDICATOR) ? " (Indicator)" : "");
						}
					}
					// now groups...
					if (afx_map->GetGroups()->size() > 0)
						printf("%d groups:\n", (int)afx_map->GetGroups()->size());
					for (int i = 0; i < afx_map->GetGroups()->size(); i++)
						printf("  Group #%d (%d lights) - %s\n", (afx_map->GetGroups()->at(i).gid & 0xffff),
							(int)afx_map->GetGroups()->at(i).lights.size(),
							afx_map->GetGroups()->at(i).name.c_str());
				}
				else {
					lfxUtil.GetStatus();
				}
				break;
			case COMMANDS::probe: {
				// probe
				int numlights = -1, devID = -1, lightID = -1;
				if (args.size()) {
					int pos = 1;
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
				CheckDevices();
				printf("Do you want to set devices and lights names?");
				gets_s(name, 255);
				if (name[0] == 'y' || name[0] == 'Y') {
					printf("\nFor each light please enter LightFX SDK light ID or light name if ID is not available\n\
Tested light become green, and turned off after testing.\n\
Just press Enter if no visible light at this ID to skip it.\n");
					if (afx_map->fxdevs.size() > 0) {
						printf("Found %d device(s)\n", (int)afx_map->fxdevs.size());
						if (have_high) {
							lfxUtil.FillInfo();
							lfxUtil.GetStatus();
						}

						for (auto cDev = afx_map->fxdevs.begin(); cDev != afx_map->fxdevs.end(); cDev++) {
							printf("Probing device VID_%04x, PID_%04x...", cDev->vid, cDev->pid);
							cDev->dev->Reset();
							printf("Old device name is %s, ", cDev->name.c_str());
							printf("Enter device name or LightFX id: ");
							gets_s(name, 255);
							cDev->name = isdigit(name[0]) && have_high ? lfxUtil.GetDevInfo(atoi(name))->desc
								: name[0] == 0 ? cDev->name : name;
							printf("Final device name is %s\n", cDev->name.c_str());
							// How many lights to check?
							int fnumlights = numlights == -1 ? cDev->vid == 0x0d62 ? 0x88 : 23 : numlights;
							for (byte i = 0; i < fnumlights; i++)
								if (lightID == -1 || lightID == i) {
									printf("Testing light #%d", i);
									AlienFX_SDK::Afx_light* lmap = afx_map->GetMappingByDev(&(*cDev), i);
									if (lmap) {
										printf(", old name %s ", lmap->name.c_str());
									}
									printf("(ENTER for skip): ");
									cDev->dev->SetAction(i, &vector<AlienFX_SDK::Afx_action>{ { 0, 0, 0, 0, 255, 0 }});
									cDev->dev->UpdateColors();
									Sleep(100);
									gets_s(name, 255);
									if (name[0] != 0) {
										//not skipped
										if (lmap) {
											lmap->name = name;
										}
										else {
											cDev->lights.push_back({ i, {0,0}, name });
										}
										afx_map->SaveMappings();
										printf("Final name is %s, ", name);
									}
									else {
										printf("Skipped, ");
									}
									cDev->dev->SetAction(i, &vector<AlienFX_SDK::Afx_action>{ { 0, 0, 0, 255, 0, 0 }});
									cDev->dev->UpdateColors();
									Sleep(100);
									afx_map->SaveMappings();
								}
						}
					}
				}
			} break;
			case COMMANDS::loop:
				cc = 1;
				break;
			case -2:
				printf("Unknown command %s\n", command.c_str());
				break;
			}
		}
		printf("Done.");
	} else
		printf("Both low-level and high-level devices not found, exiting!\n");

	if (have_high)
		lfxUtil.Release();
	delete afx_map;
    return 0;
}

