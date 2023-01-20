#include "ConfigHandler.h"
#include "resource.h"

extern void SetTrayTip();

ConfigHandler::ConfigHandler() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxgui"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Profiles"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Zones"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);

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
	AlienFX_SDK::Afx_group* grp = afx_dev.GetGroupById(groupID);
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
	for (auto prof = profiles.begin(); prof != profiles.end(); prof++)
		if ((*prof)->id == id) {
			return *prof;
		}
	return NULL;
}

profile* ConfigHandler::FindProfileByApp(string appName, bool active)
{
	profile* fprof = NULL;
	for (auto prof = profiles.begin(); prof != profiles.end(); prof++)
		if (SamePower((*prof)->triggerFlags, true) && (active || !((*prof)->flags & PROF_ACTIVE))) {
			for (auto name = (*prof)->triggerapp.begin(); name < (*prof)->triggerapp.end(); name++)
				if (*name == appName) {
					if (IsPriorityProfile(*prof))
						return *prof;
					else
						fprof = *prof;
				}
		}
	return fprof;
}

bool ConfigHandler::IsPriorityProfile(profile* prof) {
	return prof && (prof->flags & PROF_PRIORITY);
}

bool ConfigHandler::SetStates() {
	bool oldStateOn = stateOn, oldStateDim = stateDimmed, oldPBState = finalPBState;
	// Lights on state...
	stateOn = lightsOn && stateScreen && (!offOnBattery || statePower);
	// Dim state...
	stateDimmed = IsDimmed() /*|| dimmedScreen*/ || (dimmedBatt && !statePower);
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

DWORD ConfigHandler::GetRegData(HKEY key, int vindex, char* name, byte** data) {
	DWORD len, lend;
	if (*data) {
		delete[] * data;
		*data = NULL;
	}
	if (RegEnumValue(key, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS) {
		*data = new byte[lend];
		RegEnumValue(key, vindex, name, &(len = 255), NULL, NULL, *data, &lend);
		return lend;
	}
	return 0;
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
	GetReg("FanControl", &fanControl, 1);
	GetReg("ShowGridNames", &showGridNames);
	GetReg("KeyboardShortcut", &keyShortcuts, 1);
	GetReg("GESpeed", &geTact, 100);
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
	for (int vindex = 0; lend = GetRegData(hKeyProfiles, vindex, name, &data); vindex++) {
		if (sscanf_s(name, "Profile-%d", &pid) == 1) {
			FindCreateProfile(pid)->name = (char*)data;
			continue;
		}
		if (sscanf_s(name, "Profile-triggers-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			prof->triggerFlags = ((WORD*)data)[0];
			prof->triggerkey = ((WORD*)data)[1];
			continue;
		}
		if (sscanf_s(name, "Profile-gflags-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			prof->flags = ((WORD*)data)[0];;
			prof->effmode = ((WORD*)data)[1];
			continue;
		}
		if (sscanf_s(name, "Profile-app-%d-%d", &pid, &appid) == 2) {
			FindCreateProfile(pid)->triggerapp.push_back((char*)data);
			continue;
		}
		int senid, fanid;
		if (sscanf_s(name, "Profile-fan-%d-%d-%d", &pid, &fanid, &senid) == 3) {
			fan_conf->AddSensorCurve(&FindCreateProfile(pid)->fansets, fanid, senid, data, lend);
			continue;
		}
		if (sscanf_s(name, "Profile-device-%d-%d", &pid, &senid) == 2) {
			FindCreateProfile(pid)->effects.push_back(*(deviceeffect*)data);
			continue;
		}
		if (sscanf_s(name, "Profile-power-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			prof->fansets.powerStage = ((WORD*)data)[0];
			prof->fansets.gmode = HIBYTE(((WORD*)data)[1]);
		}
	}
	// Loading zones...
	for (int vindex = 0; lend = GetRegData(hKeyZones, vindex, name, &data); vindex++) {
		if (sscanf_s((char*)name, "Zone-flags-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			gset->fromColor = data[0];
			gset->gauge = data[1];
			gset->flags = ((WORD*)data)[1];
			continue;
		}
		if (sscanf_s((char*)name, "Zone-eventlist-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			event* ev = (event*)data;
			for (int i = 0; i * sizeof(event) < lend; i++) {
				// Obsolete conversion, remove after some time
				if (ev[i].state == MON_TYPE_POWER) {
					// convert power to indicator
					ev[i].state = MON_TYPE_IND;
					ev[i].source = 7;
				}
				gset->events.push_back(ev[i]);
			}
			continue;
		}
		if (sscanf_s((char*)name, "Zone-colors-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			AlienFX_SDK::Afx_action* clr = (AlienFX_SDK::Afx_action*)data;
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
			memcpy(&gset->effect, data, 7);
			AlienFX_SDK::Afx_colorcode cc;
			for (unsigned pos = 7; pos < lend; pos += 4) {
				cc.ci = *(DWORD*)(data + pos);
				gset->effect.effectColors.push_back(cc);
			}
		}
	}

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

	SetStates();
}

bool ConfigHandler::SamePower(WORD flags, bool anyFit) {
	return (flags & (statePower ? PROF_TRIGGER_AC : PROF_TRIGGER_BATTERY)) ||
		(anyFit && !(flags & (PROF_TRIGGER_AC | PROF_TRIGGER_BATTERY)));
}

profile* ConfigHandler::FindDefaultProfile() {
	for (auto res = profiles.begin(); res != profiles.end(); res++)
		if (!(*res)->triggerapp.size() && SamePower((*res)->triggerFlags))
			return *res;
	for (auto res = profiles.begin(); res != profiles.end(); res++)
		if ((*res)->flags & PROF_DEFAULT)
			return *res;
	return profiles.front();
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
	SetReg("KeyboardShortcut", keyShortcuts);
	SetReg("GESpeed", geTact);
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
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)(*jIter)->name.c_str(), (DWORD)(*jIter)->name.size());
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
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)iIter->color.data(), (DWORD)iIter->color.size() * sizeof(AlienFX_SDK::Afx_action));
			}
			if (iIter->events.size()) { //events
				fname = "Zone-eventlist-" + name;
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)iIter->events.data(), (DWORD)iIter->events.size() * sizeof(event));
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
				DWORD size = (DWORD)(7 + iIter->effect.effectColors.size() * sizeof(DWORD));
				byte* buffer = new byte[size];
				memcpy(buffer, &iIter->effect, 7);
				for (int cc = 0; cc < iIter->effect.effectColors.size(); cc++)
					*(DWORD*)(buffer + 7 + cc * sizeof(DWORD)) = iIter->effect.effectColors[cc].ci;
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, buffer, size);
				delete[] buffer;
			}
		}

		// Global effects
		for (auto it = (*jIter)->effects.begin(); it != (*jIter)->effects.end(); it++) {
			name = "Profile-device-" + to_string((*jIter)->id) + "-" + to_string(it - (*jIter)->effects.begin());
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_BINARY, (byte*)&(*it), sizeof(deviceeffect));
		}
		// Fans....
		if ((*jIter)->flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" + to_string((*jIter)->id);
			WORD ps = MAKEWORD(0, (*jIter)->fansets.gmode);
			DWORD pvalue = MAKELONG((*jIter)->fansets.powerStage, ps);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&pvalue, sizeof(DWORD));
			// save fans...
			fan_conf->SaveSensorBlocks(hKeyProfiles, "Profile-fan-" + to_string((*jIter)->id), &(*jIter)->fansets);
		}
	}
}

