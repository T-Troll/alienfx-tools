#include <vector>
#include "LFX2.h"
#include "AlienFX_SDK.h"
#include <map>

using namespace std;

AlienFX_SDK::Mappings* afx_map = NULL;
BYTE gtempo = 5;
AlienFX_SDK::group *groups[6]{NULL};
LFX_COLOR final;
LFX_RESULT state;
//map<WORD,vector<LFX_COLOR> > lastState;

void TranslateColor(LFX_COLOR src) {
	// gamma-correction...
	final.red = ((unsigned) src.red * src.red * src.brightness) / (255 * 255);
	final.green = ((unsigned) src.green * src.green * src.brightness) / (255 * 255);
	final.blue = ((unsigned) src.blue * src.blue * src.brightness) / (255 * 255);
	// brightness...
	//final.red = (((unsigned) final.red * src.brightness)) / 255;// >> 8;
	//final.green = (((unsigned) final.green * src.brightness)) / 255;// >> 8;
	//final.blue = (((unsigned) final.blue * src.brightness)) / 255;// >> 8;
}

LFX_RESULT CheckState(int did = -1, int lid = -1) {
	state = LFX_SUCCESS;
	if (!afx_map)
		state = LFX_ERROR_NOINIT;
	else
		if (did >= 0 && afx_map->fxdevs.size() <= did)
			state = LFX_ERROR_NODEVS;
		else
			if (lid >= 0 && afx_map->fxdevs[did].lights.size() <= lid)
				state = LFX_ERROR_NOLIGHTS;
	return state;
}

short GetGroupID(unsigned int mask) {
	switch (mask) {
	case LFX_ALL: return -1;
	case LFX_ALL_RIGHT: return 0;
	case LFX_ALL_LEFT: return 1;
	case LFX_ALL_UPPER: return 2;
	case LFX_ALL_LOWER: return 3;
	case LFX_ALL_FRONT: return 4;
	case LFX_ALL_REAR: return 5;
	default: return 0;
	}
}

