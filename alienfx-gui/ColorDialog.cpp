#include "alienfx-gui.h"

extern void SwitchLightTab(HWND, int);
extern bool SetColor(HWND hDlg, int id, groupset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern groupset* CreateMapping(int lid);
extern groupset* FindMapping(int mid);
extern void RemoveUnused(vector<groupset>* lightsets);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
//extern int UpdateZoneList(HWND hDlg, byte flag = 0);

extern BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern int eItem;

//extern zone* FindAmbMapping(int lid);
//extern haptics_map* FindHapMapping(int lid);
//extern void RemoveHapMapping(int devid, int lightid);
//extern void RemoveAmbMapping(int devid, int lightid);
//extern void RemoveLightFromGroup(AlienFX_SDK::group* grp, WORD devid, WORD lightid);

extern BOOL CALLBACK TabColorGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void CreateGridBlock(HWND gridTab, DLGPROC);
extern void OnGridSelChanged(HWND);
extern AlienFX_SDK::mapping* FindCreateMapping();
extern void RedrawGridButtonZone(bool recalc = false);

extern int gridTabSel;
extern AlienFX_SDK::lightgrid* mainGrid;

//vector<AlienFX_SDK::group*> lghGrp;

int effID = -1;

HWND zsDlg = NULL;

void SetEffectData(HWND hDlg, groupset* mmap) {
	bool hasEffects = false;
	if (mmap && mmap->color.size()) {
		//switch (fxhl->LocateDev(mmap->lights[0]->devid) ? fxhl->LocateDev(mmap->lights[0]->devid)->dev->GetVersion() : -1) {
		//case -1: case 1: case 2: case 3: case 4: case 7:
		//	// Have hardware effects
			hasEffects = true;
		//	break;
		//}
		// Set data
		ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_TYPE1), mmap->color[effID].type);
		SendMessage(GetDlgItem(hDlg, IDC_SPEED1), TBM_SETPOS, true, mmap->color[effID].tempo);
		SetSlider(sTip1, mmap->color[effID].tempo);
		SendMessage(GetDlgItem(hDlg, IDC_LENGTH1), TBM_SETPOS, true, mmap->color[effID].time);
		SetSlider(sTip2, mmap->color[effID].time);
	}
	EnableWindow(GetDlgItem(hDlg, IDC_TYPE1), hasEffects && !mmap->group->have_power);
	EnableWindow(GetDlgItem(hDlg, IDC_SPEED1), hasEffects);
	EnableWindow(GetDlgItem(hDlg, IDC_LENGTH1), hasEffects);
	RedrawButton(hDlg, IDC_BUTTON_C1, mmap && mmap->color.size() ? Act2Code(&mmap->color[effID]) : 0);
}

