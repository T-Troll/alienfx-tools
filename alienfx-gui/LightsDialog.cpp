#include "alienfx-gui.h"
#include <common.h>

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabGridDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void ResizeTab(HWND);

extern BOOL CALLBACK ZoneSelectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void UpdateZoneList();

extern void CreateGridBlock(HWND gridTab, DLGPROC, bool is = false);
extern void OnGridSelChanged(HWND);
extern void RedrawGridButtonZone(RECT* what = NULL);

extern void CreateTabControl(HWND parent, vector<string> names, vector<DWORD> resID, vector<DLGPROC> func);
extern void ClearOldTabs(HWND);

bool firstInit = false;
extern int tabLightSel;
extern HWND zsDlg;

void OnLightSelChanged(HWND hwndDlg);

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
	} break;
	case WM_APP + 2: {
		groupset* mmap = conf->FindMapping(eItem);
		if (mmap) {
			zonemap* zone = conf->FindZoneMap(mmap->group);
			if (zone->gridID != conf->mainGrid->id) {
				// Switch grid tab
				vector<AlienFX_SDK::Afx_grid>* grids = conf->afx_dev.GetGrids();
				for (int i = 0; i < grids->size(); i++)
					if (zone->gridID == (*grids)[i].id) {
						TabCtrl_SetCurSel(gridTab, i);
						OnGridSelChanged(gridTab);
					}
			}
			RedrawGridButtonZone();
			SendMessage(((DLGHDR*)GetWindowLongPtr(GetParent(hDlg), GWLP_USERDATA))->hwndControl, WM_APP + 2, 0, 1);
		}
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

	if (!conf->afx_dev.activeLights) {
		ShowNotification(&conf->niData, "Warning", "Please set lights first!");
		tabLightSel = TAB_DEVICES;
		TabCtrl_SetCurSel(hwndDlg, tabLightSel);
	}

	if (!pHdr->hwndDisplay)
		if (tabLightSel != TAB_DEVICES)
			pHdr->hwndDisplay = CreateDialogIndirect(hInst,
				(DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(IDD_LIGHT_TEMPLATE), RT_DIALOG))),
				hwndDlg, (DLGPROC)LightDlgFrame);
		else
			pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[tabLightSel], hwndDlg, pHdr->apProc[tabLightSel]);

	if (pHdr->hwndDisplay) {
		// create control block dialog
		if (pHdr->hwndControl) {
			DestroyWindow(pHdr->hwndControl);
			pHdr->hwndControl = NULL;
		}
		if (tabLightSel == TAB_DEVICES) {
			if (oldTab != TAB_DEVICES) {
				DestroyWindow(pHdr->hwndDisplay);
				pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[tabLightSel], hwndDlg, pHdr->apProc[tabLightSel]);
			}
		}
		else {
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
			UpdateZoneList();
		}
		ResizeTab(pHdr->hwndDisplay);
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
			{"Colors", "Events Monitoring", "Ambient", "Haptics", "Grid Effect", "Devices and Grids"},
			{ IDD_DIALOG_COLORS, IDD_DIALOG_EVENTS, IDD_DIALOG_AMBIENT, IDD_DIALOG_HAPTICS, IDD_DIALOG_GRIDEFFECT, IDD_DIALOG_DEVICES},
			{ (DLGPROC)TabColorDialog, (DLGPROC)TabEventsDialog, (DLGPROC)TabAmbientDialog, (DLGPROC)TabHapticsDialog,
			(DLGPROC)TabGridDialog, (DLGPROC)TabDevicesDialog } );
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
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_TAB_LIGHTS && ((NMHDR*)lParam)->code == TCN_SELCHANGE) {
				OnLightSelChanged(tab_list);
		}
		break;
	case WM_DESTROY:
		ClearOldTabs(tab_list);
		break;
	default: return false;
	}
	return true;
}