BYTE GetActionMode(unsigned act) {
	switch (act) {
	case LFX_ACTION_PULSE: return AlienFX_SDK::Action::AlienFX_A_Pulse; break;
	case LFX_ACTION_MORPH: return AlienFX_SDK::Action::AlienFX_A_Morph; break;
	}
	return AlienFX_SDK::AlienFX_A_Color;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Initialize() {
	if (!afx_map) {
		afx_map = new AlienFX_SDK::Mappings();
		afx_map->LoadMappings();
	}
	afx_map->AlienFXAssignDevices();
	// now map groups to zones
	for (int i = 0; i < afx_map->GetGroups()->size(); i++) {
		AlienFX_SDK::group *grp = &afx_map->GetGroups()->at(i);
		if (grp->name == "Right") groups[0] = grp;
		if (grp->name == "Left") groups[1] = grp;
		if (grp->name == "Top") groups[2] = grp;
		if (grp->name == "Bottom") groups[3] = grp;
		if (grp->name == "Front") groups[4] = grp;
		if (grp->name == "Rear") groups[5] = grp;
	}
	if (afx_map->fxdevs.size() && afx_map->GetMappings()->size())
		return LFX_SUCCESS;
	else
		return LFX_ERROR_NODEVS;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Release() {
	if (afx_map) {
		delete afx_map;
		afx_map = NULL;
	}
	return LFX_SUCCESS;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetTiming(const int time) {
	gtempo = time / 50;
	return LFX_SUCCESS;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetVersion(char *const name, const unsigned int namelen) {
	if (CheckState() == LFX_SUCCESS) {
		string vName = "V5.2";
		for (int i = 0; i < afx_map->fxdevs.size(); i++) {
			vName += ",API v" + to_string(afx_map->fxdevs[i].dev->GetVersion());
		}
		if (namelen > vName.length()) {
			strcpy_s(name, namelen, vName.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}


FN_DECLSPEC LFX_RESULT STDCALL LFX_Reset() {
	if (CheckState() == LFX_SUCCESS) {
		for (int i = 0; i < afx_map->fxdevs.size(); i++)
			afx_map->fxdevs[i].dev->Reset();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Update() {
	if (CheckState() == LFX_SUCCESS) {
		for (int i = 0; i < afx_map->fxdevs.size(); i++)
			afx_map->fxdevs[i].dev->UpdateColors();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_UpdateDefault() {
	return LFX_Update();
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumDevices(unsigned int *const num) {
	if (CheckState() == LFX_SUCCESS) {
		*num = (unsigned) afx_map->fxdevs.size();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetDeviceDescription(const unsigned int dev, char *const name, const unsigned int namelen, unsigned char *const devtype) {
	if (CheckState(dev) == LFX_SUCCESS) {
		string devName = afx_map->fxdevs[dev].desc ? afx_map->fxdevs[dev].desc->name : "Device #" + to_string(dev);
		*devtype = LFX_DEVTYPE_UNKNOWN;
		switch (afx_map->fxdevs[dev].dev->GetVersion()) {
		case 0:*devtype = LFX_DEVTYPE_DESKTOP; break;
		case 1: case 2: case 3: case 4: *devtype = LFX_DEVTYPE_NOTEBOOK; break;
		case 5: *devtype = LFX_DEVTYPE_KEYBOARD; break;
		case 6: *devtype = LFX_DEVTYPE_DISPLAY; break;
		case 7: *devtype = LFX_DEVTYPE_MOUSE; break;
		}

		if (namelen > devName.length()) {
			strcpy_s(name, namelen, devName.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumLights(const unsigned int dev, unsigned int *const numlights) {
	if (CheckState(dev) == LFX_SUCCESS) {
		*numlights = (unsigned)afx_map->fxdevs[dev].lights.size();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightDescription(const unsigned int dev, const unsigned int lid, char *const name, const unsigned int namelen) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		if (namelen > afx_map->fxdevs[dev].lights[lid]->name.length()) {
			strcpy_s(name, namelen, afx_map->fxdevs[dev].lights[lid]->name.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

bool IsInGroup(AlienFX_SDK::mapping* map, AlienFX_SDK::group* grp) {
	if (map && grp)
		for (int i = 0; i < grp->lights.size(); i++)
			if (grp->lights[i].first == map->devid &&
				grp->lights[i].second == map->lightid)
				return true;
	return false;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightLocation(const unsigned int dev, const unsigned int lid, PLFX_POSITION const pos) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = afx_map->fxdevs[dev].lights[lid];
		pos->x = IsInGroup(map, groups[1]) ? 0 : IsInGroup(map, groups[0]) ? 2 : 1;
		pos->y = IsInGroup(map, groups[3]) ? 0 : IsInGroup(map, groups[2]) ? 2 : 1;
		pos->z = IsInGroup(map, groups[5]) ? 0 : IsInGroup(map, groups[4]) ? 2 : 1;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightColor(const unsigned int dev, const unsigned int lid, PLFX_COLOR const clr) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		// ToDo: remember last color
		*clr = {0};
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightColor(const unsigned int dev, const unsigned int lid, const PLFX_COLOR clr) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		TranslateColor(*clr);
		afx_map->fxdevs[dev].dev->SetColor(afx_map->fxdevs[dev].lights[lid]->lightid, {final.blue, final.green, final.red});
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Light(const unsigned int pos, const unsigned int color) {
	// pos as a group index.
	if (CheckState() == LFX_SUCCESS) {
		AlienFX_SDK::Colorcode src; src.ci = color;
		TranslateColor({src.r,src.g,src.b,src.br});
		int gid = GetGroupID(pos);
		AlienFX_SDK::group *grp = gid < 0 ? NULL : groups[gid];
		for (int j = 0; j < afx_map->fxdevs.size(); j++) {
			vector<byte> lights;
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i].first == afx_map->fxdevs[j].dev->GetPID())
						lights.push_back((byte) grp->lights[i].second);
			} else {
				for (int i = 0; gid < 0 && i < afx_map->fxdevs[j].lights.size(); i++) {
					if (!(afx_map->fxdevs[j].lights[i]->flags & ALIENFX_FLAG_POWER))
						lights.push_back((byte) afx_map->fxdevs[j].lights[i]->lightid);
				}
			}
			afx_map->fxdevs[j].dev->SetMultiLights(&lights, {final.blue, final.green, final.red});
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColor(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr) {
	return LFX_SetLightActionColorEx(dev, lid, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColorEx(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr1, const PLFX_COLOR clr2) {
	if (CheckState(dev,lid) == LFX_SUCCESS) {
		AlienFX_SDK::act_block actions{(byte) afx_map->fxdevs[dev].lights[lid]->lightid};
		TranslateColor(*clr1);
		BYTE fact = GetActionMode(act);
		actions.act.push_back({fact, gtempo, 7, final.red, final.green, final.blue});
		TranslateColor(*clr2);
		actions.act.push_back({fact, gtempo, 7, final.red, final.green, final.blue});
		afx_map->fxdevs[dev].dev->SetAction(&actions);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColor(const unsigned int pos, const unsigned int act, const unsigned int clr) {
	return LFX_ActionColorEx(pos, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColorEx(const unsigned int pos, const unsigned int act, const unsigned int clr1, const unsigned int clr2) {
	if (CheckState() == LFX_SUCCESS) {
		AlienFX_SDK::act_block actions;
		AlienFX_SDK::Colorcode src; src.ci = clr1;
		TranslateColor({src.r,src.g,src.b,src.br});
		BYTE fact = GetActionMode(act);
		actions.act.push_back({fact, gtempo, 7, final.red, final.green, final.blue});
		src.ci = clr2;
		TranslateColor({src.r,src.g,src.b,src.br});
		actions.act.push_back({fact, gtempo, 7, final.red, final.green, final.blue});
		int gid = GetGroupID(pos);
		AlienFX_SDK::group *grp = gid < 0 ? NULL : groups[gid];
		for (int j = 0; j < afx_map->fxdevs.size(); j++) {
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i].first == afx_map->fxdevs[j].dev->GetPID()) {
						actions.index = (byte) grp->lights[i].second;
						afx_map->fxdevs[j].dev->SetAction(&actions);
					}
			} else {
				for (int i = 0; gid < 0 && i < afx_map->fxdevs[j].lights.size(); i++) {
					if (!(afx_map->fxdevs[j].lights[i]->flags & ALIENFX_FLAG_POWER))
						afx_map->fxdevs[j].dev->SetAction(&actions);
				}
			}
		}
	}
	return state;
}