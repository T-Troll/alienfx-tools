#include "alienfx-gui.h"
#include <Windows.h>
#include <windowsx.h>
#include <Shlobj.h>
#include <ColorDlg.h>
#include <algorithm>
#include "toolkit.h"
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "AlienFX_SDK.h"
#include "EventHandler.h"

#pragma comment(lib,"comctl32.lib")

// defines and structures...
#define C_PAGES 5

typedef struct tag_dlghdr {
	HWND hwndTab;       // tab control
	HWND hwndDisplay;   // current child dialog box
	RECT rcDisplay;     // display rectangle for the tab control
	DLGTEMPLATE* apRes[C_PAGES];
} DLGHDR;

// Global Variables:
NOTIFYICONDATA niData;

HWND InitInstance(HINSTANCE, int);

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

FXHelper* fxhl;
ConfigHandler* conf;
EventHandler* eve;

HWND mDlg = 0;

// Dialogues data....
// Common:
AlienFX_SDK::afx_act* mod;
HANDLE stopColorRefresh = 0;

char tBuff[4], lBuff[4]; // tooltips
HWND sTip = 0, lTip = 0;

// ConfigStatic:
int tabSel = -1;

// Colors
int effID = -1;
int eItem = -1;

// Devices
int eLid = -1, 
    eDid = -1, dItem = -1, 
	gLid = -1, gItem = -1;

// Profiles
int pCid = -1;

DWORD EvaluteToAdmin() {
	// Evaluation attempt...
	DWORD dwError = ERROR_CANCELLED;
	char szPath[MAX_PATH];
	if (!IsUserAnAdmin()) {
		if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
		{
			// Launch itself as admin
			SHELLEXECUTEINFO sei = { sizeof(sei) };
			sei.lpVerb = "runas";
			sei.lpFile = szPath;
			sei.hwnd = NULL;
			sei.nShow = SW_NORMAL;
			if (!ShellExecuteEx(&sei))
			{
				dwError = GetLastError();
			}
			else
			{
				if (mDlg) {
					conf->Save();
					SendMessage(mDlg, WM_CLOSE, 0, 0);
				} else
					_exit(1);  // Quit itself
			}
		}
		return dwError;
	}
	return 0;
}

bool DoStopService(bool kind) {
	SERVICE_STATUS_PROCESS ssp;
	ULONGLONG dwStartTime = GetTickCount64();
	DWORD dwBytesNeeded;
	DWORD dwTimeout = 10000; // 10-second time-out

	// Get a handle to the SCM database.

	SC_HANDLE schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database
		/*SC_MANAGER_ALL_ACCESS*/ GENERIC_READ);  // full access rights

	if (NULL == schSCManager)
	{
		//printf("OpenSCManager failed (%d)\n", GetLastError());
		return false;
	}

	// Get a handle to the service.

	SC_HANDLE schService = OpenService(
		schSCManager,         // SCM database
		"AWCCService",            // name of service
		/*SERVICE_STOP |*/
		SERVICE_QUERY_STATUS /*|
		SERVICE_ENUMERATE_DEPENDENTS*/);

	if (schService == NULL)
	{
		//printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return false;
	}

	// Make sure the service is not already stopped.

	if (QueryServiceStatusEx(
		schService,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded))
	{
		if (ssp.dwCurrentState == SERVICE_STOPPED)
		{
			//printf("Service is already stopped.\n");
			if (kind) {
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				conf->block_power = false;
				return false;
			}
			else {
				schService = OpenService(
					schSCManager,         // SCM database
					"AWCCService",            // name of service
					SERVICE_STOP | SERVICE_START |
					SERVICE_QUERY_STATUS /*|
					SERVICE_ENUMERATE_DEPENDENTS*/);

				if (schService != NULL)
				{
					StartService(
						schService,  // handle to service
						0,           // number of arguments
						NULL);
					conf->block_power = true;
					CloseServiceHandle(schService);
					CloseServiceHandle(schSCManager);
					return false;
				}
			}
		}

		// Evalute UAC and re-open manager and service here.

		CloseServiceHandle(schService);

		if (conf->awcc_disable && kind) {
			schService = OpenService(
				schSCManager,         // SCM database
				"AWCCService",            // name of service
				SERVICE_STOP | SERVICE_START |
				SERVICE_QUERY_STATUS /*|
				SERVICE_ENUMERATE_DEPENDENTS*/);

			if (schService == NULL)
			{
				// Evaluation attempt...
				if (EvaluteToAdmin() == ERROR_CANCELLED)
				{
					//CloseServiceHandle(schService);
					CloseServiceHandle(schSCManager);
					conf->block_power = true;
					return false;
				}
			}

			// Send a stop code to the service.

			if (!ControlService(
				schService,
				SERVICE_CONTROL_STOP,
				(LPSERVICE_STATUS)&ssp))
			{
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return false;
			}

			// Wait for the service to stop.

			while (ssp.dwCurrentState != SERVICE_STOPPED)
			{
				Sleep(ssp.dwWaitHint);
				if (!QueryServiceStatusEx(
					schService,
					SC_STATUS_PROCESS_INFO,
					(LPBYTE)&ssp,
					sizeof(SERVICE_STATUS_PROCESS),
					&dwBytesNeeded))
				{
					CloseServiceHandle(schService);
					CloseServiceHandle(schSCManager);
					return false;
				}

				if (ssp.dwCurrentState == SERVICE_STOPPED)
					break;

				if (GetTickCount64() - dwStartTime > dwTimeout)
				{
					CloseServiceHandle(schService);
					CloseServiceHandle(schSCManager);
					return false;
				}
			}
		}
		else conf->block_power = true;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return true;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	conf = new ConfigHandler();
	conf->Load();
	conf->SetStates();
	fxhl = new FXHelper(conf);

	if (fxhl->devs.size() > 0) {
		conf->wasAWCC = DoStopService(true);

		if (conf->esif_temp)
			EvaluteToAdmin();

		fxhl->Start();

		eve = new EventHandler(conf, fxhl);

		eve->ChangePowerState();

		// Perform application initialization:
		if (!(mDlg = InitInstance(hInstance, nCmdShow)))
			return FALSE;

		//register global hotkeys...
		RegisterHotKey(
			mDlg,
			1,
			MOD_CONTROL | MOD_SHIFT,
			VK_F12
		);

		RegisterHotKey(
			mDlg,
			2,
			MOD_CONTROL | MOD_SHIFT,
			VK_F11
		);

		RegisterHotKey(
			mDlg,
			4,
			MOD_CONTROL | MOD_SHIFT,
			VK_F10
		);

		RegisterHotKey(
			mDlg,
			3,
			0,
			VK_F18
		);

		// Power notifications...
		RegisterPowerSettingNotification(mDlg, &GUID_MONITOR_POWER_ON, 0);

		// minimize if needed
		if (conf->startMinimized)
			SendMessage(mDlg, WM_SIZE, SIZE_MINIMIZED, 0);
		HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALIENFXGUI));

		MSG msg;
		// Main message loop:
		while ((GetMessage(&msg, 0, 0, 0)) != 0) {
			if (!TranslateAccelerator(mDlg, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		delete eve;
		fxhl->Stop();
		fxhl->ChangeState();
		fxhl->afx_dev.SaveMappings();

		if (conf->wasAWCC) DoStopService(false);
	}
	else {
		// no fx device detected!
		MessageBox(NULL, "No Alienware light devices detected, exiting!", "Fatal error",
			MB_OK | MB_ICONSTOP);
	}

	delete fxhl;
	delete conf;

	return 0;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND dlg;

	dlg = CreateDialogParam(hInstance,
		MAKEINTRESOURCE(IDD_MAINWINDOW),
		NULL,
		(DLGPROC)DialogConfigStatic, 0);
	if (!dlg) return NULL;

	SendMessage(dlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXGUI)));
	SendMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXGUI), IMAGE_ICON, 16, 16, 0));

	ShowWindow(dlg, nCmdShow);

	return dlg;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		HRSRC hResInfo;
		DWORD dwSize;
		HGLOBAL hResData;
		LPVOID pRes, pResCopy;
		UINT uLen;
		VS_FIXEDFILEINFO* lpFfi;

		HWND version_text = GetDlgItem(hDlg, IDC_STATIC_VERSION);

		hResInfo = FindResource(hInst, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
		if (hResInfo) {
			dwSize = SizeofResource(hInst, hResInfo);
			hResData = LoadResource(hInst, hResInfo);
			if (hResData) {
				pRes = LockResource(hResData);
				pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
				if (pResCopy) {
					CopyMemory(pResCopy, pRes, dwSize);

					VerQueryValue(pResCopy, TEXT("\\"), (LPVOID*)&lpFfi, &uLen);
					char buf[255];

					DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
					DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

					sprintf_s(buf, 255, "Version: %d.%d.%d.%d", HIWORD(dwFileVersionMS), LOWORD(dwFileVersionMS), HIWORD(dwFileVersionLS), LOWORD(dwFileVersionLS));

					Static_SetText(version_text, buf);
					LocalFree(pResCopy);
				}
				FreeResource(hResData);
			}
		}
		return (INT_PTR)TRUE;
	} break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: case IDCANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		} break;
		}
		break;
	case WM_NOTIFY:
		switch (LOWORD(wParam)) {
		case IDC_SYSLINK_HOMEPAGE:
			switch (((LPNMHDR)lParam)->code)
			{
			case NM_CLICK:          // Fall through to the next case.

			case NM_RETURN:
				ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools", NULL, NULL, SW_SHOWNORMAL);
				break;
			} break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

DLGTEMPLATE* DoLockDlgRes(LPCTSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);

	HGLOBAL hglb = LoadResource(hInst, hrsrc);
	return (DLGTEMPLATE*)LockResource(hglb);
}

