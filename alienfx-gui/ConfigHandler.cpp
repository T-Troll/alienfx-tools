#include "ConfigHandler.h"
#include "ConfigFan.h"
#include "RegHelperLib.h"
#include "Common.h"
#include "resource.h"

extern HWND mDlg;
extern ConfigFan* fan_conf;
extern int eItem;

ConfigHandler::ConfigHandler() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxgui"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMain, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Profiles"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);
	RegCreateKeyEx(hKeyMain, TEXT("Zones"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);
	afx_dev.LoadMappings();
	fan_conf = new ConfigFan();
	niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFX_ON),	IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
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

AlienFX_SDK::Afx_group* ConfigHandler::FindCreateGroup(int groupID) {
	AlienFX_SDK::Afx_group* grp = nullptr;
	if (groupID) {
		if (!(grp = afx_dev.GetGroupById(groupID))) {
			afx_dev.GetGroups()->push_back({ (DWORD)groupID, "New zone #" + to_string((groupID & 0xffff) + 1) });
			grp = &afx_dev.GetGroups()->back();
		}
	}
	return grp;
}

groupset* ConfigHandler::FindCreateGroupSet(int profID, int groupID)
{
	profile* prof = FindProfile(profID);
	if (prof) {
		groupset* gset = FindMapping(groupID, &prof->lightsets);
		FindCreateGroup(groupID);
		if (!gset) {
			prof->lightsets.push_back({ groupID });
			gset = &prof->lightsets.back();
		}
		return gset;
	}
	return nullptr;
}

profile* ConfigHandler::FindProfile(int id) {
	for (profile* prof : profiles)
		if (prof->id == id) {
			return prof;
		}
	return NULL;
}

profile* ConfigHandler::FindProfileByApp(string appName, bool active)
{
	profile* fprof = NULL;
	for (profile* prof : profiles)
		if (SamePower(prof) && (active || !(prof->flags & PROF_ACTIVE))) {
			for (auto name : prof->triggerapp)
				if (name == appName) {
					if (IsPriorityProfile(prof))
						return prof;
					else
						fprof = prof;
				}
		}
	return fprof;
}

bool ConfigHandler::IsPriorityProfile(profile* prof) {
	return prof->flags & PROF_PRIORITY;
}

//bool ConfigHandler::IsActiveOnly(profile* prof) {
//	return prof->flags & PROF_ACTIVE;
//}

