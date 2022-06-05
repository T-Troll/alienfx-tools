#include "alienfx-gui.h"

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void ResizeTab(HWND, RECT&);

extern BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void CreateGridBlock(HWND gridTab, DLGPROC, bool is = false);
extern void OnGridSelChanged(HWND);
extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);

extern void CreateTabControl(HWND parent, vector<string> names, vector<DWORD> resID, vector<DLGPROC> func);

bool firstInit = false;
extern int tabLightSel;
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
	int oldTab = tabLightSel;
	tabLightSel = TabCtrl_GetCurSel(hwndDlg);

	RECT rcDisplay;

	GetClientRect(hwndDlg, &rcDisplay);
	rcDisplay.left = GetSystemMetrics(SM_CXBORDER);
	rcDisplay.top = GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER);
	rcDisplay.right -= 2 * GetSystemMetrics(SM_CXBORDER) + 1;
	rcDisplay.bottom -= GetSystemMetrics(SM_CYBORDER) + 1;

	if (!pHdr->hwndDisplay)
		if (tabLightSel != TAB_DEVICES)
			pHdr->hwndDisplay = CreateDialogIndirect(hInst,
				(DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(IDD_LIGHT_TEMPLATE), RT_DIALOG))),
				hwndDlg, (DLGPROC)LightDlgFrame);
		else
			pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[tabLightSel], hwndDlg, pHdr->apProc[tabLightSel]);

	if (pHdr->hwndDisplay != NULL) {
		// create control block dialog
		if (pHdr->hwndControl) {
			DestroyWindow(pHdr->hwndControl);
			pHdr->hwndControl = NULL;
		}
		if (tabLightSel == TAB_DEVICES) {
			DestroyWindow(pHdr->hwndDisplay);
			pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[tabLightSel], hwndDlg, pHdr->apProc[tabLightSel]);
			ResizeTab(pHdr->hwndDisplay, rcDisplay);
			return;
		} else
			if (oldTab == TAB_DEVICES) {
				DestroyWindow(pHdr->hwndDisplay);
				pHdr->hwndDisplay = CreateDialogIndirect(hInst,
					(DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(IDD_LIGHT_TEMPLATE), RT_DIALOG))),
					hwndDlg, (DLGPROC)LightDlgFrame);
			}
		pHdr->hwndControl = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[tabLightSel], pHdr->hwndDisplay, pHdr->apProc[tabLightSel]);
		RECT mRect;
		GetWindowRect(GetDlgItem(pHdr->hwndDisplay, IDC_STATIC_CONTROLS), &mRect);
		ScreenToClient(pHdr->hwndDisplay, (LPPOINT)&mRect);
		SetWindowPos(pHdr->hwndControl, NULL, mRect.left, mRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
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
		if (!conf->afx_dev.haveLights || !conf->afx_dev.GetGrids()->size())
			tabLightSel = TAB_DEVICES;
		TabCtrl_SetCurSel(tab_list, tabLightSel);
		OnLightSelChanged(tab_list);
	} break;
	case WM_WINDOWPOSCHANGING: {
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		if (!(pos->flags & (SWP_SHOWWINDOW | SWP_NOSIZE)) && (pos->cx || pos->cy)) {
			RECT oldRect;
			GetWindowRect(hDlg, &oldRect);
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
				if (conf->afx_dev.haveLights && conf->afx_dev.GetGrids()->size()) {
					OnLightSelChanged(tab_list);
				} else
					TabCtrl_SetCurSel(tab_list, tabLightSel);
			}
		} break;
		}
	} break;
	default: return false;
	}
	return true;
}