#define WIN32_LEAN_AND_MEAN
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

void CheckDevices(bool show_all) {

	GUID guid;
	HANDLE tdevHandle;

	HidD_GetHidGuid(&guid);
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return;
	}
	unsigned int dw = 0;
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData{ sizeof(SP_DEVICE_INTERFACE_DATA) };

	while (true)
	{
		if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData))
		{
			break;
		}
		dw++;
		DWORD dwRequiredSize = 0;
		if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL))
		{
			continue;
		}
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			continue;
		}
		unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new char[dwRequiredSize]);
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
		if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL))
		{
			string devicePath = deviceInterfaceDetailData->DevicePath;
			tdevHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

			if (tdevHandle != INVALID_HANDLE_VALUE)
			{
				std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
				attributes->Size = sizeof(HIDD_ATTRIBUTES);
				if (HidD_GetAttributes(tdevHandle, attributes.get()))
				{
					for (unsigned i = 0; i < NUM_VIDS; i++) {
						if (attributes->VendorID == AlienFX_SDK::vids[i]) {

							PHIDP_PREPARSED_DATA prep_caps;
							HIDP_CAPS caps;
							HidD_GetPreparsedData(tdevHandle, &prep_caps);
							HidP_GetCaps(prep_caps, &caps);
							HidD_FreePreparsedData(prep_caps);

							string apiver;
							bool supported = true;

							switch (caps.OutputReportByteLength) {
							case 0:
							{
								switch (caps.Usage) {
								case 0xcc: apiver = "RGB keyboard, APIv5"; break;
								default: supported = false; apiver = "Unknown.";
								}
							} break;
							case 8:
								apiver = "Notebook, APIv1";
								break;
							case 9:
								apiver = "Notebook, APIv2";
								break;
							case 12:
								apiver = "Notebook, APIv3";
								break; supported = true;
							case 34:
								apiver = "Notebook/Desktop, APIv4";
								break;
							case 65:
								switch (i) {
								case 2:
									apiver = "Monitor, APIv6";
									break;
								case 3:
									apiver = "Mouse, APIv7";
									break;
								case 4:
									apiver = "Keyboard, APIv8";
									break;
								}
								break;
							default: supported = false; apiver = "Unknown";
							}

							if (show_all || supported) {

								printf("===== Device VID_%04x, PID_%04x =====\n", attributes->VendorID, attributes->ProductID);
								printf("Version %d, block size %d\n", attributes->VersionNumber, attributes->Size);

								printf("Report Lengths: Output %d, Input %d, Feature %d\n", caps.OutputReportByteLength,
									caps.InputReportByteLength,
									caps.FeatureReportByteLength);
								printf("Usage ID %#x, Usage page %#x, Output caps %d, Index %d\n", caps.Usage, caps.UsagePage,
									caps.NumberOutputButtonCaps, caps.NumberOutputDataIndices);

								printf("+++++ Detected as: ");

								switch (i) {
								case 0: printf("Alienware,"); break;
								case 1: printf("DARFON,"); break;
								case 2: printf("Microchip,"); break;
								case 3: printf("Primax,"); break;
								case 4: printf("Chicony,"); break;
								}

								printf(" %s +++++\n", apiver.c_str());
							}
						}
					}
				}
			}
			CloseHandle(tdevHandle);
		}
	}
}

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
				return commands[i].id;
		}
	}
	return -2;
}

AlienFX_SDK::Mappings* afx_map = new AlienFX_SDK::Mappings();
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

