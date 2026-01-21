#include "alienfx-gui.h"
#include "common.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "GridHelper.h"

extern bool SetColor(HWND hDlg, AlienFX_SDK::Afx_colorcode&);
extern DWORD MakeRGB(AlienFX_SDK::Afx_colorcode c);
extern void RedrawButton(HWND hDlg, DWORD);
extern void UpdateZoneAndGrid();
extern AlienFX_SDK::Afx_group* FindCreateMappingGroup();

extern EventHandler* eve;
extern FXHelper* fxhl;

int clrListID = 0;

void RebuildGEColorsList(HWND hDlg) {
	HWND eff_list = GetDlgItem(hDlg, IDC_COLORS_LIST);

	ListView_DeleteAllItems(eff_list);
	ListView_SetExtendedListViewStyle(eff_list, LVS_EX_FULLROWSELECT);

	HIMAGELIST hOld = ListView_GetImageList(eff_list, LVSIL_SMALL);
	if (hOld) {
		ImageList_Destroy(hOld);
	}

	if (!ListView_GetColumnWidth(eff_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(eff_list, 0, &lCol);
		ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE_USEHEADER);
	}
	if (activeMapping) {
		COLORREF* picData = NULL;
		HBITMAP colorBox = NULL;
		HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			ILC_COLOR32, 1, 1);
		vector<AlienFX_SDK::Afx_colorcode>* colors = &activeMapping->effect.effectColors;
		for (int i = 0; i < colors->size(); i++) {
			LVITEMA lItem{ LVIF_TEXT | LVIF_IMAGE | LVIF_STATE, i };
			picData = new COLORREF[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON)];
			fill_n(picData, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON),
				/*MakeRGB(*/colors->at(i).ci);
			colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 32, picData);
			delete[] picData;
			ImageList_Add(hSmall, colorBox, NULL);
			DeleteObject(colorBox);
			string name = i ? "Phase " + to_string(i) : "Background";
			lItem.iImage = i;
			lItem.pszText = (LPSTR)name.c_str();
			// check selection...
			if (i == clrListID) {
				lItem.state = LVIS_SELECTED;
			}
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);
	}
	ListView_EnsureVisible(eff_list, clrListID, false);
}

void ChangeAddGEColor(HWND hDlg, int newColorID) {
	// change color.
	FindCreateMappingGroup();
	vector<AlienFX_SDK::Afx_colorcode>* clr = &activeMapping->effect.effectColors;
	if (newColorID < clr->size())
		SetColor(GetDlgItem(hDlg, IDC_BUT_GECOLOR), clr->at(newColorID));
	else {
		AlienFX_SDK::Afx_colorcode act{ 0 };
		// add new color
		if (clr->empty())
			clr->push_back(act);
		if (clrListID < clr->size())
			act = clr->at(clrListID);
		clr->push_back(act);
		if (SetColor(GetDlgItem(hDlg, IDC_BUT_GECOLOR), clr->at(newColorID)))
			clrListID = newColorID;
		else
			clr->pop_back();
	}
	RebuildGEColorsList(hDlg);
	UpdateZoneAndGrid();
}

void UpdateEffectInfo(HWND hDlg) {
	if (activeMapping) {
		CheckDlgButton(hDlg, IDC_CHECK_CIRCLE, activeMapping->effect.flags & GE_FLAG_CIRCLE);
		CheckDlgButton(hDlg, IDC_CHECK_RANDOM, activeMapping->effect.flags & GE_FLAG_RANDOM);
		CheckDlgButton(hDlg, IDC_CHECK_PHASE, activeMapping->effect.flags & GE_FLAG_PHASE);
		CheckDlgButton(hDlg, IDC_CHECK_BACKGROUND, activeMapping->effect.flags & GE_FLAG_BACK);
		CheckDlgButton(hDlg, IDC_CHECK_RPOS, activeMapping->effect.flags & GE_FLAG_RPOS);
		ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_TRIGGER), activeMapping->effect.trigger);
		ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_GEFFTYPE), activeMapping->effect.type);
		SetSlider(sTip2, activeMapping->effect.speed - 80);
		SetSlider(sTip3, activeMapping->effect.width);
	}
	RebuildGEColorsList(hDlg);
}

const char* triggerNames[] = { "Off", "Continues", "Keyboard", "Event", "Ambient", "" },
	*effectTypeNames[] = { "Running light", "Wave", "Gradient", "Fill", "Star field", "Fade", ""};

