#include "alienfx-gui.h"
#include <algorithm>

// Import of Alienware Command Center lighting presets into profiles.
// AWCC keeps it at SQLite database, Windows winsqlite3.dll is used to read it. Database is never modified.

typedef int (WINAPI *sqOpen)(const char*, void**, int, const char*);
typedef int (WINAPI *sqPrepare)(void*, const char*, int, void**, const char**);
typedef int (WINAPI *sqStep)(void*);
typedef const char* (WINAPI *sqColumn)(void*, int);
typedef int (WINAPI *sqClose)(void*);

#define SQLITE_ROW 100
#define SQLITE_OPEN_READONLY 1

struct awccPreset {
	string name, game, data;
};

static string ExpandPath(const char* path) {
	char res[MAX_PATH];
	return ExpandEnvironmentStrings(path, res, MAX_PATH) ? res : "";
}

static string ReadTextFile(string name) {
	string content;
	HANDLE file = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE) {
		DWORD size = GetFileSize(file, NULL), got = 0;
		content.resize(size);
		ReadFile(file, &content[0], size, &got, NULL);
		content.resize(got);
		CloseHandle(file);
	}
	return content;
}

static vector<awccPreset> ReadPresets() {
	vector<awccPreset> presets;
	HMODULE lib = LoadLibrary("winsqlite3.dll");
	if (!lib)
		return presets;
	sqOpen open = (sqOpen)GetProcAddress(lib, "sqlite3_open_v2");
	sqPrepare prepare = (sqPrepare)GetProcAddress(lib, "sqlite3_prepare_v2");
	sqStep step = (sqStep)GetProcAddress(lib, "sqlite3_step");
	sqColumn column = (sqColumn)GetProcAddress(lib, "sqlite3_column_text");
	sqClose finalize = (sqClose)GetProcAddress(lib, "sqlite3_finalize"), close = (sqClose)GetProcAddress(lib, "sqlite3_close");
	if (open && prepare && step && column && finalize && close) {
		// AWCC can use database right now, so work with a copy
		string copy = ExpandPath("%TEMP%\\alienfx-awcc.db");
		if (CopyFile(ExpandPath("%LOCALAPPDATA%\\Alienware\\Alienware Command Center\\FX\\FXRepository.db").c_str(), copy.c_str(), false)) {
			void* db = NULL, *stmt = NULL;
			if (!open(copy.c_str(), &db, SQLITE_OPEN_READONLY, NULL) && !prepare(db,
				"select g.Name, g.GameID, d.DataJson from GamePresets g join PresetDetailInfo d on d.PresetId = g.PresetId "
				"where d.CapabilityId = 1 and g.Name is not null order by g.PresetId", -1, &stmt, NULL)) {
				while (step(stmt) == SQLITE_ROW)
					presets.push_back({ column(stmt, 0), column(stmt, 1), column(stmt, 2) });
				finalize(stmt);
			}
			close(db);
			DeleteFile(copy.c_str());
		}
	}
	FreeLibrary(lib);
	return presets;
}

// Preset JSON is compact and has stable layout, so simple search is enough
static bool GetNumber(const string& json, const string& key, unsigned& val, size_t from = 0) {
	size_t pos = json.find(key, from);
	if (pos == string::npos || pos + key.size() >= json.size() || !isdigit((byte)json[pos + key.size()]))
		return false;
	val = strtoul(json.c_str() + pos + key.size(), NULL, 10);
	return true;
}

static AlienFX_SDK::Afx_colorcode ScaleColor(unsigned color, unsigned brightness) {
	AlienFX_SDK::Afx_colorcode c{ color };
	return AlienFX_SDK::Afx_colorcode((byte)(c.b * brightness / 100), (byte)(c.g * brightness / 100), (byte)(c.r * brightness / 100), 0xff);
}

