#include "alienfx-gui.h"
#include "EventHandler.h"

extern void SwitchTab(int);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);
extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);
extern haptics_map *FindHapMapping(int lid);
extern void RemoveHapMapping(haptics_map*);

extern EventHandler* eve;
extern int eItem;

int fGrpItem = -1;

DWORD WINAPI UpdateHapticsUI(LPVOID);

HANDLE auiEvent = CreateEvent(NULL, false, false, NULL), uiHapHandle = NULL;

void DrawFreq(HWND hDlg, int *freq) {
	unsigned i, rectop;
	char szSize[5]; //for labels

	HWND hysto = GetDlgItem(hDlg, IDC_LEVELS);

	if (hysto) {
		RECT levels_rect, graphZone;
		GetClientRect(hysto, &levels_rect);
		graphZone = levels_rect;

		HDC hdc_r = GetDC(hysto);

		// Double buff...
		HDC hdc = CreateCompatibleDC(hdc_r);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, graphZone.right - graphZone.left, graphZone.bottom - graphZone.top);

		SetBkMode(hdc, TRANSPARENT);

		HGDIOBJ hOld = SelectObject(hdc, hbmMem);

		//setting colors:
		SetTextColor(hdc, RGB(255, 255, 255));
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		SetBkMode(hdc, TRANSPARENT);

		if (conf->hap_conf->showAxis) {
			//draw x axis:
			MoveToEx(hdc, 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, levels_rect.right - 10, levels_rect.bottom - 21);
			LineTo(hdc, levels_rect.right - 15, levels_rect.bottom - 26);
			MoveToEx(hdc, levels_rect.right - 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, levels_rect.right - 15, levels_rect.bottom - 16);

			//draw y axis:
			MoveToEx(hdc, 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, 10, 10);
			LineTo(hdc, 15, 15);
			MoveToEx(hdc, 10, 10, (LPPOINT) NULL);
			LineTo(hdc, 5, 15);
			int oldvalue = (-1);
			double coeff = 22 / (log(22.0));
			for (i = 0; i <= 22; i++) {
				int frq = int(22 - round((log(22.0 - i) * coeff)));
				if (frq > oldvalue) {
					wsprintf(szSize, "%2d", frq);
					TextOut(hdc, ((levels_rect.right - 20) * i) / 22 + 10, levels_rect.bottom - 20, szSize, 2);
					oldvalue = frq;
				}
			}
			graphZone.top = 10;
			graphZone.bottom -= 21;
			graphZone.left = 10;
			graphZone.right -= 20;
		} else {
			graphZone.top = 2;
			graphZone.left = 1;
			graphZone.bottom--;
		}
		if (freq)
			for (i = 0; i < NUMBARS; i++) {
				rectop = (255 - freq[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top;
				Rectangle(hdc, (graphZone.right * i) / NUMBARS + graphZone.left, rectop,
						  (graphZone.right * (i + 1)) / NUMBARS - 2 + graphZone.left, graphZone.bottom);
			}

		BitBlt(hdc_r, 0, 0, levels_rect.right - levels_rect.left, levels_rect.bottom - levels_rect.top, hdc, 0, 0, SRCCOPY);

		// Free-up the off-screen DC
		SelectObject(hdc, hOld);

		DeleteObject(hbmMem);
		DeleteDC(hdc);
		ReleaseDC(hysto, hdc_r);
		DeleteDC(hdc_r);
	}
}

void SetMappingData(HWND hDlg, haptics_map* map) {
	HWND hLowSlider = GetDlgItem(hDlg, IDC_SLIDER_LOWCUT);
	HWND hHiSlider = GetDlgItem(hDlg, IDC_SLIDER_HICUT);
	haptics_map hap; hap.freqs.push_back(freq_map());
	if (!map) {
		map = &hap;
		fGrpItem = 0;
	}
	RedrawButton(hDlg, IDC_BUTTON_LPC, map->freqs[fGrpItem].colorfrom);
	RedrawButton(hDlg, IDC_BUTTON_HPC, map->freqs[fGrpItem].colorto);
	// load cuts...
	SendMessage(hLowSlider, TBM_SETPOS, true, map->freqs[fGrpItem].lowcut);
	SendMessage(hHiSlider, TBM_SETPOS, true, map->freqs[fGrpItem].hicut);
	SetSlider(sTip1, map->freqs[fGrpItem].lowcut);
	SetSlider(sTip2, map->freqs[fGrpItem].hicut);
	CheckDlgButton(hDlg, IDC_GAUGE, map->flags & MAP_GAUGE ? BST_CHECKED : BST_UNCHECKED);

}

BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND freq_list = GetDlgItem(hDlg, IDC_FREQ);
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP);
	HWND hLowSlider = GetDlgItem(hDlg, IDC_SLIDER_LOWCUT);
	HWND hHiSlider = GetDlgItem(hDlg, IDC_SLIDER_HICUT);

	haptics_map *map = FindHapMapping(eItem);

	switch (message) {
	case WM_INITDIALOG:
	{
		if (UpdateLightList(light_list, fxhl, 3) < 0) {
			// no lights, switch to setup
			SwitchTab(TAB_DEVICES);
			return false;
		}

		double coeff = 22030 / log(21);
		int prevfreq = 20;
		for (int i = 1; i < 21; i++) {
			int frq = 22050 - (int) round((log(21 - i) * coeff));
			string frqname = to_string(prevfreq) + "-" + to_string(frq) + "Hz";
			prevfreq = frq;
			ListBox_AddString(freq_list, frqname.c_str());
		}

		SendMessage(hLowSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(hHiSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));

		SendMessage(hLowSlider, TBM_SETTICFREQ, 16, 0);
		SendMessage(hHiSlider, TBM_SETTICFREQ, 16, 0);

		sTip1 = CreateToolTip(hLowSlider, sTip1);
		sTip2 = CreateToolTip(hHiSlider, sTip2);

		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);

		CheckDlgButton(hDlg, IDC_SHOWAXIS, conf->hap_conf->showAxis ? BST_CHECKED : BST_UNCHECKED);

		// Start UI update thread...
		uiHapHandle = CreateThread(NULL, 0, UpdateHapticsUI, hDlg, 0, NULL);

		if (eItem >= 0) {
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
		}
	}
	break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDC_RADIO_OUTPUT:
			conf->hap_conf->inpType = 0;
			CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);
			if (eve->audio)
				eve->audio->RestartDevice(conf->hap_conf->inpType);
		break;
		case IDC_RADIO_INPUT:
			conf->hap_conf->inpType = 1;
			CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);
		if (eve->audio)
			eve->audio->RestartDevice(conf->hap_conf->inpType);
		break;
		case IDC_LIGHTS: // should reload mappings
		switch (HIWORD(wParam)) {
		case LBN_SELCHANGE:
		{
			eItem = (int) ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
			map = FindHapMapping(eItem);
			EnableWindow(freq_list, false);
			ListBox_SetSel(freq_list, FALSE, -1);
			ListBox_ResetContent(grp_list);
			fGrpItem = -1;
			if (map) {
				// Set groups
				for (int j = 0; j < map->freqs.size(); j++) {
					ListBox_AddString(grp_list, ("Group " + to_string(j+1)).c_str());
				}
				if (map->freqs.size()) {
					fGrpItem = 0;
					ListBox_SetCurSel(grp_list, fGrpItem);
					SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_FREQ_GROUP, LBN_SELCHANGE), (LPARAM)grp_list);
				}
			}
			SetMappingData(hDlg, map);
		}
		break;
		}
		break;
		case IDC_FREQ_GROUP: // should reload mappings
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
			{
				if (map) {
					fGrpItem = ListBox_GetCurSel(grp_list);
					EnableWindow(freq_list, TRUE);
					ListBox_SetSel(freq_list, FALSE, -1);
					for (int j = 0; j < map->freqs[fGrpItem].freqID.size(); j++) {
						ListBox_SetSel(freq_list, TRUE, map->freqs[fGrpItem].freqID[j]);
					}
				}
				SetMappingData(hDlg, map);
			} break;
			}
			break;
		case IDC_FREQ:
		{ // should update mappings list
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
			{
				if (map && fGrpItem < map->freqs.size()) {
					int fid = ListBox_GetCurSel(freq_list);
					auto Iter = map->freqs[fGrpItem].freqID.begin();
					for (; Iter != map->freqs[fGrpItem].freqID.end(); Iter++)
						if (*Iter == fid)
							break;
					if (Iter == map->freqs[fGrpItem].freqID.end())
						// new mapping, add and select
						map->freqs[fGrpItem].freqID.push_back(fid);
					else
						map->freqs[fGrpItem].freqID.erase(Iter);
				}
			} break;
			}
		} break;
		case IDC_BUT_ADD_GROUP:
		{
			freq_map tMap;
			if (!map) {
				map = new haptics_map();
				if (eItem > 0xffff) {
					// group
					map->lightid = eItem;
				} else {
					// light
					AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(eItem);
					map->devid = lgh->devid;
					map->lightid = lgh->lightid;
				}
				conf->hap_conf->haptics.push_back(*map);
				map = &conf->hap_conf->haptics.back();
			}
			map->freqs.push_back(tMap);
			fGrpItem = ListBox_AddString(grp_list, ("Group " + to_string(map->freqs.size())).c_str());
			ListBox_SetCurSel(grp_list, fGrpItem);
			ListBox_SetSel(freq_list, FALSE, -1);
			SetMappingData(hDlg, map);
			EnableWindow(freq_list, true);
		} break;
		case IDC_BUT_REM_GROUP:
		{
			if (map && fGrpItem >= 0) {
				auto gIter = map->freqs.begin();
				map->freqs.erase(gIter + fGrpItem);
				ListBox_DeleteString(grp_list, fGrpItem);
				if (fGrpItem) fGrpItem--;
				ListBox_SetCurSel(grp_list, fGrpItem);
				if (!map->freqs.size()) {
					// remove mapping
					RemoveHapMapping(map);
					map = NULL; fGrpItem = -1;
					EnableWindow(freq_list, false);
				}
				SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_FREQ_GROUP, LBN_SELCHANGE), (LPARAM)grp_list);
			}
		} break;
		case IDC_BUTTON_LPC:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			if (map) {
				SetColor(hDlg, IDC_BUTTON_LPC, &map->freqs[fGrpItem].colorfrom);
			}
		} break;
		} break;
		case IDC_BUTTON_HPC:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			if (map) {
				SetColor(hDlg, IDC_BUTTON_HPC, &map->freqs[fGrpItem].colorto);
			}
		} break;
		} break;
		case IDC_GAUGE:
		{
			if (map)
				map->flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		} break;
		case IDC_BUTTON_REMOVE:
		{
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
			{
				if (map && fGrpItem >= 0) {
					map->freqs[fGrpItem] = freq_map();
					SetMappingData(hDlg, map);
					ListBox_SetSel(freq_list, FALSE, -1);
				}
			} break;
			}
		} break;
		case IDC_SHOWAXIS:
		{
			conf->hap_conf->showAxis = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
			DrawFreq(hDlg, eve->audio ? eve->audio->freqs : NULL);
		} break;
		}
	} break;
	case WM_HSCROLL:
	{
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
		{
			if (map && fGrpItem >= 0) {
				if ((HWND) lParam == hLowSlider) {
					map->freqs[fGrpItem].lowcut = (UCHAR) SendMessage(hLowSlider, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, map->freqs[fGrpItem].lowcut);
				}
				if ((HWND) lParam == hHiSlider) {
					map->freqs[fGrpItem].hicut = (UCHAR) SendMessage(hHiSlider, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, map->freqs[fGrpItem].hicut);
				}
			}
		} break;
		}
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		case IDC_BUTTON_LPC: case IDC_BUTTON_HPC:
		{
			AlienFX_SDK::Colorcode c{0};
			if (map && fGrpItem >= 0)
				if (((DRAWITEMSTRUCT *) lParam)->CtlID == IDC_BUTTON_LPC)
					c = map->freqs[fGrpItem].colorfrom;
				else
					c = map->freqs[fGrpItem].colorto;
			RedrawButton(hDlg, ((DRAWITEMSTRUCT *) lParam)->CtlID, c);
			return true;
		}
		case IDC_LEVELS:
			DrawFreq(hDlg, eve->audio ? eve->audio->freqs : NULL);
			return true;
		}
		return false;
	break;
	case WM_CLOSE: case WM_DESTROY:
		SetEvent(auiEvent);
		WaitForSingleObject(uiHapHandle, 1000);
		CloseHandle(uiHapHandle);
	break;
	default: return false;
	}

	return TRUE;
}

DWORD WINAPI UpdateHapticsUI(LPVOID lpParam) {

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	while (WaitForSingleObject(auiEvent, 40) == WAIT_TIMEOUT) {
		if (eve->audio && IsWindowVisible((HWND)lpParam)) {
			//DebugPrint("Haptics UI update...\n");
			DrawFreq((HWND)lpParam, eve->audio->freqs);
		}
	}
	return 0;
}