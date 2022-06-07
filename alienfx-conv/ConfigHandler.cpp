#include "ConfigHandler.h"
//#include "resource.h"
//#include <Shlwapi.h>

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

ConfigHandler::ConfigHandler() {

	RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Alienfxgui", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, "Events", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyEvents, NULL);
	RegCreateKeyEx(hKeyMain, "Profiles", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);
	RegCreateKeyEx(hKeyMain, "Zones", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);

	afx_dev.LoadMappings();

	//fan_conf = new ConfigFan();
	amb_conf = new ConfigAmbient();
	hap_conf = new ConfigHaptics();
}

ConfigHandler::~ConfigHandler() {
	//Save();
	//if (fan_conf) delete fan_conf;
	if (amb_conf) delete amb_conf;
	if (hap_conf) delete hap_conf;

	afx_dev.SaveMappings();

	RegCloseKey(hKeyMain);
	RegCloseKey(hKeyEvents);
	RegCloseKey(hKeyProfiles);
}

void ConfigHandler::ClearEvents()
{
	RegDeleteTree(hKeyMain, "Events");
	RegDeleteTree(HKEY_CURRENT_USER, "SOFTWARE\\Alienfxambient");
	RegDeleteTree(HKEY_CURRENT_USER, "SOFTWARE\\Alienfxhaptics");
}

void ConfigHandler::updateProfileByID(unsigned id, std::string name, std::string app, DWORD flags, DWORD* eff) {
	profile* prof = FindProfile(id);
	if (!prof) {
		// New profile
		prof = new profile{id};
		profiles.push_back(prof);
	}
	// update profile..
	if (!name.empty())
		prof->name = name;
	if (!app.empty())
		prof->triggerapp.push_back(app);
	if (flags != -1) {
		prof->flags = LOWORD(flags);
		prof->effmode = HIWORD(flags) == 3 ? 0 : HIWORD(flags) + 1;
	}
	if (eff) {
		prof->globalEffect = (byte) LOWORD(eff[0]);
		prof->globalDelay = (byte) HIWORD(eff[0]);
		prof->effColor1.ci = eff[1];
		prof->effColor2.ci = eff[2];
	}
}

void ConfigHandler::updateProfileFansByID(unsigned id, unsigned senID, fan_block* temp, DWORD flags) {

	profile* prof = FindProfile(id);
	if (prof) {
		if (temp) {
			for (int i = 0; i < prof->fansets.fanControls.size(); i++)
				if (prof->fansets.fanControls[i].sensorIndex == senID) {
					prof->fansets.fanControls[i].fans.push_back(*temp);
					return;
				}
			prof->fansets.fanControls.push_back({(short) senID, {*temp}});
		}
		if (flags != -1) {
			prof->fansets.powerStage = LOWORD(flags);
			prof->fansets.GPUPower = HIWORD(flags);
		}
		return;
	} else {
		prof = new profile{id};
		if (temp) {
			prof->fansets.fanControls.push_back({(short) senID, {*temp}});
		}
		if (flags != -1) {
			prof->fansets.powerStage = LOWORD(flags);
			prof->fansets.GPUPower = HIWORD(flags);
		}
		profiles.push_back(prof);
	}
}

AlienFX_SDK::group* ConfigHandler::FindCreateGroup(int did, int lid, string name)
{
	AlienFX_SDK::group* grp = NULL;

	auto tGrp = find_if(afx_dev.GetGroups()->begin(), afx_dev.GetGroups()->end(),
		[did, lid](auto g) {
			return g.lights.size() == 1 && g.lights.front().first == did && g.lights.front().second == lid;
		});
	if (tGrp == afx_dev.GetGroups()->end()) {
		unsigned maxID = 0x10000;
		while (afx_dev.GetGroupById(maxID))
			maxID++;
		AlienFX_SDK::mapping* lgh = afx_dev.GetMappingById(afx_dev.GetDeviceById(did), lid);
		afx_dev.GetGroups()->push_back({ maxID, name + " " + (lgh ? lgh->name : "#" + to_string(maxID & 0xffff)) });
		grp = &afx_dev.GetGroups()->back();
		grp->have_power = lgh->flags & ALIENFX_FLAG_POWER;
	}
	else
		grp = &(*tGrp);

	return grp;
}

