#include "alienfx-gui.h"
#include "common.h"

extern bool SetColor(HWND ctrl, groupset* mmap, AlienFX_SDK::Afx_action* map);
extern AlienFX_SDK::Afx_colorcode *Act2Code(AlienFX_SDK::Afx_action*);
//extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void RedrawButton(HWND ctrl, AlienFX_SDK::Afx_colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern FXHelper* fxhl;

//extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);
extern void RedrawZoneGrid(DWORD grpid);

int effID = 0;

const static vector<string> lightEffectNames{ "Color", "Pulse", "Morph", "Breath", "Spectrum", "Rainbow" };

void SetEffectData(HWND hDlg, groupset* mmap) {
	bool hasEffects = false;
	if (mmap) {
		if (mmap->color.size()) {
			hasEffects = true;
			ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_TYPE1), mmap->color[effID].type);
			SendMessage(GetDlgItem(hDlg, IDC_SPEED1), TBM_SETPOS, true, mmap->color[effID].tempo);
			SetSlider(sTip1, mmap->color[effID].tempo);
			SendMessage(GetDlgItem(hDlg, IDC_LENGTH1), TBM_SETPOS, true, mmap->color[effID].time);
			SetSlider(sTip2, mmap->color[effID].time);
		}
	}
	EnableWindow(GetDlgItem(hDlg, IDC_TYPE1), hasEffects && !conf->afx_dev.GetGroupById(mmap->group)->have_power);
	EnableWindow(GetDlgItem(hDlg, IDC_SPEED1), hasEffects);
	EnableWindow(GetDlgItem(hDlg, IDC_LENGTH1), hasEffects);
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_C1), mmap && mmap->color.size() ? Act2Code(&mmap->color[effID]) : 0);
}

void RebuildEffectList(HWND hDlg, groupset* mmap) {
	HWND eff_list = GetDlgItem(hDlg, IDC_LEFFECTS_LIST);

	ListView_DeleteAllItems(eff_list);
	ListView_SetExtendedListViewStyle(eff_list, LVS_EX_FULLROWSELECT);

	HIMAGELIST hOld = ListView_GetImageList(eff_list, LVSIL_SMALL);
	if (hOld) {
		ImageList_Destroy(hOld);
	}

	if (!ListView_GetColumnWidth(eff_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(eff_list, 0, &lCol);
		ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE_USEHEADER);// width);
	}
	if (mmap) {
		COLORREF* picData = NULL;
		HBITMAP colorBox = NULL;
		HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
								  GetSystemMetrics(SM_CYSMICON),
								  ILC_COLOR32, 1, 1);
		for (int i = 0; i < mmap->color.size(); i++) {
			LVITEMA lItem{ LVIF_TEXT | LVIF_IMAGE | LVIF_STATE, i };
			//char efName[16]{ 0 };
			picData = new COLORREF[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON)];
			fill_n(picData, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON), RGB(mmap->color[i].b, mmap->color[i].g, mmap->color[i].r));
			colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 32, picData);
			delete[] picData;
			ImageList_Add(hSmall, colorBox, NULL);
			DeleteObject(colorBox);
			lItem.iImage = i;
			lItem.pszText = mmap->color[i].type != 6 ? (LPSTR)lightEffectNames[mmap->color[i].type].c_str() : "Power";
			// check selection...
			if (i == effID) {
				lItem.state = LVIS_SELECTED;
			}
			else
				lItem.state = 0;
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);
		fxhl->RefreshOne(mmap);
	}
	SetEffectData(hDlg, mmap);
	ListView_EnsureVisible(eff_list, effID, false);
	//RedrawGridButtonZone(NULL, true);
}

