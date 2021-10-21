#include "ConfigHandler.h"
#include "resource.h"
#include <algorithm>
#include <Shlwapi.h>

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)  
#endif

ConfigHandler::ConfigHandler() {
	DWORD  dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\Alienfxgui"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey1,
		&dwDisposition);
	//RegCreateKeyEx(HKEY_CURRENT_USER,
	//	TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
	//	0,
	//	NULL,
	//	REG_OPTION_NON_VOLATILE,
	//	KEY_ALL_ACCESS,
	//	NULL,
	//	&hKey2,
	//	&dwDisposition);
	RegCreateKeyEx(HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\Alienfxgui\\Events"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey3,
		&dwDisposition);
	RegCreateKeyEx(HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\Alienfxgui\\Profiles"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey4,
		&dwDisposition);

	testColor.ci = 0;
	testColor.cs.green = 255;

	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uID = IDI_ALIENFXGUI;
	niData.uFlags = NIF_ICON | NIF_MESSAGE;
	niData.uCallbackMessage = WM_APP + 1;
}

ConfigHandler::~ConfigHandler() {
	Save();
	if (fan_conf) delete fan_conf;
	if (amb_conf) delete amb_conf;
	if (hap_conf) delete hap_conf;
	RegCloseKey(hKey1);
	//RegCloseKey(hKey2);
	RegCloseKey(hKey3);
	RegCloseKey(hKey4);
}

//bool ConfigHandler::sortMappings(lightset i, lightset j) { return (i.lightid < j.lightid); };

void ConfigHandler::updateProfileByID(unsigned id, std::string name, std::string app, DWORD flags) {
	profile* prof = FindProfile(id);
	if (prof) {
		// update profile..
		if (name != "")
			prof->name = name;
		if (app != "")
			prof->triggerapp = app;
		if (flags != -1) {
			prof->flags = LOWORD(flags);
			prof->effmode = HIWORD(flags);
		}
		return;
	}
	prof = new profile{id, LOWORD(flags), HIWORD(flags), app, name};
	profiles.push_back(*prof);
}

void ConfigHandler::updateProfileFansByID(unsigned id, unsigned senID, fan_block* temp) {

	profile* prof = FindProfile(id);
	if (prof) {
		for (int i = 0; i < prof->fansets.fanControls.size(); i++)
			if (prof->fansets.fanControls[i].sensorIndex == senID) {
				prof->fansets.fanControls[i].fans.push_back(*temp);
				return;
			}
		temp_block temp_b = {(short)senID};
		temp_b.fans.push_back(*temp);
		prof->fansets.fanControls.push_back(temp_b);
		return;
	} else {
		prof = new profile{id};
		temp_block temp_b = {(short) senID};
		temp_b.fans.push_back(*temp);
		prof->fansets.fanControls.push_back(temp_b);
		profiles.push_back(*prof);
	}
}

profile* ConfigHandler::FindProfile(int id) {
	profile* prof = NULL;
	for (int i = 0; i < profiles.size(); i++)
		if (profiles[i].id == id) {
			prof = &profiles[i];
			break;
		}
	return prof;
}

profile* ConfigHandler::FindProfileByApp(std::string appName, bool active)
{
	for (int j = 0; j < profiles.size(); j++)
		if (profiles[j].triggerapp == appName && (active || !(profiles[j].flags & PROF_ACTIVE))) {
			// app is belong to profile!
			return &profiles[j];
		}
	return NULL;
}

bool ConfigHandler::IsPriorityProfile(int id) {
	profile *prof = FindProfile(id);
	return prof && prof->flags & PROF_PRIORITY;
}

void ConfigHandler::SetStates() {
	bool oldStateOn = stateOn, oldStateDim = stateDimmed;
	// Lighs on state...
	stateOn = lightsOn && stateScreen;
	// Dim state...
	stateDimmed = IsDimmed() ||
		dimmedScreen ||
		//FindProfile(activeProfile)->flags.flags & PROF_DIMMED ||
		(dimmedBatt && !statePower);
	if (oldStateOn != stateOn || oldStateDim != stateDimmed) {
		SetIconState();
		Shell_NotifyIcon(NIM_MODIFY, &niData);
	}
}

