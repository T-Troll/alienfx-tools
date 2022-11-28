#include <vector>
#include "LFX2.h"
#include "AlienFX_SDK.h"
#include <map>

using namespace std;

AlienFX_SDK::Mappings* afx_map = NULL;
BYTE gtempo = 5;
AlienFX_SDK::Afx_group groups[6];
LFX_COLOR final;
LFX_RESULT state;
//map<WORD,vector<LFX_COLOR> > lastState;

void TranslateColor(LFX_COLOR src) {
	// gamma-correction and brightness...
	final.red = ((unsigned) src.red * src.red * src.brightness) / (255 * 255);
	final.green = ((unsigned) src.green * src.green * src.brightness) / (255 * 255);
	final.blue = ((unsigned) src.blue * src.blue * src.brightness) / (255 * 255);
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

int LightInGroup(AlienFX_SDK::Afx_group* grp, AlienFX_SDK::Afx_groupLight lgh) {
	for (auto pos = grp->lights.begin(); pos < grp->lights.end(); pos++)
		if (pos->lgh == lgh.lgh)
			return (int)(pos - grp->lights.begin());
	return -1;
}

void FillGroupsFromGrid() {
	if (afx_map->GetGrids()->size()) {
		AlienFX_SDK::Afx_grid* grid = &afx_map->GetGrids()->at(0);
		int dx = grid->x / 3,
			dy = grid->y / 3;
		for (int x = 0; x < grid->x; x++)
			for (int y = 0; y < grid->y; y++) {
				AlienFX_SDK::Afx_groupLight cLight = grid->grid[y * grid->x + x];
				if (x < dx && LightInGroup(&groups[1], cLight) < 0) //left
					groups[1].lights.push_back(cLight);
				if (grid->x - x < dx && LightInGroup(&groups[0], cLight) < 0) // right
					groups[0].lights.push_back(cLight);
				if (y < dy && LightInGroup(&groups[2], cLight) < 0) { // upper and rear
					groups[2].lights.push_back(cLight);
					groups[5].lights.push_back(cLight);
				}
				if (grid->y - y < dy && LightInGroup(&groups[3], cLight) < 0) { // lower and front
					groups[3].lights.push_back(cLight);
					groups[4].lights.push_back(cLight);
				}
			}
	}
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
	case LFX_ACTION_PULSE: return AlienFX_SDK::Action::AlienFX_A_Pulse;
	case LFX_ACTION_MORPH: return AlienFX_SDK::Action::AlienFX_A_Morph;
	}
	return AlienFX_SDK::AlienFX_A_Color;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Initialize() {
	if (!afx_map) {
		afx_map = new AlienFX_SDK::Mappings();
		afx_map->LoadMappings();
	}
	afx_map->AlienFXAssignDevices();
	// remove devices not present
	for (int i = 0; i < afx_map->fxdevs.size(); i++)
		if (!afx_map->fxdevs[i].dev) {
			afx_map->fxdevs.erase(i + afx_map->fxdevs.begin());
			i--;
		}

	if (afx_map->fxdevs.size()) {
		// now map lights to zones
		FillGroupsFromGrid();
		return LFX_SUCCESS;
	}
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
		for (auto i = afx_map->fxdevs.begin(); i < afx_map->fxdevs.end(); i++) {
			vName += ",API v" + to_string(i->dev->GetVersion());
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
		for (auto i = afx_map->fxdevs.begin(); i < afx_map->fxdevs.end(); i++)
			i->dev->Reset();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Update() {
	if (CheckState() == LFX_SUCCESS) {
		for (auto i = afx_map->fxdevs.begin(); i < afx_map->fxdevs.end(); i++)
			i->dev->UpdateColors();
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
		string devName = afx_map->fxdevs[dev].name.length() ? afx_map->fxdevs[dev].name : "Device #" + to_string(dev);
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
		if (namelen > afx_map->fxdevs[dev].lights[lid].name.length()) {
			strcpy_s(name, namelen, afx_map->fxdevs[dev].lights[lid].name.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

//bool IsInGroup(AlienFX_SDK::mapping* map, AlienFX_SDK::group* grp) {
//	if (map && grp)
//		for (int i = 0; i < grp->lights.size(); i++)
//			if (grp->lights[i]->devid == map->devid &&
//				grp->lights[i]->lightid == map->lightid)
//				return true;
//	return false;
//}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightLocation(const unsigned int dev, const unsigned int lid, PLFX_POSITION const pos) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		AlienFX_SDK::Afx_light *map = &afx_map->fxdevs[dev].lights[lid];
		if (map) {
			AlienFX_SDK::Afx_groupLight cur{ afx_map->fxdevs[dev].pid , map->lightid };
			pos->x = LightInGroup(&groups[1], cur) >= 0 ? 0 :
				LightInGroup(&groups[0], cur) >= 0 ? 2 : 1;
			pos->y = LightInGroup(&groups[3], cur) >= 0 ? 0 :
				LightInGroup(&groups[2], cur) >= 0 ? 2 : 1;
			pos->z = LightInGroup(&groups[5], cur) >= 0 ? 0 :
				LightInGroup(&groups[4], cur) >= 0 ? 2 : 1;
		}
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
		afx_map->fxdevs[dev].dev->SetColor(afx_map->fxdevs[dev].lights[lid].lightid, {final.blue, final.green, final.red});
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Light(const unsigned int pos, const unsigned int color) {
	// pos as a group index.
	if (CheckState() == LFX_SUCCESS) {
		AlienFX_SDK::Afx_colorcode src; src.ci = color;
		TranslateColor({src.r,src.g,src.b,src.br});
		int gid = GetGroupID(pos);
		AlienFX_SDK::Afx_group *grp = gid < 0 ? NULL : &groups[gid];
		for (auto j = afx_map->fxdevs.begin(); j < afx_map->fxdevs.end(); j++) {
			vector<byte> lights;
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i].did == j->pid)
						lights.push_back((byte)grp->lights[i].lid);
			} else {
				for (auto i = j->lights.begin(); gid < 0 && i < j->lights.end(); i++) {
					if (!(i->flags & ALIENFX_FLAG_POWER))
						lights.push_back((byte) i->lightid);
				}
			}
			j->dev->SetMultiColor(&lights, {final.blue, final.green, final.red});
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColor(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr) {
	return LFX_SetLightActionColorEx(dev, lid, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColorEx(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr1, const PLFX_COLOR clr2) {
	if (CheckState(dev,lid) == LFX_SUCCESS) {
		AlienFX_SDK::Afx_lightblock actions{(byte) afx_map->fxdevs[dev].lights[lid].lightid};
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
		AlienFX_SDK::Afx_lightblock actions;
		AlienFX_SDK::Afx_colorcode src; src.ci = clr1;
		TranslateColor({src.r,src.g,src.b,src.br});
		BYTE fact = GetActionMode(act);
		actions.act.push_back({fact, gtempo, 7, final.red, final.green, final.blue});
		src.ci = clr2;
		TranslateColor({src.r,src.g,src.b,src.br});
		actions.act.push_back({fact, gtempo, 7, final.red, final.green, final.blue});
		int gid = GetGroupID(pos);
		AlienFX_SDK::Afx_group *grp = gid < 0 ? NULL : &groups[gid];
		for (auto j = afx_map->fxdevs.begin(); j < afx_map->fxdevs.end(); j++) {
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i].did == j->pid) {
						actions.index = (byte)grp->lights[i].lid;
						j->dev->SetAction(&actions);
					}
			} else {
				for (auto i = j->lights.begin(); gid < 0 && i < j->lights.end(); i++) {
					if (!(i->flags & ALIENFX_FLAG_POWER))
						j->dev->SetAction(&actions);
				}
			}
		}
	}
	return state;
}