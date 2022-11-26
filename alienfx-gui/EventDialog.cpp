#include "alienfx-gui.h"
#include "MonHelper.h"
#include "common.h"

extern bool SetColor(HWND hDlg, groupset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void RedrawButton(HWND hDlg, AlienFX_SDK::Colorcode*);
extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);

extern FXHelper* fxhl;
extern MonHelper* mon;
extern int eItem;

const static vector<string> eventTypeNames{ "Power", "Performance", "Indicator" },
		perfTypeNames{ "CPU load", "RAM load", "Storage load", "GPU load", "Network", "Temperature", "Battery level",
			"Fan RPM", "Power usage" },
		indTypeNames{ "Storage activity", "Network activity", "System overheat", "Out of memory", "Low battery", "Selected language" };

int eventID = -1;

event* GetEventData(groupset* mmap) {
	if (mmap && eventID >= 0 && mmap->events.size() > eventID)
		return &mmap->events[eventID];
	return NULL;
}

void SetEventData(HWND hDlg, event* ev) {
	CheckDlgButton(hDlg, IDC_STATUS_BLINK, ev && ev->mode ? BST_CHECKED : BST_UNCHECKED);
	ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_EVENT_TYPE), ev ? ev->state : -1);
	vector<string> sources;
	switch (ev ? ev->state : -1) {
	case MON_TYPE_POWER:
		sources = { "Power" };
		break;
	case MON_TYPE_PERF:
		sources = perfTypeNames;
		break;
	case MON_TYPE_IND:
		sources = indTypeNames;
		break;
	}
	UpdateCombo(GetDlgItem(hDlg, IDC_EVENT_SOURCE), sources, ev ? ev->source : -1);
	SendMessage(GetDlgItem(hDlg, IDC_CUTLEVEL), TBM_SETPOS, true, ev ? ev->cut : 0);
	SetSlider(sTip2, ev ? ev->cut : 0);
	RedrawWindow(GetDlgItem(hDlg, IDC_BUTTON_COLORFROM), NULL, NULL, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hDlg, IDC_BUTTON_COLORTO), NULL, NULL, RDW_INVALIDATE);
}