BOOL CALLBACK TabGridDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND speed_slider = GetDlgItem(hDlg, IDC_SLIDER_SPEED),
		width_slider = GetDlgItem(hDlg, IDC_SLIDER_WIDTH),
		gs_slider = GetDlgItem(hDlg, IDC_SLIDER_TACT);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		UpdateCombo(GetDlgItem(hDlg, IDC_COMBO_TRIGGER), triggerNames);
		UpdateCombo(GetDlgItem(hDlg, IDC_COMBO_GEFFTYPE), effectTypeNames);
		SendMessage(speed_slider, TBM_SETRANGE, true, MAKELPARAM(-80, 80));
		SendMessage(width_slider, TBM_SETRANGE, true, MAKELPARAM(1, 80));
		SendMessage(gs_slider, TBM_SETRANGE, true, MAKELPARAM(5, 1000));
		SendMessage(gs_slider, TBM_SETTICFREQ, 50, 0);
		CreateToolTip(gs_slider, sTip1, conf->geTact);
		CreateToolTip(speed_slider, sTip2);
		CreateToolTip(width_slider, sTip3);
		UpdateEffectInfo(hDlg);

	} break;
	case WM_APP + 2: {
		UpdateEffectInfo(hDlg);
	} break;
	case WM_COMMAND: {
		if (!activeMapping)
			return false;
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDC_COMBO_TRIGGER:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				//conf->modifyProfile.lockWrite();
				activeMapping->effect.trigger = ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam)));
				//conf->modifyProfile.unlockWrite();
				eve->ChangeEffects();
				UpdateZoneAndGrid();
			}
			break;
		case IDC_COMBO_GEFFTYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				activeMapping->effect.type = ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam)));
			}
			break;
		case IDC_CHECK_CIRCLE:
			SetBitMask(activeMapping->effect.flags, GE_FLAG_CIRCLE, state);
			break;
		case IDC_CHECK_RPOS:
			SetBitMask(activeMapping->effect.flags, GE_FLAG_RPOS, state);
			break;
		case IDC_CHECK_RANDOM:
			SetBitMask(activeMapping->effect.flags, GE_FLAG_RANDOM, state);
			break;
		case IDC_CHECK_PHASE:
			SetBitMask(activeMapping->effect.flags, GE_FLAG_PHASE, state);
			break;
		case IDC_CHECK_BACKGROUND:
			SetBitMask(activeMapping->effect.flags, GE_FLAG_BACK, state);
			break;
		case IDC_BUT_GECOLOR:
				ChangeAddGEColor(hDlg, clrListID);
			break;
		case IDC_BUT_ADD_COLOR:
			if (HIWORD(wParam) == BN_CLICKED) {
				ChangeAddGEColor(hDlg, (int)activeMapping->effect.effectColors.size());
			}
			break;
		case IDC_BUT_REMOVE_COLOR:
			if (HIWORD(wParam) == BN_CLICKED && clrListID < activeMapping->effect.effectColors.size()) {
				if (activeMapping->effect.effectColors.size() == 2) {
					activeMapping->effect.effectColors.clear();
					clrListID = 0;
				}
				else {
					activeMapping->effect.effectColors.erase(activeMapping->effect.effectColors.begin() + clrListID);
					if (clrListID)
						clrListID--;
				}
				RebuildGEColorsList(hDlg);
				UpdateZoneAndGrid();
			}
			break;
		}
		activeMapping->gridop.passive = true;
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (activeMapping) {
				activeMapping->gridop.passive = true;
				if ((HWND)lParam == speed_slider) {
					activeMapping->effect.speed = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) + 80;
					SetSlider(sTip2, activeMapping->effect.speed - 80);
				}
				if ((HWND)lParam == width_slider) {
					activeMapping->effect.width = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip3, activeMapping->effect.width);
				}
			}
		} break;
	case WM_VSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			conf->geTact = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			SetSlider(sTip1, conf->geTact);
			eve->ChangeEffects();
			break;
		}
		break;
	case WM_DRAWITEM:
		if (activeMapping && clrListID < activeMapping->effect.effectColors.size()) {
			RedrawButton(GetDlgItem(hDlg, IDC_BUT_GECOLOR), MakeRGB(activeMapping->effect.effectColors[clrListID]));
		}
	    break;
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_COLORS_LIST) {
			switch (((NMHDR*)lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uNewState & LVIS_SELECTED && lPoint->iItem != -1) {
					// Select other item...
					clrListID = lPoint->iItem;
					if (clrListID < activeMapping->effect.effectColors.size())
						RedrawButton(GetDlgItem(hDlg, IDC_BUT_GECOLOR), MakeRGB(activeMapping->effect.effectColors[clrListID]));
				}
			} break;
			case NM_DBLCLK:
				ChangeAddGEColor(hDlg, clrListID);
				break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}