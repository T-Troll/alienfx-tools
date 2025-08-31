#include "alienfx-gui.h"
#include "FXHelper.h"
#include "EventHandler.h"
#include "common.h"

extern FXHelper* fxhl;
extern EventHandler* eve;
extern int tabLightSel;
extern void RecalcGridZone(RECT* what = NULL);
extern AlienFX_SDK::Afx_group* FindCreateMappingGroup();

HWND zsDlg;

void UpdateZoneList() {
	int rpos = -1, pos = 0;
	HWND zone_list = GetDlgItem(zsDlg, IDC_LIST_ZONES);
	ListView_DeleteAllItems(zone_list);
	ListView_SetExtendedListViewStyle(zone_list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	if (!ListView_GetColumnWidth(zone_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(zone_list, 0, &lCol);
		lCol.fmt = LVCFMT_RIGHT;
		ListView_InsertColumn(zone_list, 1, &lCol);
	}
	for (auto i = conf->activeProfile->lightsets.begin(); i < conf->activeProfile->lightsets.end(); i++) {
		AlienFX_SDK::Afx_group* grp = conf->FindCreateGroup(i->group);
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE };
		bool active = false;
		switch (tabLightSel) {
			case TAB_COLOR: active = i->color.size(); break;
			case TAB_EVENTS: active = i->events.size(); break;
			case TAB_AMBIENT: active = i->ambients.size(); break;
			case TAB_HAPTICS: active = i->haptics.size(); break;
			case TAB_GRID: active = i->effect.trigger;
		}
		string name = (active ? "+(" : "(") + to_string(grp->lights.size()) + ")";
		lItem.iItem = pos;
		lItem.lParam = i->group;
		lItem.pszText = (LPSTR)grp->name.c_str();
		if (activeMapping && grp->gid == activeMapping->group) {
			lItem.state = LVIS_SELECTED | LVIS_FOCUSED;
			rpos = pos;
		}
		ListView_InsertItem(zone_list, &lItem);
		ListView_SetItemText(zone_list, pos, 1, (LPSTR)name.c_str());
		pos++;
	}
	RECT cArea;
	GetClientRect(zone_list, &cArea);
	ListView_SetColumnWidth(zone_list, 1, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(zone_list, 0, cArea.right - ListView_GetColumnWidth(zone_list, 1));
	if (rpos < 0) { // no selection
		if (conf->activeProfile->lightsets.size()) {
			ListView_SetItemState(zone_list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}
		//else {
		//	activeMapping = NULL;
		//}
		SendMessage(GetParent(zsDlg), WM_APP + 2, 0, 0);
	} else
		ListView_EnsureVisible(zone_list, rpos, false);
}

BOOL CALLBACK AddZoneDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND grouplist = GetDlgItem(hDlg, IDC_LIST_GROUPS);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		ListBox_SetItemData(grouplist, ListBox_AddString(grouplist, "<New Zone>"), -1);
		if (conf->afx_dev.GetGroups()->empty()) {
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIST_GROUPS, LBN_SELCHANGE), 0);
		}
		else {
			for (auto t = conf->afx_dev.GetGroups()->begin(); t < conf->afx_dev.GetGroups()->end(); t++) {
				if (!conf->FindMapping(t->gid))
					ListBox_SetItemData(grouplist, ListBox_AddString(grouplist, t->name.c_str()), t->gid);
			}
			RECT pRect;
			GetWindowRect(zsDlg, &pRect);
			SetWindowPos(hDlg, NULL, pRect.left, pRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
	} break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCLOSE: case IDCANCEL: EndDialog(hDlg, IDCLOSE); break;
		case IDC_LIST_GROUPS:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
			{
				int eItem = (int)ListBox_GetItemData(grouplist, ListBox_GetCurSel(grouplist));
				if (eItem < 0) {
					activeMapping = NULL;
					FindCreateMappingGroup();
				}
				else {
					conf->activeProfile->lightsets.push_back({ eItem });
					activeMapping = &conf->activeProfile->lightsets.back();
				}
				EndDialog(hDlg, IDOK);
			} break;
			}
		}
		break;
	default: return false;
	}
	return true;
}

BOOL CALLBACK SelectLightsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	HWND llist = GetDlgItem(hDlg, IDC_LIGHTS_LIST);

	AlienFX_SDK::Afx_group* grp = FindCreateMappingGroup();

	switch (message)
	{
	case WM_INITDIALOG:
		RECT pRect;
		GetWindowRect(zsDlg, &pRect);
		SetWindowPos(hDlg, NULL, pRect.left, pRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		for (auto dev = conf->afx_dev.fxdevs.begin(); dev != conf->afx_dev.fxdevs.end(); dev++)
			for (auto lgh = dev->lights.begin(); lgh != dev->lights.end(); lgh++) {
				int pos = ListBox_AddString(llist, (LPSTR)lgh->name.c_str());
				AlienFX_SDK::Afx_groupLight lgt{ dev->pid, lgh->lightid };
				ListBox_SetItemData(llist, pos , lgt.lgh);
				if (conf->IsLightInGroup(lgt.lgh, grp))
					ListBox_SetSel(llist, true, pos);
			}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCLOSE: case IDCANCEL: {
			int numSel = ListBox_GetSelCount(llist), * buf = new int[numSel];
			ListBox_GetSelItems(llist, numSel, buf);
			if (grp)
				grp->lights.clear();
			AlienFX_SDK::Afx_groupLight t;
			for (int i = 0; i < numSel; i++) {
				t.lgh = (DWORD)ListBox_GetItemData(llist, buf[i]);
				grp->lights.push_back(t);
			}
			delete[] buf;
			EndDialog(hDlg, IDCLOSE);
		} break;
		case IDC_BUT_LIGHTSRESET:
			ListBox_SelItemRange(llist, false, 0, ListBox_GetCount(llist));
			grp->lights.clear();
			break;
		}
		break;
	default: return false;
	}
	return true;
}