// Convert preset to device effect, false if it can't be presented by one effect
static bool ParsePreset(string json, deviceeffect& eff) {
	json = json.substr(0, json.find("\"Dynamic\""));
	size_t pre = json.find("\"PredefinedAnimations\":[{");
	unsigned id, color = 0, color2 = 0, brightness = 100;
	if (pre != string::npos && GetNumber(json, "\"ID\":", id, pre)) {
		GetNumber(json, "\"Brightness\":", brightness, pre);
		GetNumber(json, "\"Colors\":[", color, pre);
		size_t colors = json.find("\"Colors\":[", pre);
		if (colors != string::npos)
			GetNumber(json, ",", color2, colors);
		switch (id) {
		case 1: eff.globalEffect = 3; break;
		case 2: eff.globalEffect = 2; break;
		case 4: eff.globalEffect = 1; break;
		case 7: eff.globalEffect = 7; break;
		case 8: eff.globalEffect = 8; break;
		case 16: case 21: eff.globalEffect = 16; break;
		default: return false;
		}
	}
	else {
		// Custom animation: only one action for all lights supported
		size_t act = json.find("\"Effect\":");
		unsigned effect;
		if (act == string::npos || json.find("\"Effect\":", act + 1) != string::npos || !GetNumber(json, "\"Effect\":", effect, act))
			return false;
		GetNumber(json, "\"Brightness\":", brightness, act);
		GetNumber(json, "\"Color\":[", color, act);
		size_t colors = json.find("\"Color\":[", act);
		size_t end = json.find(']', colors);
		size_t comma = json.find(',', colors);
		if (comma != string::npos && comma < end)
			color2 = strtoul(json.c_str() + comma + 1, NULL, 10);
		else
			color2 = color;
		switch (effect) { // AWCC effect flags
		case 1: eff.globalEffect = color == color2 ? 1 : 3; break;
		case 2: eff.globalEffect = 2; break;
		case 4: eff.globalEffect = 1; break;
		default: return false;
		}
	}
	eff.effColor1 = ScaleColor(color, brightness);
	eff.effColor2 = ScaleColor(color2, brightness);
	return true;
}

// Executable names of AWCC game from game library, it's the same as profile trigger list
static vector<string> FindGameApps(string& library, string game) {
	vector<string> apps;
	transform(game.begin(), game.end(), game.begin(), ::tolower);
	size_t pos = library.find("\"GUID\":\"" + game + "\""), start;
	string key = "\"ExePath\":\"";
	if (pos != string::npos && (start = library.rfind(key, pos)) != string::npos) {
		start += key.size();
		string paths = library.substr(start, library.find('"', start) - start);
		size_t from = 0, to;
		do {
			to = paths.find(';', from);
			string path = paths.substr(from, to == string::npos ? to : to - from);
			path = path.substr(path.find_last_of('\\') + 1);
			if (path.size() && find(apps.begin(), apps.end(), path) == apps.end())
				apps.push_back(path);
			from = to + 1;
		} while (to != string::npos);
	}
	return apps;
}

string ImportAWCCPresets() {
	AlienFX_SDK::Afx_device* dev = NULL;
	for (auto& d : conf->afx_dev.fxdevs)
		if (d.present && d.dev->version == AlienFX_SDK::API_V4 && d.dev->IsHaveGlobal())
			dev = &d;
	if (!dev)
		return "No supported device found.";
	vector<awccPreset> presets = ReadPresets();
	if (presets.empty())
		return "AWCC presets not found.";
	string library = ReadTextFile(ExpandPath("%ProgramData%\\Alienware\\Alienware Command Center\\GameLibrary\\GameLibrary.json"));
	int added = 0, present = 0;
	string skipped;
	for (auto& p : presets) {
		string name = "AWCC " + p.name;
		deviceeffect eff{ {0}, {0} };
		if (find_if(conf->profiles.begin(), conf->profiles.end(), [&name](profile* pr) { return pr->name == name; }) != conf->profiles.end()) {
			present++;
			continue;
		}
		if (!ParsePreset(p.data, eff)) {
			skipped += "\n" + p.name;
			continue;
		}
		profile* np = new profile();
		for (auto pr : conf->profiles)
			np->id = max(np->id, pr->id + 1);
		np->name = name;
		np->triggerapp = FindGameApps(library, p.game);
		np->effects[dev->devID].push_back(eff);
		conf->profiles.push_back(np);
		added++;
	}
	return "Added " + to_string(added) + ", already present " + to_string(present) +
		(skipped.size() ? ".\nCan't be presented by one device effect:" + skipped : ".");
}
