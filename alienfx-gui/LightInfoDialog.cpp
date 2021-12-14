#include "alienfx-gui.h"

extern lightset* FindMapping(int mid);
extern zone *FindAmbMapping(int lid);
extern haptics_map *FindHapMapping(int lid);
extern void SwitchTab(int);

extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);

extern int eItem;

AlienFX_SDK::Colorcode from_c, to_c, from_e, to_e, from_h, to_h, from_f, to_f;
vector<AlienFX_SDK::group *> lghGrp;

int FindLastActiveID() {

	int devid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->devid : 0,
		lightid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->lightid : eItem;

	switch (conf->GetEffect()) {
	case 0: case 3:
	{
		for (int iter = (int) conf->activeProfile->lightsets.size()-1; iter >= 0; iter--) {
			lightset *Iter = &conf->activeProfile->lightsets[iter];
			if (Iter->devid || !devid) {
				if (Iter->devid == devid && Iter->lightid == lightid) {
					return iter;
				}
			} else {
				// check groups...
				for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
					if (Iter->lightid == (*gIter)->gid) {
						return iter;
					}
			}
		}
	} break;
	case 1:
	{
		for (int iter = (int) conf->amb_conf->zones.size()-1; iter >= 0; iter--) {
			zone *Iter = &conf->amb_conf->zones[iter];
			if (Iter->devid || !devid) {
				if (Iter->devid == devid && Iter->lightid == lightid) {
					return iter;
				}
			} else {
				// check groups...
				for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
					if (Iter->lightid == (*gIter)->gid) {
						return iter;
					}
			}
		}
	} break;
	case 2:
	{
		for (int iter = (int) conf->hap_conf->haptics.size()-1; iter >= 0; iter--) {
			haptics_map *Iter = &conf->hap_conf->haptics[iter];
			if (Iter->devid || !devid) {
				if (Iter->devid == devid && Iter->lightid == lightid) {
					return iter;
				}
			} else {
				// check groups...
				for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
					if (Iter->lightid == (*gIter)->gid) {
						return iter;
					}
			}
		}
	} break;
	}
	return -1;
}

void SetLightColors(HWND hDlg, int id) {

	if (id >= 0) {
		lightset *mmap = FindMapping(id);
		zone *amap = FindAmbMapping(id);
		haptics_map *hmap = FindHapMapping(id);

		from_c = to_c = from_e = to_e = from_h = to_h = {0};

		if (mmap) {
			if (mmap->eve[0].fs.b.flags) {
				from_c = *Act2Code(&mmap->eve[0].map.front());
				to_c = *Act2Code(&mmap->eve[0].map.back());
				from_c.br = to_c.br = 255;
			} else {
				if (mmap->eve[3].fs.b.flags) {
					from_e = *Act2Code(&mmap->eve[3].map[0]);
					from_e.br = 255;
				}
				if (mmap->eve[2].fs.b.flags) {
					from_e = *Act2Code(&mmap->eve[2].map[0]);
					from_e.br = 255;
				}
			}
			if (mmap->eve[2].fs.b.flags) {
				to_e = *Act2Code(&mmap->eve[2].map[1]);
				to_e.br = 255;
			}
			if (mmap->eve[3].fs.b.flags) {
				to_e = *Act2Code(&mmap->eve[3].map[1]);
				to_e.br = 255;
			}
		}
		if (hmap) {
			from_h = hmap->freqs[0].colorfrom;
			to_h = hmap->freqs[0].colorto;
			from_h.br = to_h.br = 255;
		}
		CheckDlgButton(hDlg, IDC_CHECK_AMBIENT, amap != NULL);
	}
}

void SetFinalColor() {
	from_f = to_f = {0};
	int id = FindLastActiveID();
	if (id >= 0) {
		switch (conf->GetEffect()) {
		case 0: // monitoring
		{
			lightset *mmap = &conf->activeProfile->lightsets[id];
			if (mmap->eve[0].fs.b.flags) {
				from_f = to_f = *Act2Code(&mmap->eve[0].map.front());
				from_f.br = to_f.br = 255;
			} else {
				if (mmap->eve[3].fs.b.flags) {
					from_f = *Act2Code(&mmap->eve[3].map[0]); from_f.br = 255;
				}
				if (mmap->eve[2].fs.b.flags) {
					from_f = *Act2Code(&mmap->eve[2].map[0]); from_f.br = 255;
				}
			}
			if (mmap->eve[2].fs.b.flags) {
				to_f = *Act2Code(&mmap->eve[2].map[1]); to_f.br = 255;
			}
			if (mmap->eve[3].fs.b.flags) {
				to_f = *Act2Code(&mmap->eve[3].map[1]); from_f.br = 255;
			}
		} break;
		case 2: // haptics
		{
			haptics_map *hmap = &conf->hap_conf->haptics[id];
			from_f = hmap->freqs[0].colorfrom;
			to_f = hmap->freqs[0].colorto;
			from_f.br = to_f.br = 255;
		} break;
		case 3: // off
		{
			lightset *mmap = &conf->activeProfile->lightsets[id];
			if (mmap->eve[0].fs.b.flags) {
				from_f = *Act2Code(&mmap->eve[0].map.front());
				to_f = *Act2Code(&mmap->eve[0].map.back());
				from_f.br = to_f.br = 255;
			}
		} break;
		}
	}
}

