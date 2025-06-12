#include <vector>
#include "LFX2.h"
#include "AlienFX_SDK.h"
#include <map>
#include <queue>
#include <CustomMutex.h>

using namespace std;

struct LightQueryElement {
	WORD pid;
	byte light;
	byte command; // 0 - color, 1 - update, 2 - set brightness
	byte actsize;
	AlienFX_SDK::Afx_action actions[9];
};

struct deviceQuery {
	WORD pid;
	vector<AlienFX_SDK::Afx_lightblock> dev_query;
};

bool lightsNoDelay;
HANDLE updateThread = NULL, stopQuery, haveNewElement;

queue<LightQueryElement> lightQuery;
CustomMutex modifyQuery;

const BYTE actionCodes[]{ AlienFX_SDK::AlienFX_A_Color, AlienFX_SDK::AlienFX_A_Morph, AlienFX_SDK::AlienFX_A_Pulse, AlienFX_SDK::AlienFX_A_Color };
AlienFX_SDK::Mappings* afx_dev = NULL;
BYTE gtempo = 5;
AlienFX_SDK::Afx_group groups[4];

DWORD WINAPI CLightsProc(LPVOID param) {
	LightQueryElement current;

	HANDLE waitArray[2]{ haveNewElement, stopQuery };
	map<WORD, vector<AlienFX_SDK::Afx_lightblock>> devs_query;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	while (WaitForMultipleObjects(2, waitArray, false, INFINITE) == WAIT_OBJECT_0) {
		while (lightQuery.size()) {

			modifyQuery.lockWrite();
			current = lightQuery.front();
			lightQuery.pop();
			modifyQuery.unlockWrite();

			switch (current.command) {
			case 1: { // update command
				AlienFX_SDK::Afx_device* dev;
				for (auto devQ = devs_query.begin(); devQ != devs_query.end(); devQ++) {
					if (devQ->second.size() && (dev = afx_dev->GetDeviceById(devQ->first)) && dev->dev) {
						//DebugPrint("Updating device " + to_string(devQ->first) + ", " + to_string(devQ->second.size()) + " lights\n");
						dev->dev->SetMultiAction(&devQ->second, current.light);
						dev->dev->UpdateColors();
					}
					devQ->second.clear();
				}
			} break;
			case 0: { // set light
				AlienFX_SDK::Afx_device* dev = afx_dev->GetDeviceById(current.pid);
				// Is it NOT power or indicator button?
				if (dev && !(afx_dev->GetFlags(dev, current.light))) {
					// form actblock...
					AlienFX_SDK::Afx_lightblock ablock{ current.light };
					ablock.act.resize(current.actsize);
					memcpy(ablock.act.data(), current.actions, current.actsize * sizeof(AlienFX_SDK::Afx_action));
					auto dv = &devs_query[current.pid];
					for (auto lp = dv->begin(); lp != dv->end(); lp++)
						if (lp->index == current.light) {
							//DebugPrint("Light " + to_string(lid) + " already in query, updating data.\n");
							dv->erase(lp);
							break;
						}
					dv->push_back(ablock);
				}
			}
			}
		}
		lightsNoDelay = true;
	}
	return 0;
}

void QueryCommand(LightQueryElement& lqe) {
	if (updateThread) {
			modifyQuery.lockWrite();
			lightQuery.push(lqe);
			modifyQuery.unlockWrite();
			SetEvent(haveNewElement);
	}
}

void QueryUpdate() {
	static LightQueryElement upd{ NULL, 0, 1 };
	QueryCommand(upd);
	lightsNoDelay = lightQuery.size() < (afx_dev->activeLights << 3);
}

void SetLight(DWORD lgh, vector<AlienFX_SDK::Afx_action>* actions)
{
	//auto dev = afx_dev->GetDeviceById(LOWORD(lgh));
	if (actions->size()) {
		LightQueryElement newBlock{ LOWORD(lgh), (byte)HIWORD(lgh), 0, (byte)actions->size() };
		memcpy(newBlock.actions, actions->data(), newBlock.actsize * sizeof(AlienFX_SDK::Afx_action));
		QueryCommand(newBlock);
	}
}

AlienFX_SDK::Afx_action TranslateColor(PLFX_COLOR src, byte type) {
	// gamma-correction and brightness...
	return { actionCodes[type], gtempo, 7,
		 (byte)(((unsigned)src->red * src->red * src->brightness) / 65025),
		 (byte)(((unsigned)src->green * src->green * src->brightness) / 65025),
		 (byte)(((unsigned)src->blue * src->blue * src->brightness) / 65025) };
}