zonemap* ConfigHandler::FindZoneMap(int gid) {
	for (auto gpos = zoneMaps.begin(); gpos != zoneMaps.end(); gpos++)
		if (gpos->gID == gid)
			return &(*gpos);
	return SortGroupGauge(gid);
}

zonemap* ConfigHandler::SortGroupGauge(int gid) {
	AlienFX_SDK::Afx_group* grp = afx_dev.GetGroupById(gid);

	zoneMaps.push_back({ (DWORD)gid, mainGrid->id });
	auto zone = &zoneMaps.back();

	if (!grp || grp->lights.empty()) return zone;

	// find operational grid...
	AlienFX_SDK::Afx_groupLight lgt = grp->lights.front();
	AlienFX_SDK::Afx_grid* opGrid = NULL;
	for (auto t = afx_dev.GetGrids()->begin(); !opGrid && t < afx_dev.GetGrids()->end(); t++)
		for (int ind = 0; ind < t->x * t->y; ind++)
			if (t->grid[ind].lgh == lgt.lgh) {
				zone->gridID = t->id;
				opGrid = &(*t);
				break;
			}
	if (!opGrid) return zone;

	// scan light positions in grid...
	for (auto lgh = grp->lights.begin(); lgh < grp->lights.end(); lgh++) {
		zonelight cl{ lgh->lgh, 255, 255 };
		for (int ind = 0; ind < opGrid->x * opGrid->y; ind++)
			if (opGrid->grid[ind].lgh == lgh->lgh) {
				//cl.x = ind % opGrid->x;
				cl.x = min(cl.x, ind % opGrid->x);
				//cl.y = ind / opGrid->x;
				cl.y = min(cl.y, ind / opGrid->x);
				zone->gMaxX = max(zone->gMaxX, ind % opGrid->x);
				zone->gMaxY = max(zone->gMaxY, ind / opGrid->x);
				zone->gMinX = min(zone->gMinX, cl.x);
				zone->gMinY = min(zone->gMinY, cl.y);
				//zone->lightMap.push_back(cl);
			}
		zone->gMinX = min(zone->gMinX, cl.x);
		zone->gMinY = min(zone->gMinY, cl.y);
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
	return zone;
}

groupset* ConfigHandler::FindMapping(int mid, vector<groupset>* set)
{
	if (!set)
		set = &activeProfile->lightsets;
	for (auto res = set->begin(); res < set->end(); res++)
		if (res->group == mid)
			return &(*res);
	return nullptr;
}

void ConfigHandler::SetRandomColor(AlienFX_SDK::Afx_colorcode* clr) {
	clr->r = (byte)rclr(rnd);
	clr->g = (byte)rclr(rnd);
	clr->b = (byte)rclr(rnd);
}