void RebuildEffectList(HWND hDlg, groupset* mmap) {
	HWND eff_list = GetDlgItem(hDlg, IDC_LEFFECTS_LIST),
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
		COLORREF* picData = NULL;
		HBITMAP colorBox = NULL;
		HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
								  GetSystemMetrics(SM_CYSMICON),
								  ILC_COLOR32, 1, 1);
		for (int i = 0; i < mmap->color.size(); i++) {
			LVITEMA lItem{ LVIF_TEXT | LVIF_IMAGE | LVIF_STATE, i };
			char efName[16]{ 0 };
			picData = new COLORREF[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON)];
			fill_n(picData, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON), RGB(mmap->color[i].b, mmap->color[i].g, mmap->color[i].r));
			colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 32, picData);
			delete[] picData;
			ImageList_Add(hSmall, colorBox, NULL);
			DeleteObject(colorBox);
			lItem.iImage = i;
			switch (mmap->color[i].type) {
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
			// check selection...
			if (i == effID) {
				lItem.state = LVIS_SELECTED;
			}
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);
	}
	SetEffectData(hDlg, mmap);
	ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE);// width);
	ListView_EnsureVisible(eff_list, effID, false);
}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND //light_list = GetDlgItem(hDlg, IDC_LIGHTS),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1),
		grpList = GetDlgItem(hDlg, IDC_GRPLIST),
		gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID);

	groupset* mmap = FindMapping(eItem);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		//if (!UpdateZoneList(hDlg)) {
		//	// no lights, switch to setup
		//	SwitchLightTab(hDlg, TAB_DEVICES);
		//	return false;
		//}
		zsDlg = CreateDialog(hInst, (LPSTR)IDD_ZONESELECTION, hDlg, (DLGPROC)ZoneSelectionDialog);
		RECT mRect;
		GetWindowRect(GetDlgItem(hDlg, IDC_STATIC_ZONES), &mRect);
		ScreenToClient(hDlg, (LPPOINT)&mRect);
		SetWindowPos(zsDlg, NULL, mRect.left, mRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);

		if (!conf->afx_dev.GetMappings()->size())
			OnGridSelChanged(gridTab);

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

		// init grids...
		CreateGridBlock(gridTab, (DLGPROC)TabColorGrid);
		TabCtrl_SetCurSel(gridTab, gridTabSel);
		OnGridSelChanged(gridTab);

		// Restore selection....
		//if (eItem >= 0) {
		//	SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
		//}

	} break;
	case WM_APP + 2: {
		mmap = FindMapping(eItem);
		RebuildEffectList(hDlg, mmap);
		RedrawGridButtonZone();
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_TYPE1:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				if (mmap != NULL) {
					int lType1 = (int)ComboBox_GetCurSel(type_c1);
					mmap->color[effID].type = lType1;
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, true, true);
				}
			}
			break;
		case IDC_BUTTON_C1:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				if (eItem > 0) {
					if (!mmap) {
						// have selection, but no effect - let's create one!
						mmap = CreateMapping(eItem);
						effID = 0;
					}
					SetColor(hDlg, IDC_BUTTON_C1, mmap, &mmap->color[effID]);
					RebuildEffectList(hDlg, mmap);
					//SetLightColors(hDlg, eItem);
				}
			} break;
			} break;
		case IDC_BUT_ADD_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED) {
				AlienFX_SDK::afx_act act{ 0 };
				if (!mmap) {
					// create new mapping..
					mmap = CreateMapping(eItem);
					if (mmap->color.size() && mmap->group->have_power) {
						act.type = AlienFX_SDK::AlienFX_A_Power;
						act.time = 3;
						act.tempo = 0x64;
						mmap->color.push_back(act);
						mmap->color.push_back(act);
					} else
						mmap->color.push_back(act);
					effID = 0;
				} else
					if (!mmap->group->have_power && mmap->color.size() < 9) {
						act = mmap->color[effID];
						mmap->color.push_back(act);
						effID = (int)mmap->color.size() - 1;
					}
				RebuildEffectList(hDlg, mmap);
				//SetLightColors(hDlg, eItem);
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		case IDC_BUTT_REMOVE_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED && mmap) {
				if (mmap->group->have_power)
					mmap->color.clear();
				else {
					mmap->color.pop_back();
					if (effID == mmap->color.size())
						effID--;
				}
				// remove mapping if no colors and effects!
				if (mmap->color.empty()) {
					RemoveUnused(conf->active_set);
					RebuildEffectList(hDlg, NULL);
					effID = -1;
					fxhl->Refresh();
				} else {
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, false, true);
				}
				//SetLightColors(hDlg, eItem);
			}
			break;
		//case IDC_BUTTON_SETALL:
		//	switch (HIWORD(wParam))
		//	{
		//	case BN_CLICKED: {
		//		if (mmap && mmap->eve[0].map[0].type != AlienFX_SDK::AlienFX_A_Power &&
		//			MessageBox(hDlg, "Do you really want to set all lights to this settings?", "Warning",
		//					   MB_YESNO | MB_ICONWARNING) == IDYES) {
		//			event light = mmap->eve[0];
		//			for (int i = 0; i < conf->afx_dev.GetMappings()->size(); i++) {
		//				AlienFX_SDK::mapping* map = conf->afx_dev.GetMappings()->at(i);
		//				if (!(map->flags & ALIENFX_FLAG_POWER)) {
		//					lightset* actmap = FindMapping(i);
		//					if (!actmap) {
		//						// create new mapping
		//						actmap = CreateMapping(i);
		//					}
		//					actmap->eve[0] = light;
		//				}
		//			}
		//			fxhl->RefreshState(true);
		//		}
		//	} break;
		//	} break;
		// info block controls.
		default: return false;
		}
	} break;
	case WM_VSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (mmap != NULL) {
				if ((HWND)lParam == s1_slider) {
					mmap->color[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, mmap->color[effID].tempo);
				}
				if ((HWND)lParam == l1_slider) {
					mmap->color[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, mmap->color[effID].time);
				}
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_C1: {
			AlienFX_SDK::Colorcode* c{ NULL };
			if (mmap && effID < mmap->color.size()) {
				c = Act2Code(&mmap->color[effID]);
			}
			RedrawButton(hDlg, IDC_BUTTON_C1, c);
		} break;
		//case IDC_TO_FINAL:
		//	SetLightColors(hDlg, eItem);
		//	break;
		default: return false;
		}
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_COLOR_GRID: {
			switch (((NMHDR*)lParam)->code) {
			case TCN_SELCHANGE: {
				int newSel = TabCtrl_GetCurSel(gridTab);
				if (newSel != gridTabSel) { // selection changed!
					if (newSel < conf->afx_dev.GetGrids()->size())
						OnGridSelChanged(gridTab);
				}
			} break;
			}
		} break;
		case IDC_LEFFECTS_LIST:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & LVIS_FOCUSED) {
					// Select other item...
					effID = lPoint->iItem != -1 ? effID = lPoint->iItem : 0;
					SetEffectData(hDlg, mmap);
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
					SetColor(hDlg, IDC_BUTTON_C1, mmap, &mmap->color[effID]);
					RebuildEffectList(hDlg, mmap);
				}
			} break;
			}
			break;
		}
		break;
	case WM_DESTROY:
		//EndDialog(zsDlg, IDOK);
		DestroyWindow(zsDlg);
		break;
	default: return false;
	}
	return true;
}