AlienFX_SDK::Afx_action TranslateColor(unsigned int src, byte type) {
	PLFX_COLOR c = (PLFX_COLOR)&src;
	swap(c->red, c->blue);
	return TranslateColor(c, type);
}

LFX_RESULT CheckState(int did, int lid = -1) {
	if (!afx_dev)
		return LFX_ERROR_NOINIT;
	if (did >= afx_dev->fxdevs.size())
		return LFX_ERROR_NODEVS;
	if (lid < 0 || lid < (int)afx_dev->fxdevs[did].lights.size())
		return LFX_SUCCESS;
	return LFX_ERROR_NOLIGHTS;
}

bool LightInGroup(AlienFX_SDK::Afx_group* grp, DWORD lgh) {
	for (AlienFX_SDK::Afx_groupLight& pos : grp->lights)
		if (pos.lgh == lgh)
			return true;
	return false;
}

AlienFX_SDK::Afx_group* GetGroupID(unsigned int mask) {
	switch (mask) {
	case LFX_ALL_RIGHT: return &groups[0];
	case LFX_ALL_LEFT: return &groups[1];
	case LFX_ALL_UPPER: case LFX_ALL_REAR: return &groups[2];
	case LFX_ALL_LOWER: case LFX_ALL_FRONT: return &groups[3];
	}
	return NULL;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Initialize() {
	if (!afx_dev) {
		afx_dev = new AlienFX_SDK::Mappings();
		afx_dev->LoadMappings();
	}
	afx_dev->AlienFXEnumDevices();
	for (AlienFX_SDK::Afx_device& j : afx_dev->fxdevs) {
		afx_dev->SetDeviceBrightness(&j, 255, false);
	}
	if (!updateThread) {
		stopQuery = CreateEvent(NULL, false, false, NULL);
		haveNewElement = CreateEvent(NULL, false, false, NULL);
		updateThread = CreateThread(NULL, 0, CLightsProc, NULL, 0, NULL);
		lightsNoDelay = true;
	}
	for (int g = 0; g < 4; g++)
		groups[g].lights.clear();
	if (afx_dev->activeDevices) {
		// now map lights to zones
		for (auto grid = afx_dev->GetGrids()->begin(); grid != afx_dev->GetGrids()->end(); grid++) {
			int dx = grid->x / 3,
				dy = grid->y / 3;
			for (int x = 0; x < grid->x; x++)
				for (int y = 0; y < grid->y; y++) {
					AlienFX_SDK::Afx_groupLight cLight = grid->grid[y * grid->x + x];
					if (cLight.lgh) {
						if (x < dx && !LightInGroup(&groups[1], cLight.lgh)) //left
							groups[1].lights.push_back(cLight);
						else
							if (grid->x - x < dx && !LightInGroup(&groups[0], cLight.lgh)) // right
								groups[0].lights.push_back(cLight);
						if (y < dy && !LightInGroup(&groups[2], cLight.lgh)) // upper and rear
							groups[2].lights.push_back(cLight);
						else
							if (grid->y - y < dy && !LightInGroup(&groups[3], cLight.lgh)) // lower and front
								groups[3].lights.push_back(cLight);
					}
				}
		}
		return LFX_SUCCESS;
	}
	else
		return LFX_ERROR_NODEVS;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Release() {
	if (updateThread) {
		HANDLE oldUpate = updateThread;
		updateThread = NULL;
		SetEvent(stopQuery);
		WaitForSingleObject(oldUpate, 20000);
		CloseHandle(oldUpate);
		CloseHandle(stopQuery);
		CloseHandle(haveNewElement);
		delete afx_dev;
		afx_dev = NULL;
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetTiming(const int time) {
	gtempo = time / 50;
	return LFX_SUCCESS;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetVersion(char *const name, const unsigned int namelen) {
	static const char vName[] = "V5.2 emulated";
	if (namelen > strlen(vName)) {
		strcpy_s(name, namelen, vName);
	} else
		return LFX_ERROR_BUFFSIZE;
	return LFX_SUCCESS;
}


FN_DECLSPEC LFX_RESULT STDCALL LFX_Reset() {
	return LFX_Light(LFX_ALL, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Update() {
	if (updateThread) {
		QueryUpdate();
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_UpdateDefault() {
	return LFX_Update();
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumDevices(unsigned int *const num) {
	if (afx_dev) {
		*num = (unsigned) afx_dev->fxdevs.size(); // afx_dev->activeDevices;
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}

static const unsigned char devTypes[]{
	LFX_DEVTYPE_DESKTOP,
	LFX_DEVTYPE_NOTEBOOK,
	LFX_DEVTYPE_NOTEBOOK,
	LFX_DEVTYPE_NOTEBOOK,
	LFX_DEVTYPE_UNKNOWN,
	LFX_DEVTYPE_KEYBOARD,
	LFX_DEVTYPE_DISPLAY,
	LFX_DEVTYPE_MOUSE,
	LFX_DEVTYPE_KEYBOARD,
	LFX_DEVTYPE_DISPLAY
};

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetDeviceDescription(const unsigned int dev, char *const name, const unsigned int namelen, unsigned char *const devtype) {
	LFX_RESULT state = CheckState(dev);
	if (!state) {
		*devtype = devTypes[afx_dev->fxdevs[dev].version];
		if (namelen > afx_dev->fxdevs[dev].name.length()) {
			strcpy_s(name, afx_dev->fxdevs[dev].name.length()+1, afx_dev->fxdevs[dev].name.data());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumLights(const unsigned int dev, unsigned int *const numlights) {
	LFX_RESULT state = CheckState(dev);
	*numlights = 0;
	if (!state) {
		*numlights = (unsigned)afx_dev->fxdevs[dev].lights.size();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightDescription(const unsigned int dev, const unsigned int lid, char *const name, const unsigned int namelen) {
	LFX_RESULT state = CheckState(dev, lid);
	if (!state) {
		auto nameLen = afx_dev->fxdevs[dev].lights[lid].name.length();
		if (namelen > nameLen) {
			strcpy_s(name, nameLen+1, afx_dev->fxdevs[dev].lights[lid].name.data());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightLocation(const unsigned int dev, const unsigned int lid, PLFX_POSITION const pos) {
	LFX_RESULT state = CheckState(dev, lid);
	if (!state) {
		AlienFX_SDK::Afx_groupLight cLight{ afx_dev->fxdevs[dev].pid , afx_dev->fxdevs[dev].lights[lid].lightid };
		pos->x = LightInGroup(&groups[1], cLight.lgh) ? 0 :
			LightInGroup(&groups[0], cLight.lgh) ? 2 : 1;
		pos->y = LightInGroup(&groups[3], cLight.lgh) ? 0 :
			LightInGroup(&groups[2], cLight.lgh) ? 2 : 1;
		pos->z = LightInGroup(&groups[2], cLight.lgh) ? 0 :
			LightInGroup(&groups[3], cLight.lgh) ? 2 : 1;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightColor(const unsigned int dev, const unsigned int lid, PLFX_COLOR const clr) {
	LFX_RESULT state = CheckState(dev, lid);
	*clr = {0};
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightColor(const unsigned int dev, const unsigned int lid, const PLFX_COLOR clr) {
	LFX_RESULT state = CheckState(dev, lid);
	if (!state) {
		vector<AlienFX_SDK::Afx_action> act{ TranslateColor(clr, 0) };
		// loword - dev, hiword - lid
		SetLight(MAKELPARAM(afx_dev->fxdevs[dev].pid, afx_dev->fxdevs[dev].lights[lid].lightid), &act);
	}
	return state;
}

void SetLightList(unsigned pos, vector<AlienFX_SDK::Afx_action>* actions) {
	AlienFX_SDK::Afx_group* grp = GetGroupID(pos);
	if (grp) {
		for (auto l = grp->lights.begin(); l != grp->lights.end(); l++)
			SetLight(l->lgh, actions);
	}
	else
	{
		for (AlienFX_SDK::Afx_device& j : afx_dev->fxdevs)
			if (j.dev)
				for (AlienFX_SDK::Afx_light& i : j.lights)
					SetLight(MAKELPARAM(j.pid, i.lightid), actions);
	}
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Light(const unsigned int pos, const unsigned int color) {
	if (updateThread) {
		vector<AlienFX_SDK::Afx_action> action{ TranslateColor(color, 0) };
		SetLightList(pos, &action);
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColor(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr) {
	return LFX_SetLightActionColorEx(dev, lid, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColorEx(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr1, const PLFX_COLOR clr2) {
	LFX_RESULT state = CheckState(dev, lid);
	if (!state) {
		vector<AlienFX_SDK::Afx_action> action{ TranslateColor(clr1, act), TranslateColor(clr2, act) };
		SetLight(MAKELPARAM(afx_dev->fxdevs[dev].pid, afx_dev->fxdevs[dev].lights[lid].lightid), &action);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColor(const unsigned int pos, const unsigned int act, const unsigned int clr) {
	return LFX_ActionColorEx(pos, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColorEx(const unsigned int pos, const unsigned int act, const unsigned int clr1, const unsigned int clr2) {
	if (updateThread) {
		vector<AlienFX_SDK::Afx_action> actions{ {TranslateColor(clr1, act), TranslateColor(clr2, act)} };
		SetLightList(pos, &actions);
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}