#include "alienfx-gui.h"

extern void SwitchTab(int);
extern bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern lightset* CreateMapping(int lid);
extern lightset* FindMapping(int mid);
extern bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);

extern int eItem;

extern zone* FindAmbMapping(int lid);
extern haptics_map* FindHapMapping(int lid);
extern void RemoveHapMapping(int devid, int lightid);
extern void RemoveAmbMapping(int devid, int lightid);
extern void RemoveLightFromGroup(AlienFX_SDK::group* grp, WORD devid, WORD lightid);

vector<AlienFX_SDK::group*> lghGrp;

int effID = -1;

void RebuildEffectList(HWND hDlg, lightset* mmap) {
	HWND eff_list = GetDlgItem(hDlg, IDC_EFFECTS_LIST),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1);
	ListView_DeleteAllItems(eff_list);
	HIMAGELIST hOld = ListView_GetImageList(eff_list, LVSIL_SMALL);
	if (hOld) {
		ImageList_Destroy(hOld);
	}
	ListView_SetExtendedListViewStyle(eff_list, LVS_EX_FULLROWSELECT);
	if (!ListView_GetColumnWidth(eff_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(eff_list, 0, &lCol);
	}
	if (mmap) {
		LVITEMA lItem{ LVIF_TEXT | LVIF_IMAGE };
		char efName[16]{0};

		COLORREF* picData = NULL;
		HBITMAP colorBox = NULL;
		HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
								  GetSystemMetrics(SM_CYSMICON),
								  ILC_COLOR32, 1, 1);
		for (int i = 0; i < mmap->eve[0].map.size(); i++) {
			picData = new COLORREF[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON)];
			fill_n(picData, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON), RGB(mmap->eve[0].map[i].b, mmap->eve[0].map[i].g, mmap->eve[0].map[i].r));
			colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
									1, 32, picData);
			delete[] picData;
			ImageList_Add(hSmall, colorBox, NULL);
			DeleteObject(colorBox);
			lItem.iItem = i;
			lItem.iImage = i;
			switch (mmap->eve[0].map[i].type) {
			case AlienFX_SDK::AlienFX_A_Color:
				LoadString(hInst, IDS_TYPE_COLOR, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Pulse:
				LoadString(hInst, IDS_TYPE_PULSE, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Morph:
				LoadString(hInst, IDS_TYPE_MORPH, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Spectrum:
				LoadString(hInst, IDS_TYPE_SPECTRUM, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Breathing:
				LoadString(hInst, IDS_TYPE_BREATH, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Rainbow:
				LoadString(hInst, IDS_TYPE_RAINBOW, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Power:
				LoadString(hInst, IDS_TYPE_POWER, efName, 16);
				break;
			}
			lItem.pszText = efName;
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);

		// Set selection...
		if (effID >= ListView_GetItemCount(eff_list))
			effID = ListView_GetItemCount(eff_list) - 1;
		if (effID != -1) {
			int dev_ver = fxhl->LocateDev(mmap->devid) ? fxhl->LocateDev(mmap->devid)->dev->GetVersion() : -1;
			ListView_SetItemState(eff_list, effID, LVIS_SELECTED, LVIS_SELECTED);
			switch (dev_ver) {
			case -1: case 1: case 2: case 3: case 4: case 7:
				// Have hardware effects
				EnableWindow(type_c1, !(fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER));
				EnableWindow(s1_slider, true);
				EnableWindow(l1_slider, true);
				break;
			default:
				// No hardware effects, color only
				EnableWindow(type_c1, false);
				EnableWindow(s1_slider, false);
				EnableWindow(l1_slider, false);
			}
			// Set data
			ComboBox_SetCurSel(type_c1, mmap->eve[0].map[effID].type);
			RedrawButton(hDlg, IDC_BUTTON_C1, Act2Code(&mmap->eve[0].map[effID]));
			SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].tempo);
			SetSlider(sTip1, mmap->eve[0].map[effID].tempo);
			SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].time);
			SetSlider(sTip2, mmap->eve[0].map[effID].time);
		}
	}
	else {
		EnableWindow(type_c1, false);
		EnableWindow(s1_slider, false);
		EnableWindow(l1_slider, false);
		RedrawButton(hDlg, IDC_BUTTON_C1, {0});
	}
	ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE);// width);
	ListView_EnsureVisible(eff_list, effID, false);
}