void ConfigHandler::SetIconState() {
	// change tray icon...
	if (stateOn) {
		if (stateDimmed) {
			niData.hIcon =
				(HICON)LoadImage(GetModuleHandle(NULL),
								 MAKEINTRESOURCE(IDI_ALIENFX_DIM),
								 IMAGE_ICON,
								 GetSystemMetrics(SM_CXSMICON),
								 GetSystemMetrics(SM_CYSMICON),
								 LR_DEFAULTCOLOR);
		} else {
			niData.hIcon =
				(HICON)LoadImage(GetModuleHandle(NULL),
								 MAKEINTRESOURCE(IDI_ALIENFX_ON),
								 IMAGE_ICON,
								 GetSystemMetrics(SM_CXSMICON),
								 GetSystemMetrics(SM_CYSMICON),
								 LR_DEFAULTCOLOR);
		}
	} else {
		niData.hIcon =
			(HICON) LoadImage(GetModuleHandle(NULL),
							  MAKEINTRESOURCE(IDI_ALIENFX_OFF),
							  IMAGE_ICON,
							  GetSystemMetrics(SM_CXSMICON),
							  GetSystemMetrics(SM_CYSMICON),
							  LR_DEFAULTCOLOR);
	}
}

bool ConfigHandler::IsDimmed() {
	return FindProfile(activeProfile)->flags & PROF_DIMMED;
}

bool ConfigHandler::IsMonitoring() {
	return enableMon;// || !(FindProfile(activeProfile)->flags & PROF_NOMONITORING);
}

void ConfigHandler::SetDimmed(bool dimmed) {
	if (dimmed)
		FindProfile(activeProfile)->flags |= PROF_DIMMED;
	else
		FindProfile(activeProfile)->flags &= 0xff - PROF_DIMMED;
}
int ConfigHandler::GetEffect() {
	return FindProfile(activeProfile)->effmode;
}
void ConfigHandler::SetEffect(int newMode) {
	FindProfile(activeProfile)->effmode = newMode;
}
//
//void ConfigHandler::SetMonitoring(bool monison) {
//	if (monison)
//		FindProfile(activeProfile)->flags &= 0xff - PROF_NOMONITORING;
//	else
//		FindProfile(activeProfile)->flags |= PROF_NOMONITORING;
//}