static const string gaugetypes[] = { "Off", "Horizontal", "Vertical", "Diagonal (left)", "Diagonal (right)", "Radial", ""};

BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_INITDIALOG:
	{
		zsDlg = hDlg;
		UpdateCombo(GetDlgItem(hDlg, IDC_COMBO_GAUGE), gaugetypes);
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_BUT_ADD_ZONE: {
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_ADDGROUP), hDlg, (DLGPROC)AddZoneDialog);
			UpdateZoneList();
		} break;
		case IDC_BUT_DEL_ZONE:
			if (activeMapping) {
				conf->modifyProfile.lockWrite();
				for (auto iter = conf->activeProfile->lightsets.begin(); iter != conf->activeProfile->lightsets.end(); iter++) {
					if (iter->group == activeMapping->group) {
						auto newLightSet = conf->activeProfile->lightsets.erase(iter);
						activeMapping = conf->FindMapping(newLightSet == conf->activeProfile->lightsets.end() ?
							conf->activeProfile->lightsets.empty() ?
								0 : (newLightSet-1)->group :
							newLightSet->group);
						break;
					}
				}
				conf->modifyProfile.unlockWrite();
				conf->RemoveUnusedGroups();
				eve->ChangeEffects();
				RecalcGridZone();
				UpdateZoneList();
				fxhl->Refresh();
			}
			break;
		case IDC_CHECK_SPECTRUM:
			if (activeMapping) {
				SetBitMask(activeMapping->gaugeflags, GAUGE_GRADIENT, IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				fxhl->Refresh();
			}
			break;
		case IDC_CHECK_REVERSE:
			if (activeMapping) {
				SetBitMask(activeMapping->gaugeflags, GAUGE_REVERSE, IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				fxhl->Refresh();
			}
			break;
		case IDC_COMBO_GAUGE:
			if (activeMapping && HIWORD(wParam) == CBN_SELCHANGE) {
				activeMapping->gauge = ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam)));
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SPECTRUM), activeMapping && activeMapping->gauge);
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_REVERSE), activeMapping && activeMapping->gauge);
				fxhl->Refresh();
			}
			break;
		case IDC_BUT_FROMLIST:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_SELECTFROMLIST), hDlg, (DLGPROC)SelectLightsDialog);
			RecalcGridZone();
			UpdateZoneList();
			fxhl->Refresh();
			break;
		}
	} break;
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_LIST_ZONES) {
			switch (((NMHDR*)lParam)->code) {
			case LVN_ITEMACTIVATE: case NM_RETURN: {
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, ((NMITEMACTIVATE*)lParam)->iItem);
			} break;
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uChanged & LVIF_STATE) {
					if (lPoint->uNewState & LVIS_SELECTED) {
						int neweItem = (int)lPoint->lParam;
						// gauge and spectrum.
						if (activeMapping = conf->FindMapping(neweItem)) {
							//conf->FindCreateGroup(neweItem);
							CheckDlgButton(hDlg, IDC_CHECK_SPECTRUM, activeMapping->gaugeflags & GAUGE_GRADIENT);
							CheckDlgButton(hDlg, IDC_CHECK_REVERSE, activeMapping->gaugeflags & GAUGE_REVERSE);
							ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_GAUGE), activeMapping->gauge);
							SendMessage(GetParent(hDlg), WM_APP + 2, 0, 0/*neweItem != activeMapping->group*/);
						}
					}
					else
						if (ListView_GetItemState(lPoint->hdr.hwndFrom, lPoint->iItem, LVIS_FOCUSED)) {
							ListView_SetItemState(lPoint->hdr.hwndFrom, lPoint->iItem, LVIS_SELECTED, LVIS_SELECTED);
						}
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				AlienFX_SDK::Afx_group* grp = FindCreateMappingGroup();// conf->FindCreateGroup((int)sItem->item.lParam);
				if (sItem->item.pszText) {
					grp->name = sItem->item.pszText;
					ListView_SetItem(((NMHDR*)lParam)->hwndFrom, &sItem->item);
					RECT cArea;
					GetClientRect(((NMHDR*)lParam)->hwndFrom, &cArea);
					ListView_SetColumnWidth(((NMHDR*)lParam)->hwndFrom, 1, LVSCW_AUTOSIZE);
					ListView_SetColumnWidth(((NMHDR*)lParam)->hwndFrom, 0, cArea.right - ListView_GetColumnWidth(((NMHDR*)lParam)->hwndFrom, 1));
					//return true;
				}
				else
					return false;
			} break;
			}
		}
		break;
	default: return false;
	}
	return true;
}