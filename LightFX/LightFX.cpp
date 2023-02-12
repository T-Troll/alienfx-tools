#include <vector>
#include "LFX2.h"
#include "AlienFX_SDK.h"
#include <map>

using namespace std;

const BYTE actionCodes[]{ AlienFX_SDK::AlienFX_A_Color, AlienFX_SDK::AlienFX_A_Morph, AlienFX_SDK::AlienFX_A_Pulse, AlienFX_SDK::AlienFX_A_Color };
AlienFX_SDK::Mappings* afx_map = NULL;
BYTE gtempo = 5;
AlienFX_SDK::Afx_group groups[4];
LFX_RESULT state;
//map<WORD,vector<LFX_COLOR> > lastState;

AlienFX_SDK::Afx_action TranslateColor(LFX_COLOR src, int type) {
	// gamma-correction and brightness...
	AlienFX_SDK::Afx_action final = { actionCodes[type], gtempo, 7,
		 (byte)(((unsigned)src.red * src.red * src.brightness) / 65025),
		 (byte)(((unsigned)src.green * src.green * src.brightness) / 65025),
		 (byte)(((unsigned)src.blue * src.blue * src.brightness) / 65025) };
	return final;
}

AlienFX_SDK::Afx_action TranslateColor(unsigned int src, byte type) {
	AlienFX_SDK::Afx_colorcode c; c.ci = src;
	return TranslateColor({ c.r, c.g, c.b, c.br }, type);
}

LFX_RESULT CheckState(int did = -1, int lid = -1) {
	if (!afx_map)
		return LFX_ERROR_NOINIT;
	if (!(did < (int)afx_map->activeDevices))
		return LFX_ERROR_NODEVS;
	if (did < 0 || lid < (int)afx_map->fxdevs[did].lights.size())
		return LFX_SUCCESS;
	return LFX_ERROR_NOLIGHTS;
}

bool LightInGroup(AlienFX_SDK::Afx_group* grp, AlienFX_SDK::Afx_groupLight lgh) {
	for (auto pos = grp->lights.begin(); pos < grp->lights.end(); pos++)
		if (pos->lgh == lgh.lgh)
			return true;
	return false;
}

AlienFX_SDK::Afx_group* GetGroupID(unsigned int mask) {
	switch (mask) {
	case LFX_ALL_RIGHT: return &groups[0];
	case LFX_ALL_LEFT: return &groups[1];
	case LFX_ALL_UPPER: return &groups[2];
	case LFX_ALL_LOWER: return &groups[3];
	case LFX_ALL_FRONT: return &groups[3];
	case LFX_ALL_REAR: return &groups[2];
	}
	return NULL;
}