VOID OnSelChanged(HWND hwndDlg)
{
	// Get the dialog header data.
	DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(
		hwndDlg, GWLP_USERDATA);

	// Get the index of the selected tab.
	tabSel = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.
	if (pHdr->hwndDisplay != NULL) {
		EndDialog(pHdr->hwndDisplay, IDOK);
		DestroyWindow(pHdr->hwndDisplay);
		pHdr->hwndDisplay = NULL;
	}
	DLGPROC tdl = NULL;
	switch (tabSel) {
	case 0: tdl = (DLGPROC)TabColorDialog; break;
	case 1: tdl = (DLGPROC)TabEventsDialog; break;
	case 2: tdl = (DLGPROC)TabDevicesDialog; break;
	case 3: tdl = (DLGPROC)TabProfilesDialog; break;
	case 4: tdl = (DLGPROC)TabSettingsDialog; break;
	default: tdl = (DLGPROC)TabColorDialog;
	}
	HWND newDisplay = CreateDialogIndirect(hInst,
		(DLGTEMPLATE*)pHdr->apRes[tabSel], pHdr->hwndTab, tdl);
	if (pHdr->hwndDisplay == NULL)
		pHdr->hwndDisplay = newDisplay;
	if (pHdr->hwndDisplay != NULL)
		SetWindowPos(pHdr->hwndDisplay, NULL,
			pHdr->rcDisplay.left, pHdr->rcDisplay.top,
			(pHdr->rcDisplay.right - pHdr->rcDisplay.left), //- cxMargin - (GetSystemMetrics(SM_CXDLGFRAME)) + 1,
			(pHdr->rcDisplay.bottom - pHdr->rcDisplay.top), //- /*cyMargin - */(GetSystemMetrics(SM_CYDLGFRAME)) - GetSystemMetrics(SM_CYCAPTION) + 3,
			SWP_SHOWWINDOW);
	return;
}

void ReloadProfileList(HWND hDlg) {
	if (hDlg == NULL)
		hDlg = mDlg;
	HWND profile_list = GetDlgItem(hDlg, IDC_PROFILES);
	SendMessage(profile_list, CB_RESETCONTENT, 0, 0);
	for (int i = 0; i < conf->profiles.size(); i++) {
		int pos = (int)SendMessage(profile_list, CB_ADDSTRING, 0, (LPARAM)(conf->profiles[i].name.c_str()));
		SendMessage(profile_list, CB_SETITEMDATA, pos, conf->profiles[i].id);
		if (conf->profiles[i].id == conf->activeProfile) {
			SendMessage(profile_list, CB_SETCURSEL, pos, 0);
		}
	}

	EnableWindow(profile_list, !conf->enableProf);
}

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND tab_list = GetDlgItem(hDlg, IDC_TAB_MAIN),
		profile_list = GetDlgItem(hDlg, IDC_PROFILES);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		RECT rcTab;
		DLGHDR* pHdr = (DLGHDR*)LocalAlloc(LPTR, sizeof(DLGHDR));
		SetWindowLongPtr(tab_list, GWLP_USERDATA, (LONG_PTR)pHdr);

		pHdr->hwndTab = tab_list;

		pHdr->apRes[0] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_COLORS));
		pHdr->apRes[1] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_EVENTS));
		pHdr->apRes[2] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_DEVICES));
		pHdr->apRes[3] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_PROFILES));
		pHdr->apRes[4] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_SETTINGS));

		TCITEM tie;

		tie.mask = TCIF_TEXT;
		tie.iImage = -1;
		tie.pszText = (LPSTR)"Colors";
		SendMessage(tab_list, TCM_INSERTITEM, 0, (LPARAM)&tie);
		tie.pszText = (LPSTR)"Monitoring";
		SendMessage(tab_list, TCM_INSERTITEM, 1, (LPARAM)&tie);
		tie.pszText = (LPSTR)"Devices and Lights";
		SendMessage(tab_list, TCM_INSERTITEM, 2, (LPARAM)&tie);
		tie.pszText = (LPSTR)"Profiles";
		SendMessage(tab_list, TCM_INSERTITEM, 3, (LPARAM)&tie);
		tie.pszText = (LPSTR)"Settings";
		SendMessage(tab_list, TCM_INSERTITEM, 4, (LPARAM)&tie);

		SetRectEmpty(&rcTab);

		GetClientRect(pHdr->hwndTab, &rcTab);
		//MapDialogRect(pHdr->hwndTab, &rcTab);

		// Calculate the display rectangle.
		CopyRect(&pHdr->rcDisplay, &rcTab);
		TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

		//OffsetRect(&pHdr->rcDisplay, GetSystemMetrics(SM_CXDLGFRAME)-pHdr->rcDisplay.left - 2, -GetSystemMetrics(SM_CYDLGFRAME) - 2);// +GetSystemMetrics(SM_CYMENUSIZE));// GetSystemMetrics(SM_CXDLGFRAME), GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION));// GetSystemMetrics(SM_CYCAPTION) - pHdr->rcDisplay.top);
		pHdr->rcDisplay.left = 1;
		pHdr->rcDisplay.top -= 1; // GetSystemMetrics(SM_CYDLGFRAME);
		pHdr->rcDisplay.right += 1; // GetSystemMetrics(SM_CXDLGFRAME);// +1;
		pHdr->rcDisplay.bottom += 2;// 2 * GetSystemMetrics(SM_CYDLGFRAME) - 1;

		ReloadProfileList(hDlg);
		OnSelChanged(tab_list);

		// tray icon...
		ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
		niData.cbSize = sizeof(NOTIFYICONDATA);
		niData.uID = IDI_ALIENFXGUI;
		niData.uFlags = NIF_ICON | NIF_MESSAGE;
		niData.hIcon =
			(HICON)LoadImage(GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDI_ALIENFXGUI),
				IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON),
				GetSystemMetrics(SM_CYSMICON),
				LR_DEFAULTCOLOR);
		niData.hWnd = hDlg;
		niData.uCallbackMessage = WM_APP + 1;
		Shell_NotifyIcon(NIM_ADD, &niData);
	} break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDOK: case IDCANCEL: case IDCLOSE: case IDM_EXIT:
		{
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			//Shell_NotifyIcon(NIM_DELETE, &niData);
			//EndDialog(hDlg, IDOK);
			//DestroyWindow(hDlg);
		} break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
			break;
		case IDC_BUTTON_MINIMIZE:
			ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
			niData.cbSize = sizeof(NOTIFYICONDATA);
			niData.uID = IDI_ALIENFXGUI;
			niData.uFlags = NIF_ICON | NIF_MESSAGE;
			niData.hIcon =
				(HICON)LoadImage(GetModuleHandle(NULL),
					MAKEINTRESOURCE(IDI_ALIENFXGUI),
					IMAGE_ICON,
					GetSystemMetrics(SM_CXSMICON),
					GetSystemMetrics(SM_CYSMICON),
					LR_DEFAULTCOLOR);
			niData.hWnd = hDlg;
			niData.uCallbackMessage = WM_APP + 1;
			Shell_NotifyIcon(NIM_ADD, &niData);
			ShowWindow(hDlg, SW_HIDE);
			break;
		case IDC_BUTTON_REFRESH:
			fxhl->RefreshState(true);
			break;
		case IDC_BUTTON_SAVE:
			fxhl->afx_dev.SaveMappings();
			conf->Save();
			break;
		case ID_ACC_COLOR:
			TabCtrl_SetCurSel(tab_list, 0);
			OnSelChanged(tab_list);
			break;
		case ID_ACC_EVENTS:
			TabCtrl_SetCurSel(tab_list, 1);
			OnSelChanged(tab_list);
			break;
		case ID_ACC_DEVICES:
			TabCtrl_SetCurSel(tab_list, 2);
			OnSelChanged(tab_list);
			break;
		case ID_ACC_PROFILES:
			TabCtrl_SetCurSel(tab_list, 3);
			OnSelChanged(tab_list);
			break;
		case ID_ACC_SETTINGS:
			TabCtrl_SetCurSel(tab_list, 4);
			OnSelChanged(tab_list);
			break;
		case IDC_PROFILES: {
			int pbItem = (int)SendMessage(profile_list, CB_GETCURSEL, 0, 0);
			int prid = (int)SendMessage(profile_list, CB_GETITEMDATA, pbItem, 0);
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE: {
				eve->SwitchActiveProfile(prid);
				OnSelChanged(tab_list);
			} break;
			}
		} break;
		} break;
	} break;
	case WM_NOTIFY: {
		NMHDR* event = (NMHDR*)lParam;
		switch (event->idFrom) {
		case IDC_TAB_MAIN: {
			if (event->code == TCN_SELCHANGE) {
				int newTab = TabCtrl_GetCurSel(tab_list);
				if (newTab != tabSel) { // selection changed!
					OnSelChanged(tab_list);
				}
			}
		} break;
		}
	} break;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			// go to tray...
			ShowWindow(hDlg, SW_HIDE);
		} break;
	case WM_APP + 1: {
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
			ShowWindow(hDlg, SW_RESTORE);
			SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
			SetWindowPos(hDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
			ReloadProfileList(hDlg);
			OnSelChanged(tab_list);
			break;
		case WM_RBUTTONUP: case WM_CONTEXTMENU: {
			POINT lpClickPoint;
			HMENU tMenu = LoadMenuA(hInst, MAKEINTRESOURCEA(IDR_MENU_TRAY));
			tMenu = GetSubMenu(tMenu, 0);
			// add profiles...
			HMENU pMenu = CreatePopupMenu();
			MENUINFO mi;
			memset(&mi, 0, sizeof(mi));
			mi.cbSize = sizeof(mi);
			mi.fMask = MIM_STYLE;
			mi.dwStyle = MNS_NOTIFYBYPOS;
			SetMenuInfo(tMenu, &mi);
			MENUITEMINFO mInfo;
			mInfo.cbSize = sizeof(MENUITEMINFO);
			mInfo.fMask = MIIM_STRING | MIIM_ID;
			mInfo.wID = ID_TRAYMENU_PROFILE_SELECTED;
			for (int i = 0; i < conf->profiles.size(); i++) {
				mInfo.dwTypeData = (LPSTR)conf->profiles[i].name.c_str();
				InsertMenuItem(pMenu, i, false, &mInfo);
				if (conf->profiles[i].id == conf->activeProfile)
					CheckMenuItem(pMenu, i, MF_BYPOSITION | MF_CHECKED);
			}
			if (conf->enableProf)
				ModifyMenu(tMenu, ID_TRAYMENU_PROFILES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED | MF_STRING, 0, "&Profiles...");
			else
				ModifyMenu(tMenu, ID_TRAYMENU_PROFILES, MF_BYCOMMAND | MF_POPUP | MF_STRING, (UINT_PTR)pMenu, "&Profiles...");
			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(hDlg);
			if (conf->lightsOn) CheckMenuItem(tMenu, ID_TRAYMENU_LIGHTSON, MF_CHECKED);
			if (conf->dimmed) CheckMenuItem(tMenu, ID_TRAYMENU_DIMLIGHTS, MF_CHECKED);
			if (conf->enableMon) CheckMenuItem(tMenu, ID_TRAYMENU_MONITORING, MF_CHECKED);
			if (conf->enableProf) CheckMenuItem(tMenu, ID_TRAYMENU_PROFILESWITCH, MF_CHECKED);
			TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
				lpClickPoint.x, lpClickPoint.y, 0, hDlg, NULL);
		} break;
		}
		break;
	} break;
	case WM_MENUCOMMAND: {
		HMENU menu = (HMENU)lParam;
		int idx = LOWORD(wParam);
		switch (GetMenuItemID(menu, idx)) {
		case ID_TRAYMENU_EXIT:
		{
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		} break;
		case ID_TRAYMENU_REFRESH:
			fxhl->RefreshState(true);
			break;
		case ID_TRAYMENU_LIGHTSON:
			conf->lightsOn = !conf->lightsOn;
			fxhl->ChangeState();
			OnSelChanged(tab_list);
			break;
		case ID_TRAYMENU_DIMLIGHTS:
			conf->dimmed = !conf->dimmed;
			//fxhl->RefreshState(true);
			fxhl->ChangeState();
			OnSelChanged(tab_list);
			break;
		case ID_TRAYMENU_MONITORING:
			conf->enableMon = !conf->enableMon;
			eve->ToggleEvents();
			OnSelChanged(tab_list);
			break;
		case ID_TRAYMENU_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProf = !conf->enableProf;
			ReloadProfileList(hDlg);
			OnSelChanged(tab_list);
			eve->StartProfiles();
			break;
		case ID_TRAYMENU_RESTORE:
			ShowWindow(hDlg, SW_RESTORE);
			SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
			SetWindowPos(hDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
			ReloadProfileList(hDlg);
			OnSelChanged(tab_list);
			break;
		case ID_TRAYMENU_PROFILE_SELECTED: {
			if (!conf->enableProf && idx < conf->profiles.size() && conf->profiles[idx].id != conf->activeProfile) {
				eve->SwitchActiveProfile(conf->profiles[idx].id);
				ReloadProfileList(hDlg);
				OnSelChanged(tab_list);
			}
		} break;
		}
	} break;
	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC: {
			// resume from sleep/hybernate
#ifdef _DEBUG
			OutputDebugString("Resume from Sleep/hibernate initiated\n");
#endif
			if (fxhl->FillDevs(conf->stateOn, conf->offPowerButton) > 0) {
				fxhl->UnblockUpdates(true);
				eve->ChangePowerState();
				conf->stateScreen = true;
				//fxhl->RefreshState();
				eve->ToggleEvents();
				eve->StartProfiles();
			}
		} break;
		case PBT_APMPOWERSTATUSCHANGE:
			// ac/batt change
			eve->ChangePowerState();
			break;
		case PBT_POWERSETTINGCHANGE: {
			POWERBROADCAST_SETTING* sParams = (POWERBROADCAST_SETTING*) lParam;
			if (sParams->PowerSetting == GUID_MONITOR_POWER_ON || sParams->PowerSetting == GUID_CONSOLE_DISPLAY_STATE
				|| sParams->PowerSetting == GUID_SESSION_DISPLAY_STATUS) {
				eve->ChangeScreenState(sParams->Data[0]);
			}
		} break;
		case PBT_APMSUSPEND:
			// Sleep initiated.
#ifdef _DEBUG
			OutputDebugString("Sleep/hibernate initiated\n");
#endif
			conf->stateScreen = true;
			fxhl->ChangeState();
			eve->StopProfiles();
			eve->StopEvents();
			fxhl->UnblockUpdates(false);
			break;
		}
		break;
	case WM_QUERYENDSESSION:
		return true;
	case WM_ENDSESSION:
		// Shutdown/restart scheduled....
