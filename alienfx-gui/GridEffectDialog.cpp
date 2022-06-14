#include "alienfx-gui.h"
#include "common.h"

extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);

extern int eItem;

BOOL CALLBACK TabGridDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	groupset* mmap = FindMapping(eItem);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		UpdateCombo(GetDlgItem(hDlg, IDC_COMBO_TRIGGER), { "Off", "Continues", "Random", "Keyboard", "Event", "Haptics"});
		UpdateCombo(GetDlgItem(hDlg, IDC_COMBO_GEFFTYPE), {"Switch", "Running light", "Wave"});
		UpdateCombo(GetDlgItem(hDlg, IDC_COMBO_GAUGE), { "Off", "Horizontal", "Vertical", "Diagonal (left)", "Diagonal (right)", "Radial" });

		if (mmap) {
			CheckDlgButton(hDlg, IDC_CHECK_SPECTRUM, mmap->flags & GAUGE_GRADIENT);
			CheckDlgButton(hDlg, IDC_CHECK_REVERSE, mmap->flags & GAUGE_REVERSE);
			ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_GAUGE), mmap->gauge);
		}
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_COMBO_GAUGE:
			if (mmap && HIWORD(wParam) == CBN_SELCHANGE) {
				mmap->gauge = ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam)));
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SPECTRUM), mmap && mmap->gauge);
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_REVERSE), mmap && mmap->gauge);
			}
			break;
		case IDC_COMBO_TRIGGER:
			if (mmap && HIWORD(wParam) == CBN_SELCHANGE) {
			}
			break;
		case IDC_COMBO_GEFFTYPE:
			if (mmap && HIWORD(wParam) == CBN_SELCHANGE) {
			}
			break;
		}
	} break;
	default: return false;
	}
	return true;
}