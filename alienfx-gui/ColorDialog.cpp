#include "alienfx-gui.h"

extern void SwitchTab(int);
extern bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern lightset* CreateMapping(int lid);
extern lightset* FindMapping(int mid);
extern bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);

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
	LVCOLUMNA lCol{0};
	lCol.mask = LVCF_WIDTH;
	lCol.cx = 100;
	ListView_DeleteColumn(eff_list, 0);
	ListView_InsertColumn(eff_list, 0, &lCol);
	if (mmap) {
		LVITEMA lItem{}; char efName[16]{0};
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
			RedrawButton(hDlg, IDC_BUTTON_C1, *Act2Code(&mmap->eve[0].map[effID]));
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

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1);

	lightset* mmap = eItem < 0 ? NULL: FindMapping(eItem);

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
					// check wrong PB data...
					AlienFX_SDK::mapping *map = fxhl->afx_dev.GetMappingById(mmap->devid, mmap->lightid);
					if (map && map->flags & ALIENFX_FLAG_POWER) {
						mmap->eve[0].map[0].type = mmap->eve[0].map[1].type = AlienFX_SDK::AlienFX_A_Power;
						if (!mmap->eve[0].map[0].time)
							mmap->eve[0].map[0].time = mmap->eve[0].map[1].time = 3;
						if (!mmap->eve[0].map[0].tempo)
							mmap->eve[0].map[0].tempo = mmap->eve[0].map[1].tempo = 0x64;
					}
				}
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
				if (mmap != NULL && mmap->eve[0].map[0].type != AlienFX_SDK::AlienFX_A_Power &&
					MessageBox(hDlg, "Do you really want to set all lights to this settings?", "Warning!",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					event light = mmap->eve[0];
					for (int i = 0; i < fxhl->afx_dev.GetMappings()->size(); i++) {
						AlienFX_SDK::mapping* map = fxhl->afx_dev.GetMappings()->at(i);
						if (!(map->flags & ALIENFX_FLAG_POWER)) {
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
		case IDC_BUTTON_C1:
			AlienFX_SDK::Colorcode c{0};
			if (mmap && effID < mmap->eve[0].map.size()) {
				c = *Act2Code(&mmap->eve[0].map[effID]);
			}
			RedrawButton(hDlg, IDC_BUTTON_C1, c);
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
							RedrawButton(hDlg, IDC_BUTTON_C1, *Act2Code(&mmap->eve[0].map[effID]));
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