#ifdef _DEBUG
		OutputDebugString("Shutdown initiated\n");
#endif
		conf->Save();
		eve->StopProfiles();
		eve->StopEvents();
		fxhl->UnblockUpdates(false);
		return 0;
		break;
	case WM_HOTKEY:
		switch (wParam) {
		case 1: // on/off
			conf->lightsOn = !conf->lightsOn;
			fxhl->ChangeState();
			break;
		case 2: // dim
			conf->dimmed = !conf->dimmed;
			//fxhl->RefreshState();
			fxhl->ChangeState();
			break;
		case 3: // off-dim-full circle
			if (conf->lightsOn) {
				if (conf->dimmed) {
					conf->lightsOn = false;
					conf->dimmed = false;
					//fxhl->ChangeState();
				}
				else {
					conf->dimmed = true;
					//fxhl->RefreshState();
					//fxhl->ChangeState();
				}
			}
			else {
				conf->lightsOn = true;
				//fxhl->ChangeState();
				//fxhl->RefreshState();
			}
			fxhl->ChangeState();
			break;
		case 4: // mon
			conf->enableMon = !conf->enableMon;
			eve->ToggleEvents();
			break;
		default: return false;
		}
		break;
	case WM_CLOSE:
		Shell_NotifyIcon(NIM_DELETE, &niData);
		EndDialog(hDlg, IDOK);
		DestroyWindow(hDlg);
		return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0); break;
	default: return false;
	}
	return true;
}

DWORD CColorRefreshProc(LPVOID param) {
	AlienFX_SDK::afx_act last;
	lightset* mmap = (lightset*)param;
	last.r = mod->r;
	last.g = mod->g;
	last.b = mod->b;
	while (WaitForSingleObject(stopColorRefresh, 250)) {
		if (last.r != mod->r || last.g != mod->g || last.b != mod->b) {
			// set colors...
			last.r = mod->r;
			last.g = mod->g;
			last.b = mod->b;
			if (mmap != NULL) {
				fxhl->RefreshOne(mmap, true, true);
			}
		}
	}
	return 0;
}

UINT_PTR Lpcchookproc(
	HWND hDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
) {
	DRAWITEMSTRUCT* item = 0;
	UINT r = 0, g = 0, b = 0;

	switch (message)
	{
	case WM_INITDIALOG:
		mod = (AlienFX_SDK::afx_act*)((CHOOSECOLOR*)lParam)->lCustData;
		break;
	case WM_CTLCOLOREDIT:
		r = GetDlgItemInt(hDlg, COLOR_RED, NULL, false);
		g = GetDlgItemInt(hDlg, COLOR_GREEN, NULL, false);
		b = GetDlgItemInt(hDlg, COLOR_BLUE, NULL, false);
		if (r != mod->r || g != mod->g || b != mod->b) {
			mod->r = r;
			mod->g = g;
			mod->b = b;
		}
		break;
	}
	return 0;
}

bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map) {
	CHOOSECOLOR cc;
	bool ret;

	AlienFX_SDK::afx_act savedColor = *map;
	DWORD crThreadID;
	HANDLE crRefresh;

	// Initialize CHOOSECOLOR
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hDlg;
	cc.lpfnHook = Lpcchookproc;
	cc.lCustData = (LPARAM)map;
	cc.lpCustColors = (LPDWORD)conf->customColors;
	cc.rgbResult = RGB(map->r, map->g, map->b);
	cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR | CC_ENABLEHOOK;

	mod = map;
	stopColorRefresh = CreateEvent(NULL, false, false, NULL);
	crRefresh = CreateThread(NULL, 0, CColorRefreshProc, mmap, 0, &crThreadID);

	ret = ChooseColor(&cc);
	SetEvent(stopColorRefresh);
	WaitForSingleObject(crRefresh, 300);
	CloseHandle(crRefresh);
	CloseHandle(stopColorRefresh);

	if (!ret)
	{
		map->r = savedColor.r;
		map->g = savedColor.g;
		map->b = savedColor.b;
		fxhl->RefreshOne(mmap, true, true);
	}
	RedrawButton(hDlg, id, map->r, map->g, map->b);
	return ret;
}

lightset* FindMapping(int mid)
{
	if (!(mid < 0)) {
		if (mid > 0xffff) {
			// group
			for (int i = 0; i < conf->active_set->size(); i++)
				if (conf->active_set->at(i).devid == 0 && conf->active_set->at(i).lightid == mid) {
					return &conf->active_set->at(i);
				}
		} else {
			// mapping
			AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(mid);
			for (int i = 0; i < conf->active_set->size(); i++)
				if (conf->active_set->at(i).devid == lgh.devid && conf->active_set->at(i).lightid == lgh.lightid) {
					return &conf->active_set->at(i);
				}
		}
	}
	return nullptr;
}

