#include "alienfx-gui.h"

extern int eItem, effID;

void UpdateZoneList(HWND hDlg, byte flag = 0) {
	int rpos = -1, pos = 0;
	HWND zone_list = GetDlgItem(hDlg, IDC_LIST_ZONES);
	LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE };
	ListView_DeleteAllItems(zone_list);
	ListView_SetExtendedListViewStyle(zone_list, LVS_EX_FULLROWSELECT);
	if (!ListView_GetColumnWidth(zone_list, 0)) {
		LVCOLUMNA lCol{ LVCF_TEXT | LVCF_FMT, LVCFMT_LEFT };
		lCol.pszText = "Name";
		ListView_InsertColumn(zone_list, 0, &lCol);
		lCol.pszText = "L";
		lCol.fmt = LVCFMT_RIGHT;
		ListView_InsertColumn(zone_list, 1, &lCol);
	}
	for (int i = 0; i < conf->afx_dev.GetGroups()->size(); i++) {
		AlienFX_SDK::group grp = conf->afx_dev.GetGroups()->at(i);
		if (!flag || !grp.have_power) {
			string name = "(" + to_string(grp.lights.size()) + ")";
			lItem.iItem = pos;
			lItem.lParam = grp.gid;
			lItem.pszText = (LPSTR)grp.name.c_str();
			if (grp.gid == eItem) {
				lItem.state = LVIS_SELECTED;
				rpos = pos;
			}
			else
				lItem.state = 0;
			ListView_InsertItem(zone_list, &lItem);
			ListView_SetItemText(zone_list, pos, 1, (LPSTR)name.c_str());
			pos++;
		}
	}
	ListView_SetColumnWidth(zone_list, 0, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(zone_list, 1, LVSCW_AUTOSIZE_USEHEADER);
	ListView_EnsureVisible(zone_list, rpos, false);
}

BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_INITDIALOG:
	{
		UpdateZoneList(hDlg);
		return false;
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_BUT_ADD_ZONE: {
			AlienFX_SDK::group* grp = conf->CreateGroup("New zone");
			conf->afx_dev.GetGroups()->push_back(*grp);
			eItem = grp->gid;
			UpdateZoneList(hDlg);
			//conf->afx_dev.SaveMappings();
		} break;
		case IDC_BUT_DEL_ZONE:
			if (eItem > 0) {
				// delete from all profiles...
				for (auto Iter = conf->profiles.begin(); Iter != conf->profiles.end(); Iter++) {
					auto pos = find_if((*Iter)->lightsets.begin(), (*Iter)->lightsets.end(),
						[](auto t) {
							return t.group->gid == eItem;
						});
					if (pos != (*Iter)->lightsets.end())
						(*Iter)->lightsets.erase(pos);
				}
				for (auto Iter = conf->afx_dev.GetGroups()->begin(); Iter != conf->afx_dev.GetGroups()->end(); Iter++)
					if (Iter->gid == eItem) {
						if ((Iter + 1) != conf->afx_dev.GetGroups()->end())
							eItem = (Iter + 1)->gid;
						else
							if (Iter != conf->afx_dev.GetGroups()->begin())
								eItem = (Iter - 1)->gid;
						conf->afx_dev.GetGroups()->erase(Iter);
						break;
					}
				UpdateZoneList(hDlg);
				/*conf->afx_dev.SaveMappings();
				conf->Save();*/
			}
			break;
		}
	} break;
	//case WM_PAINT:
	//	UpdateZoneList(hDlg);
	//	return false;
	//	break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_ZONES:
			switch (((NMHDR*)lParam)->code) {
			case LVN_ITEMACTIVATE: {
				NMITEMACTIVATE* item = (NMITEMACTIVATE*)lParam;
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, ((NMITEMACTIVATE*)lParam)->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uNewState & LVIS_SELECTED && lPoint->iItem != -1) {
					// Select other item...
					if ((int)lPoint->lParam > 0)
						eItem = (int)lPoint->lParam;// lbItem;
					effID = 0;
					SendMessage(GetParent(hDlg), WM_APP + 2, 0, 1);
					//mmap = FindMapping(eItem);
					//RebuildEffectList(hDlg, mmap);
					//RedrawGridButtonZone();
				}
				//else
				//	return false;
				//	/*eItem = -1;
				//	mmap = NULL;
				//	RebuildEffectList(hDlg, mmap);
				//	RedrawGridButtonZone();*/
				//}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById((int)sItem->item.lParam);
				if (grp && sItem->item.pszText) {
					grp->name = sItem->item.pszText;
					ListView_SetItem(((NMHDR*)lParam)->hwndFrom, &sItem->item);
					//ListView_SetColumnWidth(((NMHDR*)lParam)->hwndFrom, 0, LVSCW_AUTOSIZE);
					return true;
				}
				else
					return false;
			} break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}