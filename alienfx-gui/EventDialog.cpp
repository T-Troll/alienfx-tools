#include "alienfx-gui.h"

VOID OnSelChanged(HWND hwndDlg);
bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map);
lightset* CreateMapping(int lid);
lightset* FindMapping(int mid);

extern int eItem;

void UpdateMonitoringInfo(HWND hDlg, lightset* map) {
	HWND list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	for (int i = 0; i < 4; i++) {
		CheckDlgButton(hDlg, IDC_CHECK_NOEVENT + i, (map ? map->eve[i].fs.b.flags : 0) ? BST_CHECKED : BST_UNCHECKED);

		if (i > 0) {
			if (map && map->eve[0].fs.b.flags) {
				RedrawButton(hDlg, IDC_BUTTON_CM1 + i - 1, map->eve[0].map[0].r,
							 map->eve[0].map[0].g, map->eve[0].map[0].b);
			}
			else {
				RedrawButton(hDlg, IDC_BUTTON_CM1 + i - 1, map ? map->eve[i].map[0].r : 0,
							 map ? map->eve[i].map[0].g : 0, map ? map->eve[i].map[0].b : 0);
			}
			RedrawButton(hDlg, IDC_BUTTON_CM4 + i - 1, map ? map->eve[i].map[1].r : 0,
						 map ? map->eve[i].map[1].g : 0, map ? map->eve[i].map[1].b : 0);
		}
	}

	// Alarms
	CheckDlgButton(hDlg, IDC_STATUS_BLINK, (map?map->eve[3].fs.b.proc:0) ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(s2_slider, TBM_SETPOS, true, map?map->eve[3].fs.b.cut:0);
	SetSlider(lTip, map?map->eve[3].fs.b.cut:0);

	// Events
	CheckDlgButton(hDlg, IDC_GAUGE, (map?map->eve[2].fs.b.proc:0) ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(s1_slider, TBM_SETPOS, true, map?map->eve[2].fs.b.cut:0);
	SetSlider(sTip, map?map->eve[2].fs.b.cut:0);

	SendMessage(list_counter, CB_SETCURSEL, map?map->eve[2].source:0, 0);
	SendMessage(list_status, CB_SETCURSEL, map?map->eve[3].source:0, 0);
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS_E),
		mode_light = GetDlgItem(hDlg, IDC_CHECK_NOEVENT),
		mode_power = GetDlgItem(hDlg, IDC_CHECK_POWER),
		mode_perf = GetDlgItem(hDlg, IDC_CHECK_PERF),
		mode_status = GetDlgItem(hDlg, IDC_CHECK_STATUS),
		list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0),
		lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);

	lightset* map = FindMapping(lid);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (UpdateLightList<FXHelper>(light_list, fxhl) < 0) {
			HWND tab_list = GetParent(hDlg);
			TabCtrl_SetCurSel(tab_list, 2);
			OnSelChanged(tab_list);
			return false;
		}

		// Set counter list...
		char buffer[100];
		LoadString(hInst, IDS_CPU, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_RAM, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_HDD, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_GPU, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_NET, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_TEMP, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_BATT, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		// Set indicator list
		LoadString(hInst, IDS_A_HDD, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_NET, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_HOT, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_OOM, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_LOWBATT, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		// Set sliders
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 99));
		SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 99));
		//TBM_SETTICFREQ
		SendMessage(s1_slider, TBM_SETTICFREQ, 10, 0);
		SendMessage(s2_slider, TBM_SETTICFREQ, 10, 0);
		sTip = CreateToolTip(s1_slider, sTip);
		lTip = CreateToolTip(s2_slider, lTip);

		if (eItem != (-1)) {
			SendMessage(light_list, LB_SETCURSEL, eItem, 0);
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS_E, LBN_SELCHANGE), (LPARAM)hDlg);
		}
	} break;
	case WM_COMMAND: {
		int countid = (int)SendMessage(list_counter, CB_GETCURSEL, 0, 0),
			statusid = (int)SendMessage(list_status, CB_GETCURSEL, 0, 0);
		switch (LOWORD(wParam))
		{
		case IDC_LIGHTS_E: // should reload mappings
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				eItem = lbItem;
				UpdateMonitoringInfo(hDlg, map);
				break;
			} break;
		case IDC_CHECK_NOEVENT: case IDC_CHECK_PERF: case IDC_CHECK_POWER: case IDC_CHECK_STATUS:
		{
			int eid = LOWORD(wParam) - IDC_CHECK_NOEVENT;
			if (!(lid < 0)) {
				if (!map) {
					map = CreateMapping(lid);
				}
				map->eve[eid].fs.b.flags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				fxhl->RefreshMon();
				UpdateMonitoringInfo(hDlg, map);
			} else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
		} break;
		case IDC_STATUS_BLINK:
			if (map != NULL)
				map->eve[3].fs.b.proc = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_GAUGE:
			if (map != NULL) {
				map->eve[2].fs.b.proc = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: case IDC_BUTTON_CM4: case IDC_BUTTON_CM5: case IDC_BUTTON_CM6: {
			if (map && HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) < IDC_BUTTON_CM4)
					if (map->eve[0].fs.b.flags)
						SetColor(hDlg, LOWORD(wParam), map, &map->eve[0].map[0]);
					else
						SetColor(hDlg, LOWORD(wParam), map, &map->eve[LOWORD(wParam) - IDC_BUTTON_CM1 + 1].map[0]);
				else
					SetColor(hDlg, LOWORD(wParam), map, &map->eve[LOWORD(wParam) - IDC_BUTTON_CM4 + 1].map[1]);
			}
		} break;
		case IDC_COUNTERLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
				map->eve[2].source = countid;
				fxhl->RefreshMon();
			}
			break;
		case IDC_STATUSLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
				map->eve[3].source = statusid;
				fxhl->RefreshMon();
			}
			break;
		}
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: case IDC_BUTTON_CM4: case IDC_BUTTON_CM5: case IDC_BUTTON_CM6: {
			AlienFX_SDK::afx_act c;
			if (map) {
				if (((DRAWITEMSTRUCT*)lParam)->CtlID < IDC_BUTTON_CM4)
					if (map->eve[0].fs.b.flags)
						c = map->eve[0].map[0];
					else
						c = map->eve[((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM1 + 1].map[0];
				else
					c = map->eve[((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM4 + 1].map[1];
			}
			RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.r, c.g, c.b);
		} break;
		}
		break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (map) {
				if ((HWND)lParam == s1_slider) {
					map->eve[2].fs.b.cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip, map->eve[2].fs.b.cut);
				}
				if ((HWND)lParam == s2_slider) {
					map->eve[3].fs.b.cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(lTip, map->eve[3].fs.b.cut);
				}
			}
			break;
		} break;
	default: return false;
	}
	return true;
}