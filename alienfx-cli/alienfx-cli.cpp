#define WIN32_LEAN_AND_MEAN
#ifdef _DEBUG
#define NOACPILIGHTS
#endif
#include <stdio.h>
#include <string>
#include "LFXUtil.h"
#include "AlienFX_SDK.h"
#include "Consts.h"

namespace
{
	LFXUtil::LFXUtilC lfxUtil;
}

using namespace std;

static AlienFX_SDK::Mappings afx_map;
bool have_high = false;
byte globalBright = 255;
byte sleepy = 5, longer = 5;
int devType = -1;

unsigned GetZoneCode(ARG name) {
	if (devType)
		return name.num | 0x10000;
	else
	{
		for (int i = 0; i < ARRAYSIZE(zonecodes); i++)
			if (name.str == zonecodes[i].name)
				return zonecodes[i].code;
		return LFX_ALL;
	}
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
\tUp to 9 colors can be entered.\n\
Modes: 1 - global, 2 - keypress.");
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
	if (have_high)
		lfxUtil.Update();
	else
		for (int i = 0; i < afx_map.fxdevs.size(); i++)
			afx_map.fxdevs[i].dev->UpdateColors();
}

AlienFX_SDK::Afx_colorcode Act2Code(AlienFX_SDK::Afx_action* act) {
	return { act->b,act->g,act->r, 255 };
}

LFX_COLOR Act2Lfx(AlienFX_SDK::Afx_action* act) {
	return { act->r,act->g,act->b, globalBright };
}

vector<AlienFX_SDK::Afx_action> ParseActions(vector<ARG>* args, int startPos) {
	vector<AlienFX_SDK::Afx_action> actions;
	for (int argPos = startPos; argPos + 2 < args->size(); argPos += 3) {
		byte acttype = devType ? AlienFX_SDK::Action::AlienFX_A_Color : LFX_ACTION_COLOR;
		if (argPos + 3 < args->size()) { // action
			for (int i = 0; i < ARRAYSIZE(actioncodes); i++)
				if (args->at(argPos).str == actioncodes[i].name) {
					acttype = devType ? actioncodes[i].afx_code : actioncodes[i].dell_code;
					break;
				}
			argPos++;
		}
		actions.push_back({ acttype, sleepy, longer, 
				(byte)((args->at(argPos).num * globalBright) / 255), //r
			(byte)((args->at(argPos + 1).num * globalBright) / 255), //g
			(byte)((args->at(argPos + 2).num * globalBright) / 255) //b
			});
		//AlienFX_SDK::Afx_action* color = &actions.back();
		//color->r = ((unsigned)color->r * globalBright) / 255;// >> 8;
		//color->g = ((unsigned)color->g * globalBright) / 255;// >> 8;
		//color->b = ((unsigned)color->b * globalBright) / 255;// >> 8;
	}
	if (actions.size() < 2 && actions.front().type != (devType ? AlienFX_SDK::Action::AlienFX_A_Color : LFX_ACTION_COLOR))
		actions.push_back({ actions.front().type, (BYTE)sleepy, longer, 0, 0, 0 });
	return actions;
}

