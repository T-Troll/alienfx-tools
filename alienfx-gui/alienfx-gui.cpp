#include "alienfx-gui.h"
#include <ColorDlg.h>
#include <Dbt.h>
#include "EventHandler.h"
#include "common.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")
#pragma comment(lib,"comctl32.lib")

// Global Variables:
HINSTANCE hInst;
bool isNewVersion = false;
bool needUpdateFeedback = false;
bool needRemove = false;

void ResetDPIScale();

HWND InitInstance(HINSTANCE, int);

BOOL CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabLightsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

FXHelper* fxhl;
ConfigHandler* conf;
EventHandler* eve;
AlienFan_SDK::Control* acpi = NULL;
ConfigFan* fan_conf = NULL;
MonHelper* mon = NULL;

HWND mDlg = NULL, dDlg = NULL;

// color selection:
AlienFX_SDK::afx_act* mod;
HANDLE stopColorRefresh = 0;

// tooltips
HWND sTip1 = 0, sTip2 = 0, sTip3 = 0;

// Tab selections:
int tabSel = 0;
int tabLightSel = 0;

// Explorer restart event
UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));

// last light selected
int eItem = -1;

// Effect mode list
vector<string> effModes{ "Off", "Monitoring", "Ambient", "Haptics", "Grid"};

bool DoStopService(bool kind) {
	if (conf->awcc_disable) {
		conf->Save();
		EvaluteToAdmin();
		// Get a handle to the SCM database.
		SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
		if (!schSCManager)
			return false;
		else
			return kind ? StopService(schSCManager, "AWCCService") : DemandService(schSCManager, "AWCCService");
	}
	return false;
}
bool DetectFans() {
	conf->fanControl = true;
	if (!IsUserAnAdmin()) {
		conf->Save();
		EvaluteToAdmin();
	}
	acpi = new AlienFan_SDK::Control();
	bool isProbe = false;
	if (acpi->IsActivated())
		if (isProbe = acpi->Probe())
			conf->fan_conf->SetBoosts(acpi);
		else
			ShowNotification(&conf->niData, "Error", "Compatible hardware not found, disabling fan control!", false);
	else
		ShowNotification(&conf->niData, "Error", "Fan control start failure, disabling fan control!", false);
	if (!isProbe) {
		delete acpi;
		acpi = NULL;
		conf->fanControl = false;
	}
	return isProbe;
}

void SwitchTab(int);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	ResetDPIScale();

	conf = new ConfigHandler();
	//if (conf->haveOldConfig && MessageBox(NULL, "Old configuration detected. Do you want to convert it?", "Warning",
	//	MB_YESNO | MB_ICONWARNING) == IDYES) {
	//	// convert config call
	//	ShellExecute(NULL, "open", "alienfx-conv.exe", NULL, ".", SW_NORMAL);
	//	return 0;
	//}
	conf->Load();

	fan_conf = conf->fan_conf;

	if (conf->activeProfile->flags & PROF_FANS)
		fan_conf->lastProf = &conf->activeProfile->fansets;

	conf->wasAWCC = DoStopService(true);

	if (conf->esif_temp)
		EvaluteToAdmin();

	fxhl = new FXHelper();

	if (!(InitInstance(hInstance, conf->startMinimized ? SW_HIDE : SW_NORMAL)))
		return FALSE;

	if (conf->fanControl) {
		conf->fanControl = DetectFans();
	}

	if (!fxhl->FillAllDevs(acpi) && !conf->fanControl)
		ShowNotification(&conf->niData, "Error", "No Alienware light devices detected!", false);

	eve = new EventHandler();
	eve->StartEffects();
	eve->StartProfiles();

	SwitchTab(TAB_LIGHTS);

	//register global hotkeys...
	RegisterHotKey(mDlg, 1, MOD_CONTROL | MOD_SHIFT, VK_F12);
	RegisterHotKey(mDlg, 2, MOD_CONTROL | MOD_SHIFT, VK_F11);
	RegisterHotKey(mDlg, 3, 0, VK_F18);
	RegisterHotKey(mDlg, 4, MOD_CONTROL | MOD_SHIFT, VK_F10);
	RegisterHotKey(mDlg, 5, MOD_CONTROL | MOD_SHIFT, VK_F9 );
	RegisterHotKey(mDlg, 6, 0, VK_F17);
	//profile change hotkeys...
	for (int i = 0; i < 10; i++)
		RegisterHotKey(mDlg, 10+i, MOD_CONTROL | MOD_SHIFT, 0x30 + i); // 1,2,3...
	//power mode hotkeys
	for (int i = 0; i < 6; i++)
		RegisterHotKey(mDlg, 30+i, MOD_CONTROL | MOD_ALT, 0x30 + i); // 0,1,2...
	// Power notifications...
	RegisterPowerSettingNotification(mDlg, &GUID_MONITOR_POWER_ON, 0);

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

	if (conf->wasAWCC) DoStopService(false);

	delete fxhl;

	if (acpi) {
		delete acpi;
	}

	delete conf;

	return 0;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;
	CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINWINDOW), NULL, (DLGPROC)MainDialog);

	if (mDlg) {

		SendMessage(mDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXGUI)));
		SendMessage(mDlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXGUI), IMAGE_ICON, 16, 16, 0));

		ShowWindow(mDlg, nCmdShow);
	}
	return mDlg;
}