void RebuildEffectList(HWND hDlg, lightset* mmap) {
	HWND eff_list = GetDlgItem(hDlg, IDC_EFFECTS_LIST),
		 s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		 l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		 type_c1 = GetDlgItem(hDlg, IDC_TYPE1);
	// Populate effects list...
	ListView_DeleteAllItems(eff_list);
	LVCOLUMNA lCol;
	lCol.mask = LVCF_WIDTH;
	ListView_GetColumn(eff_list, 0, & lCol);
	if (lCol.cx < 0)
		ListView_InsertColumn(eff_list, 0, &lCol);
	if (mmap) {
		HIMAGELIST hSmall;
		hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			ILC_COLOR32, 1, 1);
		for (int i = 0; i < mmap->eve[0].map.size(); i++) {
			COLORREF* picData = new COLORREF[GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON)];
			fill_n(picData, GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON), RGB(mmap->eve[0].map[i].b, mmap->eve[0].map[i].g, mmap->eve[0].map[i].r));
			HBITMAP colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
				1, 32, picData);
			delete[] picData;
			ImageList_Add(hSmall, colorBox, NULL);
			DeleteObject(colorBox);
			LVITEMA lItem; char efName[16];
			lItem.mask = LVIF_TEXT | LVIF_IMAGE;
			lItem.iItem = i;
			lItem.iImage = i;
			lItem.iSubItem = 0;
			switch (mmap->eve[0].map[i].type) {
			case AlienFX_SDK::AlienFX_A_Color:
				LoadString(hInst, IDS_TYPE_COLOR, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Pulse:
				LoadString(hInst, IDS_TYPE_PULSE, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Morph:
				LoadString(hInst, IDS_TYPE_MORPH, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Spectrum:
				LoadString(hInst, IDS_TYPE_SPECTRUM, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Breathing:
				LoadString(hInst, IDS_TYPE_BREATH, efName, 16);
				break;
			case AlienFX_SDK::AlienFX_A_Rainbow:
				LoadString(hInst, IDS_TYPE_RAINBOW, efName, 16);
				break;
			}
			lItem.pszText = efName;
			ListView_InsertItem(eff_list, &lItem);
		}
		ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);

		// Set selection...
		if (effID >= ListView_GetItemCount(eff_list))
			effID = ListView_GetItemCount(eff_list) - 1;
		ListView_SetItemState(eff_list, effID, LVIS_SELECTED, LVIS_SELECTED);
		bool flag = !(fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER);
		EnableWindow(type_c1, flag);
		EnableWindow(s1_slider, true);// flag);
		EnableWindow(l1_slider, true);// flag);
		// Set data
		SendMessage(type_c1, CB_SETCURSEL, mmap->eve[0].map[effID].type, 0);
		RedrawButton(hDlg, IDC_BUTTON_C1, mmap->eve[0].map[effID].r, mmap->eve[0].map[effID].g, mmap->eve[0].map[effID].b);
		SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].tempo);
		SetSlider(sTip, tBuff, mmap->eve[0].map[effID].tempo);
		SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].time);
		SetSlider(lTip, lBuff, mmap->eve[0].map[effID].time);
	}
	else {
		EnableWindow(type_c1, false);
		EnableWindow(s1_slider, false);
		EnableWindow(l1_slider, false);
		RedrawButton(hDlg, IDC_BUTTON_C1, 0, 0, 0);
	}
	//RECT csize;
	//GetClientRect(eff_list, &csize);
	//ListView_SetColumnWidth(eff_list, 0, csize.right - csize.left);
	ListView_SetColumnWidth(eff_list, 0, LVSCW_AUTOSIZE);// width);
	ListView_EnsureVisible(eff_list, effID, false);
}

void UpdateLightsList(HWND hDlg, int pid, int lid) {
	int pos = -1;
	size_t lights = fxhl->afx_dev.GetMappings()->size();
	HWND light_list = GetDlgItem(hDlg, IDC_LIST_LIGHTS); 

	ListView_DeleteAllItems(light_list);
	LVCOLUMNA lCol;
	lCol.mask = LVCF_WIDTH;
	ListView_GetColumn(light_list, 0, & lCol);
	if (lCol.cx < 0)
		ListView_InsertColumn(light_list, 0, &lCol);
	for (int i = 0; i < lights; i++) {
		AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(i);
		if (pid == lgh.devid) { // && fxhl->LocateDev(lgh.devid)) {
			LVITEMA lItem; 
			lItem.mask = LVIF_TEXT | LVIF_PARAM;
			lItem.iItem = i;
			lItem.iImage = 0;
			lItem.iSubItem = 0;
			lItem.lParam = lgh.lightid;
			lItem.pszText = (char*)lgh.name.c_str();
			if (lid == lgh.lightid) {
				lItem.mask |= LVIF_STATE;
				lItem.state = LVIS_SELECTED;
				pos = i;
				SetDlgItemInt(hDlg, IDC_LIGHTID, lid, false);
				CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev.GetFlags(pid, lid) & ALIENFX_FLAG_POWER ? BST_CHECKED : BST_UNCHECKED);
				CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, fxhl->afx_dev.GetFlags(pid, lid) & ALIENFX_FLAG_INACTIVE ? BST_CHECKED : BST_UNCHECKED);
			}
			ListView_InsertItem(light_list, &lItem);
		}
	}
	//RECT csize;
	//GetClientRect(light_list, &csize);
	//int width = (csize.right - csize.left) < 100 ? 100 : csize.right - csize.left;
	ListView_SetColumnWidth(light_list, 0, LVSCW_AUTOSIZE);// width);
	ListView_EnsureVisible(light_list, pos, false);
	BYTE status = fxhl->LocateDev(eDid)->AlienfxGetDeviceStatus();
	if (status && status != 0xff)
		SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Ok");
	else
		SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Error");
	//return pos;
}

lightset* CreateMapping(int lid) {
	// create new mapping..
	lightset newmap;
	AlienFX_SDK::afx_act act;
	if (lid > 0xffff) {
		// group
		//AlienFX_SDK::group* grp = fxhl->afx_dev.GetGroupById(lid);
		newmap.devid = 0;
		newmap.lightid = lid;
	} else {
		// light
		AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(lid);
		newmap.devid = lgh.devid;
		newmap.lightid = lgh.lightid;
		if (lgh.flags & ALIENFX_FLAG_POWER) {
			act.time = 3;
			act.tempo = 0x64;
			newmap.eve[0].map.push_back(act);
		}
	}
	newmap.eve[0].fs.b.flags = 1;
	newmap.eve[0].map.push_back(act);
	newmap.eve[1].map.push_back(act);
	newmap.eve[1].map.push_back(act);
	newmap.eve[2].map.push_back(act);
	newmap.eve[2].map.push_back(act);
	newmap.eve[2].fs.b.cut = 0;
	newmap.eve[3].map.push_back(act);
	newmap.eve[3].map.push_back(act);
	newmap.eve[3].fs.b.cut = 90;
	conf->active_set->push_back(newmap);
	std::sort(conf->active_set->begin(), conf->active_set->end(), ConfigHandler::sortMappings);
	return FindMapping(lid);
}

bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid) {
	// erase mappings
	for (std::vector <lightset>::iterator mIter = lightsets->begin();
		mIter != lightsets->end(); mIter++)
		if (mIter->devid == did && mIter->lightid == lid) {
			lightsets->erase(mIter);
			return true;
		}
	return false;
}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
		s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
		l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
		type_c1 = GetDlgItem(hDlg, IDC_TYPE1);

	int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0),
		lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (UpdateLightList<FXHelper>(light_list, fxhl) < 0) {
			// no lights, switch to setup
			HWND tab_list = GetParent(hDlg);
			TabCtrl_SetCurSel(tab_list, 2);
			OnSelChanged(tab_list);
			return false;
		}
		// Set types list...
		char buffer[100];
		LoadString(hInst, IDS_TYPE_COLOR, buffer, 100);
		SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_TYPE_PULSE, buffer, 100);
		SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_TYPE_MORPH, buffer, 100);
		SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_TYPE_BREATH, buffer, 100);
		SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_TYPE_SPECTRUM, buffer, 100);
		SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_TYPE_RAINBOW, buffer, 100);
		SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
		// now sliders...
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(l1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		//TBM_SETTICFREQ
		SendMessage(s1_slider, TBM_SETTICFREQ, 32, 0);
		SendMessage(l1_slider, TBM_SETTICFREQ, 32, 0);
		sTip = CreateToolTip(s1_slider);
		lTip = CreateToolTip(l1_slider);
		// Restore selection....
		if (eItem != (-1)) {
			SendMessage(light_list, LB_SETCURSEL, eItem, 0);
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
		}
	} break;
	case WM_COMMAND: {
		lightset* mmap = FindMapping(lid);
		switch (LOWORD(wParam))
		{
		case IDC_LIGHTS:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				eItem = lbItem;
				effID = 0;
				RebuildEffectList(hDlg, mmap);
				break;
			} break;
		case IDC_TYPE1:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				if (mmap != NULL) {
					int lType1 = (int)SendMessage(type_c1, CB_GETCURSEL, 0, 0);
					mmap->eve[0].map[effID].type = lType1;
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, true, true);
				}
			}
			break;
		case IDC_BUTTON_C1:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				if (lid != -1) {
					if (!mmap) {
						// have selection, but no effect - let's create one!
						mmap = CreateMapping(lid);
						effID = 0;
					}
					SetColor(hDlg, IDC_BUTTON_C1, mmap, &mmap->eve[0].map[effID]);
					RebuildEffectList(hDlg, mmap);
					//fxhl->RefreshOne(mmap, true, true);
				}
			} break;
			} break;
		case IDC_BUT_ADD_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED) {
				if (!mmap) {
					// create new mapping..
					mmap = CreateMapping(lid);
					effID = 0;
				} else
					if (!(fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER) && mmap->eve[0].map.size() < 9) {
						AlienFX_SDK::afx_act act = mmap->eve[0].map[effID];
						mmap->eve[0].map.push_back(act);
						effID = (int)mmap->eve[0].map.size() - 1;
					}
				RebuildEffectList(hDlg, mmap);
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		case IDC_BUTT_REMOVE_EFFECT:
			if (HIWORD(wParam) == BN_CLICKED && mmap) {
				mmap->eve[0].map.pop_back();
				if (effID == mmap->eve[0].map.size())
					effID--;
				// remove mapping if no colors!
				if (fxhl->afx_dev.GetFlags(mmap->devid, mmap->lightid) & ALIENFX_FLAG_POWER || mmap->eve[0].map.empty()) {
					RemoveMapping(conf->active_set, mmap->devid, mmap->lightid);
					RebuildEffectList(hDlg, NULL);
					effID = -1;
				} else {
					RebuildEffectList(hDlg, mmap);
					fxhl->RefreshOne(mmap, true, true);
				}
			}
			break;
		case IDC_BUTTON_SETALL:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				if (mmap != NULL &&
					MessageBox(hDlg, "Do you really want to set all lights to this settings?", "Warning!",
						MB_YESNO | MB_ICONWARNING) == IDYES) {
					event light = mmap->eve[0];
					for (int i = 0; i < fxhl->afx_dev.GetMappings()->size(); i++) {
						AlienFX_SDK::mapping map = fxhl->afx_dev.GetMappings()->at(i);
						if (!(map.flags && ALIENFX_FLAG_POWER)) {
							lightset* actmap = FindMapping(i);
							if (actmap)
								actmap->eve[0] = light;
							else {
								// create new mapping
								lightset* newmap = CreateMapping(i);
								newmap->eve[0] = light;
							}
						}
					}
					fxhl->RefreshState(true);
				}
			} break;
			} break;
		default: return false;
		}
	} break;
	case WM_VSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			lightset* mmap = FindMapping(lid);
			if (mmap != NULL) {
				if ((HWND)lParam == s1_slider) {
					mmap->eve[0].map[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip, tBuff, mmap->eve[0].map[effID].tempo);
				}
				if ((HWND)lParam == l1_slider) {
					mmap->eve[0].map[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(lTip, lBuff, mmap->eve[0].map[effID].time);
				}
				fxhl->RefreshOne(mmap, true, true);
			}
			break;
		} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_C1:
			AlienFX_SDK::afx_act c;
			if (eItem != -1) {
				lightset* map = FindMapping(lid);
				if (map) {
					c = map->eve[0].map[effID];
				}
			}
			RedrawButton(hDlg, IDC_BUTTON_C1, c.r, c.g, c.b);
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_EFFECTS_LIST:
			switch (((NMHDR*) lParam)->code) {
			case NM_CLICK:
			{
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*)lParam;
				// Select other color....
				effID = sItem->iItem;
				if (effID != -1) {
					lightset* mmap = FindMapping(lid);
					if (mmap != NULL) {
						RebuildEffectList(hDlg, mmap);
					}
				} else {
					effID = 0;
				}
			} break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}