LFX_RESULT FillDevs() {
	if (afx_map) {
		afx_map->AlienFXAssignDevices();
		if (afx_map->activeDevices) {
			// now map lights to zones
			for (int g = 0; g < 4; g++)
				groups[g].lights.clear();
			for (auto grid = afx_map->GetGrids()->begin(); grid != afx_map->GetGrids()->end(); grid++) {
				int dx = grid->x / 3,
					dy = grid->y / 3;
				for (int x = 0; x < grid->x; x++)
					for (int y = 0; y < grid->y; y++) {
						AlienFX_SDK::Afx_groupLight cLight = grid->grid[y * grid->x + x];
						if (cLight.lgh) {
							if (x < dx && !LightInGroup(&groups[1], cLight)) //left
								groups[1].lights.push_back(cLight);
							if (grid->x - x < dx && !LightInGroup(&groups[0], cLight)) // right
								groups[0].lights.push_back(cLight);
							if (y < dy && !LightInGroup(&groups[2], cLight)) { // upper and rear
								groups[2].lights.push_back(cLight);
								//groups[5].lights.push_back(cLight);
							}
							if (grid->y - y < dy && !LightInGroup(&groups[3], cLight)) { // lower and front
								groups[3].lights.push_back(cLight);
								//groups[4].lights.push_back(cLight);
							}
						}
					}
			}
			return LFX_SUCCESS;
		}
		else
			return LFX_ERROR_NODEVS;
	}
	return LFX_ERROR_NOINIT;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Initialize() {
	if (!afx_map) {
		afx_map = new AlienFX_SDK::Mappings();
		afx_map->LoadMappings();
	}
	return FillDevs();
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
	if ((state = FillDevs()) == LFX_SUCCESS) {
		*num = afx_map->activeDevices;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetDeviceDescription(const unsigned int dev, char *const name, const unsigned int namelen, unsigned char *const devtype) {
	if (CheckState(dev) == LFX_SUCCESS) {
		string devName = afx_map->fxdevs[dev].name.length() ? afx_map->fxdevs[dev].name : "Device #" + to_string(dev);
		*devtype = LFX_DEVTYPE_UNKNOWN;
		switch (afx_map->fxdevs[dev].dev->GetVersion()) {
		case 0: *devtype = LFX_DEVTYPE_DESKTOP; break;
		case 1: case 2: case 3: case 4: *devtype = LFX_DEVTYPE_NOTEBOOK; break;
		case 5: case 8: *devtype = LFX_DEVTYPE_KEYBOARD; break;
		case 6: *devtype = LFX_DEVTYPE_DISPLAY; break;
		case 7: *devtype = LFX_DEVTYPE_MOUSE; break;
		}

		if (namelen > devName.length()) {
			strcpy_s(name, namelen, devName.data());
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
			strcpy_s(name, namelen, afx_map->fxdevs[dev].lights[lid].name.data());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightLocation(const unsigned int dev, const unsigned int lid, PLFX_POSITION const pos) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		AlienFX_SDK::Afx_groupLight cur{ afx_map->fxdevs[dev].pid , afx_map->fxdevs[dev].lights[lid].lightid };
		pos->x = LightInGroup(&groups[1], cur) ? 0 :
			LightInGroup(&groups[0], cur) ? 2 : 1;
		pos->y = LightInGroup(&groups[3], cur) ? 0 :
			LightInGroup(&groups[2], cur) ? 2 : 1;
		pos->z = LightInGroup(&groups[2], cur) ? 0 :
			LightInGroup(&groups[3], cur) ? 2 : 1;
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
		//AlienFX_SDK::Afx_lightblock action = { (byte)afx_map->fxdevs[dev].lights[lid].lightid, { TranslateColor(*clr, 0)} };
		vector<AlienFX_SDK::Afx_action> act{ TranslateColor(*clr, 0) };
		afx_map->fxdevs[dev].dev->SetAction(afx_map->fxdevs[dev].lights[lid].lightid, &act);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Light(const unsigned int pos, const unsigned int color) {
	// pos as a group index.
	if (CheckState() == LFX_SUCCESS) {
		auto action = TranslateColor(color, 0);
		AlienFX_SDK::Afx_group* grp = GetGroupID(pos);
		for (auto j = afx_map->fxdevs.begin(); j < afx_map->fxdevs.end(); j++) {
			vector<byte> lights;
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i].did == j->pid)
						lights.push_back((byte)grp->lights[i].lid);
			} else {
				for (auto i = j->lights.begin(); i < j->lights.end(); i++) {
					if (!i->flags)
						lights.push_back((byte) i->lightid);
				}
			}
			j->dev->SetMultiColor(&lights, {action.b, action.g, action.r});
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColor(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr) {
	return LFX_SetLightActionColorEx(dev, lid, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColorEx(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr1, const PLFX_COLOR clr2) {
	if (CheckState(dev,lid) == LFX_SUCCESS) {
		vector<AlienFX_SDK::Afx_action> actions{ {TranslateColor(*clr1, act), TranslateColor(*clr2, act)} };
		afx_map->fxdevs[dev].dev->SetAction((byte)afx_map->fxdevs[dev].lights[lid].lightid, &actions);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColor(const unsigned int pos, const unsigned int act, const unsigned int clr) {
	return LFX_ActionColorEx(pos, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColorEx(const unsigned int pos, const unsigned int act, const unsigned int clr1, const unsigned int clr2) {
	if (CheckState() == LFX_SUCCESS) {
		vector<AlienFX_SDK::Afx_action> actions{ {TranslateColor(clr1, act), TranslateColor(clr2, act)} };
		AlienFX_SDK::Afx_group *grp = GetGroupID(pos);
		for (auto j = afx_map->fxdevs.begin(); j < afx_map->fxdevs.end(); j++) {
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i].did == j->pid) {
						j->dev->SetAction((byte)grp->lights[i].lid, &actions);
					}
			} else {
				for (auto i = j->lights.begin(); i < j->lights.end(); i++) {
					if (!i->flags) {
						j->dev->SetAction((byte)i->lightid, &actions);
					}
				}
			}
		}
	}
	return state;
}