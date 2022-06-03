#include "alienfx-gui.h"

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void ResizeTab(HWND, RECT&);

extern BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void CreateGridBlock(HWND gridTab, DLGPROC, bool is = false);
extern void OnGridSelChanged(HWND);
extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);
extern void UpdateZoneList(byte flag = 0);

extern void CreateTabControl(HWND parent, vector<string> names, vector<DWORD> resID, vector<DLGPROC> func);
extern void RepaintGrid(HWND hDlg);

bool firstInit = false;
int lastTab = 0;
extern HWND zsDlg;

BOOL CALLBACK LightDlgFrame(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		zsDlg = CreateDialog(hInst, (LPSTR)IDD_ZONESELECTION, hDlg, (DLGPROC)ZoneSelectionDialog);
		RECT mRect;
		GetWindowRect(GetDlgItem(hDlg, IDC_STATIC_ZONES), &mRect);
		ScreenToClient(hDlg, (LPPOINT)&mRect);
		SetWindowPos(zsDlg, NULL, mRect.left, mRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);

		CreateGridBlock(gridTab, (DLGPROC)TabGrid);
		TabCtrl_SetCurSel(gridTab, conf->gridTabSel);
		//OnGridSelChanged(gridTab);
	} break;
	case WM_APP + 2: {
		DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(GetParent(hDlg), GWLP_USERDATA);
		SendMessage(pHdr->hwndControl, WM_APP + 2, 0, 1);
		RedrawGridButtonZone();
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_COLOR_GRID: {
			switch (((NMHDR*)lParam)->code) {
			case TCN_SELCHANGE: {
				if (TabCtrl_GetCurSel(gridTab) < conf->afx_dev.GetGrids()->size())
					OnGridSelChanged(gridTab);
			} break;
			}
		} break;
		}
		break;
	case WM_DESTROY:
		DestroyWindow(zsDlg);
		break;
	default: return false;
	}
	return true;
}

void OnLightSelChanged(HWND hwndDlg)
{
	// Get the dialog header data.
	DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	// Get the index of the selected tab.
	int oldTab = lastTab;
	lastTab = TabCtrl_GetCurSel(hwndDlg);

	RECT rcDisplay;

	GetClientRect(hwndDlg, &rcDisplay);
	rcDisplay.left = GetSystemMetrics(SM_CXBORDER);
	rcDisplay.top = GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER);
	rcDisplay.right -= 2 * GetSystemMetrics(SM_CXBORDER) + 1;
	rcDisplay.bottom -= GetSystemMetrics(SM_CYBORDER) + 1;

	if (!pHdr->hwndDisplay)
		if (lastTab != TAB_DEVICES)
			pHdr->hwndDisplay = CreateDialogIndirect(hInst,
				(DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(IDD_LIGHT_TEMPLATE), RT_DIALOG))),
				hwndDlg, (DLGPROC)LightDlgFrame);
		else
			pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[lastTab], hwndDlg, pHdr->apProc[lastTab]);

	if (pHdr->hwndDisplay != NULL) {
		// create control block dialog
		if (pHdr->hwndControl) {
			DestroyWindow(pHdr->hwndControl);
			pHdr->hwndControl = NULL;
		}
		if (lastTab == TAB_DEVICES) {
			DestroyWindow(pHdr->hwndDisplay);
			pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[lastTab], hwndDlg, pHdr->apProc[lastTab]);
		}
		else {
			if (oldTab == TAB_DEVICES) {
				DestroyWindow(pHdr->hwndDisplay);
				pHdr->hwndDisplay = CreateDialogIndirect(hInst,
					(DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(IDD_LIGHT_TEMPLATE), RT_DIALOG))),
					hwndDlg, (DLGPROC)LightDlgFrame);
			}
			pHdr->hwndControl = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[lastTab], pHdr->hwndDisplay, pHdr->apProc[lastTab]);
			RECT mRect;
			GetWindowRect(GetDlgItem(pHdr->hwndDisplay, IDC_STATIC_CONTROLS), &mRect);
			ScreenToClient(pHdr->hwndDisplay, (LPPOINT)&mRect);
			SetWindowPos(pHdr->hwndControl, NULL, mRect.left, mRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
		}
		ResizeTab(pHdr->hwndDisplay, rcDisplay);
	}
}

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
		OnLightSelChanged(tab_list);
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
					OnLightSelChanged(tab_list);
				} else
					TabCtrl_SetCurSel(tab_list, lastTab);
			}
		} break;
		}
	} break;
	default: return false;
	}
	return true;
}