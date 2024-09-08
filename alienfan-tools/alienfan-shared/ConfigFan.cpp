#include "ConfigFan.h"
#include "RegHelperLib.h"

ConfigFan::ConfigFan() {

	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfan"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keyMain, NULL);
	RegCreateKeyEx(keyMain, TEXT("Sensors"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keySensors, NULL);
	RegCreateKeyEx(keyMain, TEXT("Powers"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keyPowers, NULL);
	Load();
}

ConfigFan::~ConfigFan() {
	RegCloseKey(keyPowers);
	RegCloseKey(keySensors);
	RegCloseKey(keyMain);
}

void ConfigFan::GetReg(const char *name, DWORD *value, DWORD defValue) {
	DWORD size = sizeof(DWORD);
	if (RegGetValue(keyMain, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
		*value = defValue;
}

void ConfigFan::SetReg(const char *text, DWORD value) {
	RegSetValueEx( keyMain, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

void ConfigFan::AddSensorCurve(fan_profile *prof, byte fid, WORD sid, byte* data, DWORD lend) {
	//if (!prof) prof = lastProf;
	vector<fan_point>* cp = &prof->fanControls[fid][sid].points;
	cp->resize(lend / 2);
	memcpy(cp->data(), data, lend);
}

void ConfigFan::Load() {

	GetReg("StartAtBoot", &startWithWindows);
	GetReg("StartMinimized", &startMinimized);
	GetReg("UpdateCheck", &updateCheck, 1);
	GetReg("LastPowerStage", &prof.powerSet);
	GetReg("OC", &prof.ocSettings, 100);
	GetReg("DisableAWCC", &awcc_disable);
	GetReg("KeyboardShortcut", &keyShortcuts, 1);
	GetReg("KeepSystemMode", &keepSystem, 1);
	GetReg("DPTF", &needDPTF, 1);
	GetReg("PollingRate", &pollingRate, 750);

	// Now load sensor mappings...
	char name[256];
	byte* inarray = NULL;
	DWORD lend; short fid, sid;
	for (int vindex = 0; lend = GetRegData(keySensors, vindex, name, &inarray); vindex++) {
		if (sscanf_s(name, "Fan-%hd-%hd", &fid, &sid) == 2) {
			AddSensorCurve(&prof, (byte)fid, sid, inarray, lend);
			continue;
		}
		if (sscanf_s(name, "SensorName-%hd-%hd", &sid, &fid) == 2) {
			sensors[MAKEWORD(fid, sid)] = GetRegString(inarray, lend);
			continue;
		}
	}
	for (int vindex = 0; lend = GetRegData(keyMain, vindex, name, &inarray); vindex++) {
		if (sscanf_s(name, "Boost-%hd", &fid) == 1) { // Boost block
			boosts[(byte)fid] = { inarray[0],*(WORD*)(inarray + 1) };
		}
	}
	// manual mode always available
	powers[0] = "Manual";
	for (int vindex = 0; lend = GetRegData(keyPowers, vindex, name, &inarray); vindex++) {
		if (sscanf_s(name, "Power-%hd", &fid) == 1) { // Power names
			powers[(byte)fid] = GetRegString(inarray, lend);
		}
	}
}

void ConfigFan::SaveSensorBlocks(HKEY key, string pname, fan_profile* data) {
	for (auto i = data->fanControls.begin(); i != data->fanControls.end(); i++) {
		for (auto j = i->second.begin(); j != i->second.end(); j++) {
			if (j->second.active) {
				string name = pname + "-" + to_string(i->first) + "-" + to_string(j->first);
				RegSetValueEx(key, name.c_str(), 0, REG_BINARY, (BYTE*)j->second.points.data(), (DWORD)j->second.points.size() * 2);
			}
		}
	}
}

void ConfigFan::Save() {
	string name;

	SetReg("StartAtBoot", startWithWindows);
	SetReg("StartMinimized", startMinimized);
	SetReg("LastPowerStage", prof.powerSet);
	SetReg("OC", prof.ocSettings);
	SetReg("UpdateCheck", updateCheck);
	SetReg("DisableAWCC", awcc_disable);
	SetReg("KeyboardShortcut", keyShortcuts);
	SetReg("KeepSystemMode", keepSystem);
	SetReg("DPTF", needDPTF);
	SetReg("PollingRate", pollingRate);
	// clean old data
	RegDeleteTree(keyMain, "Sensors");
	RegCreateKeyEx(keyMain, "Sensors", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keySensors, NULL);

	// save profile..
	SaveSensorBlocks(keySensors, "Fan", &prof);
	// save boosts...
	for (auto i = boosts.begin(); i != boosts.end(); i++) {
		byte outarray[sizeof(byte) + sizeof(USHORT)] = {0};
		name = "Boost-" + to_string(i->first);
		outarray[0] = i->second.maxBoost;
		*(USHORT *) (outarray + sizeof(byte)) = i->second.maxRPM;
		RegSetValueEx( keyMain, name.c_str(), 0, REG_BINARY, outarray, (DWORD) sizeof(byte) + sizeof(USHORT));
	}
	// save powers...
	for (auto i = powers.begin(); i != powers.end(); i++) {
		name = "Power-" + to_string(i->first);
		RegSetValueEx(keyPowers, name.c_str(), 0, REG_SZ, (BYTE*)i->second.c_str(), (DWORD) i->second.size());
	}
	// save sensors...
	for (auto i = sensors.begin(); i != sensors.end(); i++) {
		name = "SensorName-" + to_string(HIBYTE(i->first)) + "-" + to_string(LOBYTE(i->first));
		RegSetValueEx(keySensors, name.c_str(), 0, REG_SZ, (BYTE*)i->second.c_str(), (DWORD)i->second.size());
	}
}

string* ConfigFan::GetPowerName(int index) {
	if (powers[index].empty())
		powers[index] = "Level " + to_string(index);
	return &powers[index];
}

void ConfigFan::UpdateBoost(byte fanID, byte boost, WORD rpm) {
	boosts[fanID] = { (byte)boost, max(rpm, boosts[fanID].maxRPM) };
	Save();
}

int ConfigFan::GetFanScale(byte fanID) {
	return max(boosts[fanID].maxBoost, 100);
}

string ConfigFan::GetSensorName(AlienFan_SDK::ALIENFAN_SEN_INFO* acpi) {
	auto sen = &sensors[acpi->sid];
	if (sen->empty())
		*sen = acpi->name;
	return *sen;
}

string GetTag(string xml, string tag, size_t& pos) {
	size_t firstpos = xml.find("<" + tag + ">", pos);
	if (firstpos != string::npos) {
		firstpos += tag.length() + 2;
		pos = xml.find("</" + tag + ">", firstpos);
		return xml.substr(firstpos, pos - firstpos);
	}
	else {
		pos = string::npos;
		return "";
	}
}

string ReadFromESIF(string command, HANDLE g_hChildStd_IN_Wr, HANDLE g_hChildStd_OUT_Rd, PROCESS_INFORMATION* proc) {
	DWORD written;
	string outpart;
	WriteFile(g_hChildStd_IN_Wr, command.c_str(), (DWORD)command.length(), &written, NULL);
	while (outpart.find("Returned:") == string::npos) {
		while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && written) {
			char* buffer = new char[written + 1]{ 0 };
			ReadFile(g_hChildStd_OUT_Rd, buffer, written, &written, NULL);
			outpart += buffer;
			delete[] buffer;
		}
	}
	if (outpart.find("</result>") != string::npos) {
		size_t pos = 0;
		return GetTag(outpart, "result", pos);
	}
	else
		return "";
}

DWORD WINAPI DPTFInit(LPVOID lparam) {
	ConfigFan* conf = (ConfigFan*)lparam;
	string wdName;
	SECURITY_ATTRIBUTES attr{ sizeof(SECURITY_ATTRIBUTES), NULL, true };
	STARTUPINFO sinfo{ sizeof(STARTUPINFO), 0 };
	HANDLE g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, initHandle = NULL;
	PROCESS_INFORMATION proc;
	wdName.resize(2048);
	wdName.resize(GetWindowsDirectory((LPSTR)wdName.data(), 2047));
	wdName += "\\system32\\DriverStore\\FileRepository\\";
	WIN32_FIND_DATA file;
	HANDLE search_handle = FindFirstFile((wdName + "dptf_cpu*").c_str(), &file);
	if (conf->needDPTF = (search_handle != INVALID_HANDLE_VALUE))
	{
		wdName += string(file.cFileName) + "\\esif_uf.exe";
		FindClose(search_handle);
		CreatePipe(&g_hChildStd_OUT_Rd, &sinfo.hStdOutput, &attr, 0);
		CreatePipe(&sinfo.hStdInput, &g_hChildStd_IN_Wr, &attr, 0);
		DWORD flags = PIPE_NOWAIT;
		SetNamedPipeHandleState(g_hChildStd_IN_Wr, &flags, NULL, NULL);
		sinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		sinfo.wShowWindow = SW_HIDE;
		sinfo.hStdError = sinfo.hStdOutput;
		HWND cur = GetForegroundWindow();
		if (conf->needDPTF = CreateProcess(NULL, (LPSTR)(wdName + " client").c_str(), NULL, NULL, true,
			CREATE_NEW_CONSOLE, NULL, NULL, &sinfo, &proc)) {
			SetForegroundWindow(cur);
			// Start init...
			string parts = ReadFromESIF("format xml\nparticipants\nexit\n", g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, &proc);
			if (parts.size()) {
				conf->needDPTF = 0;
				size_t pos = 0;
				WORD sid = 0;
				while (pos != string::npos) {
					string part = GetTag(parts, "participant", pos);
					size_t descpos = 0;
					byte sID = atoi(GetTag(part, "UpId", descpos).c_str());
					string name = GetTag(part, "desc", descpos);
					int dcount = atoi(GetTag(part, "domainCount", descpos).c_str());
					// check domains...
					for (int i = 0; i < dcount; i++) {
						size_t dPos = 0;
						string domain = GetTag(part, "domain", descpos);
						string dName = GetTag(domain, "name", dPos);
						if (strtol(GetTag(domain, "capability", dPos).c_str(), NULL, 16) & 0x80) {
							conf->sensors[sid++] = name + (dcount > 1 ? " (" + dName + ")" : "");
						}
					}
				}
			}
			CloseHandle(proc.hProcess);
			CloseHandle(proc.hThread);
		}
		CloseHandle(sinfo.hStdInput);
		CloseHandle(g_hChildStd_IN_Wr);
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(sinfo.hStdOutput);
	}
	return 0;
}
