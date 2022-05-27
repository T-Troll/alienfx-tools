#include "alienfx-gui.h"

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabGroupsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void CreateTabControl(HWND parent, vector<string> names, vector<DWORD> resID, vector<DLGPROC> func);
extern void OnSelChanged(HWND hwndDlg);
extern int tabSel;

bool firstInit = false;
int lastTab = 0;

BOOL CALLBACK TabLightsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND tab_list = GetDlgItem(hDlg, IDC_TAB_LIGHTS);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		firstInit = true;
		CreateTabControl(tab_list,
			{"Colors", "Events Monitoring", "Ambient", "Haptics", "Devices and Grids"},
			{ IDD_DIALOG_COLORS, IDD_DIALOG_EVENTS, IDD_DIALOG_AMBIENT, IDD_DIALOG_HAPTICS, IDD_DIALOG_DEVICES},
			{ (DLGPROC)TabColorDialog, (DLGPROC)TabEventsDialog, (DLGPROC)TabAmbientDialog, (DLGPROC)TabHapticsDialog, (DLGPROC)TabDevicesDialog }
			);
		if (!conf->afx_dev.GetMappings()->size() || !conf->afx_dev.GetGrids()->size())
			lastTab = TAB_DEVICES;
		TabCtrl_SetCurSel(tab_list, lastTab);
		OnSelChanged(tab_list);
	} break;
	case WM_WINDOWPOSCHANGING: {
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		RECT oldRect;
		GetWindowRect(hDlg, &oldRect);
		if (!(pos->flags & SWP_SHOWWINDOW) && (pos->cx || pos->cy)) {
			int deltax = pos->cx - oldRect.right + oldRect.left,
				deltay = pos->cy - oldRect.bottom + oldRect.top;
			if (deltax || deltay) {
				GetWindowRect(tab_list, &oldRect);
				SetWindowPos(tab_list, NULL, 0, 0, oldRect.right - oldRect.left + deltax, oldRect.bottom - oldRect.top + deltay, SWP_NOZORDER | SWP_NOMOVE);
				if (!firstInit) {
					GetWindowRect(GetParent(GetParent(hDlg)), &oldRect);
					SetWindowPos(GetParent(GetParent(hDlg)), NULL, 0, 0, oldRect.right - oldRect.left + deltax, oldRect.bottom - oldRect.top + deltay, SWP_NOZORDER | SWP_NOMOVE);
				}
				firstInit = false;
			}
		}
		else
			return false;
	} break;
	case WM_NOTIFY: {
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_LIGHTS: {
			if (((NMHDR*)lParam)->code == TCN_SELCHANGE) {
				if (conf->afx_dev.GetMappings()->size() && conf->afx_dev.GetGrids()->size()) {
					OnSelChanged(tab_list);
					lastTab = tabSel;
				}
			}
		} break;
		}
	} break;
	default: return false;
	}
	return true;
}