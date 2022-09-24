#include "ConfigHandler.h"
#include "resource.h"

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

extern groupset* FindMapping(int, vector<groupset>*);
extern void SetTrayTip();

ConfigHandler::ConfigHandler() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxgui"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Profiles"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);

	// do we have old configuration?
	//if (RegOpenKeyEx(hKeyMain, "Zones", 0, KEY_ALL_ACCESS, &hKeyZones) != ERROR_SUCCESS) {
	//	if (haveOldConfig = RegOpenKeyEx(hKeyMain, "Events", 0, KEY_ALL_ACCESS, &hKeyZones) == ERROR_SUCCESS)
	//		RegCloseKey(hKeyZones);
		RegCreateKeyEx(hKeyMain, TEXT("Zones"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);
	//}
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

profile* ConfigHandler::FindCreateProfile(unsigned id) {
	profile* prof = FindProfile(id);
	if (!prof) {
		prof = new profile{ id };
		profiles.push_back(prof);
	}
	return prof;
}

groupset* ConfigHandler::FindCreateGroupSet(int profID, int groupID)
{
	profile* prof = FindProfile(profID);
	AlienFX_SDK::group* grp = afx_dev.GetGroupById(groupID);
	if (prof && grp) {
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
	auto prof = find_if(profiles.begin(), profiles.end(),
		[this,appName,active](profile* p) {
			return (active || !(p->flags & PROF_ACTIVE)) &&
				SamePower(p->triggerFlags, true) &&
				(find_if(p->triggerapp.begin(), p->triggerapp.end(),
					[appName](string app) {
						return app == appName;
					}) != p->triggerapp.end());
		});
	return prof == profiles.end() ? NULL : *prof;
}

bool ConfigHandler::IsPriorityProfile(profile* prof) {
	return prof && (prof->flags & PROF_PRIORITY);
}

bool ConfigHandler::SetStates() {
	bool oldStateOn = stateOn, oldStateDim = stateDimmed, oldPBState = finalPBState;
	// Lights on state...
	stateOn = lightsOn && stateScreen && (!offOnBattery || statePower);
	// Dim state...
	stateDimmed = IsDimmed() || dimmedScreen || (dimmedBatt && !statePower);
	finalBrightness = (byte)(stateOn ? stateDimmed ? 255 - dimmingPower : 255 : 0);
	finalPBState = finalBrightness ? stateDimmed ? (byte)dimPowerButton : 1 : (byte)offPowerButton;

	if (oldStateOn != stateOn || oldStateDim != stateDimmed || oldPBState != (bool)finalPBState) {
		SetIconState();
		SetTrayTip();
		return true;
	}
	return false;
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
	return enableMon ? activeProfile->effmode : 0;
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
	GetReg("ShowGridNames", &showGridNames);
	RegGetValue(hKeyMain, NULL, TEXT("CustomColors"), RRF_RT_REG_BINARY | RRF_ZEROONFAILURE, NULL, customColors, &size_c);

	// Ambient....
	GetReg("Ambient-Shift", &amb_shift);
	GetReg("Ambient-Mode", &amb_mode);
	GetReg("Ambient-Grid", &amb_grid, 0x30004);

	// Haptics....
	GetReg("Haptics-Input", &hap_inpType);

	char name[256];
	int pid, appid, profID, groupID, recSize;
	BYTE* data = NULL;
	DWORD len = 255, lend = 0;
	profile* prof;
	groupset* gset;

	// Profiles...
	for (int vindex = 0; RegEnumValue(hKeyProfiles, vindex, name, &len, NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		len++;
		if (data) delete[] data;
		data = new BYTE[lend];
		RegEnumValue(hKeyProfiles, vindex, name, &len, NULL, NULL, data, &lend);
		len = 255;
		if (sscanf_s(name, "Profile-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			prof->name = (char*)data;
			continue;
		}
		if (sscanf_s(name, "Profile-triggers-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			prof->triggerFlags = LOWORD(*(DWORD*)data);
			prof->triggerkey = HIWORD(*(DWORD*)data);
			continue;
		}
		if (sscanf_s(name, "Profile-gflags-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			prof->flags = LOWORD(*(DWORD*)data);
			prof->effmode = HIWORD(*(DWORD*)data);
			if (prof->effmode == 99) prof->effmode = 0;
			continue;
		}
		if (sscanf_s(name, "Profile-app-%d-%d", &pid, &appid) == 2) {
			//PathStripPath((char*)data);
			prof = FindCreateProfile(pid);
			prof->triggerapp.push_back((char*)data);
			continue;
		}
		int senid, fanid;
		if (sscanf_s(name, "Profile-fans-%d-%d-%d", &pid, &senid, &fanid) == 3) {
			// add fans...
			fan_block fan{ (short)fanid };
			for (unsigned i = 0; i < lend; i += 2) {
				fan.points.push_back({ data[i], data[i + 1] });
			}
			prof = FindCreateProfile(pid);
			auto cf = find_if(prof->fansets.fanControls.begin(), prof->fansets.fanControls.end(),
				[senid](auto t) {
					return t.sensorIndex == senid;
				});
			if (cf != prof->fansets.fanControls.end()) {
				cf->fans.push_back(fan);
			}
			else {
				prof->fansets.fanControls.push_back({ (short)senid, {fan} });
			}
			continue;
		}
		if (sscanf_s(name, "Profile-effect-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			deviceeffect de{ 0 };
			DWORD* tDat = (DWORD*)data;
			de.globalEffect = LOBYTE(LOWORD(tDat[0]));
			de.globalMode = HIBYTE(LOWORD(tDat[0]));
			de.globalDelay = (byte)HIWORD(tDat[0]);
			de.effColor1.ci = tDat[1];
			de.effColor2.ci = tDat[2];
			prof->effects.push_back(de);
			continue;
		}
		if (sscanf_s(name, "Profile-device-%d-%d", &pid, &senid) == 2) {
			prof = FindCreateProfile(pid);
			prof->effects.push_back(*(deviceeffect*)data);
			continue;
		}
		if (sscanf_s(name, "Profile-power-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			prof->fansets.powerStage = LOWORD(*(DWORD*)data);
			//prof->fansets.GPUPower = LOBYTE(HIWORD(*(DWORD*)data));
			prof->fansets.gmode = HIBYTE(HIWORD(*(DWORD*)data));
		}
	}

	// Loading zones...
	for (int vindex = 0; RegEnumValue(hKeyZones, vindex, name, &len, NULL, NULL, NULL, &lend) == ERROR_SUCCESS; vindex++) {
		len++;
		if (data) delete[] data;
		data = new BYTE[lend];
		//int profID, groupID, recSize;
		//groupset* gset;
		RegEnumValue(hKeyZones, vindex, name, &len, NULL, NULL, (LPBYTE)data, &lend);
		len = 255;
		if (sscanf_s((char*)name, "Zone-flags-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			gset->fromColor = data[0];
			gset->gauge = data[1];
			gset->flags = ((WORD*)data)[1];
			continue;
		}
		if (sscanf_s((char*)name, "Zone-events-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			for (int i = 0; i < 3; i++)
				gset->events[i] = ((event*)data)[i];
			continue;
		}
		if (sscanf_s((char*)name, "Zone-colors-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			AlienFX_SDK::afx_act* clr = (AlienFX_SDK::afx_act*)data;
			for (int i = 0; i < recSize; i++)
				gset->color.push_back(clr[i]);
			continue;
		}
		if (sscanf_s((char*)name, "Zone-ambient-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			for (int i = 0; i < recSize; i++)
				gset->ambients.push_back(data[i]);
			continue;
		}
		if (sscanf_s((char*)name, "Zone-haptics-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			byte* out = data, cbsize = 2 * sizeof(DWORD) + 3;
			freq_map newFreq;
			for (int i = 0; i < recSize; i++) {
				newFreq.freqID.clear();
				memcpy(&newFreq, out, cbsize);
				out += cbsize;
				for (int j = 0; j < newFreq.freqsize; j++) {
					if (find(newFreq.freqID.begin(), newFreq.freqID.end(), *out) == newFreq.freqID.end())
						newFreq.freqID.push_back(*out); out++;
				}
				gset->haptics.push_back(newFreq);
			}
			continue;
		}
		if (sscanf_s((char*)name, "Zone-effect-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			memcpy(&gset->effect, data, sizeof(grideffect));
		}
	}

	if (data) delete[] data;

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

bool ConfigHandler::SamePower(WORD flags, bool anyFit) {
	return (flags & (statePower ? PROF_TRIGGER_AC : PROF_TRIGGER_BATTERY)) ||
		(anyFit && !(flags & (PROF_TRIGGER_AC | PROF_TRIGGER_BATTERY)));
}

profile* ConfigHandler::FindDefaultProfile() {
	auto res = find_if(profiles.begin(), profiles.end(),
		[this](profile* prof) {
			return !prof->triggerapp.size() && SamePower(prof->triggerFlags);
		});
	if (res == profiles.end())
		res = find_if(profiles.begin(), profiles.end(),
			[](profile* prof) {
				return (prof->flags & PROF_DEFAULT);
			});
	return res == profiles.end() ? profiles.front() : *res;
}

void ConfigHandler::Save() {

	if (fan_conf) fan_conf->Save();

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
	SetReg("ShowGridNames", showGridNames);

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
		name = "Profile-gflags-" + to_string((*jIter)->id);
		flagset = MAKELONG((*jIter)->flags, (*jIter)->effmode);
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));
		flagset = MAKELONG((*jIter)->triggerFlags, (*jIter)->triggerkey);
		if (flagset) {
			name = "Profile-triggers-" + to_string((*jIter)->id);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&flagset, sizeof(DWORD));
		}

		for (int i = 0; i < (*jIter)->triggerapp.size(); i++) {
			name = "Profile-app-" + to_string((*jIter)->id) + "-" + to_string(i);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)(*jIter)->triggerapp[i].c_str(), (DWORD)(*jIter)->triggerapp[i].length());
		}

		for (auto iIter = (*jIter)->lightsets.begin(); iIter < (*jIter)->lightsets.end(); iIter++) {
			string fname;
			name = to_string((*jIter)->id) + "-" + to_string(iIter->group);
			// flags...
			fname = "Zone-flags-" + name;
			DWORD value = MAKELPARAM(MAKEWORD(iIter->fromColor, iIter->gauge), iIter->flags);
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
				DWORD size = 0, recSize = 2 * sizeof(DWORD) + 3;
				for (auto it = iIter->haptics.begin(); it < iIter->haptics.end(); it++)
					size += recSize + (DWORD)it->freqID.size();
				byte* buffer = new byte[size], * out = buffer;
				for (auto it = iIter->haptics.begin(); it < iIter->haptics.end(); it++) {
					it->freqsize = (byte) it->freqID.size();
					memcpy(out, &(*it), recSize);
					out += recSize;
					for (auto itf = it->freqID.begin(); itf < it->freqID.end(); itf++) {
						*out = *itf; out++;
					}
				}
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, size);
				delete[] buffer;
			}
			if (iIter->effect.trigger) { // Grid effects
				fname = "Zone-effect-" + name;
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)&iIter->effect, sizeof(grideffect));
			}
		}

		// Global effects
		for (auto it = (*jIter)->effects.begin(); it != (*jIter)->effects.begin(); it++) {
			name = "Profile-device-" + to_string((*jIter)->id) + "-" + to_string(it - (*jIter)->effects.begin());
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_BINARY, (byte*)&(*it), sizeof(deviceeffect));
		}
		// Fans....
		if ((*jIter)->flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" + to_string((*jIter)->id);
			WORD ps = MAKEWORD(0/*(*jIter)->fansets.GPUPower*/, (*jIter)->fansets.gmode);
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
	if (!grp || grp->lights.empty()) return;

	zonemap* zone = FindZoneMap(gid);
	if (zone)
		zone->lightMap.clear();
	else {
		zoneMaps.push_back({ (DWORD)gid });
		zone = &zoneMaps.back();
	}

	// find operational grid...
	//DWORD lgt = MAKELPARAM(grp->lights.front().first, grp->lights.front().second);
	AlienFX_SDK::lightgrid* opGrid = NULL;
	for (auto t = afx_dev.GetGrids()->begin(); !opGrid && t < afx_dev.GetGrids()->end(); t++)
		for (int ind = 0; ind < t->x * t->y; ind++)
			if (t->grid[ind] == grp->lights.front()) {
				zone->gridID = t->id;
				opGrid = &(*t);
				break;
			}
	if (!opGrid)
		return;

	// scan light positions in grid...
	zone->gMinX = zone->gMinY = 255;
	for (auto lgh = grp->lights.begin(); lgh < grp->lights.end(); lgh++) {
		//lgt = MAKELPARAM(lgh->first, lgh->second);
		zonelight cl{ *lgh, 255, 255 };
		for (int ind = 0; ind < opGrid->x * opGrid->y; ind++)
			if (opGrid->grid[ind] == *lgh) {
				cl.x = min(cl.x, ind % opGrid->x);
				cl.y = min(cl.y, ind / opGrid->x);
				zone->gMaxX = max(zone->gMaxX, ind % opGrid->x);
				zone->gMaxY = max(zone->gMaxY, ind / opGrid->x);
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