void UpdateMonitoringInfo(HWND hDlg, lightset* map) {
	HWND list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		 list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		 s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		 s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	for (int i = 0; i < 4; i++) {
		CheckDlgButton(hDlg, IDC_CHECK_NOEVENT + i, (map ? map->eve[i].fs.b.flags : 0) ? BST_CHECKED : BST_UNCHECKED);

		if (i > 0) {
			if (map && map->eve[0].fs.b.flags) {
				RedrawButton(hDlg, IDC_BUTTON_CM1 + i - 1, map->eve[0].map[0].r,
					map->eve[0].map[0].g, map->eve[0].map[0].b);
				//EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_CM1 + i), false);
			}
			else {
				RedrawButton(hDlg, IDC_BUTTON_CM1 + i - 1, map ? map->eve[i].map[0].r : 0,
					map ? map->eve[i].map[0].g : 0, map ? map->eve[i].map[0].b : 0);
				//EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_CM1 + i), true);
			}
			RedrawButton(hDlg, IDC_BUTTON_CM4 + i - 1, map ? map->eve[i].map[1].r : 0,
				map ? map->eve[i].map[1].g : 0, map ? map->eve[i].map[1].b : 0);
		}
	}

	// Alarms
	CheckDlgButton(hDlg, IDC_STATUS_BLINK, (map?map->eve[3].fs.b.proc:0) ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(s2_slider, TBM_SETPOS, true, map?map->eve[3].fs.b.cut:0);
	SetSlider(lTip, lBuff, map?map->eve[3].fs.b.cut:0);

	// Events
	CheckDlgButton(hDlg, IDC_GAUGE, (map?map->eve[2].fs.b.proc:0) ? BST_CHECKED : BST_UNCHECKED);
	SendMessage(s1_slider, TBM_SETPOS, true, map?map->eve[2].fs.b.cut:0);
	SetSlider(sTip, tBuff, map?map->eve[2].fs.b.cut:0);

	SendMessage(list_counter, CB_SETCURSEL, map?map->eve[2].source:0, 0);
	SendMessage(list_status, CB_SETCURSEL, map?map->eve[3].source:0, 0);
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS_E),
		mode_light = GetDlgItem(hDlg, IDC_CHECK_NOEVENT),
		mode_power = GetDlgItem(hDlg, IDC_CHECK_POWER),
		mode_perf = GetDlgItem(hDlg, IDC_CHECK_PERF),
		mode_status = GetDlgItem(hDlg, IDC_CHECK_STATUS),
		list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
		list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
		s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
		s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

	int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0),
		lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);

	lightset* map = FindMapping(lid);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (UpdateLightList<FXHelper>(light_list, fxhl) < 0) {
			HWND tab_list = GetParent(hDlg);
			TabCtrl_SetCurSel(tab_list, 2);
			OnSelChanged(tab_list);
			return false;
		}

		// Set counter list...
		char buffer[100];
		LoadString(hInst, IDS_CPU, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_RAM, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_HDD, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_GPU, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_NET, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_TEMP, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_BATT, buffer, 100);
		SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)buffer);
		// Set indicator list
		LoadString(hInst, IDS_A_HDD, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_NET, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_HOT, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_OOM, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		LoadString(hInst, IDS_A_LOWBATT, buffer, 100);
		SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)buffer);
		// Set sliders
		SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 99));
		SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 99));
		//TBM_SETTICFREQ
		SendMessage(s1_slider, TBM_SETTICFREQ, 10, 0);
		SendMessage(s2_slider, TBM_SETTICFREQ, 10, 0);
		sTip = CreateToolTip(s1_slider);
		lTip = CreateToolTip(s2_slider);

		if (eItem != (-1)) {
			SendMessage(light_list, LB_SETCURSEL, eItem, 0);
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS_E, LBN_SELCHANGE), (LPARAM)hDlg);
		}
	} break;
	case WM_COMMAND: {
		int countid = (int)SendMessage(list_counter, CB_GETCURSEL, 0, 0),
			statusid = (int)SendMessage(list_status, CB_GETCURSEL, 0, 0);
		switch (LOWORD(wParam))
		{
		case IDC_LIGHTS_E: // should reload mappings
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				eItem = lbItem;
				UpdateMonitoringInfo(hDlg, map);
				break;
			} break;
		case IDC_CHECK_NOEVENT: case IDC_CHECK_PERF: case IDC_CHECK_POWER: case IDC_CHECK_STATUS:
		{
			int eid = LOWORD(wParam) - IDC_CHECK_NOEVENT;
			if (!(lid < 0)) {
				if (!map) {
					map = CreateMapping(lid);
				}
				map->eve[eid].fs.b.flags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				fxhl->RefreshMon();
				UpdateMonitoringInfo(hDlg, map);
			} else
				CheckDlgButton(hDlg, LOWORD(wParam), BST_UNCHECKED);
		} break;
		case IDC_STATUS_BLINK:
			if (map != NULL)
				map->eve[3].fs.b.proc = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_GAUGE:
			if (map != NULL) {
				map->eve[2].fs.b.proc = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				fxhl->RefreshMon();
			}
			break;
		case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: case IDC_BUTTON_CM4: case IDC_BUTTON_CM5: case IDC_BUTTON_CM6: {
			if (map && HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) < IDC_BUTTON_CM4)
					if (map->eve[0].fs.b.flags)
						SetColor(hDlg, LOWORD(wParam), map, &map->eve[0].map[0]);
					else
						SetColor(hDlg, LOWORD(wParam), map, &map->eve[LOWORD(wParam) - IDC_BUTTON_CM1 + 1].map[0]);
				else
					SetColor(hDlg, LOWORD(wParam), map, &map->eve[LOWORD(wParam) - IDC_BUTTON_CM4 + 1].map[1]);
			}
		} break;
		case IDC_COUNTERLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
					map->eve[2].source = countid;
					fxhl->RefreshMon();
			}
			break;
		case IDC_STATUSLIST:
			if (map && HIWORD(wParam) == CBN_SELCHANGE) {
					map->eve[3].source = statusid;
					fxhl->RefreshMon();
			}
			break;
		}
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: case IDC_BUTTON_CM4: case IDC_BUTTON_CM5: case IDC_BUTTON_CM6: {
			AlienFX_SDK::afx_act c;
			if (map) {
				if (((DRAWITEMSTRUCT*)lParam)->CtlID < IDC_BUTTON_CM4)
					if (map->eve[0].fs.b.flags)
						c = map->eve[0].map[0];
					else
						c = map->eve[((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM1 + 1].map[0];
				else
					c = map->eve[((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM4 + 1].map[1];
			}
			RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.r, c.g, c.b);
		} break;
		}
		break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
			if (map) {
				if ((HWND)lParam == s1_slider) {
					map->eve[2].fs.b.cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip, tBuff, map->eve[2].fs.b.cut);
				}
				if ((HWND)lParam == s2_slider) {
					map->eve[3].fs.b.cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(lTip, lBuff, map->eve[3].fs.b.cut);
				}
			}
			break;
		} break;
	default: return false;
	}
	return true;
}

int UpdateGroupLights(HWND light_list, int gID, int sel) {
	int pos = -1;
	SendMessage(light_list, LB_RESETCONTENT, 0, 0);
	AlienFX_SDK::group* grp = fxhl->afx_dev.GetGroupById(gID);
	if (grp) {
		for (int i = 0; i < grp->lights.size(); i++) {
			pos = (int) SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM) (grp->lights[i]->name.c_str()));
			SendMessage(light_list, LB_SETITEMDATA, pos, i);
		}
		SendMessage(light_list, LB_SETCURSEL, sel, 0);
	}
	return pos;
}

BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND light_view = GetDlgItem(hDlg, IDC_LIST_LIGHTS),
		dev_list = GetDlgItem(hDlg, IDC_DEVICES),
		groups_list = GetDlgItem(hDlg, IDC_GROUPS),
		glights_list = GetDlgItem(hDlg, IDC_LIST_INGROUP),
		light_id = GetDlgItem(hDlg, IDC_LIGHTID);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// First, reset all light devices and re-scan!
		eve->StopProfiles();
		//eve->StopEvents();
		fxhl->UnblockUpdates(false);
		size_t numdev = fxhl->FillDevs(conf->stateOn, conf->offPowerButton),
		       numharddev = fxhl->afx_dev.GetDevices()->size(),
		       lights = fxhl->afx_dev.GetMappings()->size(),
			   numgroups = fxhl->afx_dev.GetGroups()->size();
		fxhl->UnblockUpdates(true);
		//eve->StartEvents();
		eve->StartProfiles();
		// Now check current device list..
		int cpid = (-1), pos = (-1);
		for (UINT i = 0; i < numdev; i++) {
			cpid = fxhl->devs[i]->GetPID();
			int j;
			for (j = 0; j < numharddev; j++) {
				if (cpid == fxhl->afx_dev.GetDevices()->at(j).devid) {
					pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(fxhl->afx_dev.GetDevices()->at(j).name.c_str()));
					SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)cpid);
					break;
				}
			}
			if (j == numharddev) {
				// no name
				char devName[256];
				sprintf_s(devName, 255, "Device #%X", cpid);
				pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(devName));
				SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)cpid);
				// need to add device mapping...
				AlienFX_SDK::devmap newDev;
				newDev.devid = cpid;
				newDev.name = devName;
				fxhl->afx_dev.GetDevices()->push_back(newDev);
				fxhl->afx_dev.SaveMappings();
			}
			if (cpid == conf->lastActive) {
				// select this device.
				SendMessage(dev_list, CB_SETCURSEL, pos, (LPARAM)0);
				eDid = cpid; dItem = pos;
			}
		}

		eLid = -1;

		if (numdev > 0) {
			if (eDid == -1) { // no selection, let's try to select dev#0
				dItem = 0;
				eDid = fxhl->devs[0]->GetPID();
				SendMessage(dev_list, CB_SETCURSEL, 0, (LPARAM)0);
				conf->lastActive = eDid;
			}

			UpdateLightsList(hDlg, eDid, -1);
		}

		// now fill groups....
		if (numgroups > 0) {
			if (gLid < 0)
				gLid = fxhl->afx_dev.GetGroups()->at(0).gid;
			for (UINT i = 0; i < numgroups; i++) {
				pos = (int) SendMessage(groups_list, CB_ADDSTRING, 0, (LPARAM) (fxhl->afx_dev.GetGroups()->at(i).name.c_str()));
				SendMessage(groups_list, CB_SETITEMDATA, pos, (LPARAM) fxhl->afx_dev.GetGroups()->at(i).gid);
				if (fxhl->afx_dev.GetGroups()->at(i).gid == gLid) {
					SendMessage(groups_list, CB_SETCURSEL, pos, (LPARAM) 0);
					gItem = pos;
				}
			}
			UpdateGroupLights(glights_list, gLid,0);
		} else {
			EnableWindow(groups_list, false);
			EnableWindow(glights_list, false);
		}
	} break;
	case WM_COMMAND: {
		int dbItem = (int)SendMessage(dev_list, CB_GETCURSEL, 0, 0);
		int did = (int)SendMessage(dev_list, CB_GETITEMDATA, dbItem, 0);
		int gbItem = (int)SendMessage(groups_list, CB_GETCURSEL, 0, 0);
		int gid = (int)SendMessage(groups_list, CB_GETITEMDATA, gbItem, 0);
		int glItem = (int)SendMessage(glights_list, LB_GETCURSEL, 0, 0);
		AlienFX_SDK::group* grp = fxhl->afx_dev.GetGroupById(gid);
		switch (LOWORD(wParam))
		{
		case IDC_DEVICES: {
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE: {
				conf->lastActive = did;
				UpdateLightsList(hDlg, did, -1);
				eLid = -1;
				eDid = did; dItem = dbItem;
			} break;
			case CBN_EDITCHANGE:
				char buffer[MAX_PATH];
				GetWindowTextA(dev_list, buffer, MAX_PATH);
				UINT i;
				for (i = 0; i < fxhl->afx_dev.GetDevices()->size(); i++) {
					if (fxhl->afx_dev.GetDevices()->at(i).devid == eDid) {
						fxhl->afx_dev.GetDevices()->at(i).name = buffer;
						fxhl->afx_dev.SaveMappings();
						SendMessage(dev_list, CB_DELETESTRING, dItem, 0);
						SendMessage(dev_list, CB_INSERTSTRING, dItem, (LPARAM)(buffer));
						SendMessage(dev_list, CB_SETITEMDATA, dItem, (LPARAM)eDid);
						break;
					}
				}
				break;
			}
		} break;
		case IDC_GROUPS: {
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE: {
				gLid = gid; gItem = gbItem;
				UpdateGroupLights(glights_list, gid,0);
			} break;
			case CBN_EDITCHANGE:
				char buffer[MAX_PATH];
				GetWindowTextA(groups_list, buffer, MAX_PATH);
				AlienFX_SDK::group* cgrp = fxhl->afx_dev.GetGroupById(gLid);
				if (cgrp) {
					cgrp->name = buffer;
					fxhl->afx_dev.SaveMappings();
					SendMessage(groups_list, CB_DELETESTRING, gItem, 0);
					SendMessage(groups_list, CB_INSERTSTRING, gItem, (LPARAM)(buffer));
					SendMessage(groups_list, CB_SETITEMDATA, gItem, (LPARAM)gLid);
				}
				break;
			}
		} break;
		case IDC_BUT_GRP_UP:
			if (grp && glItem > 0) {
				auto gIter = grp->lights.begin();
				iter_swap(gIter + glItem, gIter + glItem -1);
				UpdateGroupLights(glights_list,gLid, glItem-1);
			}
			break;
		case IDC_BUT_GRP_DOWN:
			if (grp && glItem < grp->lights.size() - 1 ) {
				auto gIter = grp->lights.begin();
				iter_swap(gIter + glItem, gIter + glItem +1);
				UpdateGroupLights(glights_list,gLid, glItem+1);
			}
			break;
		case IDC_BUTTON_ADDL: {
			char buffer[MAX_PATH];
			int cid = GetDlgItemInt(hDlg, IDC_LIGHTID, NULL, false);
			// let's check if we have the same ID, need to use max+1 in this case
			unsigned maxID = 0;
			size_t lights = fxhl->afx_dev.GetMappings()->size();
			for (int i = 0; i < lights; i++) {
				AlienFX_SDK::mapping* lgh = &(fxhl->afx_dev.GetMappings()->at(i));
				if (lgh->devid == eDid) {
					if (lgh->lightid > maxID)
						maxID = lgh->lightid;
					if (lgh->lightid == cid)
						cid = maxID + 1;
				}
			}
			AlienFX_SDK::mapping dev;
			dev.devid = eDid;
			dev.lightid = cid;
			sprintf_s(buffer, MAX_PATH, "Light #%d", cid);
			dev.name = buffer;
			fxhl->afx_dev.GetMappings()->push_back(dev);
			fxhl->afx_dev.SaveMappings();
			eLid = cid;
			UpdateLightsList(hDlg, eDid, cid);
			fxhl->TestLight(eDid, cid);
		} break;
		case IDC_BUTTON_ADDG: {
				char buffer[MAX_PATH];
				unsigned maxID = 0x10000;
				size_t lights = fxhl->afx_dev.GetGroups()->size();
				for (int i = 0; i < lights; i++) {
					AlienFX_SDK::group* lgh = &(fxhl->afx_dev.GetGroups()->at(i));
					if (lgh->gid >= maxID)
						maxID = lgh->gid + 1;
				}
				AlienFX_SDK::group dev;
				dev.gid = maxID;
				sprintf_s(buffer, MAX_PATH, "Group #%d", maxID & 0xffff);
				dev.name = buffer;
				fxhl->afx_dev.GetGroups()->push_back(dev);
				fxhl->afx_dev.SaveMappings();
				gLid = maxID;
				int pos = (int) SendMessage(groups_list, CB_ADDSTRING, 0, (LPARAM)(buffer));
				SendMessage(groups_list, CB_SETITEMDATA, pos, (LPARAM)gLid);
				SendMessage(groups_list, CB_SETCURSEL, pos, (LPARAM) 0);
				gItem = pos;
				EnableWindow(groups_list, true);
				EnableWindow(glights_list, true);
				UpdateGroupLights(glights_list,gLid,0);
		} break;
		case IDC_BUTTON_REMG: {
			if (gLid > 0) {
				for (std::vector <AlienFX_SDK::group>::iterator Iter = fxhl->afx_dev.GetGroups()->begin();
					 Iter != fxhl->afx_dev.GetGroups()->end(); Iter++)
					if (Iter->gid == gLid) {
						fxhl->afx_dev.GetGroups()->erase(Iter);
						break;
					}
				// store profile...
				//conf->FindProfile(conf->activeProfile)->lightsets = conf->active_set;
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					 Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, 0, gLid);
				}
				// reset active mappings
				//conf->active_set = &conf->FindProfile(conf->activeProfile)->lightsets;
				fxhl->afx_dev.SaveMappings();
				conf->Save();
				SendMessage(groups_list, CB_DELETESTRING, gItem, 0);
				if (fxhl->afx_dev.GetGroups()->size() > 0) {
					gLid = fxhl->afx_dev.GetGroups()->at(0).gid;
					gItem = 0;
				} else {
					gLid = -1;
					gItem = -1;
					EnableWindow(groups_list, false);
					EnableWindow(glights_list, false);
				}
				SendMessage(groups_list, CB_SETCURSEL, 0, 0);
				UpdateGroupLights(glights_list, gLid,0);
			}
		} break;
		case IDC_BUTTON_REML:
			if (MessageBox(hDlg, "Do you really want to remove current light name and all it's settings from all groups and profiles?", "Warning!",
				MB_YESNO | MB_ICONWARNING) == IDYES) {
				// store profile...
				//conf->FindProfile(conf->activeProfile)->lightsets = conf->active_set;
				// delete from all groups...
				for (int i = 0; i < fxhl->afx_dev.GetGroups()->size(); i++) {
					AlienFX_SDK::group* grp = &fxhl->afx_dev.GetGroups()->at(i);
					for (vector<AlienFX_SDK::mapping*>::iterator gIter = grp->lights.begin();
						 gIter < grp->lights.end(); gIter++)
						if ((*gIter)->devid == eDid && (*gIter)->lightid == eLid) {
							grp->lights.erase(gIter);
							UpdateGroupLights(glights_list,gLid, 0);
							break;
						}
				}
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, eDid, eLid);
				}
				// reset active mappings
				//conf->active_set = &conf->FindProfile(conf->activeProfile)->lightsets;
				std::vector <AlienFX_SDK::mapping>* mapps = fxhl->afx_dev.GetMappings();
				int nLid = -1;
				for (std::vector <AlienFX_SDK::mapping>::iterator Iter = mapps->begin();
					 Iter != mapps->end(); Iter++)
					if (Iter->devid == eDid && Iter->lightid == eLid) {
						mapps->erase(Iter);
						break;
					} else nLid = Iter->lightid;
				fxhl->afx_dev.SaveMappings();
				conf->Save();
				if (IsDlgButtonChecked(hDlg, IDC_ISPOWERBUTTON) == BST_CHECKED) {
					fxhl->ResetPower(did);
					MessageBox(hDlg, "Hardware Power button removed, you may need to reset light system!", "Warning!",
						MB_OK);
					CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				}
				if (nLid < 0) {
					SetDlgItemInt(hDlg, IDC_LIGHTID, 0, false);
				}
				eLid = nLid;
				UpdateLightsList(hDlg, eDid, eLid);
				fxhl->TestLight(eDid, eLid);
			}
			break;
		case IDC_BUT_ADDTOG:
			if (grp && eLid >= 0) {
				//AlienFX_SDK::group* cgrp = fxhl->afx_dev.GetGroupById(gLid);
				//if (cgrp) {
					AlienFX_SDK::mapping* clight = fxhl->afx_dev.GetMappingById(eDid, eLid);
					if (clight) {
						// TODO: check if light into the groups already!
						bool nothislight = true;
						for (int i = 0; i < grp->lights.size(); i++)
							if (grp->lights[i] == clight) {
								nothislight = false;
								break;
							}
						if (nothislight)
							grp->lights.push_back(clight);
					}
				//}
				UpdateGroupLights(glights_list,gLid, grp->lights.size()-1);
			}
			break;
		case IDC_BUT_DELFROMG:
			if (grp && glItem >= 0) {
				//AlienFX_SDK::group* cgrp = fxhl->afx_dev.GetGroupById(gLid);
				if (grp && glItem < grp->lights.size()) {
					std::vector <AlienFX_SDK::mapping*>::iterator Iter = grp->lights.begin() + glItem;
					grp->lights.erase(Iter);
				}
				UpdateGroupLights(glights_list, gLid, --glItem);
			}
			break;
		case IDC_BUTTON_RESETCOLOR:
			if (MessageBox(hDlg, "Do you really want to remove current light control settings from all profiles?", "Warning!",
				MB_YESNO | MB_ICONWARNING) == IDYES) {
				// store profile...
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, eDid, eLid);
				}
				// reset active mappings
				//conf->active_set = conf->FindProfile(conf->activeProfile)->lightsets;
			}
			break;
		case IDC_BUTTON_TESTCOLOR: {
			AlienFX_SDK::afx_act c;
			c.r = conf->testColor.cs.red;
			c.g = conf->testColor.cs.green;
			c.b = conf->testColor.cs.blue;
			SetColor(hDlg, IDC_BUTTON_TESTCOLOR, NULL, &c);
			conf->testColor.cs.red = c.r;
			conf->testColor.cs.green = c.g;
			conf->testColor.cs.blue = c.b;
			if (eLid != -1) {
				eve->StopEvents();
				fxhl->TestLight(did, eLid);
				SetFocus(light_view);
			}
		} break;
		case IDC_ISPOWERBUTTON:
			if (eLid != -1) {
				int flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED ;
				if (flags)
					if (MessageBox(hDlg, "Setting light to Hardware Power button slow down updates and can hang you light system! Are you sure?", "Warning!",
						MB_YESNO | MB_ICONWARNING) == IDYES) {
						flags = fxhl->afx_dev.GetFlags(did, eLid) & 0x2 | flags;
						fxhl->afx_dev.SetFlags(did, eLid, flags);
					}
					else
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				else {
					// remove power button config from chip config if unchecked and confirmed
					if (MessageBox(hDlg, "Hardware Power button disabled, you may need to reset light system! Do you want to reset Power button light as well?", "Warning!",
						MB_YESNO | MB_ICONWARNING) == IDYES)
						fxhl->ResetPower(did);
					flags = fxhl->afx_dev.GetFlags(did, eLid) & 0x2 | flags;
					fxhl->afx_dev.SetFlags(did, eLid, flags);
				}
				fxhl->Refresh(true);
			}
			break;
		case IDC_CHECK_INDICATOR:
		{
			int flags = (fxhl->afx_dev.GetFlags(did, eLid) & 0x1) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 1;
			fxhl->afx_dev.SetFlags(did, eLid, flags);
		} break;
		case IDC_BUTTON_DEVRESET: {
			fxhl->UnblockUpdates(false);
			fxhl->FillDevs(conf->stateOn, conf->offPowerButton);
			fxhl->UnblockUpdates(true);
			AlienFX_SDK::Functions* dev = fxhl->LocateDev(eDid);
			if (dev)
				UpdateLightsList(hDlg, eDid, eLid);
		} break;
		default: return false;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_LIGHTS:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE: {
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*) lParam;
				ListView_EditLabel(light_view, sItem->iItem);
			} break;

			case NM_CLICK:
			{
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*) lParam;
				// Select other item...
				if (sItem->iItem != -1) {
				LVITEMA lItem;
				lItem.mask = LVIF_PARAM;
				lItem.iItem = sItem->iItem;
				ListView_GetItem(light_view, &lItem);
				eLid = (int)lItem.lParam;
					SetDlgItemInt(hDlg, IDC_LIGHTID, eLid, false);
					CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev.GetFlags(eDid, eLid) & ALIENFX_FLAG_POWER ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, fxhl->afx_dev.GetFlags(eDid, eLid) & ALIENFX_FLAG_INACTIVE ? BST_CHECKED : BST_UNCHECKED);
					//eve->StopEvents();
					fxhl->UnblockUpdates(false);
					// highlight to check....
					fxhl->TestLight(eDid, eLid);
				} else {
					SetDlgItemInt(hDlg, IDC_LIGHTID, 0, false);
					CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, BST_UNCHECKED);
					eLid = -1;
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				if (sItem->item.pszText) {
					for (UINT i = 0; i < fxhl->afx_dev.GetMappings()->size(); i++) {
						AlienFX_SDK::mapping* lgh = &(fxhl->afx_dev.GetMappings()->at(i));
						if (lgh->devid == eDid &&
							lgh->lightid == sItem->item.lParam) {
							lgh->name = sItem->item.pszText;
							ListView_SetItem(light_view, &sItem->item);
							ListView_SetColumnWidth(light_view, 0, LVSCW_AUTOSIZE);
							fxhl->afx_dev.SaveMappings();
							break;
						}
					}
				} 
				SetDlgItemInt(hDlg, IDC_LIGHTID, (UINT)sItem->item.lParam, false);
				CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev.GetFlags(eDid, (int)sItem->item.lParam) ? BST_CHECKED : BST_UNCHECKED);
				eve->StopEvents();
				// StopEvents();
				// highlight to check....
				fxhl->TestLight(eDid, (int)sItem->item.lParam);
				return false;
			} break;
			case NM_KILLFOCUS:
				//eve->ToggleEvents();
				fxhl->afx_dev.SaveMappings();
				fxhl->UnblockUpdates(true);
				fxhl->RefreshState();
				break;
			}
			break;
		}
		break;
	case WM_DRAWITEM:
		if (((DRAWITEMSTRUCT*)lParam)->CtlID == IDC_BUTTON_TESTCOLOR)
			RedrawButton(hDlg, IDC_BUTTON_TESTCOLOR, conf->testColor.cs.red, conf->testColor.cs.green, conf->testColor.cs.blue);
		break;
	default: return false;
	}
	return true;
}