int ConfigHandler::Load() {
	int size = 4, size_c = 4 * 16;

	RegGetValue(hKey1,
		NULL,
		TEXT("AutoStart"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&startWindows,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("Minimized"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&startMinimized,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("Refresh"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&autoRefresh,
		(LPDWORD)&size);
	unsigned ret = RegGetValue(hKey1,
		NULL,
		TEXT("LightsOn"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&lightsOn,
		(LPDWORD)&size);
	if (ret != ERROR_SUCCESS)
		lightsOn = 1;
	ret = RegGetValue(hKey1,
		NULL,
		TEXT("Monitoring"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&enableMon,
		(LPDWORD)&size);
	if (ret != ERROR_SUCCESS)
		enableMon = 1;
	ret = RegGetValue(hKey1,
		NULL,
		TEXT("GammaCorrection"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&gammaCorrection,
		(LPDWORD)&size);
	if (ret != ERROR_SUCCESS)
		gammaCorrection = 1;
	ret = RegGetValue(hKey1,
		NULL,
		TEXT("DisableAWCC"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&awcc_disable,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("ProfileAutoSwitch"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&enableProf,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("OffWithScreen"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&offWithScreen,
		(LPDWORD)&size);
	//RegGetValue(hKey1,
	//	NULL,
	//	TEXT("EffectMode"),
	//	RRF_RT_DWORD | RRF_ZEROONFAILURE,
	//	NULL,
	//	&effectMode,
	//	(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("DimPower"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&dimPowerButton,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("DimmedOnBattery"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&dimmedBatt,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("ActiveProfile"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&activeProfile,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("OffPowerButton"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&offPowerButton,
		(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("CustomColors"),
		RRF_RT_REG_BINARY | RRF_ZEROONFAILURE,
		NULL,
		customColors,
		(LPDWORD)&size_c);
	//RegGetValue(hKey1,
	//	NULL,
	//	TEXT("LastActive"),
	//	RRF_RT_DWORD | RRF_ZEROONFAILURE,
	//	NULL,
	//	&lastActive,
	//	(LPDWORD)&size);
	RegGetValue(hKey1,
		NULL,
		TEXT("EsifTemp"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&esif_temp,
		(LPDWORD)&size);
	ret = RegGetValue(hKey1,
		NULL,
		TEXT("DimmingPower"),
		RRF_RT_DWORD | RRF_ZEROONFAILURE,
		NULL,
		&dimmingPower,
		(LPDWORD)&size);
	if (ret != ERROR_SUCCESS)
		dimmingPower = 92;
	RegGetValue(hKey1,
				NULL,
				TEXT("GlobalEffect"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&globalEffect,
				(LPDWORD)&size);
	ret = RegGetValue(hKey1,
					  NULL,
					  TEXT("GlobalTempo"),
					  RRF_RT_DWORD | RRF_ZEROONFAILURE,
					  NULL,
					  &globalDelay,
					  (LPDWORD)&size);
	if (ret != ERROR_SUCCESS || globalDelay > 0xa)
		globalDelay = 5;
	RegGetValue(hKey1,
				NULL,
				TEXT("EffectColor1"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&effColor1.ci,
				(LPDWORD)&size);
	RegGetValue(hKey1,
				NULL,
				TEXT("EffectColor2"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&effColor2.ci,
				(LPDWORD)&size);
	RegGetValue(hKey1,
				NULL,
				TEXT("FanControl"),
				RRF_RT_DWORD | RRF_ZEROONFAILURE,
				NULL,
				&fanControl,
				(LPDWORD)&size);

	unsigned vindex = 0;
	BYTE inarray[380];
	char name[256]; int pid = -1, appid = -1;
	ret = 0;
	do {
		DWORD len = 255, lend = 0; profile prof;
		ret = RegEnumValueA(
			hKey4,
			vindex,
			name,
			&len,
			NULL,
			NULL,
			NULL,
			&lend
		);
		lend++; len = 255;
		unsigned ret2 = sscanf_s((char*)name, "Profile-%d", &pid);
		if (ret == ERROR_SUCCESS && ret2 == 1) {
			char* profname = new char[lend] {0};
			ret = RegEnumValueA(
				hKey4,
				vindex,
				name,
				&len,
				NULL,
				NULL,
				(LPBYTE)profname,
				&lend
			);
			updateProfileByID(pid, profname, "", -1);
			delete[] profname;
		}
		ret2 = sscanf_s((char*)name, "Profile-flags-%d", &pid);
		if (ret == ERROR_SUCCESS && ret2 == 1) {
			DWORD pFlags;
			ret = RegEnumValueA(
				hKey4,
				vindex,
				name,
				&len,
				NULL,
				NULL,
				(LPBYTE)&pFlags,
				&lend
			);
			updateProfileByID(pid, "", "", pFlags);
		}
		ret2 = sscanf_s((char*)name, "Profile-app-%d-%d", &pid, &appid);
		if (ret == ERROR_SUCCESS && ret2 == 2) {
			char* profname = new char[lend] {0};
			ret = RegEnumValueA(
				hKey4,
				vindex,
				name,
				&len,
				NULL,
				NULL,
				(LPBYTE)profname,
				&lend
			);
			PathStripPath(profname);
			updateProfileByID(pid, "", profname, -1);
			delete[] profname;
		}
		int senid, fanid;
		ret2 = sscanf_s((char*)name, "Profile-fans-%d-%d-%d", &pid, &senid, &fanid);
		if (ret == ERROR_SUCCESS && ret2 == 3) {
			// add fans...
			byte* fanset = new byte[lend];
			fan_block fan = {(short)fanid};
			ret = RegEnumValueA(
				hKey4,
				vindex,
				name,
				&len,
				NULL,
				NULL,
				(LPBYTE)fanset,
				&lend
			);
			for (unsigned i = 0; i < lend; i += 2) {
				fan.points.push_back({fanset[i], fanset[i + 1]});
			}
			updateProfileFansByID(pid, senid, &fan);
		}
		ret2 = sscanf_s((char*)name, "Profile-power-%d", &pid);
		if (ret == ERROR_SUCCESS && ret2 == 1) {
			profile* prof = FindProfile(pid);
			if (prof) {
				DWORD pValue = 0;
				RegGetValue(hKey4,
							NULL,
							name,
							RRF_RT_DWORD | RRF_ZEROONFAILURE,
							NULL,
							&pValue,
							(LPDWORD)&size);
				prof->fansets.powerStage = LOWORD(pValue);
				prof->fansets.GPUPower = HIWORD(pValue);
			}
		}
		vindex++;
	} while (ret == ERROR_SUCCESS);
	vindex = 0; ret = 0;
	do {
		DWORD len = 255, lend = 380; lightset map;
		ret = RegEnumValueA(
			hKey3,
			vindex,
			name,
			&len,
			NULL,
			NULL,
			(LPBYTE)inarray,
			&lend
		);
		// get id(s)...
		if (ret == ERROR_SUCCESS) {
			unsigned profid;
			unsigned ret2 = sscanf_s((char*)name, "Set-%d-%d-%d", &map.devid, &map.lightid, &profid);
			if (ret2 == 3) { // incorrect mapping patch
				BYTE* inPos = inarray;
				for (int i = 0; i < 4; i++) {
					map.eve[i].fs.s = *((DWORD*)inPos);
					inPos += sizeof(DWORD);
					map.eve[i].source = *inPos;
					inPos++;
					BYTE mapSize = *inPos;
					inPos++;
					map.eve[i].map.clear();
					for (int j = 0; j < mapSize; j++) {
						map.eve[i].map.push_back(*((AlienFX_SDK::afx_act*)inPos));
						inPos += sizeof(AlienFX_SDK::afx_act);
					}
				}
				for (int i = 0; i < profiles.size(); i++)
					if (profiles[i].id == profid) {
						profiles[i].lightsets.push_back(map);
						break;
					}
			}
			vindex++;
		}
	} while (ret == ERROR_SUCCESS);
	// set active profile...
	int activeFound = 0;
	if (profiles.size() > 0) {
		for (int i = 0; i < profiles.size(); i++) {
			//std::sort(profiles[i].lightsets.begin(), profiles[i].lightsets.end(), ConfigHandler::sortMappings);
			if (profiles[i].id == activeProfile)
				activeFound = i;
			if (profiles[i].flags & PROF_DEFAULT)
				defaultProfile = profiles[i].id;
		}
	}
	else {
		// need new profile
		profile prof = {0, 1, 0, "", "Default"};
		profiles.push_back(prof);
		active_set = &(profiles.back().lightsets);
		//std::sort(active_set->begin(), active_set->end(), ConfigHandler::sortMappings);
	}
	if (profiles.size() == 1) {
		profiles[0].flags = profiles[0].flags | PROF_DEFAULT;
		defaultProfile = activeProfile = profiles[0].id;
	}
	active_set = &profiles[activeFound].lightsets;
	stateDimmed = IsDimmed();
	stateOn = lightsOn;
	//monState = !(FindProfile(activeProfile)->flags & PROF_NOMONITORING);
	//if (profiles[activeFound].flags & 0x2)
	//	monState = 0;
	//if (profiles[activeFound].flags & 0x4)
	//	stateDimmed = 1;
	conf_loaded = true;

	if (fanControl) {
		fan_conf = new ConfigHelper();
		fan_conf->Load();
	}
	amb_conf = new ConfigAmbient();
	hap_conf = new ConfigHaptics();

	return 0;
}
int ConfigHandler::Save() {
	//char name[256];
	BYTE* out;
	DWORD dwDisposition;

	//if (!conf_loaded) return 0; // do not save clear config!

	if (fan_conf) fan_conf->Save();
	if (amb_conf) amb_conf->Save();
	if (hap_conf) hap_conf->Save();

	//if (RegGetValue(hKey2, NULL, "Alienfx GUI", RRF_RT_ANY, NULL, NULL, NULL) == ERROR_SUCCESS) {
	//	// remove old start key
	//	RegDeleteValue(hKey2, "Alienfx GUI");
	//	startWindows = 0;
	//	//RegSetValueExA(hKey2, "Alienfx GUI", 0, REG_SZ, (BYTE*)&"", 1);
	//}

	RegSetValueEx(
		hKey1,
		TEXT("AutoStart"),
		0,
		REG_DWORD,
		(BYTE*)&startWindows,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("Minimized"),
		0,
		REG_DWORD,
		(BYTE*)&startMinimized,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("Refresh"),
		0,
		REG_DWORD,
		(BYTE*)&autoRefresh,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("LightsOn"),
		0,
		REG_DWORD,
		(BYTE*)&lightsOn,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("Monitoring"),
		0,
		REG_DWORD,
		(BYTE*)&enableMon,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("OffWithScreen"),
		0,
		REG_DWORD,
		(BYTE*)&offWithScreen,
		sizeof(DWORD)
	);
	//RegSetValueEx(
	//	hKey1,
	//	TEXT("EffectMode"),
	//	0,
	//	REG_DWORD,
	//	(BYTE*)&effectMode,
	//	sizeof(DWORD)
	//);
	RegSetValueEx(
		hKey1,
		TEXT("DimPower"),
		0,
		REG_DWORD,
		(BYTE*)&dimPowerButton,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("DimmedOnBattery"),
		0,
		REG_DWORD,
		(BYTE*)&dimmedBatt,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("OffPowerButton"),
		0,
		REG_DWORD,
		(BYTE*)&offPowerButton,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("DimmingPower"),
		0,
		REG_DWORD,
		(BYTE*)&dimmingPower,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("ActiveProfile"),
		0,
		REG_DWORD,
		(BYTE*)&activeProfile,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("GammaCorrection"),
		0,
		REG_DWORD,
		(BYTE*)&gammaCorrection,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("ProfileAutoSwitch"),
		0,
		REG_DWORD,
		(BYTE*)&enableProf,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("DisableAWCC"),
		0,
		REG_DWORD,
		(BYTE*)&awcc_disable,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("EsifTemp"),
		0,
		REG_DWORD,
		(BYTE*)&esif_temp,
		sizeof(DWORD)
	);
	//RegSetValueEx(
	//	hKey1,
	//	TEXT("LastActive"),
	//	0,
	//	REG_DWORD,
	//	(BYTE*)&lastActive,
	//	sizeof(DWORD)
	//);
	RegSetValueEx(
		hKey1,
		TEXT("CustomColors"),
		0,
		REG_BINARY,
		(BYTE*)customColors,
		sizeof(DWORD) * 16
	);
	RegSetValueEx(
		hKey1,
		TEXT("GlobalEffect"),
		0,
		REG_DWORD,
		(BYTE*)&globalEffect,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("GlobalTempo"),
		0,
		REG_DWORD,
		(BYTE*)&globalDelay,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("EffectColor1"),
		0,
		REG_DWORD,
		(BYTE*)&effColor1.ci,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("EffectColor2"),
		0,
		REG_DWORD,
		(BYTE*)&effColor2.ci,
		sizeof(DWORD)
	);
	RegSetValueEx(
		hKey1,
		TEXT("FanControl"),
		0,
		REG_DWORD,
		(BYTE*)&fanControl,
		sizeof(DWORD)
	);
	if (profiles.size() > 0) {
		RegDeleteTreeA(hKey1, "Profiles");
		RegCreateKeyEx(HKEY_CURRENT_USER,
			TEXT("SOFTWARE\\Alienfxgui\\Profiles"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,//KEY_WRITE,
			NULL,
			&hKey4,
			&dwDisposition);
	} else {
		DebugPrint("Attempt to save empty profiles!\n");
	}

	RegDeleteTreeA(hKey1, "Events");
	RegCreateKeyEx(HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\Alienfxgui\\Events"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,//KEY_WRITE,
		NULL,
		&hKey3,
		&dwDisposition);

	for (int j = 0; j < profiles.size(); j++) {
		string name = "Profile-" + to_string(profiles[j].id);
		RegSetValueExA(
			hKey4,
			name.c_str(),
			0,
			REG_SZ,
			(BYTE*)profiles[j].name.c_str(),
			(DWORD)profiles[j].name.length()
		);
		name = "Profile-flags-" + to_string(profiles[j].id);
		DWORD flagset = MAKELONG(profiles[j].flags, profiles[j].effmode);
		RegSetValueExA(
			hKey4,
			name.c_str(),
			0,
			REG_DWORD,
			(BYTE*)&flagset, //profiles[j].flags,
			sizeof(DWORD)
		);
		if (!profiles[j].triggerapp.empty()) {
			name = "Profile-app-" + to_string(profiles[j].id) + "-0";
			RegSetValueExA(
				hKey4,
				name.c_str(),
				0,
				REG_SZ,
				(BYTE*) profiles[j].triggerapp.c_str(),
				(DWORD) profiles[j].triggerapp.length()
			);
		}
		for (int i = 0; i < profiles[j].lightsets.size(); i++) {
			//preparing name
			lightset cur = profiles[j].lightsets[i];
			name = "Set-" + to_string(cur.devid) + "-" + to_string(cur.lightid) + "-" + to_string(profiles[j].id);
			//preparing binary....
			size_t size = 6 * 4;// (UINT)mappings[i].map.size();
			for (int j = 0; j < 4; j++)
				size += cur.eve[j].map.size() * (sizeof(AlienFX_SDK::afx_act));
			out = (BYTE*)malloc(size);
			BYTE* outPos = out;
			//Colorcode ccd;
			for (int j = 0; j < 4; j++) {
				*((int*)outPos) = cur.eve[j].fs.s;
				outPos += 4;
				*outPos = (BYTE)cur.eve[j].source;
				outPos++;
				*outPos = (BYTE)cur.eve[j].map.size();
				outPos++;
				for (int k = 0; k < cur.eve[j].map.size(); k++) {
					memcpy(outPos, &cur.eve[j].map.at(k), sizeof(AlienFX_SDK::afx_act));
					outPos += sizeof(AlienFX_SDK::afx_act);
				}
			}
			RegSetValueExA(
				hKey3,
				name.c_str(),
				0,
				REG_BINARY,
				(BYTE*)out,
				(DWORD)size
			);
			free(out);
		}
		// Fans....
		if (profiles[j].flags & PROF_FANS) {
			// save powers..
			name = "Profile-power-" + to_string(profiles[j].id);
			DWORD pvalue = MAKELONG(profiles[j].fansets.powerStage, profiles[j].fansets.GPUPower);
			RegSetValueExA(
				hKey4,
				name.c_str(),
				0,
				REG_DWORD,
				(BYTE*)&pvalue,
				sizeof(DWORD)
			);
			// save fans...
			for (int i = 0; i < profiles[j].fansets.fanControls.size(); i++) {
				temp_block* sens = &profiles[j].fansets.fanControls[i];
				for (int k = 0; k < sens->fans.size(); k++) {
					fan_block* fans = &sens->fans[k];
					name = "Profile-fans-" + to_string(profiles[j].id) + "-" + to_string(sens->sensorIndex) + "-" + to_string(fans->fanIndex);
					byte* outdata = new byte[fans->points.size() * 2];
					for (int l = 0; l < fans->points.size(); l++) {
						outdata[2 * l] = (byte) fans->points[l].temp;
						outdata[(2 * l) + 1] = (byte) fans->points[l].boost;
					}

					RegSetValueExA(
						hKey4,
						name.c_str(),
						0,
						REG_BINARY,
						(BYTE*) outdata,
						(DWORD) fans->points.size() * 2
					);
					delete[] outdata;
				}
			}
		}
	}
	return 0;
}