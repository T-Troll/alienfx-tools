#include <vector>
#include "LFX2.h"
#include "AlienFX_SDK.h"
//#include <map>

using namespace std;

const BYTE actionCodes[]{ AlienFX_SDK::AlienFX_A_Color, AlienFX_SDK::AlienFX_A_Morph, AlienFX_SDK::AlienFX_A_Pulse, AlienFX_SDK::AlienFX_A_Color };
AlienFX_SDK::Mappings* afx_map = NULL;
BYTE gtempo = 5;
AlienFX_SDK::Afx_group groups[4];

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

LFX_RESULT CheckState(int did = -1, int lid = -1) {
	if (!afx_map)
		return LFX_ERROR_NOINIT;
	if (!afx_map->activeDevices || !(did < (int)afx_map->activeDevices))
		return LFX_ERROR_NODEVS;
	if (did < 0 || lid < (int)afx_map->fxdevs[did].lights.size())
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

LFX_RESULT FillDevs() {
	if (afx_map) {
		afx_map->AlienFXAssignDevices();
		for (auto dev = afx_map->fxdevs.begin(); dev != afx_map->fxdevs.end(); dev++) {
			dev->dev->SetBrightness(255, &dev->lights, false);
		}
		for (int g = 0; g < 4; g++)
			groups[g].lights.clear();
		if (afx_map->activeDevices) {
			// now map lights to zones
			for (auto grid = afx_map->GetGrids()->begin(); grid != afx_map->GetGrids()->end(); grid++) {
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
	if (afx_map) {
		for (auto i = afx_map->fxdevs.begin(); i < afx_map->fxdevs.end(); i++)
			i->dev->Reset();
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Update() {
	if (afx_map) {
		for (auto i = afx_map->fxdevs.begin(); i < afx_map->fxdevs.end(); i++)
			i->dev->UpdateColors();
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_UpdateDefault() {
	return LFX_Update();
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumDevices(unsigned int *const num) {
	if (afx_map) {
		*num = afx_map->activeDevices;
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetDeviceDescription(const unsigned int dev, char *const name, const unsigned int namelen, unsigned char *const devtype) {
	LFX_RESULT state = CheckState(dev);
	if (!state) {
		string devName = afx_map->fxdevs[dev].name.length() ? afx_map->fxdevs[dev].name : "Device #" + to_string(dev);
		*devtype = LFX_DEVTYPE_UNKNOWN;
		switch (afx_map->fxdevs[dev].version) {
		case AlienFX_SDK::API_ACPI: *devtype = LFX_DEVTYPE_DESKTOP; break;
		case AlienFX_SDK::API_V2: case AlienFX_SDK::API_V3: case AlienFX_SDK::API_V4: *devtype = LFX_DEVTYPE_NOTEBOOK; break;
		case AlienFX_SDK::API_V5: case AlienFX_SDK::API_V8: *devtype = LFX_DEVTYPE_KEYBOARD; break;
		case AlienFX_SDK::API_V6: *devtype = LFX_DEVTYPE_DISPLAY; break;
		case AlienFX_SDK::API_V7: *devtype = LFX_DEVTYPE_MOUSE; break;
		}

		if (namelen > devName.length()) {
			strcpy_s(name, namelen, devName.data());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetNumLights(const unsigned int dev, unsigned int *const numlights) {
	LFX_RESULT state = CheckState(dev);
	if (state) {
		*numlights = (unsigned)afx_map->fxdevs[dev].lights.size();
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightDescription(const unsigned int dev, const unsigned int lid, char *const name, const unsigned int namelen) {
	LFX_RESULT state = CheckState(dev, lid);
	if (!state) {
		if (namelen > afx_map->fxdevs[dev].lights[lid].name.length()) {
			strcpy_s(name, namelen, afx_map->fxdevs[dev].lights[lid].name.data());
		} else
			return LFX_ERROR_BUFFSIZE;
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_GetLightLocation(const unsigned int dev, const unsigned int lid, PLFX_POSITION const pos) {
	LFX_RESULT state = CheckState(dev, lid);
	if (!state) {
		AlienFX_SDK::Afx_groupLight cLight{ afx_map->fxdevs[dev].pid , afx_map->fxdevs[dev].lights[lid].lightid };
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
		vector<byte> ids = { afx_map->fxdevs[dev].lights[lid].lightid };
		afx_map->fxdevs[dev].dev->SetMultiColor(&ids, TranslateColor(clr, 0));
	}
	return state;
}

void SetLightList(unsigned pos, vector<AlienFX_SDK::Afx_action>* actions) {
	vector<byte> lights;
	vector<AlienFX_SDK::Afx_lightblock> act;
	AlienFX_SDK::Afx_group* grp = GetGroupID(pos);
	for (AlienFX_SDK::Afx_device& j : afx_map->fxdevs) {
		if (grp) {
			for (AlienFX_SDK::Afx_groupLight& i : grp->lights)
				if (i.did == j.pid) {
					lights.push_back((byte)i.lid);
					act.push_back({ (byte)i.lid, *actions });
				}
		}
		else {
			for (AlienFX_SDK::Afx_light& i : j.lights) {
				if (!i.flags) {
					lights.push_back((byte)i.lightid);
					act.push_back({ (byte)i.lightid, *actions });
				}
			}
		}
		if (actions->size() == 1)
			j.dev->SetMultiColor(&lights, actions->front());
		else
			j.dev->SetMultiAction(&act);
	}
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_Light(const unsigned int pos, const unsigned int color) {
	if (afx_map) {
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
		vector<AlienFX_SDK::Afx_action> actions{ {TranslateColor(clr1, act), TranslateColor(clr2, act)} };
		afx_map->fxdevs[dev].dev->SetAction((byte)afx_map->fxdevs[dev].lights[lid].lightid, &actions);
	}
	return state;
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColor(const unsigned int pos, const unsigned int act, const unsigned int clr) {
	return LFX_ActionColorEx(pos, act, clr, 0);
}

FN_DECLSPEC LFX_RESULT STDCALL LFX_ActionColorEx(const unsigned int pos, const unsigned int act, const unsigned int clr1, const unsigned int clr2) {
	if (afx_map) {
		vector<AlienFX_SDK::Afx_action> actions{ {TranslateColor(clr1, act), TranslateColor(clr2, act)} };
		SetLightList(pos, &actions);
		return LFX_SUCCESS;
	}
	return LFX_ERROR_NOINIT;
}