void RebuildEventList(HWND hDlg, groupset* mmap) {
	HWND eff_list = GetDlgItem(hDlg, IDC_EVENTS_LIST);

	ListView_DeleteAllItems(eff_list);
	ListView_SetExtendedListViewStyle(eff_list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	HIMAGELIST hOld = ListView_GetImageList(eff_list, LVSIL_SMALL);

	if (!ListView_GetColumnWidth(eff_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(eff_list, 0, &lCol);
		ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE_USEHEADER);
	}
	if (mmap) {
		LVITEMA lItem{ LVIF_TEXT | LVIF_STATE };
		for (int i = 0; i < mmap->events.size(); i++) {
			string itemName = eventTypeNames[mmap->events[i].state];
			switch (mmap->events[i].state) {
			case MON_TYPE_PERF:
				itemName += ", " + perfTypeNames[mmap->events[i].source];
				break;
			case MON_TYPE_IND:
				itemName += ", " + indTypeNames[mmap->events[i].source];
				break;
			}
			lItem.iItem = i;
			lItem.pszText = (LPSTR) itemName.c_str();
			// check selection...
			if (i == eventID) {
				lItem.state = LVIS_SELECTED | LVIS_FOCUSED;
			}
			else
				lItem.state = 0;
			ListView_InsertItem(eff_list, &lItem);
		}
	}
	CheckDlgButton(hDlg, IDC_CHECK_NOEVENT, mmap && mmap->fromColor ? BST_CHECKED : BST_UNCHECKED);
	SetEventData(hDlg, GetEventData(mmap));
	ListView_EnsureVisible(eff_list, eventID, false);
	RedrawGridButtonZone(NULL, true);
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	groupset* map = FindMapping(eItem);
	event* ev = GetEventData(map);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		UpdateCombo(GetDlgItem(hDlg, IDC_EVENT_TYPE), eventTypeNames);

		// Set slider
		SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 100));
		SendMessage(s2_slider, TBM_SETTICFREQ, 10, 0);
		sTip2 = CreateToolTip(s2_slider, sTip2);

		if (map && map->events.size())
			eventID = 0;
		RebuildEventList(hDlg, map);

		// Start UI update thread...
		SetTimer(hDlg, 0, 300, NULL);

	} break;
	case WM_APP + 2: {
		if (map && map->events.size())
			eventID = 0;
		else
			eventID = -1;
		RebuildEventList(hDlg, map);
	} break;
	case WM_COMMAND: {
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDC_CHECK_NOEVENT:
			if (map) {
				map->fromColor = state;
				fxhl->RefreshMon();
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_STATUS_BLINK:
			if (ev)
				ev->mode = state;
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_BUTTON_COLORFROM:
			if (ev && (!map->fromColor || map->color.size())) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_COLORFROM), map, &ev->from);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_COLORTO:
			if (ev) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_COLORTO), map, &ev->to);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_EVENT_SOURCE:
			if (ev && HIWORD(wParam) == CBN_SELCHANGE) {
				ev->source = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_EVENT_SOURCE));
				RebuildEventList(hDlg, map);
				fxhl->RefreshMon();
			}
			break;
		case IDC_EVENT_TYPE:
			if (ev && HIWORD(wParam) == CBN_SELCHANGE) {
				ev->state = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_EVENT_TYPE));
				ev->source = 0;
				RebuildEventList(hDlg, map);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUT_ADD_EVENT:
			if (map) {
				map->events.push_back({ 0 });
				eventID = (int)map->events.size() - 1;
				RebuildEventList(hDlg, map);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTT_REMOVE_EVENT:
			if (ev) {
				map->events.erase(map->events.begin() + eventID);
				if (eventID)
					eventID--;
				RebuildEventList(hDlg, map);
				fxhl->Refresh();
			}
			break;
		case IDC_BUTT_EVENT_UP:
			if (ev && eventID) {
				event t = *ev;
				eventID--;
				*ev = map->events[eventID];
				map->events[eventID] = t;
				RebuildEventList(hDlg, map);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUT_EVENT_DOWN:
			if (ev && eventID < map->events.size() - 1) {
				event t = *ev;
				eventID++;
				*ev = map->events[eventID];
				map->events[eventID] = t;
				RebuildEventList(hDlg, map);
				fxhl->RefreshMon();
			}
			break;
		}
	} break;
	case WM_DRAWITEM: {
		AlienFX_SDK::Colorcode* c = NULL;
		if (ev) {
			switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
			case IDC_BUTTON_COLORFROM:
				c = Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &ev->from);
				break;
			case IDC_BUTTON_COLORTO:
				c = Act2Code(&ev->to);
				break;
			}
		}
		RedrawButton(((DRAWITEMSTRUCT*)lParam)->hwndItem, c);
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (ev && (HWND)lParam == s2_slider) {
				SetSlider(sTip2, ev->cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
				fxhl->RefreshMon();
			}
			break;
		} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_EVENTS_LIST:
			switch (((NMHDR*)lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uNewState & LVIS_FOCUSED) {
					// Select other item...
					eventID = lPoint->iItem;
				}
				else
					eventID = -1;
				SetEventData(hDlg, GetEventData(map));
			} break;
			}
			break;
		}
		break;
	case WM_TIMER:
		if (IsWindowVisible(hDlg)) {
			//DebugPrint("Events UI update...\n");
			SetDlgItemText(hDlg, IDC_VAL_CPU, (to_string(fxhl->eData.CPU) + " (" + to_string(fxhl->maxData.CPU) + ")%").c_str());
			SetDlgItemText(hDlg, IDC_VAL_RAM, (to_string(fxhl->eData.RAM) + " (" + to_string(fxhl->maxData.RAM) + ")%").c_str());
			SetDlgItemText(hDlg, IDC_VAL_GPU, (to_string(fxhl->eData.GPU) + " (" + to_string(fxhl->maxData.GPU) + ")%").c_str());
			SetDlgItemText(hDlg, IDC_VAL_PWR, (to_string(fxhl->eData.PWR * fxhl->maxData.PWR / 100) + " (" + to_string(fxhl->maxData.PWR) + ")W").c_str());
			SetDlgItemText(hDlg, IDC_VAL_BAT, (to_string(fxhl->eData.Batt) + " %").c_str());
			SetDlgItemText(hDlg, IDC_VAL_NET, (to_string(fxhl->eData.NET) + " %").c_str());
			SetDlgItemText(hDlg, IDC_VAL_TEMP, (to_string(fxhl->eData.Temp) + " (" + to_string(fxhl->maxData.Temp) + ")C").c_str());
			if (mon) {
				int maxFans = 0;
				for (auto i = mon->fanRpm.begin(); i < mon->fanRpm.end(); i++)
					maxFans = max(maxFans, *i);
				SetDlgItemText(hDlg, IDC_VAL_FAN, (to_string(maxFans) + " RPM (" + to_string(fxhl->eData.Fan) + "%)").c_str());
			}
			else
				SetDlgItemText(hDlg, IDC_VAL_FAN, "disabled");
		}
		break;
	default: return false;
	}
	return true;
}

