#include "alienfx-gui.h"

extern void SwitchLightTab(HWND, int);
extern bool SetColor(HWND hDlg, int id, groupset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern groupset* CreateMapping(int lid);
extern groupset* FindMapping(int mid);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
//extern int UpdateLightList(HWND light_list, byte flag = 0);

extern BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK TabColorGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void RedrawGridButtonZone(bool recalc = false);
extern void CreateGridBlock(HWND gridTab, DLGPROC);
extern void OnGridSelChanged(HWND);
extern int gridTabSel;

extern int eItem;

extern HWND zsDlg;

void UpdateEventUI(LPVOID);
ThreadHelper* hapUIupdate;

void UpdateMonitoringInfo(HWND hDlg, groupset *map) {
	HWND list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	CheckDlgButton(hDlg, IDC_CHECK_NOEVENT, map && map->fromColor ? BST_CHECKED : BST_UNCHECKED );

	CheckDlgButton(hDlg, IDC_CHECK_PERF, BST_CHECKED);
	CheckDlgButton(hDlg, IDC_GAUGE, map && map->perfs.size() && map->perfs[0].mode ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(s1_slider, TBM_SETPOS, true, map && map->perfs.size() ? map->perfs[0].cut : 0);
	SetSlider(sTip1, map && map->perfs.size() ? map->perfs[0].cut : 0);
	ComboBox_SetCurSel(list_counter, map && map->perfs.size() ? map->perfs[0].source : 0);

	CheckDlgButton(hDlg, IDC_CHECK_STATUS, BST_CHECKED);
	CheckDlgButton(hDlg, IDC_STATUS_BLINK, map && map->events.size() && map->events[0].blink ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(s2_slider, TBM_SETPOS, true, map && map->events.size() ? map->events[0].cut : 0);
	SetSlider(sTip2, map && map->events.size() ? map->events[0].cut : 0);
	ComboBox_SetCurSel(list_status, map && map->events.size() ? map->events[0].source : 0);

	CheckDlgButton(hDlg, IDC_CHECK_POWER, map && map->powers.size() ? BST_CHECKED : BST_UNCHECKED);

	for (int bId = 0; bId < 6; bId++)
		RedrawWindow(GetDlgItem(hDlg, IDC_BUTTON_CM1 + bId), NULL, NULL, RDW_INVALIDATE);
	RedrawGridButtonZone(true);
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND //light_list = GetDlgItem(hDlg, IDC_LIGHTS_E),
		gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID),
		list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	groupset* map = FindMapping(eItem);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		zsDlg = CreateDialog(hInst, (LPSTR)IDD_ZONESELECTION, hDlg, (DLGPROC)ZoneSelectionDialog);
		RECT mRect;
		GetWindowRect(GetDlgItem(hDlg, IDC_STATIC_ZONES_E), &mRect);
		ScreenToClient(hDlg, (LPPOINT)&mRect);
		SetWindowPos(zsDlg, NULL, mRect.left, mRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);

		if (!conf->afx_dev.GetMappings()->size())
			OnGridSelChanged(gridTab);

		// Set counter list...
		char buffer[32];
		LoadString(hInst, IDS_CPU, buffer, 32);
		ComboBox_AddString(list_counter, buffer);
		LoadString(hInst, IDS_RAM, buffer, 32);
		ComboBox_AddString(list_counter, buffer);
		LoadString(hInst, IDS_HDD, buffer, 32);
		ComboBox_AddString(list_counter, buffer);
		LoadString(hInst, IDS_GPU, buffer, 32);
		ComboBox_AddString(list_counter, buffer);
		LoadString(hInst, IDS_NET, buffer, 32);
		ComboBox_AddString(list_counter, buffer);
		LoadString(hInst, IDS_TEMP, buffer, 32);
		ComboBox_AddString(list_counter, buffer);
		LoadString(hInst, IDS_BATT, buffer, 32);
		ComboBox_AddString(list_counter, buffer);

		LoadString(hInst, IDS_FANS, buffer, 32);
		ComboBox_AddString(list_counter, buffer);

		LoadString(hInst, IDS_PWR, buffer, 32);
		ComboBox_AddString(list_counter, buffer);

		LoadString(hInst, IDS_A_HDD, buffer, 32);
		ComboBox_AddString(list_status, buffer);
		LoadString(hInst, IDS_A_NET, buffer, 32);
		ComboBox_AddString(list_status, buffer);
		LoadString(hInst, IDS_A_HOT, buffer, 32);
		ComboBox_AddString(list_status, buffer);
		LoadString(hInst, IDS_A_OOM, buffer, 32);
		ComboBox_AddString(list_status, buffer);
		LoadString(hInst, IDS_A_LOWBATT, buffer, 32);
		ComboBox_AddString(list_status, buffer);
		LoadString(hInst, IDS_A_LOCALE, buffer, 32);
		ComboBox_AddString(list_status, buffer);
		// Set sliders
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 100));
		SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 100));

		SendMessage(s1_slider, TBM_SETTICFREQ, 10, 0);
		SendMessage(s2_slider, TBM_SETTICFREQ, 10, 0);
		sTip1 = CreateToolTip(s1_slider, sTip1);
		sTip2 = CreateToolTip(s2_slider, sTip2);

		CreateGridBlock(gridTab, (DLGPROC)TabColorGrid);
		TabCtrl_SetCurSel(gridTab, gridTabSel);
		OnGridSelChanged(gridTab);

		//UpdateMonitoringInfo(hDlg, map);

		// Start UI update thread...
		hapUIupdate = new ThreadHelper(UpdateEventUI, hDlg);

		//if (eItem >= 0) {
		//	SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS_E, LBN_SELCHANGE), (LPARAM)light_list);
		//}

	} break;
	case WM_APP + 2: {
		map = FindMapping(eItem);
		UpdateMonitoringInfo(hDlg, map);
		//RedrawGridButtonZone();
	} break;
	case WM_COMMAND: {
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		//case IDC_LIGHTS_E:
		//	switch (HIWORD(wParam))
		//	{
		//	case LBN_SELCHANGE:
		//		eItem = (int)ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
		//		map = FindMapping(eItem);
		//		UpdateMonitoringInfo(hDlg, map);
		//		break;
		//	} break;
		case IDC_CHECK_NOEVENT:
			if (map) {
				map->fromColor = state;
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_PERF:
			if (eItem >= 0) {
				if (!map)
					map = CreateMapping(eItem);
				if (state)
					map->perfs.push_back({ 0 });
				else
					map->perfs.clear();
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_POWER:
			if (eItem >= 0) {
				if (!map)
					map = CreateMapping(eItem);
				if (state)
					map->powers.push_back({ 0 });
				else
					map->powers.clear();
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_STATUS:
			if (eItem >= 0) {
				if (!map)
					map = CreateMapping(eItem);
				if (state)
					map->events.push_back({ 0 });
				else
					map->events.clear();
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_STATUS_BLINK:
			if (map && map->events.size())
				map->events[0].blink = state;
			break;
		case IDC_GAUGE:
			if (map && map->perfs.size()) {
				map->perfs[0].mode = state;
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM1:
			if (map && map->powers.size() && !map->fromColor) {
				SetColor(hDlg, LOWORD(wParam), map, &map->powers[0].from);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM4:
			if (map && map->powers.size()) {
				SetColor(hDlg, LOWORD(wParam), map, &map->powers[0].to);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM2:
			if (map && map->perfs.size() && !map->fromColor) {
				SetColor(hDlg, LOWORD(wParam), map, &map->perfs[0].from);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM5:
			if (map && map->perfs.size()) {
				SetColor(hDlg, LOWORD(wParam), map, &map->perfs[0].to);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM3:
			if (map && map->events.size() && !map->fromColor) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[0].from);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM6:
			if (map && map->events.size()) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[0].to);
				fxhl->RefreshMon();
			}
			break;
		case IDC_COUNTERLIST:
			if (map && map->perfs.size() && HIWORD(wParam) == CBN_SELCHANGE) {
				map->perfs[0].source = ComboBox_GetCurSel(list_counter);
				fxhl->RefreshMon();
			}
			break;
		case IDC_STATUSLIST:
			if (map && map->events.size() && HIWORD(wParam) == CBN_SELCHANGE) {
				map->events[0].source = ComboBox_GetCurSel(list_status);
				fxhl->RefreshMon();
			}
			break;
		}
	} break;
	case WM_DRAWITEM:
		if (map) {
			AlienFX_SDK::Colorcode* c{ NULL };
			switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
			case IDC_BUTTON_CM1:
				c = map->powers.size() ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->powers[0].from) : NULL;
				break;
			case IDC_BUTTON_CM4:
				c = map->powers.size() ? Act2Code(&map->powers[0].to) : NULL;
				break;
			case IDC_BUTTON_CM2:
				c = map->perfs.size() ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->perfs[0].from) : NULL;
				break;
			case IDC_BUTTON_CM5:
				c = map->perfs.size() ? Act2Code(&map->perfs[0].to) : NULL;
				break;
			case IDC_BUTTON_CM3:
				c = map->events.size() ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->events[0].from) : NULL;
				break;
			case IDC_BUTTON_CM6:
				c = map->events.size() ? Act2Code(&map->events[0].to) : NULL;
				break;
			}
			RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c);
		}
		break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (map) {
				if ((HWND)lParam == s1_slider) {
					map->perfs[0].cut = (BYTE) SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, map->perfs[0].cut);
				}
				if ((HWND)lParam == s2_slider) {
					map->events[0].cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, map->events[0].cut);
				}
				fxhl->RefreshMon();
			}
			break;
		} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_COLOR_GRID: {
			switch (((NMHDR*)lParam)->code) {
			case TCN_SELCHANGE: {
				int newSel = TabCtrl_GetCurSel(gridTab);
				if (newSel != gridTabSel) { // selection changed!
					if (newSel < conf->afx_dev.GetGrids()->size())
						OnGridSelChanged(gridTab);
				}
			} break;
			}
		} break;
		}
		break;
	case WM_DESTROY:
		DestroyWindow(zsDlg);
		delete hapUIupdate;
		break;
	default: return false;
	}
	return true;
}

void UpdateEventUI(LPVOID lpParam) {
	if (IsWindowVisible((HWND)lpParam)) {
		//DebugPrint("Events UI update...\n");
		SetDlgItemText((HWND)lpParam, IDC_VAL_CPU, (to_string(fxhl->eData.CPU) + " (" + to_string(fxhl->maxData.CPU) + ")%").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_RAM, (to_string(fxhl->eData.RAM) + " (" + to_string(fxhl->maxData.RAM) + ")%").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_GPU, (to_string(fxhl->eData.GPU) + " (" + to_string(fxhl->maxData.GPU) + ")%").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_PWR, (to_string(fxhl->eData.PWR * fxhl->maxData.PWR / 100) + " W").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_FAN, (to_string(fxhl->eData.Fan * fxhl->maxData.Fan / 100) + " RPM").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_BAT, (to_string(fxhl->eData.Batt) + " %").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_NET, (to_string(fxhl->eData.NET * fxhl->maxData.NET / 102400) + " kb").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_TEMP, (to_string(fxhl->eData.Temp) + " (" + to_string(fxhl->maxData.Temp) + ")C").c_str());
	}
}
