#include "pch.h"
#include <vector>
#include "LFX2.h"
#include "AlienFX_SDK.h"
#include <map>

using namespace std;

union ColorU {
	struct {
		unsigned char blue;
		unsigned char green;
		unsigned char red;
		unsigned char brightness;
	};
	unsigned  ci;
};

AlienFX_SDK::Mappings* afx_map = NULL;
vector<AlienFX_SDK::Functions *> afx_devs;
BYTE gtempo = 5;
AlienFX_SDK::group *groups[6] = {NULL};
//DWORD groups[6] = {0};
LFX_COLOR final;
LFX_RESULT state;
map<WORD,vector<LFX_COLOR> > lastState;

LFX_COLOR* TranslateColor(ColorU* src) {
	// gamma-correction...
	final.red = ((unsigned) src->red * src->red) / 255;
	final.green = ((unsigned) src->green * src->green) / 255;
	final.blue = ((unsigned) src->blue * src->blue) / 255;
	// brighness...
	final.red = (((unsigned) final.red * src->brightness)) / 255;// >> 8;
	final.green = (((unsigned) final.green * src->brightness)) / 255;// >> 8;
	final.blue = (((unsigned) final.blue * src->brightness)) / 255;// >> 8;
	return &final;
}

AlienFX_SDK::Functions *LocateDev(int pid) {
	for (int i = 0; i < afx_devs.size(); i++)
		if (afx_devs[i]->GetPID() == pid)
			return afx_devs[i];
	return nullptr;
};

AlienFX_SDK::mapping *GetMapping(int dind, int lind) {
	if (afx_map) {
		if (afx_devs.size()) {
			if (afx_devs.size() > dind) {
				int pid = afx_devs[dind]->GetPID();
				int numlights = 0;
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					if (afx_map->GetMappings()->at(i)->devid == pid) {
						if (numlights == lind)
							return afx_map->GetMappings()->at(i);
						else
							numlights++;
					}
				}
			}
		}
	}
	return NULL;
}