void ConfigHandler::SetIconState() {
	// change tray icon...
	niData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
		MAKEINTRESOURCE(stateOn ? stateDimmed ? IDI_ALIENFX_DIM : IDI_ALIENFX_ON : IDI_ALIENFX_OFF),
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

void ConfigHandler::Load() {

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
	GetReg("DimmingPower", &dimmingPower, 164);
	GetReg("FullPower", &fullPower, 255);
	GetReg("OffOnBattery", &offOnBattery);
	GetReg("FanControl", &fanControl, 1);
	GetReg("FansOnBattery", &fansOnBattery, 1);
	GetReg("ShowGridNames", &showGridNames);
	GetReg("KeyboardShortcut", &keyShortcuts, 1);
	GetReg("GESpeed", &geTact, 100);
	RegGetValue(hKeyMain, NULL, TEXT("CustomColors"), RRF_RT_REG_BINARY | RRF_ZEROONFAILURE, NULL, customColors, &size_c);

	// Ambient....
	GetReg("Ambient-Shift", &amb_shift, 40);
	GetReg("Ambient-Calc", &amb_calc);
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
			FindCreateProfile(pid)->name = GetRegString(data, lend);
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
			FindCreateProfile(pid)->triggerapp.push_back(GetRegString(data, lend));
			continue;
		}
		if (sscanf_s(name, "Profile-script-%d", &pid) == 1) {
			FindCreateProfile(pid)->script = GetRegString(data, lend);
			continue;
		}
		int senid, fanid;
		if (sscanf_s(name, "Profile-fan-%d-%d-%d", &pid, &fanid, &senid) == 3) {
			((ConfigFan*)fan_conf)->AddSensorCurve(((fan_profile*)FindCreateProfile(pid)->fansets), fanid, senid, data, lend);
			continue;
		}
		if (sscanf_s(name, "Profile-effect-%d-%d", &pid, &senid) == 2) {
			FindCreateProfile(pid)->effects.push_back(*(deviceeffect*)data);
			continue;
		}
		if (sscanf_s(name, "Profile-power-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			if (!prof->fansets)
				prof->fansets = new fan_profile();
			((fan_profile*)prof->fansets)->powerSet = *(DWORD*)data;
		}
		if (sscanf_s(name, "Profile-OC-%d", &pid) == 1) {
			prof = FindCreateProfile(pid);
			if (!prof->fansets)
				prof->fansets = new fan_profile();
			((fan_profile*)prof->fansets)->ocSettings = *(DWORD*)data;
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
			ce->trigger = min(ce->trigger, 4);
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
	for (profile* prof : profiles)
		if (prof->flags & PROF_DEFAULT && SamePower(prof, NULL))
			return prof;
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
	SetReg("FullPower", fullPower);
	SetReg("ActiveProfile", activeProfile->id);
	SetReg("GammaCorrection", gammaCorrection);
	SetReg("ProfileAutoSwitch", enableProfSwitch);
	SetReg("DisableAWCC", awcc_disable);
	SetReg("EsifTemp", esif_temp);
	SetReg("FanControl", fanControl);
	SetReg("FansOnBattery", fansOnBattery);
	SetReg("ShowGridNames", showGridNames);
	SetReg("KeyboardShortcut", keyShortcuts);
	SetReg("GESpeed", geTact);
	RegSetValueEx(hKeyMain, TEXT("CustomColors"), 0, REG_BINARY, (BYTE*)customColors, sizeof(DWORD) * 16);

	// Ambient
	SetReg("Ambient-Shift", amb_shift);
	SetReg("Ambient-Calc", amb_calc);
	SetReg("Ambient-Mode", amb_mode);
	SetReg("Ambient-Grid", amb_grid.ag);

	// Haptics
	SetReg("Haptics-Input", hap_inpType);

	RegDeleteTree(hKeyMain, "Profiles");
	RegCreateKeyEx(hKeyMain, "Profiles", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyProfiles, NULL);

	RegDeleteTree(hKeyMain, "Zones");
	RegCreateKeyEx(hKeyMain, "Zones", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyZones, NULL);

	for (profile* prof : profiles) {
		string profID = to_string(prof->id);
		string name = "Profile-" + profID;
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)prof->name.c_str(), (DWORD)prof->name.size());
		name = "Profile-gflags-" + profID;
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&prof->gflags, sizeof(DWORD));
		name = "Profile-triggers-" + profID;
		RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&prof->triggers, sizeof(DWORD));
		if (prof->script.size()) {
			name = "Profile-script-" + profID;
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_SZ, (BYTE*)prof->script.c_str(), (DWORD)prof->script.size());
		}
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
			name = "Profile-effect-" + profID + "-" + to_string(it - prof->effects.begin());
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_BINARY, (byte*)&(*it), sizeof(deviceeffect));
		}
		// Fans....
		if (prof->flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" + profID;
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&((fan_profile*)prof->fansets)->powerSet, sizeof(DWORD));
			name = "Profile-OC-" + profID;
			RegSetValueEx(hKeyProfiles, name.c_str(), 0, REG_DWORD, (BYTE*)&((fan_profile*)prof->fansets)->ocSettings, sizeof(DWORD));
			// save fans...
			fan_conf->SaveSensorBlocks(hKeyProfiles, "Profile-fan-" + profID, ((fan_profile*)prof->fansets));
		}
	}
}

