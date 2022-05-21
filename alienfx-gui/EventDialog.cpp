#include "alienfx-gui.h"

extern void SwitchLightTab(HWND, int);
extern bool SetColor(HWND hDlg, int id, colorset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern colorset* CreateMapping(int lid);
extern colorset* FindMapping(int mid);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern int UpdateLightList(HWND light_list, byte flag = 0);

extern int eItem;

void UpdateEventUI(LPVOID);
ThreadHelper* hapUIupdate;

void UpdateMonitoringInfo(HWND hDlg, colorset *map) {
	HWND list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

		CheckDlgButton(hDlg, IDC_CHECK_NOEVENT, map && map->fromColor ? BST_CHECKED : BST_UNCHECKED );
	if (map && map->perf.active) { // Performance
		CheckDlgButton(hDlg, IDC_CHECK_PERF, BST_CHECKED);
		//RedrawButton(hDlg, IDC_BUTTON_CM2, Act2Code(map->fromColor && map->color.size() ?  &map->color[0] : &map->perf.from));
		//RedrawButton(hDlg, IDC_BUTTON_CM3, Act2Code( &map->perf.to ));
		CheckDlgButton(hDlg, IDC_GAUGE, map->perf.proc ? BST_CHECKED : BST_UNCHECKED);
		SendMessage(s1_slider, TBM_SETPOS, true, map->perf.cut);
		SetSlider(sTip1, map->perf.cut);
		ComboBox_SetCurSel(list_counter, map->perf.source);
	}
	if (map && map->events.active) { // events
		CheckDlgButton(hDlg, IDC_CHECK_STATUS, BST_CHECKED);
		//RedrawButton(hDlg, IDC_BUTTON_CM4, Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->events.from));
		//RedrawButton(hDlg, IDC_BUTTON_CM5, Act2Code(&map->events.to));
		CheckDlgButton(hDlg, IDC_STATUS_BLINK, map->events.proc ? BST_CHECKED : BST_UNCHECKED);
		SendMessage(s2_slider, TBM_SETPOS, true, map->events.cut);
		SetSlider(sTip2, map->events.cut);
		ComboBox_SetCurSel(list_status, map->events.source);
	}
	if (map && map->power.active) {
		CheckDlgButton(hDlg, IDC_CHECK_POWER, BST_CHECKED);
		//RedrawButton(hDlg, IDC_BUTTON_CM1, Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->power.from));
		//RedrawButton(hDlg, IDC_BUTTON_CM2, Act2Code(&map->power.to));
	}
	for (int bId = 0; bId < 6; bId++)
		RedrawWindow(GetDlgItem(hDlg, IDC_BUTTON_CM1 + bId), NULL, NULL, RDW_INVALIDATE);
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS_E),
		list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	colorset* map = FindMapping(eItem);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (eItem = UpdateLightList(light_list) < 0) {
			SwitchLightTab(hDlg, TAB_DEVICES);
			return false;
		}

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

		// Start UI update thread...
		hapUIupdate = new ThreadHelper(UpdateEventUI, hDlg);

		if (eItem >= 0) {
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS_E, LBN_SELCHANGE), (LPARAM)light_list);
		}

	} break;
	case WM_COMMAND: {
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDC_LIGHTS_E:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				eItem = (int)ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
				map = FindMapping(eItem);
				UpdateMonitoringInfo(hDlg, map);
				break;
			} break;
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
				map->perf.active = state;
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_POWER:
			if (eItem >= 0) {
				if (!map)
					map = CreateMapping(eItem);
				map->power.active = state;
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_STATUS:
			if (eItem >= 0) {
				if (!map)
					map = CreateMapping(eItem);
				map->events.active = state;
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_STATUS_BLINK:
			if (map && map->events.active)
				map->events.proc = state;
			break;
		case IDC_GAUGE:
			if (map && map->perf.active) {
				map->perf.proc = state;
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM1:
			if (map && map->power.active && !map->fromColor) {
				SetColor(hDlg, LOWORD(wParam), map, &map->power.from);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM4:
			if (map && map->power.active) {
				SetColor(hDlg, LOWORD(wParam), map, &map->power.to);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM2:
			if (map && map->perf.active && !map->fromColor) {
				SetColor(hDlg, LOWORD(wParam), map, &map->perf.from);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM5:
			if (map && map->perf.active) {
				SetColor(hDlg, LOWORD(wParam), map, &map->perf.to);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM3:
			if (map && map->events.active && !map->fromColor) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events.from);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM6:
			if (map && map->events.active) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events.to);
				fxhl->RefreshMon();
			}
			break;
		case IDC_COUNTERLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
				map->perf.source = ComboBox_GetCurSel(list_counter);
				fxhl->RefreshMon();
			}
			break;
		case IDC_STATUSLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
				map->events.source = ComboBox_GetCurSel(list_status);
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
				c = map->power.active ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->power.from) : NULL;
				break;
			case IDC_BUTTON_CM4:
				c = map->power.active ? Act2Code(&map->power.to) : NULL;
				break;
			case IDC_BUTTON_CM2:
				c = map->perf.active ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->perf.from) : NULL;
				break;
			case IDC_BUTTON_CM5:
				c = map->perf.active ? Act2Code(&map->perf.to) : NULL;
				break;
			case IDC_BUTTON_CM3:
				c = map->events.active ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->events.from) : NULL;
				break;
			case IDC_BUTTON_CM6:
				c = map->events.active ? Act2Code(&map->events.to) : NULL;
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
					map->perf.cut = (BYTE) SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, map->perf.cut);
				}
				if ((HWND)lParam == s2_slider) {
					map->events.cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, map->events.cut);
				}
				fxhl->RefreshMon();
			}
			break;
		} break;
	case WM_DESTROY:
		delete hapUIupdate;
		break;
	default: return false;
	}
	return true;
}

void UpdateEventUI(LPVOID lpParam) {
	if (IsWindowVisible((HWND)lpParam)) {
		//DebugPrint("Events UI update...\n");
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_CPU), (to_string(fxhl->eData.CPU) + " (" + to_string(fxhl->maxData.CPU) + ")%").c_str());
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_RAM), (to_string(fxhl->eData.RAM) + " (" + to_string(fxhl->maxData.RAM) + ")%").c_str());
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_GPU), (to_string(fxhl->eData.GPU) + " (" + to_string(fxhl->maxData.GPU) + ")%").c_str());
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_PWR), (to_string(fxhl->eData.PWR * fxhl->maxData.PWR / 100) + " W").c_str());
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_FAN), (to_string(fxhl->eData.Fan * fxhl->maxData.Fan / 100) + " RPM").c_str());
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_BAT), (to_string(fxhl->eData.Batt) + " %").c_str());
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_NET), (to_string(fxhl->eData.NET * fxhl->maxData.NET / 102400) + " kb").c_str());
		Static_SetText(GetDlgItem((HWND)lpParam, IDC_VAL_TEMP), (to_string(fxhl->eData.Temp) + " (" + to_string(fxhl->maxData.Temp) + ")C").c_str());
	}
}
