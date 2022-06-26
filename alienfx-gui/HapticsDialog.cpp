#include "alienfx-gui.h"
#include "EventHandler.h"
#include "Common.h"

//extern void SwitchLightTab(HWND, int);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);

extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void RemoveUnused(vector<groupset>*);

extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);

extern EventHandler* eve;
extern int eItem;

int fGrpItem = -1;
HWND hToolTip = NULL;

void UpdateHapticsUI(LPVOID);
ThreadHelper* hapUIThread;

void DrawFreq(HWND hDlg) {

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
		if (map && fGrpItem >= 0) {
			SelectObject(hdc, GetStockObject(GRAY_BRUSH));
			SelectObject(hdc, GetStockObject(NULL_PEN));
			for (auto t = map->haptics[fGrpItem].freqID.begin(); t < map->haptics[fGrpItem].freqID.end(); t++) {
				int leftpos = (graphZone.right * (*t)) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (*t + 1)) / NUMBARS - 2 + graphZone.left;
				Rectangle(hdc, leftpos, graphZone.top, rightpos + 2, graphZone.bottom);
			}
			SelectObject(hdc, GetStockObject(WHITE_PEN));
			SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		}
		if (eve->audio && eve->audio->freqs) {
			SelectObject(hdc, GetStockObject(WHITE_PEN));
			SelectObject(hdc, GetStockObject(WHITE_BRUSH));
			for (int i = 0; i < NUMBARS; i++) {
				int leftpos = (graphZone.right * i) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (i + 1)) / NUMBARS - 2 + graphZone.left;
				int rectop = (255 - eve->audio->freqs[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top;
				Rectangle(hdc, leftpos, (255 - eve->audio->freqs[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top,
					rightpos, graphZone.bottom);
			}
		}
		if (map && fGrpItem >= 0) {
			for (auto t = map->haptics[fGrpItem].freqID.begin(); t < map->haptics[fGrpItem].freqID.end(); t++) {
				int leftpos = (graphZone.right * (*t)) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (*t + 1)) / NUMBARS - 2 + graphZone.left;
				SetDCPenColor(hdc, RGB(255, 0, 0));
				SelectObject(hdc, GetStockObject(DC_PEN));
				int cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * map->haptics[fGrpItem].hicut / 255);
				Rectangle(hdc, leftpos, cutLevel, rightpos, cutLevel + 2);
				SetDCPenColor(hdc, RGB(0, 255, 0));
				SelectObject(hdc, GetStockObject(DC_PEN));
				cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * map->haptics[fGrpItem].lowcut / 255);
				Rectangle(hdc, leftpos, cutLevel, rightpos, cutLevel - 2);
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

void SetFreqGroups(HWND hDlg) {
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
	RECT pos;
	groupset* map = FindMapping(eItem);
	GetClientRect(hDlg, &pos);
	pos.right--;
	pos.bottom;
	byte cIndex = (byte)(GET_X_LPARAM(lParam) * NUMBARS / pos.right);
	int clickLevel = min((pos.bottom - GET_Y_LPARAM(lParam)) * 255 / (pos.bottom - 3), 255);

	switch (message) {
	case WM_LBUTTONDOWN: {
		if (map && map->haptics.size() && fGrpItem >= 0) {
			auto idx = find_if(map->haptics[fGrpItem].freqID.begin(), map->haptics[fGrpItem].freqID.end(),
				[cIndex](auto t) {
					return cIndex == t;
				});
			if (idx != map->haptics[fGrpItem].freqID.end()) {
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
				case 1: map->haptics[fGrpItem].hicut = clickLevel; break;
				case 2: map->haptics[fGrpItem].lowcut = clickLevel; break;
				}
			else
				map->haptics[fGrpItem].freqID.push_back(cIndex);
			cutMove = 0;
			DrawFreq(GetParent(hDlg));
		}
		cutMove = 0;
	} break;
	case WM_MOUSEMOVE: {
		if (wParam & MK_LBUTTON) {
			switch (cutMove) {
			//case 0: cutMove = 3;
			case 1: map->haptics[fGrpItem].hicut = clickLevel; break;
			case 2: map->haptics[fGrpItem].lowcut = clickLevel; break;
			}
			DrawFreq(GetParent(hDlg));
		}
		SetToolTip(hToolTip, "Freq: " + to_string(cIndex ? 22050 - (int)round((log(21 - cIndex) * 22030 / log(21))) : 20) +
			"-" + to_string(22050 - (int)round((log(20 - cIndex) * 22030 / log(21)))) + " Hz, Level: " +
			to_string(clickLevel * 100 / 255));
	} break;
	case WM_PAINT:
		DrawFreq(GetParent(hDlg));
		break;
	}
	return DefWindowProc(hDlg, message, wParam, lParam);
}

BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP),
	     gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID);

	groupset *map = FindMapping(eItem);

	switch (message) {
	case WM_INITDIALOG:
	{
		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_inpType ? BST_UNCHECKED : BST_CHECKED);

		SetWindowLongPtr(GetDlgItem(hDlg, IDC_LEVELS), GWLP_WNDPROC, (LONG_PTR)FreqLevels);
		hToolTip = CreateToolTip(GetDlgItem(hDlg, IDC_LEVELS), hToolTip);

		SetFreqGroups(hDlg);

		// Start UI update thread...
		hapUIThread = new ThreadHelper(UpdateHapticsUI, hDlg, 40);
	}
	break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDC_RADIO_OUTPUT: case IDC_RADIO_INPUT:
			conf->hap_inpType = LOWORD(wParam) == IDC_RADIO_INPUT;
			CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_inpType ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_inpType ? BST_UNCHECKED : BST_CHECKED);
			if (eve->audio)
				eve->audio->RestartDevice(conf->hap_inpType);
		break;
		case IDC_FREQ_GROUP:
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
			if (map) {
				map->haptics.push_back({});
				fGrpItem = ListBox_AddString(grp_list, ("Group " + to_string(map->haptics.size())).c_str());
				ListBox_SetCurSel(grp_list, fGrpItem);
				DrawFreq(hDlg);
				SetMappingData(hDlg, map);
			}
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
				RedrawGridButtonZone(NULL, true);
			}
			break;
		case IDC_BUTTON_HPC:
			if (map) {
				SetColor(hDlg, IDC_BUTTON_HPC, &map->haptics[fGrpItem].colorto);
				RedrawGridButtonZone(NULL, true);
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
		SetFreqGroups(hDlg);
		DrawFreq(hDlg);
		//RedrawGridButtonZone(NULL, true);
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
		}
		return false;
	break;
	case WM_CLOSE: case WM_DESTROY:
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