LFX_RESULT CheckState(int did = -1, int lid = -1) {
	state = LFX_SUCCESS;
	if (!afx_map)
		state = LFX_ERROR_NOINIT;
	else
		if (!afx_devs.size() || (did >= 0 && afx_devs.size() <= did))
			state = LFX_ERROR_NODEVS;
		else
			if (lid >= 0 && !GetMapping(did, lid))
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
	vector<pair<DWORD, DWORD>> devList = afx_map->AlienFXEnumDevices();
	for (int i = 0; i < devList.size(); i++) {
		AlienFX_SDK::Functions *dev = new AlienFX_SDK::Functions();
		int pid = dev->AlienFXInitialize(devList[i].first, devList[i].second);
		if (pid != -1) {
			afx_devs.push_back(dev);
		}
	}
	// now map groups to zones
	for (int i = 0; i < afx_map->GetGroups()->size(); i++) {
		AlienFX_SDK::group *grp = &afx_map->GetGroups()->at(i);
		if (grp->name == "Right") groups[0] = grp;// grp->gid;
		if (grp->name == "Left") groups[1] = grp;
		if (grp->name == "Top") groups[2] = grp;
		if (grp->name == "Bottom") groups[3] = grp;
		if (grp->name == "Front") groups[4] = grp;
		if (grp->name == "Rear") groups[5] = grp;
	}
	if (afx_devs.size() && afx_map->GetMappings()->size())
		return LFX_SUCCESS;
	else
		return LFX_ERROR_NODEVS;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Release() {
	if (afx_map) {
		delete afx_map;
		afx_map = NULL;
		if (afx_devs.size() > 0) {
			for (int i = 0; i < afx_devs.size(); i++)
				afx_devs[i]->AlienFXClose();
			afx_devs.clear();
		}
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
		for (int i = 0; i < afx_devs.size(); i++) {
			vName += ",API v" + to_string(afx_devs[i]->GetVersion());
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
		for (int i = 0; i < afx_devs.size(); i++)
			afx_devs[i]->Reset();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Update() {
	if (CheckState() == LFX_SUCCESS) {
		for (int i = 0; i < afx_devs.size(); i++)
			afx_devs[i]->UpdateColors();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_UpdateDefault() {
	return LFX_Update();
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumDevices(unsigned int *const num) {
	if (CheckState() == LFX_SUCCESS) {
		*num = (unsigned) afx_devs.size();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetDeviceDescription(const unsigned int dev, char *const name, const unsigned int namelen, unsigned char *const devtype) {
	if (CheckState(dev) == LFX_SUCCESS) {
		int vid = afx_devs[dev]->GetVid(),
			pid = afx_devs[dev]->GetPID();
		AlienFX_SDK::devmap *map = afx_map->GetDeviceById(pid, vid);
		string devName;
		*devtype = LFX_DEVTYPE_UNKNOWN;
		devName = map ? map->name : "Device #" + to_string(dev);

		if (namelen > devName.length()) {
			strcpy_s(name, namelen, devName.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumLights(const unsigned int dev, unsigned int *const numlights) {
	if (CheckState(dev) == LFX_SUCCESS) {
		int pid = afx_devs[dev]->GetPID();
		*numlights = 0;
		for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
			if (afx_map->GetMappings()->at(i)->devid == pid) {
				(*numlights)++;
			}
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightDescription(const unsigned int dev, const unsigned int lid, char *const name, const unsigned int namelen) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		if (map->name.length() < namelen) {
			strcpy_s(name, namelen, map->name.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

bool IsInGroup(AlienFX_SDK::mapping* map, AlienFX_SDK::group* grp) {
	//AlienFX_SDK::group *grp = gid ? afx_map->GetGroupById(gid) : NULL;
	if (map && grp)
		for (int i = 0; i < grp->lights.size(); i++)
			if (grp->lights[i]->devid == map->devid &&
				grp->lights[i]->lightid == map->lightid)
				return true;
	return false;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightLocation(const unsigned int dev, const unsigned int lid, PLFX_POSITION const pos) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		pos->x = IsInGroup(map, groups[1]) ? 0 : 1;
		pos->x = IsInGroup(map, groups[0]) ? 2 : pos->x;
		pos->y = IsInGroup(map, groups[3]) ? 0 : 1;
		pos->y = IsInGroup(map, groups[2]) ? 2 : pos->y;
		pos->z = IsInGroup(map, groups[5]) ? 0 : 1;
		pos->z = IsInGroup(map, groups[4]) ? 2 : pos->z;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightColor(const unsigned int dev, const unsigned int lid, PLFX_COLOR const clr) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		// ToDo: remember last color
		*clr = {0}; //clr->brightness = 255;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightColor(const unsigned int dev, const unsigned int lid, const PLFX_COLOR clr) {
	if (CheckState(dev, lid) == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		ColorU src = {clr->blue, clr->green, clr->red, clr->brightness};
		LFX_COLOR *fin = TranslateColor(&src);
		LocateDev(map->devid)->SetColor(map->lightid, fin->red, fin->green, fin->blue);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Light(const unsigned int pos, const unsigned int color) {
	// pos as a group index.
	if (CheckState() == LFX_SUCCESS) {
		ColorU src; src.ci = color;
		LFX_COLOR *fin = TranslateColor(&src);
		int gid = GetGroupID(pos);
		AlienFX_SDK::group *grp = NULL;
		if (gid >= 0)
			grp = groups[gid];
		for (int j = 0; j < afx_devs.size(); j++) {
			vector<UCHAR> lights;
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i]->devid == afx_devs[j]->GetPID() &&
						!(grp->lights[i]->flags & ALIENFX_FLAG_POWER))
						lights.push_back((UCHAR) grp->lights[i]->lightid);
			} else {
				for (int i = 0; gid < 0 && i < afx_map->GetMappings()->size(); i++) {
					AlienFX_SDK::mapping *lgh = afx_map->GetMappings()->at(i);
					if (lgh->devid == afx_devs[j]->GetPID() &&
						!(lgh->flags & ALIENFX_FLAG_POWER))
						lights.push_back((UCHAR) lgh->lightid);
				}
			}
			afx_devs[j]->SetMultiLights((int) lights.size(), lights.data(), fin->red, fin->green, fin->blue);
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColor(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr) {
	return LFX_SetLightActionColorEx(dev, lid, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColorEx(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr1, const PLFX_COLOR clr2) {
	if (CheckState() == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		vector<AlienFX_SDK::afx_act> actions;
		ColorU src = {clr1->blue, clr1->green, clr1->red, clr1->brightness};
		LFX_COLOR* fin = TranslateColor(&src);
		BYTE fact = GetActionMode(act);
		actions.push_back({fact, gtempo, 7, fin->red, fin->green, fin->blue});
		src = {clr2->blue, clr2->green, clr2->red, clr2->brightness};
		fin = TranslateColor(&src);
		actions.push_back({fact, gtempo, 7, fin->red, fin->green, fin->blue});
		afx_devs[dev]->SetAction(map->lightid, actions);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColor(const unsigned int pos, const unsigned int act, const unsigned int clr) {
	return LFX_ActionColorEx(pos, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColorEx(const unsigned int pos, const unsigned int act, const unsigned int clr1, const unsigned int clr2) {
	if (CheckState() == LFX_SUCCESS) {
		vector<AlienFX_SDK::afx_act> actions;
		ColorU fclr; fclr.ci = clr1;
		LFX_COLOR *fin = TranslateColor(&fclr);
		BYTE fact = GetActionMode(act);
		actions.push_back({fact, gtempo, 7, fin->red, fin->green, fin->blue});
		fclr.ci = clr2;
		fin = TranslateColor(&fclr);
		actions.push_back({fact, gtempo, 7, fin->red, fin->green, fin->blue});
		int gid = GetGroupID(pos);
		AlienFX_SDK::group *grp = NULL;
		if (gid >= 0)
			grp = groups[gid];
		for (int j = 0; j < afx_devs.size(); j++) {
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i]->devid == afx_devs[j]->GetPID() &&
						!(grp->lights[i]->flags & ALIENFX_FLAG_POWER))
						afx_devs[j]->SetAction(grp->lights[i]->lightid, actions);
			} else {
				for (int i = 0; gid < 0 && i < afx_map->GetMappings()->size(); i++) {
					AlienFX_SDK::mapping *lgh = afx_map->GetMappings()->at(i);
					if (lgh->devid == afx_devs[j]->GetPID() &&
						!(lgh->flags & ALIENFX_FLAG_POWER))
						afx_devs[j]->SetAction(lgh->lightid, actions);
				}
			}
		}
	}
	return state;
}