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
	RegCreateKeyEx(hKeyMain, TEXT("Zones"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Profiles"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);

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
		prof->effmode = HIWORD(flags);
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

groupset* ConfigHandler::FindCreateGroupSet(int profID, int groupID)
{
	profile* prof = FindProfile(profID);
	if (prof) {
		groupset* gset = FindMapping(groupID, &prof->lightsets);
		if (!gset) {
			AlienFX_SDK::group* grp = afx_dev.GetGroupById(groupID);
			if (grp) {
				prof->lightsets.push_back({ grp });
				gset = &prof->lightsets.back();
			}
			else
				return nullptr;
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
	sprintf_s(niData.szTip, 128, "Profile: %s\nEffect: %s", activeProfile->name.c_str(), effModes[GetEffect()].c_str());
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
	if (RegGetValueA(hKeyMain, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
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
		if ((ret = RegEnumValueA(hKeyProfiles, vindex, name, &len, NULL, NULL, NULL, &lend)) == ERROR_SUCCESS) {
			lend++; len++;
			BYTE *data = new BYTE[lend];
			RegEnumValueA(hKeyProfiles, vindex, name, &len, NULL, NULL, data, &lend);
			vindex++;
			if (sscanf_s(name, "Profile-%d", &pid) == 1) {
				updateProfileByID(pid, (char*)data, "", -1, NULL);
				goto nextRecord;
			}
			if (sscanf_s(name, "Profile-flags-%d", &pid) == 1) {
				DWORD newData = MAKELPARAM(LOWORD(*(DWORD*)data), HIWORD(*(DWORD*)data) == 3 ? 0 : *(DWORD*)data + 1);
				updateProfileByID(pid, "", "", newData, NULL);
				goto nextRecord;
			}
			if (sscanf_s(name, "Profile-gflags-%d", &pid) == 1) {
				updateProfileByID(pid, "", "", *(DWORD*)data, NULL);
				goto nextRecord;
			}
			if (sscanf_s(name, "Profile-app-%d-%d", &pid, &appid) == 2) {
				//PathStripPath((char*)data);
				updateProfileByID(pid, "", (char*)data, -1, NULL);
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
				updateProfileByID(pid, "", "", -1, (DWORD*)data);
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
		DWORD len = 255, lend = 0; lightset map;
		// get id(s)...
		if ((ret = RegEnumValueA( hKeyZones, vindex, name, &len, NULL, NULL, NULL, &lend )) == ERROR_SUCCESS) {
			BYTE *inarray = new BYTE[lend]; len++;
			int profID, groupID, recSize;
			groupset* gset;
			RegEnumValueA(hKeyZones, vindex, name, &len, NULL, NULL, (LPBYTE) inarray, &lend);
			vindex++;
			if (sscanf_s((char*)name, "Zone-flags-%d-%d", &profID, &groupID) == 2 &&
				(gset = FindCreateGroupSet(profID, groupID))) {
				gset->fromColor = inarray[0];
				gset->gauge = inarray[1];
				gset->gradient = inarray[2];
				gset->group->have_power = inarray[3];
				if (gset->gauge)
					SortGroupGauge(gset);
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

	RegDeleteTreeA(hKeyMain, "Profiles");
	RegCreateKeyEx(hKeyMain, "Profiles", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);

	RegDeleteTreeA(hKeyMain, "Zones");
	RegCreateKeyEx(hKeyMain, "Zones", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);

	for (auto jIter = profiles.begin(); jIter < profiles.end(); jIter++) {
		string name = "Profile-" + to_string((*jIter)->id);
		RegSetValueExA(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)(*jIter)->name.c_str(), (DWORD)(*jIter)->name.length());
		name = "Profile-flags-" + to_string((*jIter)->id);
		DWORD flagset = MAKELONG((*jIter)->flags, (*jIter)->effmode ? (*jIter)->effmode - 1 : 3);
		RegSetValueExA(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));
		name = "Profile-gflags-" + to_string((*jIter)->id);
		flagset = MAKELONG((*jIter)->flags, (*jIter)->effmode);
		RegSetValueExA(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));

		for (int i = 0; i < (*jIter)->triggerapp.size(); i++) {
			name = "Profile-app-" + to_string((*jIter)->id) + "-" + to_string(i);
			RegSetValueExA(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)(*jIter)->triggerapp[i].c_str(), (DWORD)(*jIter)->triggerapp[i].length());
		}

		for (auto iIter = (*jIter)->lightsets.begin(); iIter < (*jIter)->lightsets.end(); iIter++) {
			string fname;
			name = to_string((*jIter)->id) + "-" + to_string(iIter->group->gid);
			// flags...
			fname = "Zone-flags-" + name;
			DWORD value = 0; byte* buffer = (BYTE*)&value;
			buffer[0] = iIter->fromColor;
			buffer[1] = iIter->gauge;
			buffer[2] = iIter->gradient;
			buffer[3] = iIter->group->have_power;
			RegSetValueExA(hKeyZones, fname.c_str(), 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));

			if (iIter->color.size()) { // colors
				fname = "Zone-colors-" + name + "-" + to_string(iIter->color.size());
				AlienFX_SDK::afx_act* buffer = new AlienFX_SDK::afx_act[iIter->color.size()];
				for (int i = 0; i < iIter->color.size(); i++)
					buffer[i] = iIter->color[i];
				RegSetValueExA(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, (DWORD)iIter->color.size() * sizeof(AlienFX_SDK::afx_act));
				delete[] buffer;
			}
			if (iIter->events[0].state + iIter->events[1].state + iIter->events[2].state) { //events
				fname = "Zone-events-" + name;
				RegSetValueExA(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)iIter->events, (DWORD)3 * sizeof(event));
			}
			if (iIter->ambients.size()) { // ambient
				fname = "Zone-ambient-" + name + "-" + to_string(iIter->ambients.size());
				byte* buffer = new byte[iIter->ambients.size()];
				for (int i = 0; i < iIter->ambients.size(); i++)
					buffer[i] = iIter->ambients[i];
				RegSetValueExA(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, (DWORD)iIter->ambients.size());
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
				RegSetValueExA(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, size);
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
			RegSetValueExA(hKeyProfiles, name.c_str(), 0, REG_BINARY, (BYTE*)buffer, 3 * sizeof(DWORD));
		}
		// Fans....
		if ((*jIter)->flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" + to_string((*jIter)->id);
			DWORD pvalue = MAKELONG((*jIter)->fansets.powerStage, (*jIter)->fansets.GPUPower);
			RegSetValueExA(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&pvalue, sizeof(DWORD));
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

					RegSetValueExA(hKeyProfiles, name.c_str(), 0, REG_BINARY, (BYTE*)outdata, (DWORD)fans->points.size() * 2);
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
	if (map->gauge && afx_dev.GetGrids()->size()) {
		int tempX = 0, tempY = 0;
		vector<vector<DWORD>> tempGrid;
		for (auto t = afx_dev.GetGrids()->begin(); t < afx_dev.GetGrids()->end(); t++) {
			tempY = max(tempY, t->y); tempX = max(tempX, t->x);
			tempGrid.resize(tempY, vector<DWORD>(tempX));
		}
		int minX = tempGrid[0].size(), minY = tempGrid.size(), maxX = 0, maxY = 0;
		//DWORD lgt = MAKELPARAM(map->group->lights[i].first, map->group->lights[i].second);
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
		if (minY > maxY) // no lights in all grids!
			return;
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
				int pos = (int)(tY - tempGrid.begin());
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
			map->lightMap.x = (byte)tempGrid[0].size(); map->lightMap.y = (byte)tempGrid.size();
			map->lightMap.grid = new DWORD[map->lightMap.x * map->lightMap.y];
			for (int y = 0; y < map->lightMap.y; y++)
				for (int x = 0; x < map->lightMap.x; x++)
					map->lightMap.grid[y * map->lightMap.x + x] = tempGrid[y][x];
		}
	}
}