void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode* act) {
	RECT rect;
	HBRUSH Brush = NULL;
	HWND tl = GetDlgItem(hDlg, id);
	GetClientRect(tl, &rect);
	HDC cnt = GetWindowDC(tl);
	// BGR!
	if (act) {
		Brush = CreateSolidBrush(RGB(act->r, act->g, act->b));
		FillRect(cnt, &rect, Brush);
		DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
	}
	else {
		Brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		FillRect(cnt, &rect, Brush);
	}
	DeleteObject(Brush);
	ReleaseDC(tl, cnt);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		SetDlgItemText(hDlg, IDC_STATIC_VERSION, ("Version: " + GetAppVersion()).c_str());
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
			case NM_CLICK:
			case NM_RETURN:
			{
				ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools", NULL, NULL, SW_SHOWNORMAL);
			} break;
			} break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void ResizeTab(HWND tab, RECT &rcDisplay) {
	RECT dRect;
	GetClientRect(tab, &dRect);
	int deltax = dRect.right - (rcDisplay.right - rcDisplay.left),
		deltay = dRect.bottom - (rcDisplay.bottom - rcDisplay.top);
	if (deltax || deltay) {
		//DebugPrint("Resize needed!\n");
		GetWindowRect(GetParent(GetParent(tab)), &dRect);
		SetWindowPos(GetParent(GetParent(tab)), NULL, 0, 0, dRect.right - dRect.left + deltax, dRect.bottom - dRect.top + deltay, SWP_NOZORDER | SWP_NOMOVE);
		rcDisplay.right += deltax;
		rcDisplay.bottom += deltay;
	}
	SetWindowPos(tab, NULL,
		rcDisplay.left, rcDisplay.top,
		rcDisplay.right - rcDisplay.left,
		rcDisplay.bottom - rcDisplay.top,
		SWP_SHOWWINDOW /*| SWP_NOSIZE*/);
}

void OnSelChanged(HWND hwndDlg)
{
	// Get the dialog header data.
	DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA);

	// Get the index of the selected tab.
	tabSel = TabCtrl_GetCurSel(hwndDlg);
	if (tabSel == TAB_LIGHTS && !fxhl->numActiveDevs) {
		TabCtrl_SetCurSel(hwndDlg, TAB_SETTINGS);
		tabSel = TAB_SETTINGS;
	}

	// Destroy the current child dialog box, if any.
	if (pHdr->hwndDisplay != NULL) {
		//EndDialog(pHdr->hwndDisplay, IDOK);
		DestroyWindow(pHdr->hwndDisplay);
		pHdr->hwndDisplay = NULL;
	}

	RECT rcDisplay;

	GetClientRect(hwndDlg, &rcDisplay);
	rcDisplay.left = GetSystemMetrics(SM_CXBORDER);
	rcDisplay.top = GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER);
	rcDisplay.right -= 2 * GetSystemMetrics(SM_CXBORDER) + 1;
	rcDisplay.bottom -= GetSystemMetrics(SM_CYBORDER) + 1;

	HWND newDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[tabSel], hwndDlg, pHdr->apProc[tabSel]/*tdl*/);
	if (pHdr->hwndDisplay == NULL)
		pHdr->hwndDisplay = newDisplay;

	if (pHdr->hwndDisplay != NULL) {
		ResizeTab(pHdr->hwndDisplay, rcDisplay);
	}
}

