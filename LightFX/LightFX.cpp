#include "pch.h"
#include <vector>
#include "LFX2.h"
#include "AlienFX_SDK.h"
#include <map>

using namespace std;

struct ColorS {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char brightness;
};

union ColorU {
	ColorS cs;
	unsigned  ci;
};

AlienFX_SDK::Mappings* afx_map = NULL;
vector<AlienFX_SDK::Functions *> afx_devs;
BYTE gtempo = 5;
DWORD groups[6] = {0};
LFX_COLOR final;
//std::map<DWORD,LFX_COLOR> lastState;

LFX_COLOR* TranslateColor(PLFX_COLOR src) {
	// gamma-correction...
	final.red = ((unsigned) src->red * src->red) >> 8;
	final.green = ((unsigned) src->green * src->green) >> 8;
	final.blue = ((unsigned) src->blue * src->blue) >> 8;
	// brighness...
	final.red = ((unsigned) final.red * src->brightness) >> 8;
	final.green = ((unsigned) final.green * src->brightness) >> 8;
	final.blue = ((unsigned) final.blue * src->brightness) >> 8;
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
					if (afx_map->GetMappings()->at(i).devid == pid) {
						if (numlights == lind)
							return &afx_map->GetMappings()->at(i);
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
	if (!afx_map)
		return LFX_ERROR_NOINIT;
	if (!afx_devs.size() || (did >= 0 && afx_devs.size() <= did))
		return LFX_ERROR_NODEVS;
	if (lid >= 0 && !GetMapping(did, lid))
		return LFX_ERROR_NOLIGHTS;
	return LFX_SUCCESS;
}

DWORD GetGroupID(unsigned int mask) {
	switch (mask) {
	case LFX_ALL: return -1;
	case LFX_ALL_RIGHT: return groups[0];
	case LFX_ALL_LEFT: return groups[1];
	case LFX_ALL_UPPER: return groups[2];
	case LFX_ALL_LOWER: return groups[3];
	case LFX_ALL_FRONT: return groups[4];
	case LFX_ALL_REAR: return groups[5];
	default: return 0;
	}
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Initialize() {
	afx_map = new AlienFX_SDK::Mappings();
	afx_map->LoadMappings();
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
		if (grp->name == "Right") groups[0] = grp->gid;
		if (grp->name == "Left") groups[1] = grp->gid;
		if (grp->name == "Upper") groups[2] = grp->gid;
		if (grp->name == "Lower") groups[3] = grp->gid;
		if (grp->name == "Front") groups[4] = grp->gid;
		if (grp->name == "Rear") groups[5] = grp->gid;
	}
	if (afx_devs.size())
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
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
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
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		for (int i = 0; i < afx_devs.size(); i++)
			afx_devs[i]->Reset();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Update() {
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		for (int i = 0; i < afx_devs.size(); i++)
			afx_devs[i]->UpdateColors();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_UpdateDefault() {
	return LFX_Update();
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumDevices(unsigned int *const num) {
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		*num = (unsigned) afx_devs.size();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetDeviceDescription(const unsigned int dev, char *const name, const unsigned int namelen, unsigned char *const devtype) {
	LFX_RESULT state = CheckState(dev);
	if (state == LFX_SUCCESS) {
		int vid = afx_devs[dev]->GetVid(),
			pid = afx_devs[dev]->GetPID();
		AlienFX_SDK::devmap *map = afx_map->GetDeviceById(pid, vid);
		string devName;
		*devtype = LFX_DEVTYPE_UNKNOWN;
		if (map)
			devName = map->name;
		else
			devName = "Device #" + to_string(dev);
		if (namelen > devName.length()) {
			strcpy_s(name, namelen, devName.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumLights(const unsigned int dev, unsigned int *const numlights) {
	LFX_RESULT state = CheckState(dev);
	if (state == LFX_SUCCESS) {
		int pid = afx_devs[dev]->GetPID();
		*numlights = 0;
		for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
			if (afx_map->GetMappings()->at(i).devid == pid) {
				(*numlights)++;
			}
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightDescription(const unsigned int dev, const unsigned int lid, char *const name, const unsigned int namelen) {
	LFX_RESULT state = CheckState(dev, lid);
	if (state == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		if (map->name.length() < namelen) {
			strcpy_s(name, namelen, map->name.c_str());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightLocation(const unsigned int dev, const unsigned int lid, PLFX_POSITION const pos) {
	LFX_RESULT state = CheckState(dev, lid);
	if (state == LFX_SUCCESS) {
		*pos = {0};
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightColor(const unsigned int dev, const unsigned int lid, PLFX_COLOR const clr) {
	LFX_RESULT state = CheckState(dev, lid);
	if (state == LFX_SUCCESS) {
		// ToDo: remember last color
		*clr = {0};
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightColor(const unsigned int dev, const unsigned int lid, const PLFX_COLOR clr) {
	LFX_RESULT state = CheckState(dev, lid);
	if (state == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		LFX_COLOR* fin = TranslateColor(clr);
		LocateDev(map->devid)->SetColor(map->lightid, fin->red, fin->green, fin->blue);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Light(const unsigned int pos, const unsigned int color) {
	// pos as a group index.
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		ColorU fclr;
		fclr.ci = color;
		LFX_COLOR src = {fclr.cs.red, fclr.cs.green, fclr.cs.blue, fclr.cs.brightness};
		LFX_COLOR* fin = TranslateColor(&src);
		DWORD gid = GetGroupID(pos);
		AlienFX_SDK::group *grp = NULL;
		if (gid > 0)
			grp = afx_map->GetGroupById(gid);
		for (int j = 0; j < afx_devs.size(); j++) {
			vector<UCHAR> lights;
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i]->devid == afx_devs[j]->GetPID() &&
						!(grp->lights[i]->flags & ALIENFX_FLAG_POWER))
						lights.push_back((UCHAR) grp->lights[i]->lightid);
			} else {
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					if (afx_map->GetMappings()->at(i).devid == afx_devs[j]->GetPID() &&
						!(afx_map->GetMappings()->at(i).flags & ALIENFX_FLAG_POWER))
						lights.push_back((UCHAR) afx_map->GetMappings()->at(i).lightid);
				}
			}
			afx_devs[j]->SetMultiLights((int) lights.size(), lights.data(), fin->red, fin->green, fin->blue);
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColor(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr) {
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		LFX_COLOR* fin = TranslateColor(clr);
		vector<AlienFX_SDK::afx_act> actions;
		AlienFX_SDK::afx_act action = {(BYTE)act, 5, gtempo, fin->red, fin->green, fin->blue};
		actions.push_back(action);
		afx_devs[dev]->SetAction(map->lightid, actions);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_SetLightActionColorEx(const unsigned int dev, const unsigned int lid, const unsigned int act, const PLFX_COLOR clr1, const PLFX_COLOR clr2) {
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		AlienFX_SDK::mapping *map = GetMapping(dev, lid);
		vector<AlienFX_SDK::afx_act> actions;
		LFX_COLOR* fin = TranslateColor(clr1);
		AlienFX_SDK::afx_act action = {(BYTE)act, 5, gtempo, fin->red, fin->green, fin->blue};
		actions.push_back(action);
		fin = TranslateColor(clr2);
		action.r = fin->red; action.g = fin->green; action.b = fin->blue;
		actions.push_back(action);
		afx_devs[dev]->SetAction(map->lightid, actions);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColor(const unsigned int pos, const unsigned int act, const unsigned int clr) {
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		ColorU fclr;
		fclr.ci = clr;
		LFX_COLOR src = {fclr.cs.red, fclr.cs.green, fclr.cs.blue, fclr.cs.brightness};
		LFX_COLOR *fin = TranslateColor(&src);
		vector<AlienFX_SDK::afx_act> actions;
		AlienFX_SDK::afx_act action = {(BYTE) act, 5, gtempo, fin->red, fin->green, fin->blue};
		actions.push_back(action);
		DWORD gid = GetGroupID(pos);
		AlienFX_SDK::group *grp = NULL;
		if (gid > 0)
			grp = afx_map->GetGroupById(gid);
		for (int j = 0; j < afx_devs.size(); j++) {
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i]->devid == afx_devs[j]->GetPID() &&
						!(grp->lights[i]->flags & ALIENFX_FLAG_POWER))
						afx_devs[j]->SetAction(grp->lights[i]->lightid, actions);
			} else {
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					if (afx_map->GetMappings()->at(i).devid == afx_devs[j]->GetPID() &&
						!(afx_map->GetMappings()->at(i).flags & ALIENFX_FLAG_POWER))
						afx_devs[j]->SetAction(afx_map->GetMappings()->at(i).lightid, actions);
				}
			}
		}
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColorEx(const unsigned int pos, const unsigned int act, const unsigned int clr1, const unsigned int clr2) {
	LFX_RESULT state = CheckState();
	if (state == LFX_SUCCESS) {
		vector<AlienFX_SDK::afx_act> actions;
		ColorU fclr;
		fclr.ci = clr1;
		LFX_COLOR src = {fclr.cs.red, fclr.cs.green, fclr.cs.blue, fclr.cs.brightness};
		LFX_COLOR *fin = TranslateColor(&src);
		AlienFX_SDK::afx_act action = {(BYTE) act, 5, gtempo, fin->red, fin->green, fin->blue};
		actions.push_back(action);
		fclr.ci = clr2;
		src = {fclr.cs.red, fclr.cs.green, fclr.cs.blue, fclr.cs.brightness};
		fin = TranslateColor(&src);
		action.r = fin->red; action.g = fin->green; action.b = fin->blue;
		actions.push_back(action);
		DWORD gid = GetGroupID(pos);
		AlienFX_SDK::group *grp = NULL;
		if (gid > 0)
			grp = afx_map->GetGroupById(gid);
		for (int j = 0; j < afx_devs.size(); j++) {
			if (grp) {
				for (int i = 0; i < grp->lights.size(); i++)
					if (grp->lights[i]->devid == afx_devs[j]->GetPID() &&
						!(grp->lights[i]->flags & ALIENFX_FLAG_POWER))
						afx_devs[j]->SetAction(grp->lights[i]->lightid, actions);
			} else {
				for (int i = 0; i < afx_map->GetMappings()->size(); i++) {
					if (afx_map->GetMappings()->at(i).devid == afx_devs[j]->GetPID() &&
						!(afx_map->GetMappings()->at(i).flags & ALIENFX_FLAG_POWER))
						afx_devs[j]->SetAction(afx_map->GetMappings()->at(i).lightid, actions);
				}
			}
		}
	}
	return state;
}