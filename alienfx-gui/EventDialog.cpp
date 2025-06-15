#include "alienfx-gui.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "MonHelper.h"
#include "common.h"

extern bool SetColor(HWND ctrl, AlienFX_SDK::Afx_action* map, bool update = true);
DWORD MakeRGB(AlienFX_SDK::Afx_action* act);
//extern AlienFX_SDK::Afx_colorcode Act2Code(AlienFX_SDK::Afx_action*);
extern void RedrawButton(HWND hDlg, DWORD);
extern void UpdateZoneAndGrid();
extern void UpdateZoneList();

extern FXHelper* fxhl;
extern MonHelper* mon;
extern EventHandler* eve;

event* ev = NULL;

const char* eventTypeNames[] = { "Performance", "Indicator" };
const char* eventNamesP[] = { "CPU load", "RAM load", "Storage load", "GPU load", "Network", "Temperature", "Battery level",
			"Fans RPM", "Power usage", "Power mode", ""},
			*eventNamesI[] = { "Storage activity", "Network activity", "System overheat", "Out of memory", "Low battery", "Selected language",
				"BIOS Power mode", "Power source", ""};

int eventID = 0;

void SetEventData(HWND hDlg) {
	if (ev) {
		CheckDlgButton(hDlg, IDC_STATUS_BLINK, ev->mode ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_PERF, ev->state == MON_TYPE_PERF ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_IND, ev->state == MON_TYPE_IND ? BST_CHECKED : BST_UNCHECKED);
		UpdateCombo(GetDlgItem(hDlg, IDC_EVENT_SOURCE), ev->state < 2 ? eventNamesP : eventNamesI, ev->source);
		SendMessage(GetDlgItem(hDlg, IDC_CUTLEVEL), TBM_SETPOS, true, ev->cut);
		SetSlider(sTip2, ev->cut);
	}
	else {
		CheckDlgButton(hDlg, IDC_RADIO_PERF, BST_CHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_IND, BST_UNCHECKED);
		UpdateCombo(GetDlgItem(hDlg, IDC_EVENT_SOURCE), eventNamesP, 0);
	}

	RedrawWindow(GetDlgItem(hDlg, IDC_BUTTON_COLORFROM), NULL, NULL, RDW_INVALIDATE);
	RedrawWindow(GetDlgItem(hDlg, IDC_BUTTON_COLORTO), NULL, NULL, RDW_INVALIDATE);
}