int main(int argc, char* argv[])
{
	printf("alienfx-cli v9.0.1\n");
	if (argc < 2)
	{
		printUsage();
		return 0;
	}

	afx_map.LoadMappings();
	afx_map.AlienFXAssignDevices();

	if (have_high = (lfxUtil.InitLFX() == -1)) {
		printf("Dell API ready");
		devType = 0;
	} else
		printf("Dell API not found");

	if (afx_map.activeDevices) {
		printf(", %d devices found.\n", afx_map.activeDevices);
		devType = 1;
		// set brightness
		for (auto dev = afx_map.fxdevs.begin(); dev != afx_map.fxdevs.end(); dev++)
			dev->dev->SetBrightness(255, &dev->lights, true);
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
				if (devType) {
					for (auto cd = afx_map.fxdevs.begin(); cd != afx_map.fxdevs.end(); cd++) {
						vector<byte> lights;
						for (auto i = cd->lights.begin(); i != cd->lights.end(); i++) {
							if (!(i->flags & ALIENFX_FLAG_POWER))
								lights.push_back((byte)i->lightid);
						}
						cd->dev->SetMultiColor(&lights, ParseActions(&args, 0).front());
					}
				}
				else {
					lfxUtil.SetLFXColor(LFX_ALL, Act2Code(&ParseActions(&args, 0).front()).ci);
				}
				Update();
			} break;
			case COMMANDS::setone: {
				if (devType) {
					if (args[0].num < afx_map.fxdevs.size()) {
						AlienFX_SDK::Afx_lightblock act{ (byte)args[1].num, ParseActions(&args, 2) };
						afx_map.fxdevs[args[0].num].dev->SetAction(&act);
					}
				}
				else {
					lfxUtil.SetOneLFXColor(args[0].num, args[1].num, &Act2Lfx(&ParseActions(&args, 2).front()));
				}
				Update();
			} break;
			case COMMANDS::setzone: {
				unsigned zoneCode = GetZoneCode(args[0]);
				if (devType) {
					AlienFX_SDK::Afx_group* grp = afx_map.GetGroupById(zoneCode);
					if (grp) {
						for (auto j = afx_map.fxdevs.begin(); j != afx_map.fxdevs.end(); j++) {
							vector<UCHAR> lights;
							for (auto i = grp->lights.begin(); i != grp->lights.end(); i++) {
								if (i->did == j->pid)
									lights.push_back((byte)i->lid);
							}
							j->dev->SetMultiColor(&lights, ParseActions(&args, 1).front());
						}
					}
				}
				else {
					lfxUtil.SetLFXColor(zoneCode, Act2Code(&ParseActions(&args, 1).front()).ci);
				}
				Update();
			} break;
			case COMMANDS::setaction: {
				AlienFX_SDK::Afx_lightblock act{ (byte)args[1].num, ParseActions(&args, 2) };
				if (devType) {
					if (args[0].num < afx_map.fxdevs.size())
						afx_map.fxdevs[args[0].num].dev->SetAction(&act);
				}
				else {
					lfxUtil.SetLFXAction(act.act.front().type, args[0].num, act.index,
						&Act2Lfx(&act.act.front()), &Act2Lfx(&act.act.back()));
				}
				//delete act;
				Update();
			} break;
			case COMMANDS::setzoneact: {
				unsigned zoneCode = GetZoneCode(args[0]);
				AlienFX_SDK::Afx_lightblock act = { 0, ParseActions(&args, 1) };
				if (devType) {
					AlienFX_SDK::Afx_group* grp = afx_map.GetGroupById(zoneCode);
					if (grp) {
						AlienFX_SDK::Afx_device* dev;
						for (auto i = grp->lights.begin(); i != grp->lights.end(); i++) {
							if (dev = afx_map.GetDeviceById(i->did)) {
								act.index = (byte)i->lid;
								dev->dev->SetAction(&act);
							}
						}
					}
				}
				else {
					lfxUtil.SetLFXZoneAction(act.act.front().type, zoneCode, Act2Code(&act.act.front()).ci, Act2Code(&act.act.back()).ci);
				}
				//delete act;
				Update();
			} break;
			case COMMANDS::setpower:
				if (devType) {
					vector<AlienFX_SDK::Afx_lightblock> act{ {(byte)args[1].num,
									  {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
									  (byte)args[2].num, (byte)args[3].num, (byte)args[4].num},
									  {AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
									  (byte)args[5].num, (byte)args[6].num, (byte)args[7].num}}} };
					afx_map.fxdevs[args[0].num].dev->SetPowerAction(&act, true);
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
				if (devType && args.size() > 1 && args[0].num < afx_map.fxdevs.size())
					afx_map.fxdevs[args.front().num].dev->SetBrightness(args.back().num, &afx_map.fxdevs[args.front().num].lights, false);
				else
					globalBright = args.front().num;
				break;
			case COMMANDS::setglobal:
				// set-global
				if (devType && args[0].num < afx_map.fxdevs.size()) {
					byte cmode = args.size() < 6 ? 3 : args.size() < 9 ? 1 : 2;
					args.resize(9);
					afx_map.fxdevs[args[0].num].dev->SetGlobalEffects(args[1].num, args[2].num, cmode, sleepy,
						{ 0,0,0, (byte)args[3].num, (byte)args[4].num, (byte)args[5].num },
						{ 0,0,0, (byte)args[6].num, (byte)args[7].num, (byte)args[8].num });
				}
				break;
			case COMMANDS::lowlevel:
				// low-level
				if (afx_map.fxdevs.size()) {
					devType = 1;
					printf("USB Device selected\n");
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
					for (auto i = afx_map.fxdevs.begin(); i < afx_map.fxdevs.end(); i++) {
						printf("Device #%d - %s, VID#%d, PID#%d, APIv%d, %d lights\n", (int)(i - afx_map.fxdevs.begin()),
							i->name.c_str(), i->vid, i->pid, i->version, (int)i->lights.size());
						for (int k = 0; k < i->lights.size(); k++)
							printf("  Light ID#%d - %s%s%s\n", i->lights[k].lightid, i->lights[k].name.c_str(),
								(i->lights[k].flags & ALIENFX_FLAG_POWER) ? " (Power button)" : "",
								(i->lights[k].flags & ALIENFX_FLAG_INDICATOR) ? " (Indicator)" : "");
					}
					// now groups...
					if (afx_map.GetGroups()->size()) {
						printf("%d groups:\n", (int)afx_map.GetGroups()->size());
						for (int i = 0; i < afx_map.GetGroups()->size(); i++)
							printf("  Group #%d (%d lights) - %s\n", (afx_map.GetGroups()->at(i).gid & 0xffff),
								(int)afx_map.GetGroups()->at(i).lights.size(),
								afx_map.GetGroups()->at(i).name.c_str());
					}
				}
				else
					lfxUtil.GetStatus();
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
				for (auto i = afx_map.fxdevs.begin(); i != afx_map.fxdevs.end(); i++) {
					printf("===== Device VID_%04x, PID_%04x =====\n+++++ Detected as: %s, APIv%d +++++\n",
						i->vid, i->pid, i->dev->description.c_str(), i->version);
				}
				printf("Do you want to set devices and lights names?");
				char name[256];
				gets_s(name, 255);
				if (name[0] == 'y' || name[0] == 'Y') {
//					printf("\nFor each light please enter LightFX SDK light ID or light name if ID is not available\n\
//Tested light become green, and turned off after testing.\n\
//Just press Enter if no visible light at this ID to skip it.\n");
					if (afx_map.fxdevs.size() > 0) {
						printf("Found %d device(s)\n", (int)afx_map.fxdevs.size());
						//if (have_high) {
						//	lfxUtil.FillInfo();
						//	//lfxUtil.GetStatus();
						//}

						for (auto cDev = afx_map.fxdevs.begin(); cDev != afx_map.fxdevs.end(); cDev++) {
							printf("Probing device VID_%04x, PID_%04x", cDev->vid, cDev->pid);
							printf(", current name %s", cDev->name.c_str());
							printf(", New name (ENTER to skip): ");
							gets_s(name, 255);
							if (name[0])
								//cDev->name = isdigit(name[0]) && have_high ? lfxUtil.GetDevInfo(atoi(name))->desc : name;
								cDev->name = name;
							printf("New name %s\n", cDev->name.c_str());
							// How many lights to check?
							int fnumlights = numlights == -1 ? cDev->vid == 0x0d62 ? 0x88 : 23 : numlights;
							AlienFX_SDK::Afx_lightblock lon{ 0, { { 0 } } };
							for (byte i = 0; i < fnumlights; i++)
								if (lightID == -1 || lightID == i) {
									printf("Testing light #%d", i);
									lon.index = i;
									lon.act.front().g = 255;
									AlienFX_SDK::Afx_light* lmap = afx_map.GetMappingByDev(&(*cDev), i);
									if (lmap)
										printf(", current name %s", lmap->name.c_str());
									printf(", New name (ENTER to skip): ");
									cDev->dev->SetAction(&lon);
									cDev->dev->UpdateColors();
									gets_s(name, 255);
									if (name[0]) {
										if (lmap)
											lmap->name = name;
										else
											cDev->lights.push_back({ i, {0,0}, name });
										afx_map.SaveMappings();
										printf("New name is %s, ", name);
									}
									else
										printf("Skipped, ");
									lon.act.front().g = 0;
									cDev->dev->SetAction(&lon);
									cDev->dev->UpdateColors();
									afx_map.SaveMappings();
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
		printf("Light devices not found, exiting!\n");

	if (have_high)
		lfxUtil.Release();

	return 0;
}