void SwitchTab(int num) {
	HWND tab_list = GetDlgItem(mDlg, IDC_TAB_MAIN);
	TabCtrl_SetCurSel(tab_list, num);
	OnSelChanged(tab_list);
}

void SwitchLightTab(HWND dlg, int num) {
	HWND tab_list = GetDlgItem(mDlg, IDC_TAB_LIGHTS);
	TabCtrl_SetCurSel(tab_list, num);
	OnSelChanged(tab_list);
}

void ReloadModeList(HWND mode_list = NULL, int mode = conf->GetEffect()) {
	if (mode_list == NULL) {
		mode_list = GetDlgItem(mDlg, IDC_EFFECT_MODE);
		EnableWindow(mode_list, conf->enableMon);
	}
	UpdateCombo(mode_list, effModes, mode, { 0,1,2,3,4 });
	if (conf->haveV5) {
		ComboBox_SetItemData(mode_list, ComboBox_AddString(mode_list, "Global"), 99);
		if (mode == 99)
			ComboBox_SetCurSel(mode_list, effModes.size());
	}
}

void ReloadProfileList() {
	HWND tab_list = GetDlgItem(mDlg, IDC_TAB_MAIN),
		profile_list = GetDlgItem(mDlg, IDC_PROFILES),
		mode_list = GetDlgItem(mDlg, IDC_EFFECT_MODE);
	ComboBox_ResetContent(profile_list);
	for (int i = 0; i < conf->profiles.size(); i++) {
		ComboBox_SetItemData(profile_list, ComboBox_AddString(profile_list, conf->profiles[i]->name.c_str()), conf->profiles[i]->id);
		if (conf->profiles[i]->id == conf->activeProfile->id) {
			ComboBox_SetCurSel(profile_list, i);
			ComboBox_SetCurSel(mode_list, conf->profiles[i]->effmode);
		}
	}

	EnableWindow(mode_list, conf->enableMon);

	switch (tabSel) {
	case TAB_LIGHTS: case TAB_FANS:
		OnSelChanged(tab_list);
	}

	ReloadModeList();

	DebugPrint("Profile list reloaded.\n");
}

void UpdateState() {
	fxhl->ChangeState();
	if (tabSel == TAB_SETTINGS)
		OnSelChanged(GetDlgItem(mDlg, IDC_TAB_MAIN));
}

void RestoreApp() {
	ShowWindow(mDlg, SW_RESTORE);
	SetForegroundWindow(mDlg);
	//ReloadProfileList();
}

void CreateTabControl(HWND parent, vector<string> names, vector<DWORD> resID, vector<DLGPROC> func) {

	DLGHDR* pHdr = (DLGHDR*)LocalAlloc(LPTR, sizeof(DLGHDR));
	SetWindowLongPtr(parent, GWLP_USERDATA, (LONG_PTR)pHdr);

	int tabsize = (int)names.size();

	pHdr->apRes = (DLGTEMPLATE**)LocalAlloc(LPTR, tabsize * sizeof(DLGTEMPLATE*));
	pHdr->apProc = (DLGPROC*)LocalAlloc(LPTR, tabsize * sizeof(DLGPROC));

	TCITEM tie{ TCIF_TEXT };

	for (int i = 0; i < tabsize; i++) {
		pHdr->apRes[i] = (DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(resID[i]), RT_DIALOG)));
		tie.pszText = (LPSTR)names[i].c_str();
		SendMessage(parent, TCM_INSERTITEM, i, (LPARAM)&tie);
		pHdr->apProc[i] = func[i];
	}
}