void ChangeAddColor(HWND hDlg, groupset* mmap, int newEffID) {
	// change color.
	if (mmap) {
		if (newEffID < mmap->color.size())
			SetColor(GetDlgItem(hDlg, IDC_BUTTON_C1), mmap, &mmap->color[newEffID]);
		else {
			AlienFX_SDK::Afx_action act{ 0 };
			// add new effect
			if (conf->afx_dev.GetGroupById(mmap->group)->have_power) {
				if (mmap->color.empty()) {
					act = { AlienFX_SDK::AlienFX_A_Power, 3, 0x64 };
					mmap->color.push_back(act);
					mmap->color.push_back(act);
					newEffID = 0;
				}
			} else
				if (mmap->color.size() < 9) {
					if (effID < mmap->color.size())
						act = mmap->color[effID];
					mmap->color.push_back(act);
					newEffID = (int)mmap->color.size() - 1;
				}
			if (newEffID < mmap->color.size())
				if (SetColor(GetDlgItem(hDlg, IDC_BUTTON_C1), mmap, &mmap->color[newEffID]))
					effID = newEffID;
				else {
					if (conf->afx_dev.GetGroupById(mmap->group)->have_power)
						mmap->color.clear();
					else
						mmap->color.erase(mmap->color.begin() + newEffID);
				}
		}
		RebuildEffectList(hDlg, mmap);
		RedrawZoneGrid(mmap->group);
	}
}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1);

	groupset* mmap = conf->FindMapping(eItem);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Set types and gauge list...
		UpdateCombo(GetDlgItem(hDlg, IDC_TYPE1), lightEffectNames);
		// now sliders...
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(l1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		//TBM_SETTICFREQ
		SendMessage(s1_slider, TBM_SETTICFREQ, 32, 0);
		SendMessage(l1_slider, TBM_SETTICFREQ, 32, 0);
		sTip1 = CreateToolTip(s1_slider, sTip1);
		sTip2 = CreateToolTip(l1_slider, sTip2);
		RebuildEffectList(hDlg, mmap);
	} break;
	case WM_APP + 2: {
		effID = 0;
		RebuildEffectList(hDlg, mmap);
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_TYPE1:
			if (HIWORD(wParam) == CBN_SELCHANGE && mmap) {
				int lType1 = (int)ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_TYPE1));
				mmap->color[effID].type = lType1;
				RebuildEffectList(hDlg, mmap);
			}
			break;
		case IDC_BUTTON_C1:
			if (HIWORD(wParam) == BN_CLICKED) {
				ChangeAddColor(hDlg, mmap, effID);
			}
			break;
		case IDC_BUT_ADD_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED && mmap) {
				ChangeAddColor(hDlg, mmap, (int)mmap->color.size());
			}
			break;
		case IDC_BUT_REMOVE_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED && mmap && effID < mmap->color.size()) {
				if (conf->afx_dev.GetGroupById(mmap->group)->have_power) {
					mmap->color.clear();
					effID = 0;
				}
				else {
					mmap->color.erase(mmap->color.begin() + effID);
					if (effID)
						effID--;
				}
				RebuildEffectList(hDlg, mmap);
				RedrawZoneGrid(mmap->group);
			}
			break;
		}
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (mmap) {
				if ((HWND)lParam == s1_slider) {
					mmap->color[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, mmap->color[effID].tempo);
				}
				if ((HWND)lParam == l1_slider) {
					mmap->color[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, mmap->color[effID].time);
				}
				fxhl->RefreshOne(mmap);
			}
			break;
		} break;
	case WM_DRAWITEM: {
		AlienFX_SDK::Afx_colorcode* c = mmap && effID < mmap->color.size() ? c = Act2Code(&mmap->color[effID]) : NULL;
		RedrawButton(((DRAWITEMSTRUCT*)lParam)->hwndItem, c);
	} break;
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_LEFFECTS_LIST) {
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & LVIS_FOCUSED && lPoint->iItem != -1) {
					// Select other item...
					effID = lPoint->iItem;
					SetEffectData(hDlg, mmap);
				}
			} break;
			case NM_DBLCLK:
				ChangeAddColor(hDlg, mmap, effID);
				break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}
