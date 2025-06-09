#include "alienfx-gui.h"
#include "EventHandler.h"
#include "WSAudioIn.h"
#include "Common.h"

extern void RedrawButton(HWND hDlg, DWORD);
extern bool SetColor(HWND hDlg, AlienFX_SDK::Afx_colorcode&);
extern DWORD MakeRGB(AlienFX_SDK::Afx_colorcode c);
extern void UpdateZoneAndGrid();

extern EventHandler* eve;

int freqItem = 0;
freq_map* freqBlock = NULL;
HWND hToolTip = NULL;
int cutMove = 0;

static const double log_divider = log(21);

void DrawFreq(HWND hysto) {

	if (hysto) {
		RECT levels_rect;
		GetClientRect(hysto, &levels_rect);
		RECT graphZone = levels_rect;

		HDC hdc_r = GetDC(hysto);

		// Double buff...
		HDC hdc = CreateCompatibleDC(hdc_r);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, graphZone.right /*- graphZone.left*/, graphZone.bottom /*- graphZone.top*/);

		//SetBkMode(hdc, TRANSPARENT);

		HGDIOBJ hOld = SelectObject(hdc, hbmMem);

		//setting colors:
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		SetBkMode(hdc, TRANSPARENT);

		graphZone.top = 2;
		graphZone.left = 1;
		graphZone.bottom--;
		WSAudioIn* audio = (WSAudioIn*)eve->audio;
		if (audio) {
			SelectObject(hdc, GetStockObject(WHITE_PEN));
			SelectObject(hdc, GetStockObject(WHITE_BRUSH));
			for (int i = 0; i < NUMBARS; i++) {
				int leftpos = (graphZone.right * i) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (i + 1)) / NUMBARS - 2 + graphZone.left;
				Rectangle(hdc, leftpos, (255 - audio->freqs[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top,
					rightpos, graphZone.bottom);
			}
		}
		if (freqBlock) {
			for (auto t = freqBlock->freqID.begin(); t < freqBlock->freqID.end(); t++) {
				int leftpos = (graphZone.right * (*t)) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (*t + 1)) / NUMBARS - 2 + graphZone.left;
				int rectop = graphZone.bottom;
				// gray block
				if (audio)
					rectop = (255 - audio->freqs[*t]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top;
				SelectObject(hdc, GetStockObject(GRAY_BRUSH));
				SelectObject(hdc, GetStockObject(NULL_PEN));
				Rectangle(hdc, leftpos, graphZone.top, rightpos + 2, rectop);

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

		BitBlt(hdc_r, 0, 0, levels_rect.right, levels_rect.bottom, hdc, 0, 0, SRCCOPY);

		// Free-up the off-screen DC
		SelectObject(hdc, hOld);

		DeleteObject(hbmMem);
		DeleteDC(hdc);
		ReleaseDC(hysto, hdc_r);
		DeleteDC(hdc_r);
	}
}

void SetMappingData(HWND hDlg) {
	DrawFreq(GetDlgItem(hDlg, IDC_LEVELS));
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_LPC), freqBlock ? MakeRGB(freqBlock->colorfrom) : 0xff000000);
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_HPC), freqBlock ? MakeRGB(freqBlock->colorto) : 0xff000000);
	CheckDlgButton(hDlg, IDC_CHECK_RANDOM, freqBlock ? freqBlock->colorto.br : 0);
}

void SetFreqGroups(HWND hDlg) {
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP);
	ListBox_ResetContent(grp_list);
	freqBlock = NULL;
	if (mmap && mmap->haptics.size()) {
		// Set groups
		for (int j = 0; j < mmap->haptics.size(); j++) {
			int pos = ListBox_AddString(grp_list, ("Group " + to_string(j + 1)).c_str());
			if (freqItem == pos) {
				ListBox_SetCurSel(grp_list, pos);
				freqBlock = &mmap->haptics[pos];
			}
		}
	}
	SetMappingData(hDlg);
}