BOOL CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND tab_list = GetDlgItem(hDlg, IDC_TAB_MAIN),
		profile_list = GetDlgItem(hDlg, IDC_PROFILES),
		mode_list = GetDlgItem(hDlg, IDC_EFFECT_MODE);

	if (message == newTaskBar) {
		// Started/restarted explorer...
		Shell_NotifyIcon(NIM_ADD, &conf->niData);
		if (conf->updateCheck)
			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		return true;
	}

	switch (message)
	{
	case WM_INITDIALOG:
	{
		mDlg = hDlg;

		CreateTabControl(tab_list,
			{"Lights", "Fans and Power", "Profiles", "Settings"},
			{ IDD_DIALOG_LIGHTS, IDD_DIALOG_FAN, IDD_DIALOG_PROFILES, IDD_DIALOG_SETTINGS},
			{ (DLGPROC)TabLightsDialog, (DLGPROC)TabFanDialog, (DLGPROC)TabProfilesDialog, (DLGPROC)TabSettingsDialog }
			);

		//ReloadModeList();
		ReloadProfileList();

		conf->niData.hWnd = hDlg;
		conf->SetIconState();

		if (Shell_NotifyIcon(NIM_ADD, &conf->niData) && conf->updateCheck) {
			// check update....
			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		}

		conf->SetStates();

	} break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_BUT_PREV: case IDC_BUT_NEXT: case IDC_BUT_LAST: case IDC_BUT_FIRST:
			if (dDlg)
				SendMessage(dDlg, message, wParam, lParam);
			break;
		case IDOK: case IDCANCEL: case IDCLOSE: case IDM_EXIT:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
			break;
		case IDM_HELP:
			ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools/blob/master/Doc/alienfx-gui.md", NULL, NULL, SW_SHOWNORMAL);
			break;
		case IDM_CHECKUPDATE:
			// check update....
			needUpdateFeedback = true;
			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
			break;
		case IDC_BUTTON_MINIMIZE:
			SendMessage(hDlg, WM_SIZE, SIZE_MINIMIZED, 0);
			break;
		case IDC_BUTTON_REFRESH:
			fxhl->Refresh();
			ReloadProfileList();
			break;
		case IDC_BUTTON_SAVE:
			conf->afx_dev.SaveMappings();
			conf->Save();
			fxhl->Refresh(2); // set def. colors
			ShowNotification(&conf->niData, "Configuration saved!", "Configuration saved successfully.", true);
			break;
		case IDC_EFFECT_MODE:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				conf->activeProfile->effmode = (WORD) ComboBox_GetItemData(mode_list, ComboBox_GetCurSel(mode_list));
				eve->ChangeEffectMode();
				if (tabSel == TAB_LIGHTS)
					OnSelChanged(tab_list);
			} break;
			}
		} break;
		case IDC_PROFILES: {
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE: {
				int prid = (int)ComboBox_GetItemData(profile_list, ComboBox_GetCurSel(profile_list));
				eve->SwitchActiveProfile(conf->FindProfile(prid));
				eve->profileChanged = false;
				ReloadModeList();
				OnSelChanged(tab_list);
			} break;
			}
		} break;
		} break;
	} break;
	case WM_NOTIFY: {
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_MAIN: {
			if (((NMHDR*)lParam)->code == TCN_SELCHANGE) {
				if (TabCtrl_GetCurSel(tab_list) == TAB_FANS && !mon) {
					//if (MessageBox(NULL, "Fan control disabled!\nDo you want to enable it?", "Warning",
					//	MB_YESNO | MB_ICONWARNING) == IDYES)
						if (DetectFans())
							eve->StartFanMon();
					if (!mon)
						TabCtrl_SetCurSel(tab_list, TAB_SETTINGS);
				}
				OnSelChanged(tab_list);
			}
		} break;
		}
	} break;
	case WM_WINDOWPOSCHANGING: {
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		if (pos->flags & SWP_SHOWWINDOW && eve) {
			eve->StopProfiles();
		} else
			if (pos->flags & SWP_HIDEWINDOW && eve) {
				eve->StartProfiles();
			}
			else
			if (!(pos->flags & SWP_NOSIZE) && (pos->cx || pos->cy)) {
				RECT oldRect;
				GetWindowRect(mDlg, &oldRect);
				int deltax = pos->cx - oldRect.right + oldRect.left,
					deltay = pos->cy - oldRect.bottom + oldRect.top;
				if (deltax || deltay) {
					GetWindowRect(tab_list, &oldRect);
					SetWindowPos(tab_list, NULL, 0, 0, oldRect.right - oldRect.left + deltax, oldRect.bottom - oldRect.top + deltay, SWP_NOZORDER | SWP_NOMOVE);
					GetWindowRect(GetDlgItem(mDlg, IDC_PROFILES), &oldRect);
					SetWindowPos(GetDlgItem(mDlg, IDC_PROFILES), NULL, 0, 0, oldRect.right - oldRect.left + deltax, oldRect.bottom - oldRect.top, SWP_NOOWNERZORDER | SWP_NOMOVE);
					GetWindowRect(GetDlgItem(mDlg, IDC_EFFECT_MODE), &oldRect);
					ScreenToClient(mDlg, (LPPOINT)&oldRect);
					SetWindowPos(GetDlgItem(mDlg, IDC_EFFECT_MODE), NULL, oldRect.left + deltax, oldRect.top, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
					GetWindowRect(GetDlgItem(mDlg, IDC_STATIC_EFFECTS), &oldRect);
					ScreenToClient(mDlg, (LPPOINT)&oldRect);
					SetWindowPos(GetDlgItem(mDlg, IDC_STATIC_EFFECTS), NULL, oldRect.left + deltax, oldRect.top, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
					GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_REFRESH), &oldRect);
					ScreenToClient(mDlg, (LPPOINT)&oldRect);
					SetWindowPos(GetDlgItem(mDlg, IDC_BUTTON_REFRESH), NULL, oldRect.left, oldRect.top + deltay, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
					GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_MINIMIZE), &oldRect);
					ScreenToClient(mDlg, (LPPOINT)&oldRect);
					SetWindowPos(GetDlgItem(mDlg, IDC_BUTTON_MINIMIZE), NULL, oldRect.left + deltax, oldRect.top + deltay, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
					GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_SAVE), &oldRect);
					ScreenToClient(mDlg, (LPPOINT)&oldRect);
					SetWindowPos(GetDlgItem(mDlg, IDC_BUTTON_SAVE), NULL, oldRect.left + deltax, oldRect.top + deltay, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				}
			}
		return false;
	} break;
	case WM_SIZE:
		switch (wParam) {
		case SIZE_MINIMIZED: {
			// go to tray...
			ShowWindow(hDlg, SW_HIDE);
		} break;
		}
		break;
	case WM_ACTIVATE:
		if (wParam & 3 && eve && eve->profileChanged) {
			//DebugPrint(("Activated " + to_string(wParam) + "\n").c_str());
			ReloadProfileList();
			eve->profileChanged = false;
		}
		break;
	case WM_APP + 1: {
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
			RestoreApp();
			break;
		case WM_RBUTTONUP: case WM_CONTEXTMENU:
		{
			POINT lpClickPoint;
			HMENU tMenu = LoadMenuA(hInst, MAKEINTRESOURCEA(IDR_MENU_TRAY));
			tMenu = GetSubMenu(tMenu, 0);
			MENUINFO mi{ sizeof(mi), MIM_STYLE, MNS_NOTIFYBYPOS };
			SetMenuInfo(tMenu, &mi);
			MENUITEMINFO mInfo{ sizeof(MENUITEMINFO), MIIM_STRING | MIIM_ID };
			HMENU pMenu;
			// add profiles...
			if (!conf->enableProf) {
				pMenu = CreatePopupMenu();
				mInfo.wID = ID_TRAYMENU_PROFILE_SELECTED;
				for (int i = 0; i < conf->profiles.size(); i++) {
					mInfo.dwTypeData = (LPSTR) conf->profiles[i]->name.c_str();
					InsertMenuItem(pMenu, i, false, &mInfo);
					if (conf->profiles[i]->id == conf->activeProfile->id)
						CheckMenuItem(pMenu, i, MF_BYPOSITION | MF_CHECKED);
				}
				ModifyMenu(tMenu, ID_TRAYMENU_PROFILES, MF_BYCOMMAND | MF_STRING | MF_POPUP, (UINT_PTR) pMenu, "Profiles...");
			} else
				ModifyMenu(tMenu, ID_TRAYMENU_PROFILES, MF_STRING, NULL, ("Profile - " + conf->activeProfile->name).c_str());
			// add effects menu...
			if (conf->enableMon) {
				pMenu = CreatePopupMenu();
				mInfo.wID = ID_TRAYMENU_MONITORING_SELECTED;
				int i = 0;
				for (i=0; i < effModes.size(); i++) {
					mInfo.dwTypeData = (LPSTR)effModes[i].c_str();
					InsertMenuItem(pMenu, i, false, &mInfo);
				}
				if (conf->haveV5) {
					mInfo.dwTypeData = "Global";
					InsertMenuItem(pMenu, i, false, &mInfo);
				}
				CheckMenuItem(pMenu, conf->GetEffect() == 99 ? (UINT)effModes.size() : conf->GetEffect(), MF_BYPOSITION | MF_CHECKED);
				ModifyMenu(tMenu, ID_TRAYMENU_MONITORING, MF_BYCOMMAND | MF_POPUP | MF_STRING, (UINT_PTR) pMenu, "Effects...");
			}

			CheckMenuItem(tMenu, ID_TRAYMENU_ENABLEEFFECTS, conf->enableMon ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_LIGHTSON, conf->lightsOn ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_DIMLIGHTS, conf->IsDimmed() ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_PROFILESWITCH, conf->enableProf? MF_CHECKED : MF_UNCHECKED);

			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(hDlg);
			TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
				lpClickPoint.x, lpClickPoint.y, 0, hDlg, NULL);
		} break;
		case NIN_BALLOONHIDE : case NIN_BALLOONTIMEOUT:
			if (!isNewVersion && needRemove) {
				Shell_NotifyIcon(NIM_DELETE, &conf->niData);
				Shell_NotifyIcon(NIM_ADD, &conf->niData);
				needRemove = false;
			} else
				isNewVersion = false;
			break;
		case NIN_BALLOONUSERCLICK:
		{
			if (isNewVersion) {
				ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools/releases", NULL, NULL, SW_SHOWNORMAL);
				isNewVersion = false;
			}
		} break;
		}
		break;
	} break;
	case WM_MENUCOMMAND: {
		int idx = LOWORD(wParam);
		switch (GetMenuItemID((HMENU)lParam, idx)) {
		case ID_TRAYMENU_EXIT:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		    return true;
		case ID_TRAYMENU_REFRESH:
			fxhl->Refresh();
			break;
		case ID_TRAYMENU_LIGHTSON:
			conf->lightsOn = !conf->lightsOn;
			UpdateState();
			eve->ChangeEffectMode();
			break;
		case ID_TRAYMENU_DIMLIGHTS:
		    conf->SetDimmed();
			UpdateState();
			break;
		case ID_TRAYMENU_ENABLEEFFECTS:
			conf->enableMon = !conf->enableMon;
			eve->ChangeEffectMode();
			break;
		case ID_TRAYMENU_MONITORING_SELECTED:
			conf->activeProfile->effmode = idx < effModes.size() ? idx : 99;
			eve->ChangeEffectMode();
			break;
		case ID_TRAYMENU_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProf = !conf->enableProf;
			eve->StartProfiles();
			break;
		case ID_TRAYMENU_RESTORE:
			RestoreApp();
			break;
		case ID_TRAYMENU_PROFILE_SELECTED: {
			if (conf->profiles[idx]->id != conf->activeProfile->id) {
				eve->SwitchActiveProfile(conf->profiles[idx]);
			}
		} break;
		}
		//ReloadProfileList();
	} break;
	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC: {
			// resume from sleep/hibernate

			DebugPrint("Resume from Sleep/hibernate initiated\n");

			if (fxhl->FillAllDevs(acpi)) {
				eve->ChangeScreenState();
				eve->ChangePowerState();
				//conf->SetStates();
				//eve->ChangeEffectMode();
				//eve->StartProfiles();
			}
			fxhl->Start();
			eve = new EventHandler();
			eve->StartEffects();
			eve->StartProfiles();

			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
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

			DebugPrint("Sleep/hibernate initiated\n");

			// need to restore lights if screen off (lid closed)
			conf->stateScreen = true;
			fxhl->ChangeState();
			delete eve;
			//fxhl->Refresh(2);
			fxhl->Stop();
			break;
		}
		break;
	case WM_DEVICECHANGE:
		if (wParam == DBT_DEVNODES_CHANGED) {
			vector<pair<WORD, WORD>> devs = conf->afx_dev.AlienFXEnumDevices();
			if (devs.size() != fxhl->numActiveDevs) {
				// Device added or removed, need to rescan devices...
				fxhl->Stop();
				//bool wasNotLocked = !fxhl->updateLock;
				//if (wasNotLocked) {
				//	fxhl->UnblockUpdates(false, true);
				//}
				fxhl->FillAllDevs(acpi);
				fxhl->Start();
				//if (wasNotLocked)
				//	fxhl->UnblockUpdates(true, true);
			}
		}
		break;
	case WM_DISPLAYCHANGE:
		// Monitor configuration changed
	    if (eve->capt) eve->capt->Restart();
	break;
	case WM_ENDSESSION:
		// Shutdown/restart scheduled....
		DebugPrint("Shutdown initiated\n");
		conf->Save();
		conf->fan_conf->Save();
		delete eve;
		fxhl->Refresh(2);
		//fxhl->UnblockUpdates(false);
		fxhl->Stop();
		return 0;
	case WM_HOTKEY:
		if (wParam > 9 && wParam < 21) {
			if (wParam == 10)
				eve->SwitchActiveProfile(conf->FindDefaultProfile());
			else
				if (wParam - 11 < conf->profiles.size())
					eve->SwitchActiveProfile(conf->profiles[wParam - 11]);
			ReloadProfileList();
			break;
		}
		if (wParam > 29 && wParam < 36 && acpi && wParam - 30 < acpi->HowManyPower()) {
			conf->fan_conf->lastProf->powerStage = (WORD)wParam - 30;
			acpi->SetPower(conf->fan_conf->lastProf->powerStage);
			if (tabSel == TAB_FANS)
				OnSelChanged(tab_list);
			break;
		}
		switch (wParam) {
		case 1: // on/off
			conf->lightsOn = !conf->lightsOn;
			UpdateState();
			break;
		case 2: // dim
		    conf->SetDimmed();
			UpdateState();
			break;
		case 3: // off-dim-full circle
			if (conf->lightsOn) {
				if (conf->IsDimmed())
					conf->lightsOn = false;
				conf->SetDimmed();
			} else
				conf->lightsOn = true;
			UpdateState();
			break;
		case 4: // effects
			conf->enableMon = !conf->enableMon;
			eve->ChangeEffectMode();
			ComboBox_SetCurSel(mode_list, conf->GetEffect());
			break;
		case 5: // profile autoswitch
			eve->StopProfiles();
			conf->enableProf = !conf->enableProf;
			eve->StartProfiles();
			ReloadProfileList();
			break;
		case 6: // G-key for Dell G-series power switch
			if (acpi && mon->oldGmode >= 0) {
				conf->fan_conf->lastProf->gmode = !conf->fan_conf->lastProf->gmode;
				acpi->SetGMode(conf->fan_conf->lastProf->gmode);
				if (IsWindowVisible(hDlg) && tabSel == TAB_FANS)
					OnSelChanged(tab_list);
			}
			break;
		default: return false;
		}
		break;
	case WM_CLOSE:
		fxhl->Refresh(2);
		//EndDialog(hDlg, IDOK);
		DestroyWindow(hDlg);
		break;
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &conf->niData);
		conf->Save();
		PostQuitMessage(0); break;
	default: return false;
	}
	return true;
}