void RebuildEventList(HWND hDlg) {
	HWND eff_list = GetDlgItem(hDlg, IDC_EVENTS_LIST);

	ListView_DeleteAllItems(eff_list);
	ListView_SetExtendedListViewStyle(eff_list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	//HIMAGELIST hOld = ListView_GetImageList(eff_list, LVSIL_SMALL);

	if (!ListView_GetColumnWidth(eff_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(eff_list, 0, &lCol);
		ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE_USEHEADER);
	}
	ev = NULL;
	LVITEMA lItem{ LVIF_TEXT | LVIF_STATE };
	if (mmap) {
		for (int i = 0; i < mmap->events.size(); i++) {
			if (!mmap->events[i].state)
				mmap->events[i].state = 2;
			int type = mmap->events[i].state - 1;
			string itemName = string(eventTypeNames[type]) + ", " +
				(type ? eventNamesI[mmap->events[i].source] : eventNamesP[mmap->events[i].source]);
				//eventNames[type][mmap->events[i].source];
			lItem.iItem = i;
			lItem.pszText = (LPSTR) itemName.c_str();
			lItem.state = i == eventID ? LVIS_SELECTED | LVIS_FOCUSED : 0;
			// check selection...
			//if (i == eventID) {
			//	lItem.state = LVIS_SELECTED | LVIS_FOCUSED;
			//}
			ListView_InsertItem(eff_list, &lItem);
		}
	}
	CheckDlgButton(hDlg, IDC_CHECK_NOEVENT, mmap && mmap->fromColor ? BST_CHECKED : BST_UNCHECKED);
	//SetEventData(hDlg);
	ListView_EnsureVisible(eff_list, eventID, false);
	//UpdateZoneAndGrid();
	//fxhl->RefreshCounters();
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Set slider
		SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 100));
		SendMessage(s2_slider, TBM_SETTICFREQ, 10, 0);
		sTip2 = CreateToolTip(s2_slider, sTip2);
		// Start UI update thread...
		SetTimer(hDlg, 0, 500, NULL);
	} break;
	case WM_APP + 2: {
		if (mmap && !(eventID < mmap->events.size()))
			eventID = 0;
		RebuildEventList(hDlg);
		SetEventData(hDlg);
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_CHECK_NOEVENT:
			if (mmap) {
				mmap->fromColor = IsDlgButtonChecked(hDlg, IDC_CHECK_NOEVENT) == BST_CHECKED;
				UpdateZoneAndGrid();
			}
			break;
		case IDC_STATUS_BLINK:
			if (ev)
				ev->mode = IsDlgButtonChecked(hDlg, IDC_STATUS_BLINK) == BST_CHECKED;;
			break;
		case IDC_BUTTON_COLORFROM:
			if (mmap && ev && (!mmap->fromColor || mmap->color.size())) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_COLORFROM), &ev->from);
				UpdateZoneAndGrid();
			}
			break;
		case IDC_BUTTON_COLORTO:
			if (ev) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_COLORTO), &ev->to);
				UpdateZoneAndGrid();
			}
			break;
		case IDC_EVENT_SOURCE:
			if (ev && HIWORD(wParam) == CBN_SELCHANGE) {
				ev->source = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_EVENT_SOURCE));
				RebuildEventList(hDlg);
			}
			break;
		case IDC_RADIO_PERF: case IDC_RADIO_IND: {
			int ctype = LOWORD(wParam) == IDC_RADIO_IND;
			UpdateCombo(GetDlgItem(hDlg, IDC_EVENT_SOURCE), ctype ? eventNamesI : eventNamesP, 0);
			if (ev) {
				ev->state = ctype + 1;
				ev->source = 0;
				RebuildEventList(hDlg);
			}
		} break;
		case IDC_BUT_ADD_EVENT:
			if (mmap) {
				mmap->events.push_back({ (byte)(IsDlgButtonChecked(hDlg, IDC_RADIO_PERF) == BST_CHECKED ? 1 : 2),
					(byte)ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_EVENT_SOURCE)),
					(byte)SendMessage(GetDlgItem(hDlg, IDC_CUTLEVEL), TBM_GETPOS, 0, 0),
					(byte)IsDlgButtonChecked(hDlg, IDC_STATUS_BLINK) == BST_CHECKED	});
				eventID = (int)mmap->events.size() - 1;
				eve->ChangeEffects();
				RebuildEventList(hDlg);
				UpdateZoneAndGrid();
			}
			break;
		case IDC_BUTT_REMOVE_EVENT:
			if (mmap && ev) {
				mmap->events.erase(mmap->events.begin() + eventID);
				if (eventID)
					eventID--;
				eve->ChangeEffects();
				RebuildEventList(hDlg);
				UpdateZoneAndGrid();
			}
			break;
		case IDC_BUTT_EVENT_UP:
			if (mmap && ev && eventID) {
				eventID--;
				swap(*ev, mmap->events[eventID]);
				RebuildEventList(hDlg);
				UpdateZoneAndGrid();
			}
			break;
		case IDC_BUT_EVENT_DOWN:
			if (mmap && ev && eventID < mmap->events.size() - 1) {
				eventID++;
				swap(*ev, mmap->events[eventID]);
				RebuildEventList(hDlg);
				UpdateZoneAndGrid();
			}
			break;
		}
	} break;
	case WM_DRAWITEM: {
		DWORD c = 0xff000000;
		if (mmap && ev) {
			switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
			case IDC_BUTTON_COLORFROM:
				c = MakeRGB(mmap->fromColor && mmap->color.size() ? &mmap->color.front() : &ev->from);
				break;
			case IDC_BUTTON_COLORTO:
				c = MakeRGB(&ev->to);
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
				fxhl->RefreshCounters();
			}
			break;
		} break;
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->code == LVN_ITEMCHANGED) {
			NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
			if (mmap && lPoint->uNewState & LVIS_FOCUSED /*&& lPoint->iItem >= 0*/) {
				// Select other item...
				eventID = lPoint->iItem;
				ev = &mmap->events[eventID];
			}
			else {
				ev = NULL; eventID = 0;
			}
			SetEventData(hDlg);
		}
		break;
	case WM_TIMER: {
		//DebugPrint("Events UI update...\n");
		int maxFans = 0;
		if (mon) {
			for (int i = 0; i < mon->fansize; i++)
				maxFans = max(maxFans, mon->fanRpm[i]);
		}
		//LightEventData* eData = &fxhl->eData, *maxData = &fxhl->maxData;
#define eData fxhl->eData
#define maxData fxhl->maxData
		string usage = to_string(eData.CPU) + " (" + to_string(maxData.CPU) + ")%\n" +
			to_string(eData.RAM) + " (" + to_string(maxData.RAM) + ")%\n" +
			to_string(eData.GPU) + " (" + to_string(maxData.GPU) + ")%\n" +
			to_string(eData.Temp) + " (" + to_string(maxData.Temp) + ")C\n" +
			to_string(eData.Batt) + " %\n" +
			to_string(eData.PWR * maxData.PWR / 100) + " (" + to_string(maxData.PWR) + ")W\n" +
			to_string(maxFans) + " RPM (" + to_string(eData.Fan) + "%)\n" +
			to_string(eData.NET) /*+ " (" + to_string(maxData.NET)*/ + " %";
		SetDlgItemText(hDlg, IDC_VAL_USAGE, usage.c_str());
	} break;
	default: return false;
	}
	return true;
}

