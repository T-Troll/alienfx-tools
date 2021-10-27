#include "alienfx-gui.h"

VOID OnSelChanged(HWND hwndDlg);
bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map);
lightset* CreateMapping(int lid);
lightset* FindMapping(int mid);
bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);
void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b);
HWND CreateToolTip(HWND hwndParent, HWND oldTip);
void SetSlider(HWND tt, int value);
int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);

extern int eItem;

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
	LVCOLUMNA lCol;
	lCol.mask = LVCF_WIDTH;
	lCol.cx = 100;
	ListView_DeleteColumn(eff_list, 0);
	ListView_InsertColumn(eff_list, 0, &lCol);
	if (mmap) {
		LVITEMA lItem{}; char efName[16] = {0};
		lItem.mask = LVIF_TEXT | LVIF_IMAGE;
		lItem.iSubItem = 0;
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
			}
			lItem.pszText = efName;
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);
		//ImageList_Destroy(hSmall);

		// Set selection...
		if (effID >= ListView_GetItemCount(eff_list))
			effID = ListView_GetItemCount(eff_list) - 1;
		if (effID != -1) {
			int dev_ver = fxhl->LocateDev(mmap->devid) ? fxhl->LocateDev(mmap->devid)->GetVersion() : -1;
			ListView_SetItemState(eff_list, effID, LVIS_SELECTED, LVIS_SELECTED);
			if (dev_ver > 0 && dev_ver < 5 || dev_ver == -1) {
				// Have hardware effects
				EnableWindow(type_c1, !(fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER));
				EnableWindow(s1_slider, true);
				EnableWindow(l1_slider, true);
			} else {
				// No hardware effects, color only
				EnableWindow(type_c1, false);
				EnableWindow(s1_slider, false);
				EnableWindow(l1_slider, false);
			}	
			// Set data
			ComboBox_SetCurSel(type_c1, mmap->eve[0].map[effID].type);
			RedrawButton(hDlg, IDC_BUTTON_C1, mmap->eve[0].map[effID].r, mmap->eve[0].map[effID].g, mmap->eve[0].map[effID].b);
			SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].tempo);
			SetSlider(sTip, mmap->eve[0].map[effID].tempo);
			SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].time);
			SetSlider(lTip, mmap->eve[0].map[effID].time);
		}
	}
	else {
		EnableWindow(type_c1, false);
		EnableWindow(s1_slider, false);
		EnableWindow(l1_slider, false);
		RedrawButton(hDlg, IDC_BUTTON_C1, 0, 0, 0);
	}
	ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE);// width);
	ListView_EnsureVisible(eff_list, effID, false);
}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1);

	lightset* mmap = FindMapping(eItem);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (UpdateLightList(light_list, fxhl) < 0) {
			// no lights, switch to setup
			HWND tab_list = GetParent(hDlg);
			TabCtrl_SetCurSel(tab_list, 6);
			OnSelChanged(tab_list);
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
		sTip = CreateToolTip(s1_slider, sTip);
		lTip = CreateToolTip(l1_slider, lTip);
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
				RebuildEffectList(hDlg, mmap);
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
				// remove mapping if no colors!
				if (fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER || mmap->eve[0].map.empty()) {
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
				if (mmap != NULL &&
					MessageBox(hDlg, "Do you really want to set all lights to this settings?", "Warning!",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					event light = mmap->eve[0];
					for (int i = 0; i < fxhl->afx_dev.GetMappings()->size(); i++) {
						AlienFX_SDK::mapping* map = fxhl->afx_dev.GetMappings()->at(i);
						if (!(map->flags && ALIENFX_FLAG_POWER)) {
							lightset* actmap = FindMapping(i);
							if (actmap)
								actmap->eve[0] = light;
							else {
								// create new mapping
								lightset* newmap = CreateMapping(i);
								newmap->eve[0] = light;
							}
						}
					}
					fxhl->RefreshState(true);
				}
			} break;
			} break;
		default: return false;
		}
	} break;
	case WM_VSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			//lightset* mmap = FindMapping(eItem);
			if (mmap != NULL) {
				if ((HWND)lParam == s1_slider) {
					mmap->eve[0].map[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip, mmap->eve[0].map[effID].tempo);
				}
				if ((HWND)lParam == l1_slider) {
					mmap->eve[0].map[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(lTip, mmap->eve[0].map[effID].time);
				}
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_C1:
			AlienFX_SDK::afx_act c = {0};
			//lightset* map = FindMapping(eItem);
			if (mmap && effID < mmap->eve[0].map.size()) {
				c = mmap->eve[0].map[effID];
			}
			RedrawButton(hDlg, IDC_BUTTON_C1, c.r, c.g, c.b);
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
						//lightset* mmap = FindMapping(eItem);
						if (mmap != NULL) {
							// Set data
							ComboBox_SetCurSel(type_c1, mmap->eve[0].map[effID].type);
							RedrawButton(hDlg, IDC_BUTTON_C1, mmap->eve[0].map[effID].r, mmap->eve[0].map[effID].g, mmap->eve[0].map[effID].b);
							SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].tempo);
							SetSlider(sTip, mmap->eve[0].map[effID].tempo);
							SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].time);
							SetSlider(lTip, mmap->eve[0].map[effID].time);
						}
					} else {
						effID = 0;
					}
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