void ReloadProfileView(HWND hDlg, int cID) {
	int rpos = 0;
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		profile_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);
	ListView_DeleteAllItems(profile_list);
	LVCOLUMNA lCol;
	lCol.mask = LVCF_WIDTH;
	ListView_GetColumn(profile_list, 0, & lCol);
	if (lCol.cx < 0)
		ListView_InsertColumn(profile_list, 0, &lCol);
	for (int i = 0; i < conf->profiles.size(); i++) {
			LVITEMA lItem; 
			lItem.mask = LVIF_TEXT | LVIF_PARAM;
			lItem.iItem = i;
			lItem.iImage = 0;
			lItem.iSubItem = 0;
			lItem.lParam = conf->profiles[i].id;
			lItem.pszText = (char*)conf->profiles[i].name.c_str();
			if (conf->profiles[i].id == cID) {
				lItem.mask |= LVIF_STATE;
				lItem.state = LVIS_SELECTED;
				CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, conf->profiles[i].flags & PROF_DEFAULT ? BST_CHECKED : BST_UNCHECKED);
				CheckDlgButton(hDlg, IDC_CHECK_NOMON, conf->profiles[i].flags & PROF_NOMONITORING ? BST_CHECKED : BST_UNCHECKED);
				CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, conf->profiles[i].flags & PROF_DIMMED ? BST_CHECKED : BST_UNCHECKED);
				CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, conf->profiles[i].flags & PROF_ACTIVE ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(app_list, LB_RESETCONTENT, 0, 0);
				SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM)(conf->profiles[i].triggerapp.c_str()));
				rpos = i;
			}
			ListView_InsertItem(profile_list, &lItem);
	}
	//RECT csize;
	//GetClientRect(profile_list, &csize);
	ListView_SetColumnWidth(profile_list, 0, LVSCW_AUTOSIZE);// csize.right - csize.left - 1);
	ListView_EnsureVisible(profile_list, rpos, false);
	//return rpos;
}

BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		p_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		ReloadProfileView(hDlg, conf->activeProfile);
		pCid = conf->activeProfile;
	} break;
	case WM_COMMAND:
	{
		profile* prof = conf->FindProfile(pCid);

		switch (LOWORD(wParam))
		{
		case IDC_ADDPROFILE: {
			char buf[128]; unsigned vacID = 0;
			for (int i = 0; i < conf->profiles.size(); i++)
				if (vacID == conf->profiles[i].id) {
					vacID++; i = -1;
				}
			sprintf_s(buf, 128, "Profile %d", vacID);
			profile prof = {vacID, 0, "", buf};
			conf->profiles.push_back(prof);
			conf->profiles.back().lightsets = *conf->active_set;
			ReloadProfileView(hDlg, vacID);
			pCid = vacID;
			ReloadProfileList(NULL);
		} break;
		case IDC_REMOVEPROFILE: {
			if (prof != NULL && !(prof->flags & PROF_DEFAULT) && conf->profiles.size() > 1) {
				if (MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning!",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					// is this active profile? Switch needed!
					if (conf->activeProfile == pCid) {
						// switch to default profile..
						eve->SwitchActiveProfile(conf->defaultProfile);
					}
					// Now remove profile....
					int nCid = conf->activeProfile;
					for (std::vector <profile>::iterator Iter = conf->profiles.begin();
						 Iter != conf->profiles.end(); Iter++)
						if (Iter->id == pCid) { //prid) {
							conf->profiles.erase(Iter);
							break;
						} else
							nCid = Iter->id;
						conf->active_set = &(conf->FindProfile(conf->activeProfile)->lightsets);
						ReloadProfileView(hDlg, nCid);
						pCid = nCid;
						ReloadProfileList(NULL);
				}
			}
			else
				MessageBox(hDlg, "Can't delete last or default profile!", "Error!",
					MB_OK | MB_ICONERROR);
		} break;
		case IDC_BUT_PROFRESET:
			if (prof != NULL && MessageBox(hDlg, "Do you really want to reset all lights settings for this profile?", "Warning!",
				MB_YESNO | MB_ICONWARNING) == IDYES) {
				prof->lightsets.clear();
				//if (prof->id == conf->activeProfile) {
				//	conf->active_set.clear();
				//}
			}
			break;
		case IDC_APP_RESET:
			if (prof != NULL) {
				prof->triggerapp = "";
				SendMessage(app_list, LB_RESETCONTENT, 0, 0);
			}
			break;
		case IDC_APP_BROWSE: {
			if (prof != NULL) {
				// fileopen dialogue...
				OPENFILENAMEA fstruct;
				char* appName = new char[32767];
				strcpy_s(appName, MAX_PATH, prof->triggerapp.c_str());
				ZeroMemory(&fstruct, sizeof(OPENFILENAMEA));
				fstruct.lStructSize = sizeof(OPENFILENAMEA);
				fstruct.hwndOwner = hDlg;
				fstruct.hInstance = hInst;
				fstruct.lpstrFile = appName;
				fstruct.nMaxFile = 32767;
				fstruct.lpstrFilter = "Applications (*.exe)\0*.exe\0\0";
				fstruct.lpstrCustomFilter = NULL;
				fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
				if (GetOpenFileNameA(&fstruct)) {
					prof->triggerapp = appName;
					SendMessage(app_list, LB_RESETCONTENT, 0, 0);
					SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM)(prof->triggerapp.c_str()));
				}
				delete[] appName;
			}
		} break;
		case IDC_CHECK_DEFPROFILE:
			if (prof != NULL) {
				bool nflags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				if (nflags) {
					profile* old_def = conf->FindProfile(conf->defaultProfile);
					if (old_def != NULL)
						old_def->flags = old_def->flags & ~PROF_DEFAULT;
					prof->flags = prof->flags | PROF_DEFAULT;
					conf->defaultProfile = prof->id;
				}
				else
					CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_CHECKED);
			}
			break;
		case IDC_CHECK_NOMON:
			if (prof != NULL) {
				prof->flags = (prof->flags & ~PROF_NOMONITORING) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 1;
				if (prof->id == conf->activeProfile) {
					DWORD oldState = conf->monState;
					conf->monState = prof->flags & PROF_NOMONITORING ? 0 : conf->enableMon;
					if (oldState != conf->monState)
						eve->ToggleEvents();
				}
			}
			break;
		case IDC_CHECK_PROFDIM:
			if (prof != NULL) {
				prof->flags = (prof->flags & ~PROF_DIMMED) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 2;
				if (prof->id == conf->activeProfile) {
					conf->stateDimmed = prof->flags & PROF_DIMMED;
					fxhl->RefreshState();
				}
			}
			break;
		case IDC_CHECK_FOREGROUND:
			if (prof != NULL) {
				prof->flags = (prof->flags & ~PROF_ACTIVE) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 3;
			}
			break;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_PROFILES:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE: {
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*) lParam;
				ListView_EditLabel(p_list, sItem->iItem);
			} break;

			case NM_CLICK:
			{
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*) lParam;
				// Select other item...
				if (sItem->iItem != -1) {
					LVITEMA lItem;
					lItem.mask = LVIF_PARAM;
					lItem.iItem = sItem->iItem;
					ListView_GetItem(p_list, &lItem);
					pCid = (int) lItem.lParam;
					profile* prof = conf->FindProfile(pCid);
					if (prof) {
						CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof->flags & PROF_DEFAULT ? BST_CHECKED : BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_NOMON, prof->flags & PROF_NOMONITORING ? BST_CHECKED : BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof->flags & PROF_DIMMED ? BST_CHECKED : BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, prof->flags & PROF_ACTIVE ? BST_CHECKED : BST_UNCHECKED);
						SendMessage(app_list, LB_RESETCONTENT, 0, 0);
						SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM) (prof->triggerapp.c_str()));
					}
				} else {
					pCid = -1;
					CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_NOMON, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, BST_UNCHECKED);
					SendMessage(app_list, LB_RESETCONTENT, 0, 0);
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				profile* prof = conf->FindProfile((int)sItem->item.lParam);
				if (prof && sItem->item.pszText) {
					prof->name = sItem->item.pszText;
					ListView_SetItem(p_list, &sItem->item);
					ListView_SetColumnWidth(p_list, 0, LVSCW_AUTOSIZE);
					ReloadProfileList(NULL);
					return true;
				} else 
					return false;
			} break;

			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}

BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND dim_slider = GetDlgItem(hDlg, IDC_SLIDER_DIMMING);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// system settings...
		if (conf->startWindows) CheckDlgButton(hDlg, IDC_STARTW, BST_CHECKED);
		if (conf->startMinimized) CheckDlgButton(hDlg, IDC_STARTM, BST_CHECKED);
		if (conf->autoRefresh) CheckDlgButton(hDlg, IDC_AUTOREFRESH, BST_CHECKED);
		if (conf->dimmedBatt) CheckDlgButton(hDlg, IDC_BATTDIM, BST_CHECKED);
		if (conf->offWithScreen) CheckDlgButton(hDlg, IDC_SCREENOFF, BST_CHECKED);
		if (conf->enableMon) CheckDlgButton(hDlg, IDC_BUT_MONITOR, BST_CHECKED);
		if (conf->lightsOn) CheckDlgButton(hDlg, IDC_CHECK_LON, BST_CHECKED);
		if (conf->dimmed) CheckDlgButton(hDlg, IDC_CHECK_DIM, BST_CHECKED);
		if (conf->dimPowerButton) CheckDlgButton(hDlg, IDC_POWER_DIM, BST_CHECKED);
		if (conf->gammaCorrection) CheckDlgButton(hDlg, IDC_CHECK_GAMMA, BST_CHECKED);
		if (conf->offPowerButton) CheckDlgButton(hDlg, IDC_OFFPOWERBUTTON, BST_CHECKED);
		if (conf->enableProf) CheckDlgButton(hDlg, IDC_BUT_PROFILESWITCH, BST_CHECKED);
		if (conf->awcc_disable) CheckDlgButton(hDlg, IDC_AWCC, BST_CHECKED);
		if (conf->esif_temp) CheckDlgButton(hDlg, IDC_ESIFTEMP, BST_CHECKED);
		SendMessage(dim_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(dim_slider, TBM_SETTICFREQ, 16, 0);
		SendMessage(dim_slider, TBM_SETPOS, true, conf->dimmingPower);
		sTip = CreateToolTip(dim_slider);
		SetSlider(sTip, tBuff, conf->dimmingPower);
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_STARTM:
			conf->startMinimized = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_AUTOREFRESH:
			conf->autoRefresh = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_STARTW:
			conf->startWindows = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_BATTDIM:
			conf->dimmedBatt = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->RefreshState();
			break;
		case IDC_SCREENOFF:
			conf->offWithScreen = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_BUT_MONITOR:
			conf->enableMon = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			eve->ToggleEvents();
			break;
		case IDC_BUT_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProf = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			ReloadProfileList(NULL);
			eve->StartProfiles();
			break;
		case IDC_CHECK_LON:
			conf->lightsOn = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->ChangeState();
			break;
		case IDC_CHECK_DIM:
			conf->dimmed = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			//fxhl->RefreshState();
			fxhl->ChangeState();
			break;
		case IDC_CHECK_GAMMA:
			conf->gammaCorrection = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->RefreshState();
			break;
		case IDC_OFFPOWERBUTTON:
			conf->offPowerButton = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			//fxhl->RefreshState();
			fxhl->ChangeState();
			break;
		case IDC_POWER_DIM:
			conf->dimPowerButton = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			//fxhl->RefreshState();
			fxhl->ChangeState();
			break;
		case IDC_AWCC:
			conf->awcc_disable = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (!conf->awcc_disable) {
				if (conf->wasAWCC) DoStopService(false);
			}
			else
				conf->wasAWCC = DoStopService(true);
			break;
		case IDC_ESIFTEMP:
			conf->esif_temp = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (conf->esif_temp)
				EvaluteToAdmin(); // Check admin rights!
			break;
		default: return false;
		}
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBPOSITION: case TB_ENDTRACK: {
			if ((HWND)lParam == dim_slider) {
				conf->dimmingPower = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				SetSlider(sTip, tBuff, conf->dimmingPower);
			}
			//fxhl->RefreshState();
			fxhl->ChangeState();
		} break;
		} break;
	default: return false;
	}
	return true;
}