int main(int argc, char* argv[])
{
	int devType = -1;
	UINT sleepy = 0;

	printf("alienfx-cli v6.2.4\n");
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

	for (int i = 0; i < afx_map->fxdevs.size(); i++) {
		if (afx_map->fxdevs[i].dev) {
			have_low = true;
			printf("Low-level device ready.\n");
			devType = 1;
		}
		else {
			afx_map->fxdevs.erase(i + afx_map->fxdevs.begin());
			i--;
		}
	}
	if (!have_low)
		printf("Low-level not found.\n");

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
				for (auto cd = afx_map->fxdevs.begin(); cd < afx_map->fxdevs.end(); cd++) {
					vector<byte> lights;
					for (auto i = cd->lights.begin(); i < cd->lights.end(); i++) {
						if (!(i->flags & ALIENFX_FLAG_POWER))
							lights.push_back((byte)i->lightid);
					}
					cd->dev->SetMultiLights(&lights, color);
				}
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
				if (args[0].num < afx_map->fxdevs.size())
					afx_map->fxdevs[args[0].num].dev->SetColor(args[1].num, color);
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
							if (LOWORD(grp->lights[i]) == afx_map->fxdevs[j].pid)
								lights.push_back((byte) HIWORD(grp->lights[i]));
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
				if (act.act.size() < 2) {
					act.act.push_back(AlienFX_SDK::afx_act({ (BYTE)actionCode, (BYTE)sleepy, 7, 0, 0, 0 }));
				}
				if (args[0].num < afx_map->fxdevs.size())
					afx_map->fxdevs[args[0].num].dev->SetAction(&act);
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
				for (auto j = afx_map->fxdevs.begin(); j < afx_map->fxdevs.end(); j++) {
					for (int i = 0; i < (grp ? grp->lights.size() : j->lights.size()); i++) {
						if ((grp && LOWORD(grp->lights[i]) == j->pid) || !grp) {
							act.index = (byte)(grp ? HIWORD(grp->lights[i]) : j->lights[i].lightid);
							j->dev->SetAction(&act);
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
				vector<AlienFX_SDK::act_block> act{ {(byte)args[1].num,
								  {{AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
								  (byte)args[2].num, (byte)args[3].num, (byte)args[4].num},
								  {AlienFX_SDK::AlienFX_A_Power, 3, 0x64,
								  (byte)args[5].num, (byte)args[6].num, (byte)args[7].num}}} };
				afx_map->fxdevs[args[0].num].dev->SetPowerAction(&act);
			}
			break;
		case 6:
			sleepy = args[0].num;
			if (!devType) {
				lfxUtil.SetTempo(args[0].num);
				Update();
			}
			break;
		//case 7:
		//	// set-dev
		//	FindDevice(args[0].num);
		//	printf("Device #%d selected\n", cdev->GetPID());
		//	break;
		case 8:
			// set-dim
			if (devType && args[0].num < afx_map->fxdevs.size())
				afx_map->fxdevs[args[0].num].dev->ToggleState(args[0].num, &afx_map->fxdevs[args[0].num].lights, false);
			break;
		case 9:
			// set-global
			if (devType)
				for (auto t = afx_map->fxdevs.begin(); t < afx_map->fxdevs.end(); t++)
					if (t->dev->GetVersion() == 5)
						t->dev->SetGlobalEffects(args[0].num, sleepy,
							{ 0,0,0, (byte)args[1].num, (byte)args[2].num, (byte)args[3].num },
							{ 0,0,0, (byte)args[4].num, (byte)args[5].num, (byte)args[6].num });
			break;
		case 10:
			// low-level
			if (have_low) {
				devType = 1;
				printf("Low-level device selected\n");
			}
			break;
		case 11:
			// high-level
			if (have_high) {
				devType = 0;
				printf("High-level device selected\n");
			}
			break;
		case 13:
			// status
			if (devType) {
				for (auto i = afx_map->fxdevs.begin(); i < afx_map->fxdevs.end(); i++) {
					printf("Device #%d - %s, ", (int)(i - afx_map->fxdevs.begin()), i->name.c_str());
					string typeName = "Unknown";
					if (i->dev)
						switch (i->dev->GetVersion()) {
						case 0: typeName = "Desktop"; break;
						case 1: case 2: case 3: typeName = "Notebook"; break;
						case 4: typeName = "Desktop/Notebook"; break;
						case 5: typeName = "Keyboard"; break;
						case 6: typeName = "Display"; break;
						case 7: typeName = "Mouse"; break;
						}

					printf("%s, VID#%d, PID#%d, APIv%d, %d lights\n", typeName.c_str(),
						i->vid, i->pid, i->dev->GetVersion(), (int) i->lights.size());

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
		case 12: {
			// probe
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
				//AlienFX_SDK::Functions* afx_dev = new AlienFX_SDK::Functions();
				//vector<pair<WORD, WORD>> pids = afx_map->AlienFXEnumDevices();

				if (afx_map->fxdevs.size() > 0) {
					printf("Found %d device(s)\n", (int)afx_map->fxdevs.size());
					if (have_high) {
						lfxUtil.FillInfo();
						lfxUtil.GetStatus();
					}

					for (auto cDev = afx_map->fxdevs.begin(); cDev < afx_map->fxdevs.end(); cDev++)
						if (devID == -1 || devID == cDev->pid) {
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
							for (int i = 0; i < fnumlights; i++)
								if (lightID == -1 || lightID == i) {
									printf("Testing light #%d", i);
									AlienFX_SDK::mapping* lmap = afx_map->GetMappingById(&(*cDev), i);
									if (lmap) {
										printf(", old name %s ", lmap->name.c_str());
									}
									printf("(ENTER for skip): ");
									cDev->dev->SetColor(i, { 0, 255, 0 });
									cDev->dev->UpdateColors();
									Sleep(100);
									gets_s(name, 255);
									if (name[0] != 0) {
										//not skipped
										if (lmap) {
											lmap->name = name;
										}
										else {
											cDev->lights.push_back({ (WORD)i, 0, name });
										}
										afx_map->SaveMappings();
										//}
										printf("Final name is %s, ", name);
									}
									else {
										printf("Skipped, ");
									}
									cDev->dev->SetColor(i, { 0, 0, 255 });
									cDev->dev->UpdateColors();
									//afx_dev->Reset();
									Sleep(100);
									afx_map->SaveMappings();
								}
						}
				}
			}
		} break;
		case 14:
			if (devType)
				for (int i = 0; i < afx_map->fxdevs.size(); i++)
					afx_map->fxdevs[i].dev->ToggleState(255, &afx_map->fxdevs[i].lights, false);
			break;
		case 15:
			if (devType)
				for (int i = 0; i < afx_map->fxdevs.size(); i++)
					afx_map->fxdevs[i].dev->ToggleState(0, &afx_map->fxdevs[i].lights, false);
			break;
		case 16:
			if (devType)
				for (int i = 0; i < afx_map->fxdevs.size(); i++)
					afx_map->fxdevs[i].dev->Reset();
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