BOOL CALLBACK LightInfoDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	HWND grpList = GetDlgItem(hDlg, IDC_GRPLIST);
	int devid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->devid : 0,
		lightid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->lightid : eItem;

	switch (message) {
	case WM_INITDIALOG:
	{
		lghGrp.clear();
		if (devid)
			ListBox_SetItemData(grpList, ListBox_AddString(grpList, "(Light)"), eItem);
		else
			ListBox_SetItemData(grpList, ListBox_AddString(grpList, fxhl->afx_dev.GetGroupById(eItem)->name.c_str()), eItem);
		ListBox_SetCurSel(grpList, 0);
		// check groups....
		for (auto Iter = fxhl->afx_dev.GetGroups()->begin(); devid && Iter < fxhl->afx_dev.GetGroups()->end(); Iter++)
			for (auto lIter = Iter->lights.begin(); lIter < Iter->lights.end(); lIter++)
				if ((*lIter)->devid == devid && (*lIter)->lightid == lightid) {
					lghGrp.push_back(&(*Iter));
					ListBox_SetItemData(grpList,ListBox_AddString(grpList, Iter->name.c_str()),Iter->gid);
				}
		SetLightColors(hDlg, eItem);
		SetFinalColor();
	} break;
	case WM_COMMAND:
	{
		int grpID = (int) ListBox_GetItemData(grpList, ListBox_GetCurSel(grpList));

		switch (LOWORD(wParam)) {
		case IDOK:
		{
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		} break;
		case IDC_GRPLIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				SetLightColors(hDlg, grpID);
				InvalidateRgn(hDlg, NULL, false);
				UpdateWindow(hDlg);
			}
			break;
		case IDC_REM_GROUP:
		{
			AlienFX_SDK::group *grp = fxhl->afx_dev.GetGroupById(grpID);
			if (grp && lightid >= 0) {
				for (auto Iter = grp->lights.begin(); Iter < grp->lights.end(); Iter++)
					if ((*Iter)->devid == devid && (*Iter)->lightid == lightid) {
						grp->lights.erase(Iter);
						ListBox_DeleteString(grpList, ListBox_GetCurSel(grpList));
						ListBox_SetCurSel(grpList, 0);
						SetLightColors(hDlg, eItem);
						SetFinalColor();
						InvalidateRgn(hDlg, NULL, false);
						UpdateWindow(hDlg);
						break;
					}
			}
		} break;
		case IDC_REM_COLOR:
			if (lightid >= 0) {
				for (auto Iter = conf->activeProfile->lightsets.begin(); Iter < conf->activeProfile->lightsets.end(); Iter++)
					if (Iter->devid == devid && Iter->lightid == lightid) {
						if (Iter->eve[1].fs.b.flags || Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)
							Iter->eve[0].fs.b.flags = false;
						else
							conf->activeProfile->lightsets.erase(Iter);
						SetLightColors(hDlg, eItem);
						SetFinalColor();
						InvalidateRgn(hDlg, NULL, false);
						UpdateWindow(hDlg);
						break;
					}
			}
			break;
		case IDC_REM_EFFECT:
			if (lightid >= 0) {
				for (auto Iter = conf->activeProfile->lightsets.begin(); Iter < conf->activeProfile->lightsets.end(); Iter++)
					if (Iter->devid == devid && Iter->lightid == lightid) {
						if (Iter->eve[0].fs.b.flags)
							Iter->eve[1].fs.b.flags = Iter->eve[2].fs.b.flags = Iter->eve[3].fs.b.flags = false;
						else
							conf->activeProfile->lightsets.erase(Iter);
						SetLightColors(hDlg, eItem);
						SetFinalColor();
						InvalidateRgn(hDlg, NULL, false);
						UpdateWindow(hDlg);
						break;
					}
			}
			break;
		case IDC_REM_AMBIENT:
			if (lightid >= 0) {
				for (auto Iter = conf->amb_conf->zones.begin(); Iter < conf->amb_conf->zones.end(); Iter++)
					if (Iter->devid == devid && Iter->lightid == lightid) {
						conf->amb_conf->zones.erase(Iter);
						SetLightColors(hDlg, eItem);
						SetFinalColor();
						InvalidateRgn(hDlg, NULL, false);
						UpdateWindow(hDlg);
						break;
					}
			}
			break;
		case IDC_REM_HAPTICS:
			if (lightid >= 0) {
				for (auto Iter = conf->hap_conf->haptics.begin(); Iter < conf->hap_conf->haptics.end(); Iter++)
					if (Iter->devid == devid && Iter->lightid == lightid) {
						conf->hap_conf->haptics.erase(Iter);
						SetLightColors(hDlg, eItem);
						SetFinalColor();
						InvalidateRgn(hDlg, NULL, false);
						UpdateWindow(hDlg);
						break;
					}
			}
			break;
		}
	} break;
	case WM_CLOSE:
		EndDialog(hDlg, IDOK);
		SwitchTab(TAB_COLOR);
		break;
	case WM_DRAWITEM:
	{
		AlienFX_SDK::Colorcode c{0};
		switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		case IDC_FROM_COLOR:
			c = from_c;
			break;
		case IDC_TO_COLOR:
			c = to_c;
			break;
		case IDC_FROM_EFFECT:
			c = from_e;
			break;
		case IDC_TO_EFFECT:
			c = to_e;
			break;
		case IDC_FROM_HAPTICS:
			c = from_h;
			break;
		case IDC_TO_HAPTICS:
			c = to_h;
			break;
		case IDC_FROM_FINAL:
			c = from_f;
			break;
		case IDC_TO_FINAL:
			c = to_f;
			break;
		}
		if (c.br)
			RedrawButton(hDlg, ((DRAWITEMSTRUCT *) lParam)->CtlID, c);
	} break;
	default: return false;
	}
	return true;
}