AlienFX_SDK::Colorcode *Act2Code(AlienFX_SDK::afx_act *act) {
	return new AlienFX_SDK::Colorcode({act->b,act->g,act->r});
}

AlienFX_SDK::afx_act *Code2Act(AlienFX_SDK::Colorcode *c) {
	return new AlienFX_SDK::afx_act({0,0,0,c->r,c->g,c->b});
}

DWORD CColorRefreshProc(LPVOID param) {
	AlienFX_SDK::afx_act last = *mod;
	groupset* mmap = (groupset*)param;
	int delay = conf->afx_dev.GetGroupById(mmap->group)->have_power ? 1000 : 200;
	while (WaitForSingleObject(stopColorRefresh, delay)) {
		if (last.r != mod->r || last.g != mod->g || last.b != mod->b) {
			last = *mod;
			if (mmap) fxhl->RefreshOne(mmap);
		}
	}
	return 0;
}

UINT_PTR Lpcchookproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	DRAWITEMSTRUCT* item = 0;

	switch (message)
	{
	case WM_INITDIALOG:
		mod = (AlienFX_SDK::afx_act*)((CHOOSECOLOR*)lParam)->lCustData;
		break;
	case WM_CTLCOLOREDIT:
		mod->r = GetDlgItemInt(hDlg, COLOR_RED, NULL, false);
		mod->g = GetDlgItemInt(hDlg, COLOR_GREEN, NULL, false);
		mod->b = GetDlgItemInt(hDlg, COLOR_BLUE, NULL, false);
		break;
	}
	return 0;
}

