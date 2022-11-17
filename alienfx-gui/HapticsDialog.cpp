#include "alienfx-gui.h"
#include "EventHandler.h"
#include "Common.h"

extern void RedrawButton(HWND hDlg, AlienFX_SDK::Colorcode*);
extern bool SetColor(HWND hDlg, AlienFX_SDK::Colorcode*);
extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);

extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);

extern EventHandler* eve;
extern int eItem;

int fGrpItem = -1;
freq_map* freqBlock = NULL;
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
		freqBlock = (fGrpItem < 0 ? NULL : &map->haptics[fGrpItem]);
		if (map && freqBlock/*fGrpItem >= 0 && fGrpItem < map->haptics.size()*/) {
			SelectObject(hdc, GetStockObject(GRAY_BRUSH));
			SelectObject(hdc, GetStockObject(NULL_PEN));
			for (auto t = freqBlock->freqID.begin(); t < freqBlock->freqID.end(); t++) {
				int leftpos = (graphZone.right * (*t)) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (*t + 1)) / NUMBARS - 2 + graphZone.left;
				Rectangle(hdc, leftpos, graphZone.top, rightpos + 2, graphZone.bottom);
			}
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
		if (map && freqBlock /*&& fGrpItem < map->haptics.size()*/) {
			for (auto t = freqBlock->freqID.begin(); t < freqBlock->freqID.end(); t++) {
				int leftpos = (graphZone.right * (*t)) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (*t + 1)) / NUMBARS - 2 + graphZone.left;
				SetDCPenColor(hdc, RGB(255, 0, 0));
				SelectObject(hdc, GetStockObject(DC_PEN));
				int cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * freqBlock->hicut / 255);
				Rectangle(hdc, leftpos, cutLevel, rightpos, cutLevel + 2);
				SetDCPenColor(hdc, RGB(0, 255, 0));
				SelectObject(hdc, GetStockObject(DC_PEN));
				cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * freqBlock->lowcut / 255);
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
	DrawFreq(hDlg);
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_LPC), map && map->haptics.size() ? &freqBlock->colorfrom : NULL);
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_HPC), map && map->haptics.size() ? &freqBlock->colorto : NULL);
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
	freqBlock = (fGrpItem < 0 ? NULL : &map->haptics[fGrpItem]);
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
			//auto freqBlock = &map->haptics[fGrpItem];
			for (auto idx = freqBlock->freqID.begin(); idx != freqBlock->freqID.end(); idx++)
				if (*idx == cIndex) {
					if (abs(clickLevel - freqBlock->hicut) < 10)
						cutMove = 1;
					if (abs(clickLevel - freqBlock->lowcut) < 10)
						cutMove = 2;
					break;
				}
		}
	} break;
	case WM_LBUTTONUP: {
		if (map && map->haptics.size() && fGrpItem >= 0) {
			//auto freqBlock = &map->haptics[fGrpItem];
			auto idx = find_if(freqBlock->freqID.begin(), freqBlock->freqID.end(),
				[cIndex](auto t) {
					return cIndex == t;
				});
			if (idx != freqBlock->freqID.end())
				switch (cutMove) {
				case 0:
					hapUIThread->Stop();
					freqBlock->freqID.erase(idx);
					hapUIThread->Start();
					break;
				case 1: freqBlock->hicut = clickLevel; break;
				case 2: freqBlock->lowcut = clickLevel; break;
				}
			else
				freqBlock->freqID.push_back(cIndex);
			cutMove = 0;
			DrawFreq(GetParent(hDlg));
		}
		cutMove = 0;
	} break;
	case WM_MOUSEMOVE: {
		if (wParam & MK_LBUTTON) {
			switch (cutMove) {
			//case 0: cutMove = 3;
			case 1: freqBlock->hicut = clickLevel; break;
			case 2: freqBlock->lowcut = clickLevel; break;
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
				if (map) {
					fGrpItem = ListBox_GetCurSel(grp_list);
					//DrawFreq(hDlg);
					SetMappingData(hDlg, map);
				}
			break;
			}
			break;
		case IDC_BUT_ADD_GROUP:
		{
			if (map) {
				map->haptics.push_back({});
				fGrpItem = ListBox_AddString(grp_list, ("Group " + to_string(map->haptics.size())).c_str());
				ListBox_SetCurSel(grp_list, fGrpItem);
				//DrawFreq(hDlg);
				SetMappingData(hDlg, map);
				RedrawGridButtonZone(NULL, true);
			}
		} break;
		case IDC_BUT_REM_GROUP:
			if (map && fGrpItem >= 0) {
				map->haptics.erase(map->haptics.begin() + fGrpItem);
				ListBox_DeleteString(grp_list, fGrpItem);
				if (fGrpItem == map->haptics.size())
					fGrpItem--;
				ListBox_SetCurSel(grp_list, fGrpItem);
				//DrawFreq(hDlg);
				SetMappingData(hDlg, map);
				RedrawGridButtonZone(NULL, true);
			}
			break;
		case IDC_BUTTON_LPC:
			if (map) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_LPC), &freqBlock->colorfrom);
				RedrawGridButtonZone(NULL, true);
			}
			break;
		case IDC_BUTTON_HPC:
			if (map) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_HPC), &freqBlock->colorto);
				RedrawGridButtonZone(NULL, true);
			}
			break;
		case IDC_BUTTON_REMOVE:
			if (map) {
				freqBlock->freqID.clear();
				//DrawFreq(hDlg);
				SetMappingData(hDlg, map);
				RedrawGridButtonZone(NULL, true);
			}
		}
	} break;
	case WM_APP + 2: {
		SetFreqGroups(hDlg);
		//DrawFreq(hDlg);
		//RedrawGridButtonZone(NULL, true);
	} break;
	case WM_DRAWITEM:
		//switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		//case IDC_BUTTON_LPC: case IDC_BUTTON_HPC:
		{
			AlienFX_SDK::Colorcode* c = NULL;
			if (map && fGrpItem >= 0)
				if (((DRAWITEMSTRUCT *) lParam)->CtlID == IDC_BUTTON_LPC)
					c = &freqBlock->colorfrom;
				else
					c = &freqBlock->colorto;
			RedrawButton(((DRAWITEMSTRUCT *) lParam)->hwndItem, c);
			return true;
		}
		break;
		//}
		//return false;
	break;
	case WM_DESTROY:
		delete hapUIThread;
	break;
	default: return false;
	}

	return true;
}

void UpdateHapticsUI(LPVOID lpParam) {
	if (eve->audio && IsWindowVisible((HWND)lpParam)) {
		//DebugPrint("Haptics UI update...\n");
		DrawFreq((HWND)lpParam);
	}
}