profile* ConfigHandler::FindProfile(int id) {
	for (int i = 0; i < profiles.size(); i++)
		if (profiles[i]->id == id) {
			return profiles[i];
		}
	return NULL;
}

profile* ConfigHandler::FindProfileByApp(string appName, bool active)
{
	for (int j = 0; j < profiles.size(); j++)
		if (active || !(profiles[j]->flags & PROF_ACTIVE)) {
			for (int i = 0; i < profiles[j]->triggerapp.size(); i++)
				if (profiles[j]->triggerapp[i] == appName)
					// app is belong to profile!
					return profiles[j];
		}
	return NULL;
}

bool ConfigHandler::IsPriorityProfile(profile* prof) {
	return prof && prof->flags & PROF_PRIORITY;
}

//bool ConfigHandler::SetStates() {
//	bool oldStateOn = stateOn, oldStateDim = stateDimmed, oldPBState = finalPBState;
//	// Lights on state...
//	stateOn = lightsOn && stateScreen && (!offOnBattery || statePower);
//	// Dim state...
//	stateDimmed = IsDimmed() || dimmedScreen || (dimmedBatt && !statePower);
//	finalBrightness = (byte)(stateOn ? stateDimmed ? 255 - dimmingPower : 255 : 0);
//	finalPBState = finalBrightness > 0 ? (byte)dimPowerButton : (byte)offPowerButton;
//
//	if (oldStateOn != stateOn || oldStateDim != stateDimmed || oldPBState != (bool)finalPBState) {
//		SetIconState();
//		Shell_NotifyIcon(NIM_MODIFY, &niData);
//		return true;
//	}
//	return false;
//}
//
//void ConfigHandler::SetIconState() {
//	//// change tray icon...
//	//niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
//	//					stateOn ? stateDimmed ? MAKEINTRESOURCE(IDI_ALIENFX_DIM) : MAKEINTRESOURCE(IDI_ALIENFX_ON) : MAKEINTRESOURCE(IDI_ALIENFX_OFF),
//	//					IMAGE_ICON,	GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
//}

bool ConfigHandler::IsDimmed() {
	return dimmed || (!activeProfile->ignoreDimming && activeProfile->flags & PROF_DIMMED);
}

void ConfigHandler::SetDimmed() {
	if (activeProfile->flags & PROF_DIMMED) {
		if (!dimmed && !activeProfile->ignoreDimming) {
			activeProfile->ignoreDimming = true;
			return;
		}
		if (!dimmed || !activeProfile->ignoreDimming)
			activeProfile->ignoreDimming = !activeProfile->ignoreDimming;
	}

	dimmed = !dimmed;
}
int ConfigHandler::GetEffect() {
	return enableMon ? activeProfile->effmode : 3;
}

//AlienFX_SDK::group* ConfigHandler::CreateGroup(string name)
//{
//	unsigned maxID = 0x10000;
//	while (afx_dev.GetGroupById(maxID))
//		maxID++;
//	AlienFX_SDK::group* dev = new AlienFX_SDK::group({ maxID, name + " #" + to_string(maxID & 0xffff) });
//
//	return dev;
//}