zonemap* ConfigHandler::FindZoneMap(int gid, bool reset) {
	zoneUpdate.lock();
		if (!(zoneMaps[gid].gMinX == 255 || reset)) {
		zoneUpdate.unlock();
		return &zoneMaps[gid];
	}

	// create new zoneMap
	auto zone = &zoneMaps[gid];
	*zone = { mainGrid->id };

	AlienFX_SDK::Afx_group* grp = afx_dev.GetGroupById(gid);
	if (grp && grp->lights.size()) {
		// find operational grid...
		DWORD lgt = grp->lights.front().lgh;
		AlienFX_SDK::Afx_grid* opGrid = NULL;
		for (auto t = afx_dev.GetGrids()->begin(); !opGrid && t < afx_dev.GetGrids()->end(); t++)
			for (int ind = 0; ind < t->x * t->y; ind++)
				if (t->grid[ind].lgh == lgt) {
					zone->gridID = t->id;
					opGrid = &(*t);
					break;
				}

		if (opGrid) {

			// scan light positions in grid and set power
			zone->havePower = false;
			for (AlienFX_SDK::Afx_groupLight& lgh : grp->lights) {
				if (afx_dev.GetFlags(lgh.did, lgh.lid) & ALIENFX_FLAG_POWER)
					zone->havePower = true;
				zonelight cl{ lgh.lgh, 255, 255 };
				for (int ind = 0; ind < opGrid->x * opGrid->y; ind++)
					if (opGrid->grid[ind].lgh == lgh.lgh) {
						cl.x = min(cl.x, ind % opGrid->x);
						cl.y = min(cl.y, ind / opGrid->x);
						zone->xMax = max(zone->xMax, ind % opGrid->x);
						zone->yMax = max(zone->yMax, ind / opGrid->x);
						//zone->gMaxX = max(zone->gMaxX, cl.x);
						//zone->gMaxY = max(zone->gMaxY, cl.y);
						//zone->gMinX = min(zone->gMinX, cl.x);
						//zone->gMinY = min(zone->gMinY, cl.y);
						//zone->lightMap.push_back(cl);
						//break;
					}
				// Ignore light if not in grid
				if (cl.x < 255 && cl.y < 255) {
					zone->gMaxX = max(zone->gMaxX, cl.x);
					zone->gMaxY = max(zone->gMaxY, cl.y);
					zone->gMinX = min(zone->gMinX, cl.x);
					zone->gMinY = min(zone->gMinY, cl.y);
					zone->lightMap.push_back(cl);
				}
			}

			// now shrink axis...
			for (zonelight& t : zone->lightMap) {
				t.x -= zone->gMinX; t.y -= zone->gMinY;
			}
			// Scales...
			zone->scaleX = zone->gMaxX = zone->gMaxX + 1 - zone->gMinX;
			zone->scaleY = zone->gMaxY = zone->gMaxY + 1 - zone->gMinY;
			for (int x = 1; x < zone->gMaxX; x++)
				if (none_of(zone->lightMap.begin(), zone->lightMap.end(),
					[x](auto t) {
						return t.x == x;
					})) {
					zone->scaleX--;
				}
			for (int y = 1; y < zone->gMaxY; y++)
				if (none_of(zone->lightMap.begin(), zone->lightMap.end(),
					[y](auto t) {
						return t.y == y;
					})) {
					zone->scaleY--;
				}
		}
		else
			zone->gMinX = 0;
	}
	zoneUpdate.unlock();
	return zone;
}

groupset* ConfigHandler::FindMapping(int mid, vector<groupset>* set)
{
	if (!set)
		set = &activeProfile->lightsets;
	for (groupset& res : *set)
		if (res.group == mid)
			return &res;
	return nullptr;
}

void ConfigHandler::SetRandomColor(AlienFX_SDK::Afx_colorcode* clr) {
	clr->r = (byte)rclr(rnd);
	clr->g = (byte)rclr(rnd);
	clr->b = (byte)rclr(rnd);
}
