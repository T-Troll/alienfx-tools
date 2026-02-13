#include "alienfx-gui.h"
#include "FXHelper.h"
#include "Common.h"

extern bool SetColor(HWND ctrl, AlienFX_SDK::Afx_action* map, bool update = true);
extern AlienFX_SDK::Afx_colorcode Act2Code(AlienFX_SDK::Afx_action*);
extern DWORD MakeRGB(AlienFX_SDK::Afx_action* act);
extern void RedrawButton(HWND ctrl, DWORD);
extern void UpdateZoneList();
extern void UpdateZoneAndGrid();
extern AlienFX_SDK::Afx_group* FindCreateMappingGroup();
extern FXHelper* fxhl;

int effID = 0;

const char* lightEffectNames[] { "Color", "Pulse", "Morph", "Breath", "Spectrum", "Rainbow", "" };

void SetEffectData(HWND hDlg) {
	bool hasEffects = activeMapping && activeMapping->color.size();
	if (hasEffects) {
			ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_TYPE1), activeMapping->color[effID].type & 0xf);
			SetSlider(sTip1, activeMapping->color[effID].tempo);
			SetSlider(sTip2, activeMapping->color[effID].time);
			CheckDlgButton(hDlg, IDC_ACCENT, activeMapping->color[effID].type & 0xf0);
	}
	EnableWindow(GetDlgItem(hDlg, IDC_TYPE1), hasEffects);
	EnableWindow(GetDlgItem(hDlg, IDC_SPEED1), hasEffects);
	EnableWindow(GetDlgItem(hDlg, IDC_LENGTH1), hasEffects);
	EnableWindow(GetDlgItem(hDlg, IDC_ACCENT), hasEffects);
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_C1), activeMapping && activeMapping->color.size() ?
		activeMapping->color[effID].type & 0xf0 ?
			conf->accentColor :
			MakeRGB(&activeMapping->color[effID]) :
		0xff000000);
}

void RebuildEffectList(HWND hDlg) {
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
	if (activeMapping) {
		COLORREF* picData = NULL;
		HBITMAP colorBox = NULL;
		HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
								  GetSystemMetrics(SM_CYSMICON),
								  ILC_COLOR32, 1, 1);
		for (int i = 0; i < activeMapping->color.size(); i++) {
			LVITEMA lItem{ LVIF_TEXT | LVIF_IMAGE | LVIF_STATE, i };
			picData = new COLORREF[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON)];
			fill_n(picData, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON), Act2Code(&activeMapping->color[i]).ci);
			colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 32, picData);
			delete[] picData;
			ImageList_Add(hSmall, colorBox, NULL);
			DeleteObject(colorBox);
			lItem.iImage = i;
			// Patch for incorrect type
			if (activeMapping->color[i].type == AlienFX_SDK::Action::AlienFX_A_Power)
				activeMapping->color[i].type = 0;
			lItem.pszText = (LPSTR)lightEffectNames[activeMapping->color[i].type & 0xf];
			// check selection...
			if (i == effID) {
				lItem.state = LVIS_SELECTED;
			}
			//else
			//	lItem.state = 0;
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);
	}
	SetEffectData(hDlg);
	ListView_EnsureVisible(eff_list, effID, false);
}

void ChangeAddColor(HWND hDlg, int newEffID) {
	FindCreateMappingGroup();
	if (newEffID < activeMapping->color.size())
		SetColor(GetDlgItem(hDlg, IDC_BUTTON_C1), &activeMapping->color[newEffID]);
	else {
		AlienFX_SDK::Afx_action act{ 0 };
		bool isPower = conf->FindZoneMap(activeMapping->group)->havePower;
		if (isPower && activeMapping->color.empty())
			activeMapping->color.push_back(act);
		if (activeMapping->color.size() < 9) {
			if (effID < activeMapping->color.size())
				act = activeMapping->color[effID];
			activeMapping->color.push_back(act);
			if (SetColor(GetDlgItem(hDlg, IDC_BUTTON_C1), &activeMapping->color[newEffID]))
				effID = newEffID;
			else {
				if (isPower && activeMapping->color.size() > 2)
					activeMapping->color.pop_back();
				else
					activeMapping->color.clear();
			}
		}
	}
	RebuildEffectList(hDlg);
	UpdateZoneAndGrid();
}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1);

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
		CreateToolTip(s1_slider, sTip1);
		CreateToolTip(l1_slider, sTip2);
		RebuildEffectList(hDlg);
	} break;
	case WM_APP + 2: {
		effID = 0;
		RebuildEffectList(hDlg);
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_TYPE1:
			if (HIWORD(wParam) == CBN_SELCHANGE && activeMapping) {
				activeMapping->color[effID].type = IsDlgButtonChecked(hDlg, IDC_ACCENT) == BST_CHECKED ?
					0x10 : 0 | ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_TYPE1));
				RebuildEffectList(hDlg);
				fxhl->RefreshZone(activeMapping);
			}
			break;
		case IDC_ACCENT:
			if (activeMapping) {
				activeMapping->color[effID].type = IsDlgButtonChecked(hDlg, IDC_ACCENT) == BST_CHECKED ?
					activeMapping->color[effID].type | 0x10 :
					activeMapping->color[effID].type & 0xf;
				SetEffectData(hDlg);
				fxhl->RefreshZone(activeMapping);
			}
			break;
		case IDC_BUTTON_C1:
			if (HIWORD(wParam) == BN_CLICKED) {
				ChangeAddColor(hDlg, effID);
			}
			break;
		case IDC_BUT_ADD_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED) {
				ChangeAddColor(hDlg, (int)activeMapping->color.size());
			}
			break;
		case IDC_BUT_REMOVE_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED && activeMapping && effID < activeMapping->color.size()) {
				if (conf->FindZoneMap(activeMapping->group)->havePower && activeMapping->color.size() == 2) {
					activeMapping->color.clear();
					effID = 0;
				}
				else {
					activeMapping->color.erase(activeMapping->color.begin() + effID);
					if (effID)
						effID--;
				}
				RebuildEffectList(hDlg);
				UpdateZoneAndGrid();
				fxhl->Refresh();
			}
			break;
		}
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (activeMapping) {
				if ((HWND)lParam == s1_slider) {
					activeMapping->color[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, activeMapping->color[effID].tempo);
				}
				if ((HWND)lParam == l1_slider) {
					activeMapping->color[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, activeMapping->color[effID].time);
				}
				fxhl->Refresh();
			}
			break;
		} break;
	case WM_DRAWITEM: {
		RedrawButton(((DRAWITEMSTRUCT*)lParam)->hwndItem, activeMapping && effID < activeMapping->color.size() ? MakeRGB(&activeMapping->color[effID]) : 0xff000000);
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
					SetEffectData(hDlg);
				}
			} break;
			case NM_DBLCLK:
				ChangeAddColor(hDlg, effID);
				break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}