void ConfigHandler::GetReg(char *name, DWORD *value, DWORD defValue) {
	DWORD size = sizeof(DWORD);
	if (RegGetValue(hKeyMain, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigHandler::SetReg(char *text, DWORD value) {
	RegSetValueEx( hKeyMain, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

void ConfigHandler::Load() {
	DWORD size_c = 16*sizeof(DWORD);// 4 * 16
	DWORD activeProfileID = 0;

	GetReg("AutoStart", &startWindows);
	GetReg("Minimized", &startMinimized);
	GetReg("UpdateCheck", &updateCheck, 1);
	GetReg("LightsOn", &lightsOn, 1);
	GetReg("Dimmed", &dimmed);
	GetReg("Monitoring", &enableMon, 1);
	GetReg("GammaCorrection", &gammaCorrection, 1);
	GetReg("DisableAWCC", &awcc_disable);
	GetReg("ProfileAutoSwitch", &enableProf);
	GetReg("OffWithScreen", &offWithScreen);
	GetReg("NoDesktopSwitch", &noDesktop);
	GetReg("DimPower", &dimPowerButton);
	GetReg("DimmedOnBattery", &dimmedBatt);
	GetReg("ActiveProfile", &activeProfileID);
	GetReg("OffPowerButton", &offPowerButton);
	GetReg("EsifTemp", &esif_temp);
	GetReg("DimmingPower", &dimmingPower, 92);
	GetReg("OffOnBattery", &offOnBattery);
	GetReg("FanControl", &fanControl);
	RegGetValue(hKeyMain, NULL, TEXT("CustomColors"), RRF_RT_REG_BINARY | RRF_ZEROONFAILURE, NULL, customColors, &size_c);

	// Ambient....
	GetReg("Ambient-Shift", &shift);
	GetReg("Ambient-Mode", &mode);
	GetReg("Ambient-Grid", &grid, 0x30004);

	// Haptics....
	GetReg("Haptics-Input", &inpType);

	int vindex = 0;
	char name[256];
	int pid = -1, appid = -1;
	LSTATUS ret = 0;
	// Profiles...
	do {
		DWORD len = 255, lend = 0;
		if ((ret = RegEnumValue(hKeyProfiles, vindex, name, &len, NULL, NULL, NULL, &lend)) == ERROR_SUCCESS) {
			lend++; len++;
			BYTE *data = new BYTE[lend];
			RegEnumValue(hKeyProfiles, vindex, name, &len, NULL, NULL, data, &lend);
			vindex++;
			if (sscanf_s(name, "Profile-%d", &pid) == 1) {
				updateProfileByID(pid, (char*)data, "", -1, NULL);
			}
			if (sscanf_s(name, "Profile-flags-%d", &pid) == 1) {
				updateProfileByID(pid, "", "", *(DWORD*)data, NULL);
			}
			if (sscanf_s(name, "Profile-app-%d-%d", &pid, &appid) == 2) {
				//PathStripPath((char*)data);
				updateProfileByID(pid, "", (char*)data, -1, NULL);
			}
			int senid, fanid;
			if (sscanf_s(name, "Profile-fans-%d-%d-%d", &pid, &senid, &fanid) == 3) {
				// add fans...
				fan_block fan{(short) fanid};
				for (unsigned i = 0; i < lend; i += 2) {
					fan.points.push_back({data[i], data[i+1]});
				}
				updateProfileFansByID(pid, senid, &fan, -1);
			}
			if (sscanf_s(name, "Profile-effect-%d", &pid) == 1) {
				updateProfileByID(pid, "", "", -1, (DWORD*)data);
			}
			if (sscanf_s(name, "Profile-power-%d", &pid) == 1) {
				updateProfileFansByID(pid, -1, NULL, *(DWORD*)data);
			}
			delete[] data;
		}
	} while (ret == ERROR_SUCCESS);
	vindex = 0;
	// Event mappings...
	do {
		DWORD len = 255, lend = 0; lightset map;
		// get id(s)...
		if ((ret = RegEnumValue( hKeyEvents, vindex, name, &len, NULL, NULL, NULL, &lend )) == ERROR_SUCCESS) {
			BYTE *inarray = new BYTE[lend]; len++;
			RegEnumValue(hKeyEvents, vindex, name, &len, NULL, NULL, (LPBYTE) inarray, &lend);
			vindex++;
			if (sscanf_s((char*)name, "Set-%d-%d-%d", &map.devid, &map.lightid, &pid) == 3) {
				BYTE* inPos = inarray;
				for (int i = 0; i < 4; i++) {
					FlagSet fs; fs.s = *((DWORD*)inPos);
					map.flags |= fs.flags << i;
					map.eve[i].cut = fs.cut;
					map.eve[i].proc = fs.proc;
					inPos += sizeof(DWORD);
					map.eve[i].source = *inPos++;
					BYTE mapSize = *inPos++;
					map.eve[i].map.clear();
					for (int j = 0; j < mapSize; j++) {
						map.eve[i].map.push_back(*((AlienFX_SDK::afx_act*)inPos));
						inPos += sizeof(AlienFX_SDK::afx_act);
					}
				}
				// now load new structure...
				profile* prof = FindProfile(pid);
				if (prof) {
					groupset t;
					t.color = map.eve[0].map;
					t.fromColor = map.flags & LEVENT_COLOR;
					if (map.devid) {
						t.group = FindCreateGroup(map.devid, map.lightid, "Light");
					}
					else
						t.group = afx_dev.GetGroupById(map.lightid);
					if (map.flags & LEVENT_POWER) {
						t.events[0] = { true, 0, 0, 0, map.eve[1].map[0], map.eve[1].map[1] };
					}
					if (map.flags & LEVENT_PERF) {
						t.events[1] = { true, map.eve[2].source, map.eve[2].cut, map.eve[2].proc, map.eve[2].map[0], map.eve[2].map[1] };
						if (map.eve[2].proc)
							t.gauge = 1;
					}
					if (map.flags & LEVENT_ACT) {
						t.events[2] = { true, map.eve[3].source, map.eve[3].cut, map.eve[3].proc, map.eve[3].map[0], map.eve[3].map[1] };
					}
					//SortGroupGauge(&t);
					auto pos = find_if(prof->lightsets.begin(), prof->lightsets.end(),
						[t](auto cp) {
							return t.group->gid == cp.group->gid;
						});
					prof->lightsets.push_back(t);
				}
			}
			delete[] inarray;
		}
	} while (ret == ERROR_SUCCESS);

	if (!profiles.size()) {
		// need new profile
		profile* prof = new profile({0, 1, 0, {}, "Default"});
		profiles.push_back(prof);
	}
	//defaultProfile = profiles.front();
	if (profiles.size() == 1) {
		profiles.front()->flags |= PROF_DEFAULT;
	}
	activeProfile = FindProfile(activeProfileID);
	if (!activeProfile) activeProfile = FindDefaultProfile();
	active_set = &activeProfile->lightsets;

	// Parse old mappings and haptics...
	// Ambients...
	for (auto it = amb_conf->zones.begin(); it < amb_conf->zones.end(); it++) {
		AlienFX_SDK::group* grp = FindCreateGroup(it->devid, it->lightid, "Ambient");
		for (auto cp = profiles.begin(); cp < profiles.end(); cp++) {
			if ((*cp)->effmode == 2 || (*cp)->flags & PROF_DEFAULT) {
				auto pos = find_if((*cp)->lightsets.begin(), (*cp)->lightsets.end(),
					[grp](auto t) {
						return t.group->gid == grp->gid;
					});
				groupset* tSet;
				if (pos != (*cp)->lightsets.end())
					tSet = &(*pos);
				else {
					tSet = new groupset{ grp };
					(*cp)->lightsets.push_back(*tSet);
					tSet = &(*cp)->lightsets.back();
				}
				tSet->ambients = it->map;
			}
		}
	}
	// Haptics...
	for (auto it = hap_conf->haptics.begin(); it < hap_conf->haptics.end(); it++) {
		AlienFX_SDK::group* grp = FindCreateGroup(it->devid, it->lightid,"Haptic");
		for (auto cp = profiles.begin(); cp < profiles.end(); cp++) {
			if ((*cp)->effmode == 3 || (*cp)->flags & PROF_DEFAULT) {
				auto pos = find_if((*cp)->lightsets.begin(), (*cp)->lightsets.end(),
					[grp](auto t) {
						return t.group->gid == grp->gid;
					});
				groupset* tSet;
				if (pos != (*cp)->lightsets.end())
					tSet = &(*pos);
				else {
					tSet = new groupset{ grp };
					(*cp)->lightsets.push_back(*tSet);
					tSet = &(*cp)->lightsets.back();
				}
				tSet->haptics = it->freqs;
				if (it->flags)
					tSet->gauge = 1;
			}
		}
	}

	stateDimmed = IsDimmed();
	stateOn = lightsOn;

	// convert old haptic settings
	inpType = hap_conf->inpType;
	mode = amb_conf->mode;
	shift = amb_conf->shift;
	grid = MAKELPARAM(amb_conf->grid.x, amb_conf->grid.y);
}

profile* ConfigHandler::FindDefaultProfile() {
	auto res = find_if(profiles.begin(), profiles.end(),
		[](profile* prof) {
			return (prof->flags & PROF_DEFAULT);
	});
	return res == profiles.end() ? profiles.front() : *res;
}

void ConfigHandler::Save() {

	//if (fan_conf) fan_conf->Save();
	//if (amb_conf) amb_conf->Save();
	//if (hap_conf) hap_conf->Save();

	char name[256];
	DWORD len = 256, lend;
	if (RegEnumValue(hKeyZones, 0, name, &len, NULL, NULL, NULL, &lend) == ERROR_SUCCESS) { // have some keys!
		printf("New settings detected! Do you want to override it (y/n)? ");
		gets_s(name, 255);
		if (name[0] != 'y' && name[0] != 'Y')
			return;
	}

	SetReg("AutoStart", startWindows);
	SetReg("Minimized", startMinimized);
	SetReg("UpdateCheck", updateCheck);
	SetReg("LightsOn", lightsOn);
	SetReg("Dimmed", dimmed);
	SetReg("Monitoring", enableMon);
	SetReg("OffWithScreen", offWithScreen);
	SetReg("NoDesktopSwitch", noDesktop);
	SetReg("DimPower", dimPowerButton);
	SetReg("DimmedOnBattery", dimmedBatt);
	SetReg("OffPowerButton", offPowerButton);
	SetReg("OffOnBattery", offOnBattery);
	SetReg("DimmingPower", dimmingPower);
	SetReg("ActiveProfile", activeProfile->id);
	SetReg("GammaCorrection", gammaCorrection);
	SetReg("ProfileAutoSwitch", enableProf);
	SetReg("DisableAWCC", awcc_disable);
	SetReg("EsifTemp", esif_temp);
	SetReg("FanControl", fanControl);

	RegSetValueEx( hKeyMain, TEXT("CustomColors"), 0, REG_BINARY, (BYTE*)customColors, sizeof(DWORD) * 16 );

	// Ambient
	SetReg("Ambient-Shift", shift);
	SetReg("Ambient-Mode", mode);
	SetReg("Ambient-Grid", grid);

	// Haptics
	SetReg("Haptics-Input", inpType);

	RegDeleteTree(hKeyMain, "Profiles");
	RegCreateKeyEx(hKeyMain, "Profiles", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);// &dwDisposition);

	RegDeleteTree(hKeyMain, "Zones");
	RegCreateKeyEx(hKeyMain, "Zones", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);// &dwDisposition);

	for (auto jIter = profiles.begin(); jIter < profiles.end(); jIter++) {
		string name = "Profile-" + to_string((*jIter)->id);
		RegSetValueEx( hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)(*jIter)->name.c_str(), (DWORD)(*jIter)->name.length() );
		name = "Profile-flags-" + to_string((*jIter)->id);
		DWORD flagset = MAKELONG((*jIter)->flags, (*jIter)->effmode ? (*jIter)->effmode - 1 : 3);
		RegSetValueEx( hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));
		name = "Profile-gflags-" + to_string((*jIter)->id);
		flagset = MAKELONG((*jIter)->flags, (*jIter)->effmode);
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));

		for (int i = 0; i < (*jIter)->triggerapp.size(); i++) {
			name = "Profile-app-" + to_string((*jIter)->id) + "-" + to_string(i);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE *)(*jIter)->triggerapp[i].c_str(), (DWORD)(*jIter)->triggerapp[i].length());
		}

		for (auto iIter = (*jIter)->lightsets.begin(); iIter < (*jIter)->lightsets.end(); iIter++) {
			string fname;
			name = to_string((*jIter)->id) + "-" + to_string(iIter->group->gid);
			// flags...
			fname = "Zone-flags-" + name;
			DWORD value = 0; byte* buffer = (BYTE*)&value;
			buffer[0] = iIter->fromColor;
			buffer[1] = iIter->gauge;
			buffer[2] = iIter->flags;
			buffer[3] = iIter->group->have_power;
			RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));

			if (iIter->color.size()) { // colors
				fname = "Zone-colors-" + name + "-" + to_string(iIter->color.size());
				AlienFX_SDK::afx_act* buffer = new AlienFX_SDK::afx_act[iIter->color.size()];
				for (int i = 0; i < iIter->color.size(); i++)
					buffer[i] = iIter->color[i];
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, (DWORD)iIter->color.size()*sizeof(AlienFX_SDK::afx_act));
				delete[] buffer;
			}
			if (iIter->events[0].state + iIter->events[1].state + iIter->events[2].state) { //events
				fname = "Zone-events-" + name;
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)iIter->events, (DWORD)3 * sizeof(event));
			}
			if (iIter->ambients.size()) { // ambient
				fname = "Zone-ambient-" + name + "-" + to_string(iIter->ambients.size());
				byte* buffer = new byte[iIter->ambients.size()];
				for (int i = 0; i < iIter->ambients.size(); i++)
					buffer[i] = iIter->ambients[i];
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, (DWORD)iIter->ambients.size());
				delete[] buffer;
			}
			if (iIter->haptics.size()) { // haptics
				fname = "Zone-haptics-" + name + "-" + to_string(iIter->haptics.size());
				DWORD size = 0;
				for (auto it = iIter->haptics.begin(); it < iIter->haptics.end(); it++)
					size += 2 * sizeof(DWORD) + 3 + it->freqID.size();
				byte* buffer = new byte[size], *out = buffer;
				for (auto it = iIter->haptics.begin(); it < iIter->haptics.end(); it++) {
					*(DWORD*)out = it->colorfrom.ci; out += sizeof(DWORD);
					*(DWORD*)out = it->colorto.ci; out += sizeof(DWORD);
					*out = it->hicut; out++;
					*out = it->lowcut; out++;
					*out = (byte)it->freqID.size(); out++;
					for (auto itf = it->freqID.begin(); itf < it->freqID.end(); itf++) {
						*out = *itf; out++;
					}
				}
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, size);
				delete[] buffer;
			}
		}

		// Global effects
		if ((*jIter)->flags & PROF_GLOBAL_EFFECTS) {
			DWORD buffer[3];
			name = "Profile-effect-" + to_string((*jIter)->id);
			buffer[0] = MAKELONG((*jIter)->globalEffect, (*jIter)->globalDelay);
			buffer[1] = (*jIter)->effColor1.ci;
			buffer[2] = (*jIter)->effColor2.ci;
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_BINARY, (BYTE*)buffer, 3*sizeof(DWORD));
		}
		// Fans....
		if ((*jIter)->flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" + to_string((*jIter)->id);
			DWORD pvalue = MAKELONG((*jIter)->fansets.powerStage, (*jIter)->fansets.GPUPower);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&pvalue, sizeof(DWORD));
			// save fans...
			for (int i = 0; i < (*jIter)->fansets.fanControls.size(); i++) {
				temp_block* sens = &(*jIter)->fansets.fanControls[i];
				for (int k = 0; k < sens->fans.size(); k++) {
					fan_block* fans = &sens->fans[k];
					name = "Profile-fans-" + to_string((*jIter)->id) + "-" + to_string(sens->sensorIndex) + "-" + to_string(fans->fanIndex);
					byte* outdata = new byte[fans->points.size() * 2];
					for (int l = 0; l < fans->points.size(); l++) {
						outdata[2 * l] = (byte) fans->points[l].temp;
						outdata[(2 * l) + 1] = (byte) fans->points[l].boost;
					}

					RegSetValueEx( hKeyProfiles, name.c_str(), 0, REG_BINARY, (BYTE*) outdata, (DWORD) fans->points.size() * 2 );
					delete[] outdata;
				}
			}
		}
	}
}

