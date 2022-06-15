#include "ConfigHandler.h"
#include "resource.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

extern groupset* FindMapping(int, vector<groupset>*);
extern vector<string> effModes;

ConfigHandler::ConfigHandler() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxgui"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Profiles"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);

	// do we have old configuration?
	if (RegOpenKeyEx(hKeyMain, "Zones", 0, KEY_ALL_ACCESS, &hKeyZones) != ERROR_SUCCESS) {
		if (haveOldConfig = RegOpenKeyEx(hKeyMain, "Events", 0, KEY_ALL_ACCESS, &hKeyZones) == ERROR_SUCCESS)
			RegCloseKey(hKeyZones);
		RegCreateKeyEx(hKeyMain, TEXT("Zones"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);
	}
	afx_dev.LoadMappings();

	fan_conf = new ConfigFan();
}

ConfigHandler::~ConfigHandler() {
	Save();
	if (fan_conf) delete fan_conf;

	afx_dev.SaveMappings();

	RegCloseKey(hKeyMain);
	RegCloseKey(hKeyZones);
	RegCloseKey(hKeyProfiles);
}

void ConfigHandler::updateProfileByID(unsigned id, std::string name, std::string app, DWORD flags, DWORD tFlags, DWORD* eff) {
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
		prof->effmode = HIWORD(flags);
		if (prof->flags & PROF_GLOBAL_EFFECTS) {
			prof->flags &= ~PROF_GLOBAL_EFFECTS;
			prof->effmode = 99;
		}
	}
	if (tFlags != -1) {
		prof->triggerFlags = LOWORD(tFlags);
		prof->triggerkey = HIWORD(tFlags);
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
			prof->fansets.GPUPower = LOBYTE(HIWORD(flags));
			prof->fansets.gmode = HIBYTE(HIWORD(flags));
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

groupset* ConfigHandler::FindCreateGroupSet(int profID, int groupID)
{
	profile* prof = FindProfile(profID);
	if (prof) {
		groupset* gset = FindMapping(groupID, &prof->lightsets);
		if (!gset) {
			prof->lightsets.push_back({ groupID });
			gset = &prof->lightsets.back();
		}
		return gset;
	}
	return nullptr;
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

bool ConfigHandler::SetStates() {
	bool oldStateOn = stateOn, oldStateDim = stateDimmed, oldPBState = finalPBState;
	// Lights on state...
	stateOn = lightsOn && stateScreen && (!offOnBattery || statePower);
	// Dim state...
	stateDimmed = IsDimmed() || dimmedScreen || (dimmedBatt && !statePower);
	finalBrightness = (byte)(stateOn ? stateDimmed ? 255 - dimmingPower : 255 : 0);
	finalPBState = finalBrightness > 0 ? (byte)dimPowerButton : (byte)offPowerButton;

	if (oldStateOn != stateOn || oldStateDim != stateDimmed || oldPBState != (bool)finalPBState) {
		SetIconState();
		SetToolTip();
		return true;
	}
	return false;
}

void ConfigHandler::SetToolTip() {
	string name = "Profile: " + activeProfile->name + "\nEffect: " + (GetEffect() < effModes.size() ? effModes[GetEffect()] : "Global");
	strcpy_s(niData.szTip, 128, name.c_str());
	Shell_NotifyIcon(NIM_MODIFY, &niData);
}

void ConfigHandler::SetIconState() {
	// change tray icon...
	niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
						stateOn ? stateDimmed ? MAKEINTRESOURCE(IDI_ALIENFX_DIM) : MAKEINTRESOURCE(IDI_ALIENFX_ON) : MAKEINTRESOURCE(IDI_ALIENFX_OFF),
						IMAGE_ICON,	GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
}

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
	GetReg("Ambient-Shift", &amb_shift);
	GetReg("Ambient-Mode", &amb_mode);
	GetReg("Ambient-Grid", &amb_grid, 0x30004);

	// Haptics....
	GetReg("Haptics-Input", &hap_inpType);

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
				updateProfileByID(pid, (char*)data, "", -1, -1, NULL);
				goto nextRecord;
			}
			//if (sscanf_s(name, "Profile-flags-%d", &pid) == 1) {
			//	DWORD newData = MAKELPARAM(LOWORD(*(DWORD*)data), HIWORD(*(DWORD*)data) == 3 ? 0 : *(DWORD*)data + 1);
			//	updateProfileByID(pid, "", "", newData, NULL);
			//	goto nextRecord;
			//}
			if (sscanf_s(name, "Profile-triggers-%d", &pid) == 1) {
				updateProfileByID(pid, "", "", -1, *(DWORD*)data, NULL);
				goto nextRecord;
			}
			if (sscanf_s(name, "Profile-gflags-%d", &pid) == 1) {
				updateProfileByID(pid, "", "", *(DWORD*)data, -1, NULL);
				goto nextRecord;
			}
			if (sscanf_s(name, "Profile-app-%d-%d", &pid, &appid) == 2) {
				//PathStripPath((char*)data);
				updateProfileByID(pid, "", (char*)data, -1, -1, NULL);
				goto nextRecord;
			}
			int senid, fanid;
			if (sscanf_s(name, "Profile-fans-%d-%d-%d", &pid, &senid, &fanid) == 3) {
				// add fans...
				fan_block fan{(short) fanid};
				for (unsigned i = 0; i < lend; i += 2) {
					fan.points.push_back({data[i], data[i+1]});
				}
				updateProfileFansByID(pid, senid, &fan, -1);
				goto nextRecord;
			}
			if (sscanf_s(name, "Profile-effect-%d", &pid) == 1) {
				updateProfileByID(pid, "", "", -1, -1, (DWORD*)data);
				goto nextRecord;
			}
			if (sscanf_s(name, "Profile-power-%d", &pid) == 1) {
				updateProfileFansByID(pid, -1, NULL, *(DWORD*)data);
				goto nextRecord;
			}
nextRecord:
			delete[] data;
		}
	} while (ret == ERROR_SUCCESS);
	vindex = 0;
	// Zones...
	do {
		DWORD len = 255, lend = 0;
		// get id(s)...
		if ((ret = RegEnumValue( hKeyZones, vindex, name, &len, NULL, NULL, NULL, &lend )) == ERROR_SUCCESS) {
			BYTE *inarray = new BYTE[lend]; len++;
			int profID, groupID, recSize;
			groupset* gset;
			RegEnumValue(hKeyZones, vindex, name, &len, NULL, NULL, (LPBYTE) inarray, &lend);
			vindex++;
			if (sscanf_s((char*)name, "Zone-flags-%d-%d", &profID, &groupID) == 2 &&
				(gset = FindCreateGroupSet(profID, groupID))) {
				gset->fromColor = inarray[0];
				gset->gauge = inarray[1];
				gset->flags = inarray[2];
				goto nextZone;
			}
			if (sscanf_s((char*)name, "Zone-events-%d-%d", &profID, &groupID) == 2 &&
				(gset = FindCreateGroupSet(profID, groupID))) {
				for (int i = 0; i < 3; i++)
					gset->events[i] = ((event*)inarray)[i];
				goto nextZone;
			}
			if (sscanf_s((char*)name, "Zone-colors-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
				(gset = FindCreateGroupSet(profID, groupID))) {
				AlienFX_SDK::afx_act* clr = (AlienFX_SDK::afx_act*)inarray;
				for (int i = 0; i < recSize; i++)
					gset->color.push_back(clr[i]);
				goto nextZone;
			}
			if (sscanf_s((char*)name, "Zone-ambient-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
				(gset = FindCreateGroupSet(profID, groupID))) {
				for (int i = 0; i < recSize; i++)
					gset->ambients.push_back(inarray[i]);
				goto nextZone;
			}
			if (sscanf_s((char*)name, "Zone-haptics-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
				(gset = FindCreateGroupSet(profID, groupID))) {
				byte* out = inarray;
				for (int i = 0; i < recSize; i++) {
					freq_map newFreq;
					newFreq.colorfrom.ci = *(DWORD*)out; out += sizeof(DWORD);
					newFreq.colorto.ci = *(DWORD*)out; out += sizeof(DWORD);
					newFreq.hicut = *out; out++;
					newFreq.lowcut = *out; out++;
					byte freqsize = *out; out++;
					for (int j = 0; j < freqsize; j++) {
						newFreq.freqID.push_back(*out); out++;
					}
					gset->haptics.push_back(newFreq);
				}
				goto nextZone;
			}
			nextZone:
			delete[] inarray;
		}
	} while (ret == ERROR_SUCCESS);

	if (!profiles.size()) {
		// need new profile
		profile* prof = new profile({0, 1, 0, {}, "Default"});
		profiles.push_back(prof);
	}

	if (profiles.size() == 1) {
		profiles.front()->flags |= PROF_DEFAULT;
	}
	activeProfile = FindProfile(activeProfileID);
	if (!activeProfile) activeProfile = FindDefaultProfile();
	active_set = &activeProfile->lightsets;

	SortAllGauge();
	SetStates();
}

profile* ConfigHandler::FindDefaultProfile() {
	auto res = find_if(profiles.begin(), profiles.end(),
		[](profile* prof) {
			return (prof->flags & PROF_DEFAULT);
	});
	return res == profiles.end() ? profiles.front() : *res;
}

void ConfigHandler::Save() {

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
	SetReg("Ambient-Shift", amb_shift);
	SetReg("Ambient-Mode", amb_mode);
	SetReg("Ambient-Grid", amb_grid);

	// Haptics
	SetReg("Haptics-Input", hap_inpType);

	RegDeleteTree(hKeyMain, "Profiles");
	RegCreateKeyEx(hKeyMain, "Profiles", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);

	RegDeleteTree(hKeyMain, "Zones");
	RegCreateKeyEx(hKeyMain, "Zones", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);

	for (auto jIter = profiles.begin(); jIter < profiles.end(); jIter++) {
		DWORD flagset;
		string name = "Profile-" + to_string((*jIter)->id);
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)(*jIter)->name.c_str(), (DWORD)(*jIter)->name.length());
		/*name = "Profile-flags-" + to_string((*jIter)->id);
		DWORD flagset = MAKELONG((*jIter)->flags, (*jIter)->effmode ? (*jIter)->effmode - 1 : 3);
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));*/
		name = "Profile-gflags-" + to_string((*jIter)->id);
		flagset = MAKELONG((*jIter)->flags, (*jIter)->effmode);
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));
		name = "Profile-triggers-" + to_string((*jIter)->id);
		flagset = MAKELONG((*jIter)->triggerFlags, (*jIter)->triggerkey);
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));

		for (int i = 0; i < (*jIter)->triggerapp.size(); i++) {
			name = "Profile-app-" + to_string((*jIter)->id) + "-" + to_string(i);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)(*jIter)->triggerapp[i].c_str(), (DWORD)(*jIter)->triggerapp[i].length());
		}

		for (auto iIter = (*jIter)->lightsets.begin(); iIter < (*jIter)->lightsets.end(); iIter++) {
			string fname;
			name = to_string((*jIter)->id) + "-" + to_string(iIter->group);
			// flags...
			fname = "Zone-flags-" + name;
			DWORD value = 0; byte* buffer = (BYTE*)&value;
			buffer[0] = iIter->fromColor;
			buffer[1] = iIter->gauge;
			// ToDo: flags now bytes 2-3
			buffer[2] = iIter->flags;
			//buffer[3] = iIter->group->have_power;
			RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));

			if (iIter->color.size()) { // colors
				fname = "Zone-colors-" + name + "-" + to_string(iIter->color.size());
				AlienFX_SDK::afx_act* buffer = new AlienFX_SDK::afx_act[iIter->color.size()];
				for (int i = 0; i < iIter->color.size(); i++)
					buffer[i] = iIter->color[i];
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, (DWORD)iIter->color.size() * sizeof(AlienFX_SDK::afx_act));
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
					size += 2 * sizeof(DWORD) + 3 + (DWORD)it->freqID.size();
				byte* buffer = new byte[size], * out = buffer;
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
		//if ((*jIter)->flags & PROF_GLOBAL_EFFECTS) {
		DWORD buffer[3];
		name = "Profile-effect-" + to_string((*jIter)->id);
		buffer[0] = MAKELONG((*jIter)->globalEffect, (*jIter)->globalDelay);
		buffer[1] = (*jIter)->effColor1.ci;
		buffer[2] = (*jIter)->effColor2.ci;
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_BINARY, (BYTE*)buffer, 3 * sizeof(DWORD));
		//}
		// Fans....
		if ((*jIter)->flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" + to_string((*jIter)->id);
			WORD ps = MAKEWORD((*jIter)->fansets.GPUPower, (*jIter)->fansets.gmode);
			DWORD pvalue = MAKELONG((*jIter)->fansets.powerStage, ps);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&pvalue, sizeof(DWORD));
			// save fans...
			for (int i = 0; i < (*jIter)->fansets.fanControls.size(); i++) {
				temp_block* sens = &(*jIter)->fansets.fanControls[i];
				for (int k = 0; k < sens->fans.size(); k++) {
					fan_block* fans = &sens->fans[k];
					name = "Profile-fans-" + to_string((*jIter)->id) + "-" + to_string(sens->sensorIndex) + "-" + to_string(fans->fanIndex);
					byte* outdata = new byte[fans->points.size() * 2];
					for (int l = 0; l < fans->points.size(); l++) {
						outdata[2 * l] = (byte)fans->points[l].temp;
						outdata[(2 * l) + 1] = (byte)fans->points[l].boost;
					}

					RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_BINARY, (BYTE*)outdata, (DWORD)fans->points.size() * 2);
					delete[] outdata;
				}
			}
		}
	}
}