INT_PTR CALLBACK FreqLevels(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT pos;
	GetClientRect(hDlg, &pos);
	pos.right--;
	pos.bottom;
	byte cIndex = (byte)(GET_X_LPARAM(lParam) * NUMBARS / pos.right);
	int clickLevel = min((pos.bottom - GET_Y_LPARAM(lParam)) * 255 / (pos.bottom - 3), 255);

	switch (message) {
	case WM_LBUTTONDOWN: {
		if (freqBlock)
			for (auto idx = freqBlock->freqID.begin(); idx != freqBlock->freqID.end(); idx++)
				if (*idx == cIndex) {
					if (abs(clickLevel - freqBlock->hicut) < 10)
						cutMove = 1;
					else
						if (abs(clickLevel - freqBlock->lowcut) < 10)
							cutMove = 2;
					break;
				}
	} break;
	case WM_LBUTTONUP: {
		if (freqBlock) {
			if (cutMove) {
				if (freqBlock->lowcut > freqBlock->hicut)
					swap(freqBlock->lowcut, freqBlock->hicut);
				cutMove = 0;
			}
			else {
				for (auto idx = freqBlock->freqID.begin(); idx != freqBlock->freqID.end(); idx++)
					if (*idx == cIndex) {
						freqBlock->freqID.erase(idx);
						DrawFreq(hDlg);
						return true;
					}
				freqBlock->freqID.push_back(cIndex);
			}
			DrawFreq(hDlg);
		}
	} break;
	case WM_MOUSEMOVE: {
		if (freqBlock && wParam & MK_LBUTTON) {
			switch (cutMove) {
			case 1: freqBlock->hicut = clickLevel; break;
			case 2: freqBlock->lowcut = clickLevel; break;
			}
			DrawFreq(hDlg);
		}
		int freq = eve->audio ? ((WSAudioIn*)eve->audio)->pwfx->nSamplesPerSec >> 1 : 22050;
		SetToolTip(hToolTip, "Freq: " + to_string(cIndex ? freq - (int)round((log(21 - cIndex) * freq / log_divider)) : 20) +
			"-" + to_string(freq - (int)round((log(20 - cIndex) * freq / log_divider))) + " Hz, Level: " +
			to_string(clickLevel * 100 / 255));
	} break;
	case WM_PAINT:
		DrawFreq(hDlg);
		break;
	case WM_TIMER:
		//DebugPrint("Haptics UI update...\n");
		DrawFreq(hDlg);
		break;
	case WM_ERASEBKGND:
		return true;
	}
	return DefWindowProc(hDlg, message, wParam, lParam);
}

BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP),
	     gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID);

	switch (message) {
	case WM_INITDIALOG:
	{
		CheckDlgButton(hDlg, conf->hap_inpType ? IDC_RADIO_INPUT : IDC_RADIO_OUTPUT, BST_CHECKED);

		SetWindowLongPtr(GetDlgItem(hDlg, IDC_LEVELS), GWLP_WNDPROC, (LONG_PTR)FreqLevels);
		hToolTip = CreateToolTip(GetDlgItem(hDlg, IDC_LEVELS), hToolTip);

		//SetFreqGroups(hDlg);

		// Start UI update...
		SetTimer(GetDlgItem(hDlg, IDC_LEVELS), 0, 50, NULL);
	}
	break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDC_RADIO_OUTPUT: case IDC_RADIO_INPUT: {
			conf->hap_inpType = LOWORD(wParam) == IDC_RADIO_INPUT;
			if (eve->audio)
				((WSAudioIn*)eve->audio)->RestartDevice();
		} break;
		case IDC_FREQ_GROUP:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				if (mmap) {
					freqItem = ListBox_GetCurSel(grp_list);
					freqBlock = &mmap->haptics[freqItem];
					SetMappingData(hDlg);
				}
			break;
			}
			break;
		case IDC_BUT_ADD_GROUP:
		{
			if (mmap) {
				mmap->haptics.push_back({0,0,255});
				freqItem = (int)mmap->haptics.size() - 1;
				eve->ChangeEffects();
				UpdateZoneAndGrid();
			}
		} break;
		case IDC_BUT_REM_GROUP:
			if (mmap && freqBlock) {
				mmap->haptics.erase(mmap->haptics.begin() + freqItem);
				ListBox_DeleteString(grp_list, freqItem);
				if (mmap->haptics.size()) {
					if (freqItem) {
						freqItem--;
					}
				}
				else {
					eve->ChangeEffects();
				}
				UpdateZoneAndGrid();
			}
			break;
		case IDC_BUTTON_LPC:
			if (freqBlock) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_LPC), freqBlock->colorfrom);
				UpdateZoneAndGrid();
			}
			break;
		case IDC_BUTTON_HPC:
			if (freqBlock) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_HPC), freqBlock->colorto);
				UpdateZoneAndGrid();
			}
			break;
		case IDC_BUTTON_REMOVE:
			if (freqBlock) {
				freqBlock->freqID.clear();
				SetMappingData(hDlg);
				//UpdateZoneAndGrid();
			}
		case IDC_CHECK_RANDOM:
			if (freqBlock) {
				freqBlock->colorto.br = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
			}
		}
	} break;
	case WM_APP + 2: {
		SetFreqGroups(hDlg);
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		case IDC_BUTTON_LPC: case IDC_BUTTON_HPC:
		{
			DWORD c = 0xff000000;
			if (freqBlock)
				if (((DRAWITEMSTRUCT *) lParam)->CtlID == IDC_BUTTON_LPC)
					c = MakeRGB(freqBlock->colorfrom);
				else
					c = MakeRGB(freqBlock->colorto);
			RedrawButton(((DRAWITEMSTRUCT *) lParam)->hwndItem, c);
			return true;
		}
		break;
		}
		return false;
	break;
	default: return false;
	}

	return true;
}