void ConfigHandler::SortGroupGauge(groupset* map) {
	if (map->lightMap.grid)
		delete[] map->lightMap.grid;
	ZeroMemory(&map->lightMap, sizeof(AlienFX_SDK::lightgrid));
	if (map->gauge) {
		int tempX = 0, tempY = 0;
		vector<vector<DWORD>> tempGrid;
		for (auto t = afx_dev.GetGrids()->begin(); t < afx_dev.GetGrids()->end(); t++) {
			tempY = max(tempY, t->y); tempX = max(tempX, t->x);
			tempGrid.resize(tempY, vector<DWORD>(tempX));
		}
		int minX = tempGrid.size() ? tempGrid[0].size() : 0, minY = tempGrid.size(), maxX = 0, maxY = 0;
		// fill grid...
		for (auto lgh = map->group->lights.begin(); lgh < map->group->lights.end(); lgh++) {
			DWORD lgt = MAKELPARAM(lgh->first, lgh->second);
			for (auto t = afx_dev.GetGrids()->begin(); t < afx_dev.GetGrids()->end(); t++) {
				for (int ind = 0; ind < t->x * t->y; ind++)
					if (t->grid[ind] == lgt) {
						tempGrid[ind / t->x][ind % t->x] = t->grid[ind];
						minX = min(minX, ind % t->x);
						minY = min(minY, ind / t->x);
						maxX = max(maxX, ind % t->x);
						maxY = max(maxY, ind / t->x);
						break;
					}
			}
		}
		// resize grid...
		tempGrid.resize(maxY + 1);
		tempGrid.erase(tempGrid.begin(), tempGrid.begin() + minY);
		for (auto tY = tempGrid.begin(); tY < tempGrid.end(); ) {
			tY->resize(maxX + 1);
			tY->erase(tY->begin(), tY->begin() + minX);
			// remove empty rows...
			if (find_if(tY->begin(), tY->end(),
				[](auto t) {
					return t;
				}) == tY->end()) {
				int pos = tY - tempGrid.begin();
				tempGrid.erase(tY);
				tY = pos + tempGrid.begin();
			}
			else
				tY++;
		}
		// remove empty columns
		if (tempGrid.size()) {
			for (int i = 0; i < tempGrid[0].size(); i++) {
				if (find_if(tempGrid.begin(), tempGrid.end(),
					[i](auto t) {
						return t[i];
					}) == tempGrid.end()) {
					for (auto tg = tempGrid.begin(); tg < tempGrid.end(); tg++)
						tg->erase(tg->begin() + i);
					i--;
				}
			}
			map->lightMap.x = tempGrid[0].size(); map->lightMap.y = tempGrid.size();
			map->lightMap.grid = new DWORD[map->lightMap.x * map->lightMap.y];
			for (int y = 0; y < map->lightMap.y; y++)
				for (int x = 0; x < map->lightMap.x; x++)
					map->lightMap.grid[y * map->lightMap.x + x] = tempGrid[y][x];
		}
	}
}