bool SetColor(HWND hDlg, int id, groupset* mmap, AlienFX_SDK::afx_act* map) {
	CHOOSECOLOR cc{ sizeof(cc), hDlg, NULL, RGB(map->r, map->g, map->b), (LPDWORD)conf->customColors,
		CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR | CC_ENABLEHOOK, (LPARAM)map, Lpcchookproc };
	bool ret;

	AlienFX_SDK::afx_act savedColor = *map;

	mod = map;
	stopColorRefresh = CreateEvent(NULL, false, false, NULL);
	HANDLE crRefresh = CreateThread(NULL, 0, CColorRefreshProc, mmap, 0, NULL);

	if (!(ret=ChooseColor(&cc)))
		(*map) = savedColor;

	SetEvent(stopColorRefresh);
	WaitForSingleObject(crRefresh, 500);
	CloseHandle(crRefresh);
	CloseHandle(stopColorRefresh);

	fxhl->RefreshOne(mmap);
	RedrawButton(hDlg, id, Act2Code(map));

	return ret;
}

bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode *clr) {
	CHOOSECOLOR cc{ sizeof(cc), hDlg };
	bool ret;

	cc.lpCustColors = (LPDWORD) conf->customColors;
	cc.rgbResult = RGB(clr->r, clr->g, clr->b);
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ret = ChooseColor(&cc)) {
		clr->r = cc.rgbResult & 0xff;
		clr->g = cc.rgbResult >> 8 & 0xff;
		clr->b = cc.rgbResult >> 16 & 0xff;
	}
	RedrawButton(hDlg, id, clr);
	return ret;
}

groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set)
{
	for (auto res = set->begin(); res < set->end(); res++)
		if (res->group == mid)
			return &(*res);
	return nullptr;
}

bool IsLightInGroup(DWORD lgh, AlienFX_SDK::group* grp) {
	if (grp)
		for (auto pos = grp->lights.begin(); pos < grp->lights.end(); pos++)
			if (*pos == lgh)
				return true;
	return false;
}

void RemoveUnused(vector<groupset>* lightsets) {
	for (auto it = lightsets->begin(); it < lightsets->end();)
		if (!(it->color.size() + it->events[0].state + it->events[1].state +
			it->events[2].state + it->ambients.size() + it->haptics.size() + it->effect.type)) {
			lightsets->erase(it);
			it = lightsets->begin();
		}
		else
			it++;
}

void RemoveLightFromGroup(AlienFX_SDK::group* grp, WORD devid, WORD lightid) {
	DWORD lgh = MAKELPARAM(devid, lightid);
	for (auto pos = grp->lights.begin(); pos < grp->lights.end(); pos++)
		if (*pos == lgh) {
			// is it power button?
			if (conf->afx_dev.GetFlags(devid, lightid) & ALIENFX_FLAG_POWER)
				grp->have_power = false;
			grp->lights.erase(pos);
			return;
		}
}

void RemoveLightAndClean(int dPid, int eLid) {
	// delete from all groups...
	for (auto iter = conf->afx_dev.GetGroups()->begin(); iter < conf->afx_dev.GetGroups()->end(); iter++) {
		RemoveLightFromGroup(&(*iter), dPid, eLid);
		iter->have_power = false;
	}
	// Clean from grids...
	DWORD lcode = MAKELPARAM(dPid, eLid);
	for (auto g = conf->afx_dev.GetGrids()->begin(); g < conf->afx_dev.GetGrids()->end(); g++) {
		for (int ind = 0; ind < g->x * g->y; ind++)
			if (g->grid[ind] == lcode)
				g->grid[ind] = 0;
	}
}