int FindLastActiveID() {

	int devid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->devid : 0,
		lightid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->lightid : eItem;

	switch (conf->GetEffect()) {
	case 0: case 3:
	{
		for (int iter = (int)conf->activeProfile->lightsets.size() - 1; iter >= 0; iter--) {
			lightset* Iter = &conf->activeProfile->lightsets[iter];
			if (Iter->devid || !devid) {
				if (Iter->devid == devid && Iter->lightid == lightid) {
					return iter;
				}
			}
			else {
				// check groups...
				for (auto gIter = lghGrp.begin(); gIter < lghGrp.end(); gIter++)
					if (Iter->lightid == (*gIter)->gid && (conf->GetEffect() != 3 || Iter->flags & LEVENT_COLOR)) {
						return iter;
					}
			}
		}
	} break;
	case 1:
	{
		for (int iter = (int)conf->amb_conf->zones.size() - 1; iter >= 0; iter--) {
			zone* Iter = &conf->amb_conf->zones[iter];
			if (Iter->devid || !devid) {
				if (Iter->devid == devid && Iter->lightid == lightid) {
					return iter;
				}
			}
			else {
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
		for (int iter = (int)conf->hap_conf->haptics.size() - 1; iter >= 0; iter--) {
			haptics_map* Iter = &conf->hap_conf->haptics[iter];
			if (Iter->devid || !devid) {
				if (Iter->devid == devid && Iter->lightid == lightid) {
					return iter;
				}
			}
			else {
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
		lightset* mmap = FindMapping(id);
		zone* amap = FindAmbMapping(id);
		haptics_map* hmap = FindHapMapping(id);

		AlienFX_SDK::Colorcode* from_c, * to_c, * from_e, * to_e, * from_h, * to_h, * from_f, * to_f;
		from_c = to_c = from_e = to_e = from_h = to_h = from_f = to_f = { NULL };

		if (mmap) {
			if (mmap->flags & LEVENT_ACT) {
				from_e = Act2Code(&mmap->eve[3].map[0]);
				to_e = Act2Code(&mmap->eve[3].map[1]);
			}
			else
				if (mmap->flags & LEVENT_PERF) {
					from_e = Act2Code(&mmap->eve[2].map[0]);
					to_e = Act2Code(&mmap->eve[2].map[1]);
				} else
					if (mmap->flags & LEVENT_POWER) {
						from_e = Act2Code(&mmap->eve[1].map[0]);
						to_e = Act2Code(&mmap->eve[1].map[1]);
					}
			if (mmap->flags & LEVENT_COLOR) {
				from_c = Act2Code(&mmap->eve[0].map.front());
				to_c = Act2Code(&mmap->eve[0].map.back());
				if (from_e)
					from_e = from_c;
			}
		}
		if (hmap) {
			from_h = &hmap->freqs[0].colorfrom;
			to_h = &hmap->freqs[0].colorto;
		}
		RedrawButton(hDlg, IDC_FROM_COLOR, from_c);
		RedrawButton(hDlg, IDC_TO_COLOR, to_c);
		RedrawButton(hDlg, IDC_FROM_EFFECT, from_e);
		RedrawButton(hDlg, IDC_TO_EFFECT, to_e);
		RedrawButton(hDlg, IDC_FROM_HAPTICS, from_h);
		RedrawButton(hDlg, IDC_TO_HAPTICS, to_h);
		CheckDlgButton(hDlg, IDC_CHECK_AMBIENT, amap != NULL);

		int idf = FindLastActiveID();
		if (idf >= 0) {
			switch (conf->GetEffect()) {
			case 0: // monitoring
			{
				mmap = &conf->activeProfile->lightsets[idf];
				if (mmap->flags & LEVENT_ACT) {
					from_f = Act2Code(&mmap->eve[3].map[0]);
					to_f = Act2Code(&mmap->eve[3].map[1]);
				}
				else
					if (mmap->flags & LEVENT_PERF) {
						from_f = Act2Code(&mmap->eve[2].map[0]);
						to_f = Act2Code(&mmap->eve[2].map[1]);
					} else
						if (mmap->flags & LEVENT_POWER) {
							from_f = Act2Code(&mmap->eve[1].map[0]);
							to_f = Act2Code(&mmap->eve[1].map[1]);
						}
				if (mmap->flags & LEVENT_COLOR) {
					if (!from_f)
						to_f = Act2Code(&mmap->eve[0].map.back());
					from_f = Act2Code(&mmap->eve[0].map.front());
				}
			} break;
			case 2: // haptics
			{
				hmap = &conf->hap_conf->haptics[idf];
				from_f = &hmap->freqs[0].colorfrom;
				to_f = &hmap->freqs[0].colorto;
			} break;
			case 3: // off
			{
				mmap = &conf->activeProfile->lightsets[idf];
				if (mmap->flags & LEVENT_COLOR) {
					to_f = Act2Code(&mmap->eve[0].map.back());
					from_f = Act2Code(&mmap->eve[0].map.front());
				}
			} break;
			}
		}

		RedrawButton(hDlg, IDC_FROM_FINAL, from_f);
		RedrawButton(hDlg, IDC_TO_FINAL, to_f);
	}
}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1),
		grpList = GetDlgItem(hDlg, IDC_GRPLIST);

	lightset* mmap = eItem < 0 ? NULL: FindMapping(eItem);

	int devid = eItem >= 0 && eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->devid : 0,
		lightid = eItem >= 0 && eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->lightid : eItem;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (UpdateLightList(light_list, fxhl) < 0) {
			// no lights, switch to setup
			SwitchTab(TAB_DEVICES);
			return false;
		}
		// Set types list...
		char buffer[100];
		LoadString(hInst, IDS_TYPE_COLOR, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_PULSE, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_MORPH, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_BREATH, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_SPECTRUM, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		LoadString(hInst, IDS_TYPE_RAINBOW, buffer, 100);
		ComboBox_AddString(type_c1, buffer);
		// now sliders...
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(l1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		//TBM_SETTICFREQ
		SendMessage(s1_slider, TBM_SETTICFREQ, 32, 0);
		SendMessage(l1_slider, TBM_SETTICFREQ, 32, 0);
		sTip1 = CreateToolTip(s1_slider, sTip1);
		sTip2 = CreateToolTip(l1_slider, sTip2);

		// Restore selection....
		if (eItem >= 0) {
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
		}
	} break;
	case WM_COMMAND: {
		int lid = (int) ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
		switch (LOWORD(wParam))
		{
		case IDC_LIGHTS:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				eItem = lid;// lbItem;
				effID = 0;
				mmap = FindMapping(lid);
				if (mmap) {
					AlienFX_SDK::mapping *map = fxhl->afx_dev.GetMappingById(mmap->devid, mmap->lightid);
					if (map && (map->flags & ALIENFX_FLAG_POWER)) {
						mmap->eve[0].map[0].type = mmap->eve[0].map[1].type = AlienFX_SDK::AlienFX_A_Power;
						if (!mmap->eve[0].map[0].time)
							mmap->eve[0].map[0].time = mmap->eve[0].map[1].time = 3;
						if (!mmap->eve[0].map[0].tempo)
							mmap->eve[0].map[0].tempo = mmap->eve[0].map[1].tempo = 0x64;
					}
				}
				RebuildEffectList(hDlg, mmap);

				// update light info...
				devid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->devid : 0;
				lightid = eItem < 0x10000 ? fxhl->afx_dev.GetMappings()->at(eItem)->lightid : eItem;
				lghGrp.clear();
				ListBox_ResetContent(grpList);
				if (devid) {
					ListBox_SetItemData(grpList, ListBox_AddString(grpList, "(Light)"), eItem);
					for (auto Iter = fxhl->afx_dev.GetGroups()->begin(); Iter < fxhl->afx_dev.GetGroups()->end(); Iter++)
						for (auto lIter = Iter->lights.begin(); lIter < Iter->lights.end(); lIter++)
							if ((*lIter)->devid == devid && (*lIter)->lightid == lightid) {
								lghGrp.push_back(&(*Iter));
								ListBox_SetItemData(grpList, ListBox_AddString(grpList, Iter->name.c_str()), Iter->gid);
							}
				}
				else {
					ListBox_SetItemData(grpList, ListBox_AddString(grpList, fxhl->afx_dev.GetGroupById(eItem)->name.c_str()), eItem);
				}
				ListBox_SetCurSel(grpList, 0);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_GROUP), false);
				SetLightColors(hDlg, eItem);

				break;
			} break;
		case IDC_TYPE1:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				if (mmap != NULL) {
					int lType1 = (int)ComboBox_GetCurSel(type_c1);
					mmap->eve[0].map[effID].type = lType1;
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, true, true);
				}
			}
			break;
		case IDC_BUTTON_C1:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				if (lid != -1) {
					if (!mmap) {
						// have selection, but no effect - let's create one!
						mmap = CreateMapping(lid);
						effID = 0;
					}
					SetColor(hDlg, IDC_BUTTON_C1, mmap, &mmap->eve[0].map[effID]);
					RebuildEffectList(hDlg, mmap);
				}
			} break;
			} break;
		case IDC_BUT_ADD_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED) {
				if (!mmap) {
					// create new mapping..
					mmap = CreateMapping(lid);
					effID = 0;
				} else
					if (!(fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER) && mmap->eve[0].map.size() < 9) {
						AlienFX_SDK::afx_act act = mmap->eve[0].map[effID];
						mmap->eve[0].map.push_back(act);
						effID = (int)mmap->eve[0].map.size() - 1;
					}
				RebuildEffectList(hDlg, mmap);
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		case IDC_BUTT_REMOVE_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED && mmap) {
				mmap->eve[0].map.pop_back();
				if (effID == mmap->eve[0].map.size())
					effID--;
				// remove mapping if no colors and effects!
				if (fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER || mmap->eve[0].map.empty()) {
					//if (mmap->eve[1].fs.flags || mmap->eve[2].fs.flags || mmap->eve[3].fs.flags)
					//	mmap->eve[0].fs.flags = false;
					//else
						RemoveMapping(conf->active_set, mmap->devid, mmap->lightid);
					RebuildEffectList(hDlg, NULL);
					effID = -1;
				} else {
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, true, true);
				}
			}
			break;
		case IDC_BUTTON_SETALL:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				if (mmap && mmap->eve[0].map[0].type != AlienFX_SDK::AlienFX_A_Power &&
					MessageBox(hDlg, "Do you really want to set all lights to this settings?", "Warning",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					event light = mmap->eve[0];
					for (int i = 0; i < fxhl->afx_dev.GetMappings()->size(); i++) {
						AlienFX_SDK::mapping* map = fxhl->afx_dev.GetMappings()->at(i);
						if (!(map->flags & ALIENFX_FLAG_POWER)) {
							lightset* actmap = FindMapping(i);
							if (!actmap) {
								// create new mapping
								actmap = CreateMapping(i);
							}
							actmap->eve[0] = light;
						}
					}
					fxhl->RefreshState(true);
				}
			} break;
			} break;
		// info block controls.
		case IDC_GRPLIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				int selGrp = ListBox_GetCurSel(grpList);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_GROUP), selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_COLOR), !selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_EFFECT), !selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_AMBIENT), !selGrp);
				EnableWindow(GetDlgItem(hDlg, IDC_REM_HAPTICS), !selGrp);
				SetLightColors(hDlg, (int)ListBox_GetItemData(grpList, selGrp));
			}
			break;
		case IDC_REM_GROUP:
		{
			AlienFX_SDK::group* grp = fxhl->afx_dev.GetGroupById((DWORD)ListBox_GetItemData(grpList, ListBox_GetCurSel(grpList)));
			if (grp && devid) {
				ListBox_DeleteString(grpList, ListBox_GetCurSel(grpList));
				ListBox_SetCurSel(grpList, 0);
				RemoveLightFromGroup(grp, devid, lightid);
				SetLightColors(hDlg, eItem);
				fxhl->RefreshState(true);
			}
		} break;
		case IDC_REM_COLOR:
			if (mmap) {
				if (mmap->flags & ~LEVENT_COLOR)
					mmap->flags &= ~LEVENT_COLOR;
				else
					RemoveMapping(conf->active_set, mmap->devid, mmap->lightid);
				SetLightColors(hDlg, eItem);
				fxhl->RefreshState(true);
			}
			break;
		case IDC_REM_EFFECT:
			if (mmap) {
				if (mmap->flags & LEVENT_COLOR)
					mmap->flags |= LEVENT_COLOR;
				else
					RemoveMapping(conf->active_set, mmap->devid, mmap->lightid);
				SetLightColors(hDlg, eItem);
				fxhl->RefreshState(true);
			}
			break;
		case IDC_REM_AMBIENT:
			if (lightid >= 0) {
				RemoveAmbMapping(devid, lightid);
				SetLightColors(hDlg, eItem);
			}
			break;
		case IDC_REM_HAPTICS:
			if (lightid >= 0) {
				RemoveHapMapping(devid, lightid);
				SetLightColors(hDlg, eItem);
			}
			break;
		default: return false;
		}
	} break;
	case WM_VSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (mmap != NULL) {
				if ((HWND)lParam == s1_slider) {
					mmap->eve[0].map[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, mmap->eve[0].map[effID].tempo);
				}
				if ((HWND)lParam == l1_slider) {
					mmap->eve[0].map[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, mmap->eve[0].map[effID].time);
				}
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_C1: {
			AlienFX_SDK::Colorcode* c{ NULL };
			if (mmap && effID < mmap->eve[0].map.size()) {
				c = Act2Code(&mmap->eve[0].map[effID]);
			}
			RedrawButton(hDlg, IDC_BUTTON_C1, c);
		} break;
		case IDC_TO_FINAL:
			SetLightColors(hDlg, eItem);
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_EFFECTS_LIST:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & LVIS_FOCUSED) {
					// Select other item...
					if (lPoint->iItem != -1) {
						// Select other color....
						effID = lPoint->iItem;
						if (mmap) {
							// Set data
							ComboBox_SetCurSel(type_c1, mmap->eve[0].map[effID].type);
							RedrawButton(hDlg, IDC_BUTTON_C1, Act2Code(&mmap->eve[0].map[effID]));
							SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].tempo);
							SetSlider(sTip1, mmap->eve[0].map[effID].tempo);
							SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].time);
							SetSlider(sTip2, mmap->eve[0].map[effID].time);
						}
					} else {
						effID = 0;
					}
				}
			} break;
			case NM_DBLCLK:
			{
				// change color.
				if (eItem != -1) {
					if (!mmap) {
						// have selection, but no effect - let's create one!
						mmap = CreateMapping(eItem);
						effID = 0;
					}
					SetColor(hDlg, IDC_BUTTON_C1, mmap, &mmap->eve[0].map[effID]);
					RebuildEffectList(hDlg, mmap);
				}
			} break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}
