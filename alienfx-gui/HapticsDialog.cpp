#include "alienfx-gui.h"
#include "EventHandler.h"
#include "Common.h"

extern void SwitchLightTab(HWND, int);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
//extern void SetSlider(HWND tt, int value);
extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);

extern groupset* CreateMapping(int lid);
extern groupset* FindMapping(int mid);
extern void RemoveUnused(vector<groupset>*);

extern BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK TabColorGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void CreateGridBlock(HWND gridTab, DLGPROC, bool is = false);
extern void OnGridSelChanged(HWND);
extern AlienFX_SDK::mapping* FindCreateMapping();
extern void RedrawGridButtonZone(bool recalc = false);

extern int gridTabSel;
extern HWND zsDlg;

extern EventHandler* eve;
extern int eItem;

int fGrpItem = -1;
HWND hToolTip = NULL;

void UpdateHapticsUI(LPVOID);
ThreadHelper* hapUIThread;

void DrawFreq(HWND hDlg) {
	//unsigned rectop;

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
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		SetBkMode(hdc, TRANSPARENT);

		graphZone.top = 2;
		graphZone.left = 1;
		graphZone.bottom--;

		groupset* map = FindMapping(eItem);
		for (int i = 0; i < NUMBARS; i++) {
			int leftpos = (graphZone.right * i) / NUMBARS + graphZone.left,
				rightpos = (graphZone.right * (i + 1)) / NUMBARS - 2 + graphZone.left;
			if (map && fGrpItem >= 0) {
				if (find_if(map->haptics[fGrpItem].freqID.begin(), map->haptics[fGrpItem].freqID.end(),
					[i](auto t) {
						return t == i;
					}) != map->haptics[fGrpItem].freqID.end()) {
					SelectObject(hdc, GetStockObject(GRAY_BRUSH));
					SelectObject(hdc, GetStockObject(NULL_PEN));
					Rectangle(hdc, leftpos, graphZone.top, rightpos+2, graphZone.bottom);
					SetDCPenColor(hdc, RGB(255, 0, 0));
					SelectObject(hdc, GetStockObject(DC_PEN));
					int cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * map->haptics[fGrpItem].hicut / 255);
					MoveToEx(hdc, leftpos, cutLevel, NULL);
					LineTo(hdc, rightpos, cutLevel);
					SetDCPenColor(hdc, RGB(0, 255, 0));
					SelectObject(hdc, GetStockObject(DC_PEN));
					cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * map->haptics[fGrpItem].lowcut / 255);
					MoveToEx(hdc, leftpos, cutLevel, NULL);
					LineTo(hdc, rightpos, cutLevel);
					SelectObject(hdc, GetStockObject(WHITE_PEN));
					SelectObject(hdc, GetStockObject(WHITE_BRUSH));
				}
			}
			if (eve->audio && eve->audio->freqs) {
				int rectop = (255 - eve->audio->freqs[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top;
				Rectangle(hdc, leftpos, rectop, rightpos, graphZone.bottom);
				//Rectangle(hdc, (graphZone.right * (i + 1)) / NUMBARS - 2 + graphZone.left, graphZone.bottom - 5,
				//	(graphZone.right * (i + 1)) / NUMBARS - 1 + graphZone.left, graphZone.bottom);
			}
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

void SetMappingData(HWND hDlg, groupset* map) {
	RedrawButton(hDlg, IDC_BUTTON_LPC, map && map->haptics.size() ? &map->haptics[fGrpItem].colorfrom : NULL);
	RedrawButton(hDlg, IDC_BUTTON_HPC, map && map->haptics.size() ? &map->haptics[fGrpItem].colorto : NULL);
}

void SetFeqGroups(HWND hDlg) {
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP);
	groupset* map = FindMapping(eItem);
	fGrpItem = -1;
	if (map) {
		// Set groups
		ListBox_ResetContent(grp_list);
		for (int j = 0; j < map->haptics.size(); j++) {
			ListBox_AddString(grp_list, ("Group " + to_string(j + 1)).c_str());
		}
		if (map->haptics.size()) {
			ListBox_SetCurSel(grp_list, 0);
			fGrpItem = 0;
		}
	}
	SetMappingData(hDlg, map);
}

int cutMove = 0;

INT_PTR CALLBACK FreqLevels(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	groupset* map = FindMapping(eItem);
	RECT pos;

	GetClientRect(hDlg, &pos);
	pos.right--;
	pos.bottom -= 3;
	byte cIndex = (byte)((GET_X_LPARAM(lParam)+1) * 20 / pos.right);

	switch (message) {
	case WM_LBUTTONDOWN: {
		if (map && map->haptics.size() && fGrpItem >= 0) {
			auto idx = find_if(map->haptics[fGrpItem].freqID.begin(), map->haptics[fGrpItem].freqID.end(),
				[cIndex](auto t) {
					return cIndex == t;
				});
			if (idx != map->haptics[fGrpItem].freqID.end()) {
				// ToDo - bug with vertical position (some pixels lower)
				int temp = GET_Y_LPARAM(lParam);
				int clickLevel = (pos.bottom - GET_Y_LPARAM(lParam) + 2) * 255 / pos.bottom;
				//int cutLevel = pos.bottom - ((pos.bottom - pos.top) * map->haptics[fGrpItem].hicut / 256);
				if (abs(clickLevel - map->haptics[fGrpItem].hicut) < 5)
					cutMove = 1;
				if (abs(clickLevel - map->haptics[fGrpItem].lowcut) < 5)
					cutMove = 2;
			}
		}
	} break;
	case WM_LBUTTONUP: {
		if (map && map->haptics.size() && fGrpItem >= 0) {
			auto idx = find_if(map->haptics[fGrpItem].freqID.begin(), map->haptics[fGrpItem].freqID.end(),
				[cIndex](auto t) {
					return cIndex == t;
				});
			if (idx != map->haptics[fGrpItem].freqID.end())
				switch (cutMove) {
				case 0: map->haptics[fGrpItem].freqID.erase(idx); break;
				case 1: map->haptics[fGrpItem].hicut = (pos.bottom - GET_Y_LPARAM(lParam) - 2) * 256 / pos.bottom; break;
				case 2: map->haptics[fGrpItem].lowcut = (pos.bottom - GET_Y_LPARAM(lParam) - 2) * 256 / pos.bottom; break;
				}
			else
				map->haptics[fGrpItem].freqID.push_back(cIndex);
			DrawFreq(GetParent(hDlg));
		}
		cutMove = 0;
	} break;
	case WM_MOUSEMOVE: {
		if (wParam & MK_LBUTTON) {
			// cut drag...
			DrawFreq(GetParent(hDlg));
		}
		SetToolTip(hToolTip, "Freq: " + to_string(cIndex ? 22050 - (int)round((log(21 - cIndex) * 22030 / log(21))) : 20) +
			"-" + to_string(22050 - (int)round((log(20 - cIndex) * 22030 / log(21)))) + " Hz, Level: " +
			to_string(100 - ((GET_Y_LPARAM(lParam)+2) * 100 / pos.bottom)));
	} break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hDlg, &ps);
		DrawFreq(GetParent(hDlg));
		EndPaint(hDlg, &ps);
		return true;
	} break;
	case WM_ERASEBKGND:
		return true;
		break;
	}
	return DefWindowProc(hDlg, message, wParam, lParam);
}

BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	//HWND freq_list = GetDlgItem(hDlg, IDC_FREQ);
	//HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP);
	HWND /*hLowSlider = GetDlgItem(hDlg, IDC_SLIDER_LOWCUT);
	HWND hHiSlider = GetDlgItem(hDlg, IDC_SLIDER_HICUT),*/
		gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID);

	groupset *map = FindMapping(eItem);

	switch (message) {
	case WM_INITDIALOG:
	{
		zsDlg = CreateDialog(hInst, (LPSTR)IDD_ZONESELECTION, hDlg, (DLGPROC)ZoneSelectionDialog);
		RECT mRect;
		GetWindowRect(GetDlgItem(hDlg, IDC_STATIC_ZONES), &mRect);
		ScreenToClient(hDlg, (LPPOINT)&mRect);
		SetWindowPos(zsDlg, NULL, mRect.left, mRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);

		if (!conf->afx_dev.GetMappings()->size())
			OnGridSelChanged(gridTab);

		//SendMessage(hLowSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		//SendMessage(hHiSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));

		//SendMessage(hLowSlider, TBM_SETTICFREQ, 16, 0);
		//SendMessage(hHiSlider, TBM_SETTICFREQ, 16, 0);

		//sTip1 = CreateToolTip(hLowSlider, sTip1);
		//sTip2 = CreateToolTip(hHiSlider, sTip2);

		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);

		//CheckDlgButton(hDlg, IDC_SHOWAXIS, conf->hap_conf->showAxis ? BST_CHECKED : BST_UNCHECKED);

		SetWindowLongPtr(GetDlgItem(hDlg, IDC_LEVELS), GWLP_WNDPROC, (LONG_PTR)FreqLevels);
		hToolTip = CreateToolTip(GetDlgItem(hDlg, IDC_LEVELS), hToolTip);

		// init grids...
		CreateGridBlock(gridTab, (DLGPROC)TabColorGrid);
		TabCtrl_SetCurSel(gridTab, gridTabSel);
		OnGridSelChanged(gridTab);

		SetFeqGroups(hDlg);

		// Start UI update thread...
		hapUIThread = new ThreadHelper(UpdateHapticsUI, hDlg, 40);
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
		case IDC_FREQ_GROUP: // should reload mappings
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				if (map) fGrpItem = ListBox_GetCurSel(grp_list);
				DrawFreq(hDlg);
				SetMappingData(hDlg, map);
			break;
			}
			break;
		case IDC_BUT_ADD_GROUP:
		{
			freq_map tMap;
			if (!map) {
				map = CreateMapping(eItem);
			}
			map->haptics.push_back({});
			fGrpItem = ListBox_AddString(grp_list, ("Group " + to_string(map->haptics.size() + 1)).c_str());
			ListBox_SetCurSel(grp_list, fGrpItem);
			DrawFreq(hDlg);
			SetMappingData(hDlg, map);
		} break;
		case IDC_BUT_REM_GROUP:
			if (map && fGrpItem >= 0) {
				map->haptics.erase(map->haptics.begin() + fGrpItem);
				ListBox_DeleteString(grp_list, fGrpItem);
				fGrpItem--;
				if (map->haptics.empty())
					RemoveUnused(conf->active_set);
				ListBox_SetCurSel(grp_list, fGrpItem);
				DrawFreq(hDlg);
				SetMappingData(hDlg, map);
			}
			break;
		case IDC_BUTTON_LPC:
			if (map) {
				SetColor(hDlg, IDC_BUTTON_LPC, &map->haptics[fGrpItem].colorfrom);
			}
			break;
		case IDC_BUTTON_HPC:
			if (map) {
				SetColor(hDlg, IDC_BUTTON_HPC, &map->haptics[fGrpItem].colorto);
			}
			break;
		case IDC_BUTTON_REMOVE:
			if (map) {
				map->haptics[fGrpItem].freqID.clear();
				DrawFreq(hDlg);
			}
		}
	} break;
	case WM_APP + 2: {
		SetFeqGroups(hDlg);
		DrawFreq(hDlg);
		RedrawGridButtonZone();
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		case IDC_BUTTON_LPC: case IDC_BUTTON_HPC:
		{
			AlienFX_SDK::Colorcode* c{NULL};
			if (map && fGrpItem >= 0)
				if (((DRAWITEMSTRUCT *) lParam)->CtlID == IDC_BUTTON_LPC)
					c = &map->haptics[fGrpItem].colorfrom;
				else
					c = &map->haptics[fGrpItem].colorto;
			RedrawButton(hDlg, ((DRAWITEMSTRUCT *) lParam)->CtlID, c);
			return true;
		}
		//case IDC_LEVELS:
		//	DrawFreq(hDlg);
		//	return true;
		}
		return false;
	break;
	case WM_CLOSE: case WM_DESTROY:
		DestroyWindow(zsDlg);
		delete hapUIThread;
	break;
	default: return false;
	}

	return TRUE;
}

void UpdateHapticsUI(LPVOID lpParam) {
	if (eve->audio && IsWindowVisible((HWND)lpParam)) {
		//DebugPrint("Haptics UI update...\n");
		DrawFreq((HWND)lpParam);
	}
}