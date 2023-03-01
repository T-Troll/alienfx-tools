#include "ConfigHandler.h"
#include "ConfigFan.h"
#include "common.h"
#include "resource.h"

extern HWND mDlg;
extern ConfigFan* fan_conf;

ConfigHandler::ConfigHandler() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxgui"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Profiles"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Zones"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);

	fan_conf = new ConfigFan();
}

ConfigHandler::~ConfigHandler() {
	Save();
	delete fan_conf;
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
		if (SamePower(*prof) && (active || !IsActiveOnly(*prof))) {
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
	return (prof ? prof->flags : activeProfile->flags) & PROF_PRIORITY;
}

bool ConfigHandler::IsActiveOnly(profile* prof) {
	return (prof ? prof->flags : activeProfile->flags) & PROF_ACTIVE;
}

bool ConfigHandler::SetStates() {
	bool oldStateOn = stateOn, oldStateDim = stateDimmed, oldStateEffect = stateEffects;
	// Lights on state...
	stateOn = lightsOn && stateScreen && (!offOnBattery || statePower);
	// Dim state...
	stateDimmed = dimmed || activeProfile->flags & PROF_DIMMED || (dimmedBatt && !statePower);
	// Effects state...
	stateEffects = stateOn && enableEffects && (effectsOnBattery || statePower) && activeProfile->effmode;
	// Brightness
	finalBrightness = (byte)(stateOn ? stateDimmed ? 255 - dimmingPower : 255 : 0);
	// Power button state
	finalPBState = stateOn ? !stateDimmed || dimPowerButton : offPowerButton;

	if (oldStateOn != stateOn || oldStateDim != stateDimmed || oldStateEffect != stateEffects) {
		if (mDlg)
			SetIconState();
		return true;
	}
	return false;
}

void ConfigHandler::SetIconState() {
	// change tray icon...
	niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
						stateOn ? stateDimmed ? MAKEINTRESOURCE(IDI_ALIENFX_DIM) : MAKEINTRESOURCE(IDI_ALIENFX_ON) : MAKEINTRESOURCE(IDI_ALIENFX_OFF),
						IMAGE_ICON,	GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	AddTrayIcon(&niData, false);
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

	afx_dev.LoadMappings();

	DWORD size_c = 16*sizeof(DWORD);// 4 * 16
	DWORD activeProfileID = 0;

	GetReg("AutoStart", &startWindows);
	GetReg("Minimized", &startMinimized);
	GetReg("UpdateCheck", &updateCheck, 1);
	GetReg("LightsOn", &lightsOn, 1);
	GetReg("Dimmed", &dimmed);
	GetReg("Monitoring", &enableEffects, 1);
	GetReg("EffectsOnBattery", &effectsOnBattery, 1);
	GetReg("GammaCorrection", &gammaCorrection, 1);
	GetReg("DisableAWCC", &awcc_disable);
	GetReg("ProfileAutoSwitch", &enableProfSwitch);
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
	GetReg("Ambient-Shift", &amb_shift, 40);
	GetReg("Ambient-Mode", &amb_mode);
	GetReg("Ambient-Grid", &amb_grid.ag, 0x30004);

	// Haptics....
	GetReg("Haptics-Input", &hap_inpType);

	char name[256];
	int pid, appid, profID, groupID, recSize;
	BYTE* data = NULL;
	DWORD len = 255, lend = 0;
	profile* prof;
	groupset* gset;
	byte cbsize = 2 * sizeof(DWORD) + 3;

	// Profiles...
	for (int vindex = 0; lend = GetRegData(hKeyProfiles, vindex, name, &data); vindex++) {
		if (sscanf_s(name, "Profile-%d", &pid) == 1) {
			FindCreateProfile(pid)->name = (char*)data;
			continue;
		}
		if (sscanf_s(name, "Profile-triggers-%d", &pid) == 1) {
			FindCreateProfile(pid)->triggers = *(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "Profile-gflags-%d", &pid) == 1) {
			FindCreateProfile(pid)->gflags = *(DWORD*)data;
			continue;
		}
		if (sscanf_s(name, "Profile-app-%d-%d", &pid, &appid) == 2) {
			FindCreateProfile(pid)->triggerapp.push_back((char*)data);
			continue;
		}
		int senid, fanid;
		if (sscanf_s(name, "Profile-fan-%d-%d-%d", &pid, &fanid, &senid) == 3) {
			((ConfigFan*)fan_conf)->AddSensorCurve(((fan_profile*)FindCreateProfile(pid)->fansets), fanid, senid, data, lend);
			continue;
		}
		if (sscanf_s(name, "Profile-device-%d-%d", &pid, &senid) == 2) {
			FindCreateProfile(pid)->effects.push_back(*(deviceeffect*)data);
			continue;
		}
		if (sscanf_s(name, "Profile-power-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			if (!prof->fansets)
				prof->fansets = new fan_profile();
			((fan_profile*)prof->fansets)->powerSet = *(DWORD*)data;
			//((fan_profile*)prof->fansets)->powerStage = ((WORD*)data)[0];
			//((fan_profile*)prof->fansets)->gmode = HIBYTE(((WORD*)data)[1]);
		}
	}
	// Loading zones...
	for (int vindex = 0; lend = GetRegData(hKeyZones, vindex, name, &data); vindex++) {
		if (sscanf_s((char*)name, "Zone-flags-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			gset->fromColor = data[0];
			gset->gauge = data[1];
			gset->gaugeflags = ((WORD*)data)[1];
			continue;
		}
		if (sscanf_s((char*)name, "Zone-eventlist-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			event* ev = (event*)data;
			for (int i = 0; i * sizeof(event) < lend; i++) {
				// Bugfix for broken events
				if (ev[i].state <= MON_TYPE_IND && ev[i].source <= 10) {
					gset->events.push_back(ev[i]);
				}
			}
			continue;
		}
		if (sscanf_s((char*)name, "Zone-colors-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			AlienFX_SDK::Afx_action* clr = (AlienFX_SDK::Afx_action*)data;
			for (int i = 0; i < recSize; i++) {
				// Power mode patch (not used in UI anymore
				if (clr[i].type == AlienFX_SDK::AlienFX_A_Power)
					clr[i].type = AlienFX_SDK::AlienFX_A_Color;
				gset->color.push_back(clr[i]);
			}
			continue;
		}
		if (sscanf_s((char*)name, "Zone-ambient-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			gset->ambients.resize(recSize);
			memcpy(gset->ambients.data(), data, recSize);
			continue;
		}
		if (sscanf_s((char*)name, "Zone-haptics-%d-%d-%d", &profID, &groupID, &recSize) == 3 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			byte* out = data;
			freq_map newFreq;
			for (int i = 0; i < recSize; i++) {
				memcpy(&newFreq, out, cbsize);
				out += cbsize;
				newFreq.freqID.resize(newFreq.freqsize);
				memcpy(newFreq.freqID.data(), out, newFreq.freqsize);
				out += newFreq.freqsize;
				gset->haptics.push_back(newFreq);
			}
			continue;
		}
		if (sscanf_s((char*)name, "Zone-effect-%d-%d", &profID, &groupID) == 2 &&
			(gset = FindCreateGroupSet(profID, groupID))) {
			auto ce = &gset->effect;
			memcpy(ce, data, 7);
			//ce->effectColors.resize(gset->effect.numclr);
			//memcpy(ce->effectColors.data(), data + 7, gset->effect.numclr);
			for (unsigned pos = 7; pos < lend; pos += sizeof(DWORD)) {
				ce->effectColors.push_back(*(AlienFX_SDK::Afx_colorcode*)(data+pos));
			}
		}
	}

	if (profiles.empty()) {
		// need new profile
		profile* prof = new profile({ 0, "Default", {PROF_DEFAULT, 1} });
		profiles.push_back(prof);
	}

	if (!(activeProfile = FindProfile(activeProfileID))) activeProfile = FindDefaultProfile();

}

bool ConfigHandler::SamePower(profile* cur, profile* prof) {
	WORD cflags = prof ? prof->triggerFlags & (PROF_TRIGGER_AC | PROF_TRIGGER_BATTERY) : statePower ? PROF_TRIGGER_AC : PROF_TRIGGER_BATTERY;
	return !(cur->triggerFlags & (PROF_TRIGGER_AC | PROF_TRIGGER_BATTERY)) || !cflags || cur->triggerFlags & cflags;
}

profile* ConfigHandler::FindDefaultProfile() {
	for (auto prof = profiles.begin(); prof != profiles.end(); prof++)
		if ((*prof)->flags & PROF_DEFAULT && SamePower(*prof, NULL))
			return (*prof);
	return profiles.front();
}

void ConfigHandler::Save() {

	((ConfigFan*)fan_conf)->Save();

	SetReg("AutoStart", startWindows);
	SetReg("Minimized", startMinimized);
	SetReg("UpdateCheck", updateCheck);
	SetReg("LightsOn", lightsOn);
	SetReg("Dimmed", dimmed);
	SetReg("Monitoring", enableEffects);
	SetReg("EffectsOnBattery", effectsOnBattery);
	SetReg("OffWithScreen", offWithScreen);
	SetReg("NoDesktopSwitch", noDesktop);
	SetReg("DimPower", dimPowerButton);
	SetReg("DimmedOnBattery", dimmedBatt);
	SetReg("OffPowerButton", offPowerButton);
	SetReg("OffOnBattery", offOnBattery);
	SetReg("DimmingPower", dimmingPower);
	SetReg("ActiveProfile", activeProfile->id);
	SetReg("GammaCorrection", gammaCorrection);
	SetReg("ProfileAutoSwitch", enableProfSwitch);
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
	SetReg("Ambient-Grid", amb_grid.ag);

	// Haptics
	SetReg("Haptics-Input", hap_inpType);

	RegDeleteTree(hKeyMain, "Profiles");
	RegCreateKeyEx(hKeyMain, "Profiles", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);

	RegDeleteTree(hKeyMain, "Zones");
	RegCreateKeyEx(hKeyMain, "Zones", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);

	for (auto jIter = profiles.begin(); jIter < profiles.end(); jIter++) {
		auto prof = *jIter;
		string profID = to_string(prof->id);
		string name = "Profile-" + profID;
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)prof->name.c_str(), (DWORD)prof->name.size());
		name = "Profile-gflags-" + profID;
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&prof->gflags, sizeof(DWORD));
		name = "Profile-triggers-" + profID;
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&prof->triggers, sizeof(DWORD));
		// Trigger applications
		for (int i = 0; i < prof->triggerapp.size(); i++) {
			name = "Profile-app-" +profID + "-" + to_string(i);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)prof->triggerapp[i].c_str(), (DWORD)prof->triggerapp[i].size());
		}
		// Zone sets
		for (auto iIter = prof->lightsets.begin(); iIter < prof->lightsets.end(); iIter++) {
			string fname;
			name =profID + "-" + to_string(iIter->group);
			// flags...
			fname = "Zone-flags-" + name;
			DWORD value = MAKELPARAM(MAKEWORD(iIter->fromColor, iIter->gauge), iIter->gaugeflags);
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
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)iIter->ambients.data(), (DWORD)iIter->ambients.size());
			}
			if (iIter->haptics.size()) { // haptics
				// ToDo: rebuild to per-group save
				fname = "Zone-haptics-" + name + "-" + to_string(iIter->haptics.size());
				DWORD size = 0, recSize = 2 * sizeof(DWORD) + 3;
				for (auto it = iIter->haptics.begin(); it < iIter->haptics.end(); it++) {
					it->freqsize = (byte)it->freqID.size();
					size += recSize + it->freqsize;
				}
				byte* buffer = new byte[size], * out = buffer;
				for (auto it = iIter->haptics.begin(); it < iIter->haptics.end(); it++) {
					memcpy(out, &(*it), recSize);
					out += recSize;
					memcpy(out, it->freqID.data(), it->freqsize);
					out += it->freqsize;
				}
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, (BYTE*)buffer, size);
				delete[] buffer;
			}
			if (iIter->effect.trigger) { // Grid effects
				fname = "Zone-effect-" + name;
				DWORD size = (DWORD)(iIter->effect.effectColors.size() * sizeof(DWORD));
				byte* buffer = new byte[size + 7];
				memcpy(buffer, &iIter->effect, 7);
				memcpy(buffer + 7, iIter->effect.effectColors.data(), size);
				RegSetValueEx(hKeyZones, fname.c_str(), 0, REG_BINARY, buffer, size + 7);
				delete[] buffer;
			}
		}

		// Global effects
		for (auto it = prof->effects.begin(); it != prof->effects.end(); it++) {
			name = "Profile-device-" +profID + "-" + to_string(it - prof->effects.begin());
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_BINARY, (byte*)&(*it), sizeof(deviceeffect));
		}
		// Fans....
		if (prof->flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" +profID;
			//WORD ps = MAKEWORD(0, prof->fansets.gmode);
			//DWORD pvalue = MAKELONG(prof->fansets.powerStage, ps);
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&((fan_profile*)prof->fansets)->powerSet, sizeof(DWORD));
			// save fans...
			fan_conf->SaveSensorBlocks(hKeyProfiles, "Profile-fan-" +profID, ((fan_profile*)prof->fansets));
		}
	}
}