void ConfigHandler::SortAllGauge() {
	for (auto t = afx_dev.GetGroups()->begin(); t < afx_dev.GetGroups()->end(); t++)
		SortGroupGauge(t->gid);
}

zonemap* ConfigHandler::FindZoneMap(int gid) {
	auto gpos = find_if(zoneMaps.begin(), zoneMaps.end(),
		[gid](auto t) {
			return t.gID == gid;
		});
	if (gpos != zoneMaps.end())
		return &(*gpos);
	return nullptr;
}

void ConfigHandler::SortGroupGauge(int gid) {
	AlienFX_SDK::group* grp = afx_dev.GetGroupById(gid);
	zonemap* zone = FindZoneMap(gid);
	if (zone)
		zone->lightMap.clear();
	else
		if (grp) {
			zoneMaps.push_back({ (DWORD)gid });
			zone = &zoneMaps.back();
		}

	if (!grp) return;

	// scan lights in grid...
	//int minX = 999, minY = 999;
	zone->gMinX = zone->gMinY = 200;
	for (auto lgh = grp->lights.begin(); lgh < grp->lights.end(); lgh++) {
		DWORD lgt = MAKELPARAM(lgh->first, lgh->second);
		zonelight cl{ {lgh->first, lgh->second }, 255, 255 };
		for (auto t = afx_dev.GetGrids()->begin(); t < afx_dev.GetGrids()->end(); t++) {
			for (int ind = 0; ind < t->x * t->y; ind++)
				if (t->grid[ind] == lgt) {
					cl.x = min(cl.x, ind % t->x);
					cl.y = min(cl.y, ind / t->x);
					zone->gMaxX = max(zone->gMaxX, ind % t->x);
					zone->gMaxY = max(zone->gMaxY, ind / t->x);
				}
		}
		zone->gMinX = min(zone->gMinX, cl.x), zone->gMinY = min(zone->gMinY, cl.y);
		zone->lightMap.push_back(cl);
	}

	// now shrink axis...
	for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++) {
		t->x -= zone->gMinX; t->y -= zone->gMinY;
	}
	zone->xMax = zone->gMaxX - zone->gMinX; zone->yMax = zone->gMaxY - zone->gMinY;
	for (int x = 1; x < zone->xMax; x++) {
		if (find_if(zone->lightMap.begin(), zone->lightMap.end(),
			[x](auto t) {
				return t.x == x;
			}) == zone->lightMap.end()) {
			byte minDelta = 255;
			for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
				if (t->x > x) minDelta = min(minDelta, t->x-x);
			if (minDelta < 255) {
				for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
					if (t->x > x) t->x -= minDelta;
				zone->xMax -= minDelta;
			}
			else
				zone->xMax = x - 1;
		}
	}
	for (int y = 1; y < zone->yMax; y++) {
		if (find_if(zone->lightMap.begin(), zone->lightMap.end(),
			[y](auto t) {
				return t.y == y;
			}) == zone->lightMap.end()) {
			byte minDelta = 255;
			for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
				if (t->y > y) minDelta = min(minDelta, t->y - y);
			if (minDelta < 255) {
				for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++)
					if (t->y > y) t->y -= minDelta;
				zone->yMax -= minDelta;
			}
			else
				zone->yMax = y - 1;
		}
	}
}