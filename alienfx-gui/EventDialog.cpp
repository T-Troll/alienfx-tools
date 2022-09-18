#include "alienfx-gui.h"
#include "EventHandler.h"

extern bool SetColor(HWND hDlg, int id, groupset* mmap, AlienFX_SDK::afx_act* map);
extern AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act*);
extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);

//extern EventHandler* eve;
extern MonHelper* mon;

extern int eItem;

void UpdateEventUI(LPVOID);
ThreadHelper* hapUIupdate;

void UpdateMonitoringInfo(HWND hDlg, groupset *map) {

	CheckDlgButton(hDlg, IDC_CHECK_NOEVENT, map && map->fromColor ? BST_CHECKED : BST_UNCHECKED );
	bool setState = map && map->events[1].state;
	CheckDlgButton(hDlg, IDC_CHECK_PERF, setState ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(GetDlgItem(hDlg, IDC_MINPVALUE), TBM_SETPOS, true, setState ? map->events[1].cut : 0);
	SetSlider(sTip1, setState ? map->events[1].cut : 0);
	ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COUNTERLIST), setState ? map->events[1].source : 0);
	setState = map && map->events[2].state;
	CheckDlgButton(hDlg, IDC_CHECK_STATUS, setState ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_STATUS_BLINK, setState && map->events[2].mode ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(GetDlgItem(hDlg, IDC_CUTLEVEL), TBM_SETPOS, true, setState ? map->events[2].cut : 0);
	SetSlider(sTip2, setState ? map->events[2].cut : 0);
	ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATUSLIST), setState ? map->events[2].source : 0);

	CheckDlgButton(hDlg, IDC_CHECK_POWER, map && map->events[0].state ? BST_CHECKED : BST_UNCHECKED);

	for (int bId = 0; bId < 6; bId++)
		RedrawWindow(GetDlgItem(hDlg, IDC_BUTTON_CM1 + bId), NULL, NULL, RDW_INVALIDATE);

	RedrawGridButtonZone(NULL, true);
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	groupset* map = FindMapping(eItem);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Set counter list...
		UpdateCombo(list_counter,
			{ "CPU load", "RAM load", "Storage load", "GPU load", "Network traffic", "Max. temperature", "Battery level",
			"Fan RPM", "Power consumption" });
		UpdateCombo(list_status, { "Storage activity", "Network activity", "System overheat", "Out of memory", "Low battery", "Selected language" });

		// Set sliders
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 100));
		SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 100));

		SendMessage(s1_slider, TBM_SETTICFREQ, 10, 0);
		SendMessage(s2_slider, TBM_SETTICFREQ, 10, 0);
		sTip1 = CreateToolTip(s1_slider, sTip1);
		sTip2 = CreateToolTip(s2_slider, sTip2);

		UpdateMonitoringInfo(hDlg, map);

		// Start UI update thread...
		hapUIupdate = new ThreadHelper(UpdateEventUI, hDlg, DEFAULT_MON_DELAY);

	} break;
	case WM_APP + 2: {
		UpdateMonitoringInfo(hDlg, map);
	} break;
	case WM_COMMAND: {
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDC_CHECK_NOEVENT:
			if (map) {
				map->fromColor = state;
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_PERF:
			if (map) {
				map->events[1].state = state;
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_POWER:
			if (map) {
				map->events[0].state = state;
				UpdateMonitoringInfo(hDlg, map);
				fxhl->Refresh();
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_CHECK_STATUS:
			if (map) {
				if (!map->events[2].cut) {
					map->events[2].cut = 90;
				}
				map->events[2].state = state;
				UpdateMonitoringInfo(hDlg, map);
			}
			else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
			break;
		case IDC_STATUS_BLINK:
			if (map)
				map->events[2].mode = state;
			break;
		case IDC_BUTTON_CM1:
			if (map && (!map->fromColor || map->color.size())) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[0].from);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM4:
			if (map) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[0].to);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM2:
			if (map && (!map->fromColor || map->color.size())) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[1].from);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM5:
			if (map) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[1].to);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM3:
			if (map && (!map->fromColor || map->color.size())) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[2].from);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM6:
			if (map) {
				SetColor(hDlg, LOWORD(wParam), map, &map->events[2].to);
				RedrawGridButtonZone(NULL, true);
				fxhl->RefreshMon();
			}
			break;
		case IDC_COUNTERLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
				map->events[1].source = ComboBox_GetCurSel(list_counter);
				fxhl->RefreshMon();
			}
			break;
		case IDC_STATUSLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
				map->events[2].source = ComboBox_GetCurSel(list_status);
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
				c = map->events[0].state ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->events[0].from) : NULL;
				break;
			case IDC_BUTTON_CM4:
				c = map->events[0].state ? Act2Code(&map->events[0].to) : NULL;
				break;
			case IDC_BUTTON_CM2:
				c = map->events[1].state ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->events[1].from) : NULL;
				break;
			case IDC_BUTTON_CM5:
				c = map->events[1].state ? Act2Code(&map->events[1].to) : NULL;
				break;
			case IDC_BUTTON_CM3:
				c = map->events[2].state ? Act2Code(map->fromColor && map->color.size() ? &map->color[0] : &map->events[2].from) : NULL;
				break;
			case IDC_BUTTON_CM6:
				c = map->events[2].state ? Act2Code(&map->events[2].to) : NULL;
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
					SetSlider(sTip1, map->events[1].cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
				}
				if ((HWND)lParam == s2_slider) {
					SetSlider(sTip2, map->events[2].cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
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
		SetDlgItemText((HWND)lpParam, IDC_VAL_CPU, (to_string(fxhl->eData.CPU) + " (" + to_string(fxhl->maxData.CPU) + ")%").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_RAM, (to_string(fxhl->eData.RAM) + " (" + to_string(fxhl->maxData.RAM) + ")%").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_GPU, (to_string(fxhl->eData.GPU) + " (" + to_string(fxhl->maxData.GPU) + ")%").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_PWR, (to_string(fxhl->eData.PWR * fxhl->maxData.PWR / 100) + " (" + to_string(fxhl->maxData.PWR) + ")W").c_str());
		if (mon) {
			int maxFans = 0;
			for (auto i = mon->fanRpm.begin(); i < mon->fanRpm.end(); i++)
				maxFans = max(maxFans, *i);
				SetDlgItemText((HWND)lpParam, IDC_VAL_FAN, (to_string(maxFans) + " RPM (" + to_string(fxhl->eData.Fan) + "%)").c_str());
		}
		else
			SetDlgItemText((HWND)lpParam, IDC_VAL_FAN, "disabled");
		SetDlgItemText((HWND)lpParam, IDC_VAL_BAT, (to_string(fxhl->eData.Batt) + " %").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_NET, (to_string(fxhl->eData.NET) + " %").c_str());
		SetDlgItemText((HWND)lpParam, IDC_VAL_TEMP, (to_string(fxhl->eData.Temp) + " (" + to_string(fxhl->maxData.Temp) + ")C").c_str());
	}
}