zonemap* ConfigHandler::FindZoneMap(int gid, bool reset) {
	zonemap* zone = NULL;
	zoneUpdate.lock();
	for (auto gpos = zoneMaps.begin(); gpos != zoneMaps.end(); gpos++)
		if (gpos->gID == gid) {
			if (reset)
				zoneMaps.erase(gpos);
			else
				zone = &(*gpos);
			break;
		}
	if (!zone) {
		// create new zoneMap
		AlienFX_SDK::Afx_group* grp = afx_dev.GetGroupById(gid);

		zoneMaps.push_back({ (DWORD)gid, mainGrid->id });
		zone = &zoneMaps.back();

		if (grp && grp->lights.size()) {
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

			if (opGrid) {

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
							break;
						}
					// Ignore light if not in grid
					if (cl.x < 255) {
						zone->gMinX = min(zone->gMinX, cl.x);
						zone->gMinY = min(zone->gMinY, cl.y);
						zone->lightMap.push_back(cl);
					}
				}

				// now shrink axis...
				for (auto t = zone->lightMap.begin(); t < zone->lightMap.end(); t++) {
					t->x -= zone->gMinX; t->y -= zone->gMinY;
				}
				zone->xMax = zone->gMaxX - zone->gMinX + 1; zone->yMax = zone->gMaxY - zone->gMinY + 1;
				for (int x = 1; x < zone->xMax; x++) {
					if (find_if(zone->lightMap.begin(), zone->lightMap.end(),
						[x](auto t) {
							return t.x == x;
						}) == zone->lightMap.end()) {
						byte minDelta = 255;
						for (auto t = zone->lightMap.begin(); t != zone->lightMap.end(); t++)
							if (t->x > x) minDelta = min(minDelta, t->x - x);
						if (minDelta < 255) {
							for (auto t = zone->lightMap.begin(); t != zone->lightMap.end(); t++)
								if (t->x > x) t->x -= minDelta;
							zone->xMax -= minDelta;
						}
						else
							zone->xMax = x;
					}
				}
				for (int y = 1; y < zone->yMax; y++) {
					if (find_if(zone->lightMap.begin(), zone->lightMap.end(),
						[y](auto t) {
							return t.y == y;
						}) == zone->lightMap.end()) {
						byte minDelta = 255;
						for (auto t = zone->lightMap.begin(); t != zone->lightMap.end(); t++)
							if (t->y > y) minDelta = min(minDelta, t->y - y);
						if (minDelta < 255) {
							for (auto t = zone->lightMap.begin(); t != zone->lightMap.end(); t++)
								if (t->y > y) t->y -= minDelta;
							zone->yMax -= minDelta;
						}
						else
							zone->yMax = y;
					}
				}
			}
		}
	}
	zoneUpdate.unlock();
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
