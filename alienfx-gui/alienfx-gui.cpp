#include "alienfx-gui.h"
#include <ColorDlg.h>
#include <Dbt.h>
#include "EventHandler.h"
#include "FXHelper.h"
#include "MonHelper.h"
#include "common.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"msimg32.lib")

// Global Variables:
HINSTANCE hInst;
bool isNewVersion = false;
bool needUpdateFeedback = false;

extern void dxgi_Restart();

BOOL CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabLightsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

FXHelper* fxhl;
ConfigHandler* conf;
EventHandler* eve;
ConfigFan* fan_conf = NULL;
MonHelper* mon = NULL;
ThreadHelper* updateUI = NULL;

// Main and device dialog, then active
HWND mDlg = NULL, dDlg = NULL;

// color selection:
AlienFX_SDK::Afx_action* mod;

HANDLE haveLightFX;
//bool noLightFX = true;

// tooltips
HWND sTip1 = 0, sTip2 = 0, sTip3 = 0;

// Tab selections:
int tabSel = 0;
int tabLightSel = 0;

// Explorer restart event
UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));

// last light selected
int eItem = 0;
// last zone selected
groupset* mmap = NULL;
// last device selected
AlienFX_SDK::Afx_device* activeDevice = NULL;

extern string GetFanName(int ind, bool forTray = false);
extern void AlterGMode(HWND);

bool DetectFans() {
	if (conf->fanControl && (conf->fanControl = EvaluteToAdmin(mDlg))) {
		mon = new MonHelper();
		if (!(conf->fanControl = mon->acpi->isSupported)) {
			delete mon;
			mon = NULL;
		}
	}
	return conf->fanControl;
}

void SetHotkeys() {
	if (conf->keyShortcuts) {
		//register global hotkeys...
		RegisterHotKey(mDlg, 1, MOD_CONTROL | MOD_SHIFT, VK_F12);
		RegisterHotKey(mDlg, 2, MOD_CONTROL | MOD_SHIFT, VK_F11);
		RegisterHotKey(mDlg, 3, 0, VK_F18);
		RegisterHotKey(mDlg, 4, MOD_CONTROL | MOD_SHIFT, VK_F10);
		RegisterHotKey(mDlg, 5, MOD_CONTROL | MOD_SHIFT, VK_F9);
		for (int i = 0; i < 10; i++)
			RegisterHotKey(mDlg, 10 + i, MOD_CONTROL | MOD_SHIFT, 0x30 + i); // 1,2,3...
		if (mon) {
			for (int i = 0; i < mon->acpi->powers.size(); i++)
				RegisterHotKey(mDlg, 30 + i, MOD_CONTROL | MOD_ALT, 0x30 + i); // 0,1,2...
			RegisterHotKey(mDlg, 6, 0, VK_F17);
		}
	}
	else {
		//unregister global hotkeys...
		for (int i = 1; i < 7; i++)
			UnregisterHotKey(mDlg, i);
		for (int i = 0; i < 10; i++) {
			UnregisterHotKey(mDlg, 10 + i);
			UnregisterHotKey(mDlg, 30 + i);
		}
	}
}

void FillAllDevs() {
	fxhl->Stop();
	conf->afx_dev.AlienFXAssignDevices(false, mon ? mon->acpi : NULL);
	if (conf->afx_dev.activeDevices)
		fxhl->Start();
	fxhl->SetState();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	hInst = hInstance;

	ResetDPIScale(lpCmdLine);

	conf = new ConfigHandler();
	conf->Load();

	auto grids = conf->afx_dev.GetGrids();
	if (grids->empty()) {
		grids->push_back({ 0, 20, 8, "Main" });
		grids->back().grid = new AlienFX_SDK::Afx_groupLight[20 * 8]{ 0 };
	}
	conf->mainGrid = &conf->afx_dev.GetGrids()->front();

	if (conf->activeProfile->flags & PROF_FANS)
		fan_conf->lastProf = (fan_profile*)conf->activeProfile->fansets;

	if (conf->esif_temp || conf->fanControl || conf->awcc_disable)
		if (EvaluteToAdmin()) {
			conf->wasAWCC = DoStopService(conf->awcc_disable, true);
			DetectFans();
		}
		else {
			conf->fanControl = conf->esif_temp = false;
		}

	fxhl = new FXHelper();
	FillAllDevs();
	eve = new EventHandler();

	if (CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINWINDOW), NULL, (DLGPROC)MainDialog)) {

		SendMessage(mDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXGUI)));
		SendMessage(mDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXGUI), IMAGE_ICON, 16, 16, 0));

		SetHotkeys();
		// Power notifications...
		RegisterPowerSettingNotification(mDlg, &GUID_MONITOR_POWER_ON, 0);
		RegisterPowerSettingNotification(mDlg, &GUID_LIDSWITCH_STATE_CHANGE, 0);

		ShowWindow(mDlg, conf->startMinimized ? SW_HIDE : SW_SHOW);

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
	}

	fxhl->Refresh(true);
	delete eve;
	delete fxhl;

	DoStopService(conf->wasAWCC, false);

	if (mon) {
		delete mon;
	}

	delete conf;

	return 0;
}

void RedrawButton(HWND ctrl, AlienFX_SDK::Afx_colorcode* act) {
	RECT rect;
	HBRUSH Brush = act ? CreateSolidBrush(RGB(act->r, act->g, act->b)) : CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	GetClientRect(ctrl, &rect);
	HDC cnt = GetWindowDC(ctrl);
	FillRect(cnt, &rect, Brush);
	DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
	DeleteObject(Brush);
	ReleaseDC(ctrl, cnt);
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

void ResizeTab(HWND tab) {
	RECT dRect, rcDisplay;
	GetClientRect(GetParent(tab), &rcDisplay);
	rcDisplay.left = GetSystemMetrics(SM_CXBORDER);
	rcDisplay.top = GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER);
	rcDisplay.right -= 2 * GetSystemMetrics(SM_CXBORDER) + 1;
	rcDisplay.bottom -= GetSystemMetrics(SM_CYBORDER) + 1;
	GetClientRect(tab, &dRect);
	int deltax = dRect.right - (rcDisplay.right - rcDisplay.left),
		deltay = dRect.bottom - (rcDisplay.bottom - rcDisplay.top);
	if (deltax || deltay) {
		GetWindowRect(GetParent(GetParent(tab)), &dRect);
		SetWindowPos(GetParent(GetParent(tab)), NULL, 0, 0, dRect.right - dRect.left + deltax, dRect.bottom - dRect.top + deltay, SWP_NOZORDER | SWP_NOMOVE);
		rcDisplay.right += deltax;
		rcDisplay.bottom += deltay;
	}
	SetWindowPos(tab, NULL,
		rcDisplay.left, rcDisplay.top,
		rcDisplay.right - rcDisplay.left,
		rcDisplay.bottom - rcDisplay.top,
		SWP_SHOWWINDOW);
}

void OnSelChanged()
{
	HWND hwndDlg = GetDlgItem(mDlg, IDC_TAB_MAIN);
	// Get the dialog header data.
	DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA);

	// Get the index of the selected tab.
	TCITEM tie{ TCIF_PARAM };
	TabCtrl_GetItem(hwndDlg, TabCtrl_GetCurSel(hwndDlg), &tie);
	tabSel = (int)tie.lParam;

	// Destroy the current child dialog box, if any.
	if (pHdr->hwndDisplay)
		DestroyWindow(pHdr->hwndDisplay);

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes[tabSel], hwndDlg, pHdr->apProc[tabSel]);
	ResizeTab(pHdr->hwndDisplay);
}

void UpdateProfileList() {
	HWND profile_list = GetDlgItem(mDlg, IDC_PROFILES);
	ComboBox_ResetContent(profile_list);
	int id;
	for (auto i = conf->profiles.begin(); i != conf->profiles.end(); i++) {
		id = ComboBox_AddString(profile_list, (*i)->name.c_str());
		if ((*i)->id == conf->activeProfile->id) {
			ComboBox_SetCurSel(profile_list, id);
			CheckDlgButton(mDlg, IDC_PROFILE_EFFECTS, conf->activeProfile->effmode);
		}
	}
	DebugPrint("Profile list reloaded.\n");
}

void SelectProfile(profile* prof = conf->activeProfile) {
	eve->SwitchActiveProfile(prof);
	UpdateProfileList();
	if (tabSel == TAB_FANS || tabSel == TAB_LIGHTS)
		OnSelChanged();
}

void UpdateState(bool checkMode) {
	eve->ChangeEffectMode();
	if (checkMode) {
		CheckDlgButton(mDlg, IDC_PROFILE_EFFECTS, conf->activeProfile->effmode);
		if (tabSel == TAB_LIGHTS)
			OnSelChanged();
	}
	if (tabSel == TAB_SETTINGS)
		OnSelChanged();
}

void RestoreApp() {
	ShowWindow(mDlg, SW_RESTORE);
	SetForegroundWindow(mDlg);
	UpdateProfileList();
	OnSelChanged();
}

void ClearOldTabs(HWND tab) {
	DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(tab, GWLP_USERDATA);

	if (pHdr) {
		// close tab dialog and clean data
		DestroyWindow(pHdr->hwndControl);
		DestroyWindow(pHdr->hwndDisplay);
		delete[] pHdr->apRes;
		delete[] pHdr->apProc;
		delete pHdr;
		TabCtrl_DeleteAllItems(tab);
	}
}

void CreateTabControl(HWND parent, vector<string> names, vector<DWORD> resID, vector<DLGPROC> func) {

	ClearOldTabs(parent);

	DLGHDR* pHdr = new DLGHDR{ 0 };// (DLGHDR*)LocalAlloc(LPTR, sizeof(DLGHDR));
	SetWindowLongPtr(parent, GWLP_USERDATA, (LONG_PTR)pHdr);

	int tabsize = (int)names.size();

	pHdr->apRes = new DLGTEMPLATE*[tabsize];// (DLGTEMPLATE**)LocalAlloc(LPTR, tabsize * sizeof(DLGTEMPLATE*));
	pHdr->apProc = new DLGPROC[tabsize];// (DLGPROC*)LocalAlloc(LPTR, tabsize * sizeof(DLGPROC));

	TCITEM tie{ TCIF_TEXT | TCIF_PARAM };

	for (int i = 0; i < tabsize; i++) {
		if (tabsize < 5)
			switch (i) { // check disabled tabs
			case 0: if (!conf->afx_dev.activeDevices) continue;
				break;
			case 1: if (!mon) continue;
			}
		pHdr->apRes[i] = (DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(resID[i]), RT_DIALOG)));
		tie.pszText = (LPSTR)names[i].c_str();
		tie.lParam = i;
		pHdr->apProc[i] = func[i];
		int pos = TabCtrl_InsertItem(parent, i, (LPARAM)&tie);
		if ((tabsize < 5 && i == tabSel) || (tabsize > 4 && i == tabLightSel))
			TabCtrl_SetCurSel(parent, pos);
	}
}

void SetMainTabs() {
	CreateTabControl(GetDlgItem(mDlg, IDC_TAB_MAIN),
		{ "Lights", "Fans and Power", "Profiles", "Settings" },
		{ IDD_DIALOG_LIGHTS, IDD_DIALOG_FAN, IDD_DIALOG_PROFILES, IDD_DIALOG_SETTINGS },
		{ (DLGPROC)TabLightsDialog, (DLGPROC)TabFanDialog, (DLGPROC)TabProfilesDialog, (DLGPROC)TabSettingsDialog }
	);
	OnSelChanged();
}

HWND tip;

BOOL CALLBACK MainDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND tab_list = GetDlgItem(hDlg, IDC_TAB_MAIN),
		profile_list = GetDlgItem(hDlg, IDC_PROFILES);

	// Started/restarted explorer...
	if (message == newTaskBar) {
		conf->SetIconState(conf->updateCheck);
		return true;
	}

	switch (message)
	{
	case WM_INITDIALOG:
	{
		conf->niData.hWnd = mDlg = hDlg;
		SetMainTabs();
		UpdateProfileList();
		conf->SetIconState(conf->updateCheck);
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
			break;
		case IDC_BUTTON_SAVE:
			conf->afx_dev.SaveMappings();
			conf->Save();
			fxhl->Refresh(true);
			ShowNotification(&conf->niData, "Configuration saved!", "Configuration saved successfully.");
			break;
		case IDC_PROFILE_EFFECTS:
			conf->activeProfile->effmode = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			UpdateState(true);
			break;
		case IDC_PROFILES: {
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				SelectProfile(conf->profiles[ComboBox_GetCurSel(profile_list)]);
			break;
			}
		} break;
		} break;
	} break;
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_TAB_MAIN && ((NMHDR*)lParam)->code == TCN_SELCHANGE) {
			OnSelChanged();
		}
		break;
	case WM_WINDOWPOSCHANGING: {
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		if (pos->flags & SWP_SHOWWINDOW) {
			eve->StopProfiles();
		} else
			if (pos->flags & SWP_HIDEWINDOW) {
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
						GetWindowRect(GetDlgItem(mDlg, IDC_PROFILE_EFFECTS), &oldRect);
						ScreenToClient(mDlg, (LPPOINT)&oldRect);
						SetWindowPos(GetDlgItem(mDlg, IDC_PROFILE_EFFECTS), NULL, oldRect.left + deltax, oldRect.top, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
						//GetWindowRect(GetDlgItem(mDlg, IDC_STATIC_EFFECTS), &oldRect);
						//ScreenToClient(mDlg, (LPPOINT)&oldRect);
						//SetWindowPos(GetDlgItem(mDlg, IDC_STATIC_EFFECTS), NULL, oldRect.left + deltax, oldRect.top, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
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
			DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(tab_list, GWLP_USERDATA);
			DestroyWindow(pHdr->hwndDisplay);
			pHdr->hwndDisplay = NULL;
		} break;
		}
		break;
	case WM_APP + 1: {
		switch (LOWORD(lParam)) {
		case WM_LBUTTONDBLCLK: case WM_LBUTTONUP:
			RestoreApp();
			break;
		case WM_RBUTTONUP: case NIN_KEYSELECT: case WM_CONTEXTMENU:
		{
			POINT lpClickPoint;
			HMENU tMenu = LoadMenu(hInst, MAKEINTRESOURCEA(IDR_MENU_TRAY));
			tMenu = GetSubMenu(tMenu, 0);
			MENUINFO mi{ sizeof(MENUINFO), MIM_STYLE, MNS_NOTIFYBYPOS | MNS_MODELESS };
			SetMenuInfo(tMenu, &mi);
			MENUITEMINFO mInfo{ sizeof(MENUITEMINFO), MIIM_STRING | MIIM_ID | MIIM_STATE };
			HMENU pMenu;
			// add profiles...
			//if (!conf->enableProfSwitch) {
				pMenu = CreatePopupMenu();
				mInfo.wID = ID_TRAYMENU_PROFILE_SELECTED;
				for (auto i = conf->profiles.begin(); i != conf->profiles.end(); i++) {
					mInfo.dwTypeData = (LPSTR)(*i)->name.c_str();
					mInfo.fState = (*i)->id == conf->activeProfile->id ? MF_CHECKED : MF_UNCHECKED;
					InsertMenuItem(pMenu, (UINT)(i - conf->profiles.begin()), false, &mInfo);
				}
				ModifyMenu(tMenu, ID_TRAYMENU_PROFILES, MF_ENABLED | MF_BYCOMMAND | MF_STRING | MF_POPUP, (UINT_PTR)pMenu, ("Profiles (" + 
					conf->activeProfile->name + ")").c_str());
			//}

			CheckMenuItem(tMenu, ID_TRAYMENU_ENABLEEFFECTS, conf->enableEffects ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_LIGHTSON, conf->lightsOn ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_DIMLIGHTS, conf->dimmed ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_PROFILESWITCH, conf->enableProfSwitch ? MF_CHECKED : MF_UNCHECKED);

			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(hDlg);
			TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
				lpClickPoint.x, lpClickPoint.y, 0, hDlg, NULL);
			PostMessage(hDlg, WM_NULL, 0, 0);
		} break;
		case NIN_BALLOONTIMEOUT:
			if (!isNewVersion) {
				Shell_NotifyIcon(NIM_DELETE, &conf->niData);
				Shell_NotifyIcon(NIM_ADD, &conf->niData);
			}
			else
				isNewVersion = false;
			break;
		case NIN_BALLOONUSERCLICK:
			if (isNewVersion) {
				ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools/releases", NULL, NULL, SW_SHOWNORMAL);
				isNewVersion = false;
			}
			break;
		case WM_MOUSEMOVE: {
			string name = (string)"Lights: " + (conf->stateOn ? conf->stateDimmed ? "Dimmed" : "On" : "Off") + 
				"\nProfile: " + conf->activeProfile->name + "\nEffects: ";
			if (conf->stateEffects) {
				if (eve->sysmon) name += "Monitoring ";
				if (eve->capt) name += "Ambient ";
				if (eve->audio) name += "Haptics ";
				if (eve->grid) name += "Grid";
			}
			else
				name += "Off";
			if (mon) {
				name += "\nPower: ";
				if (fan_conf->lastProf->powerStage == mon->acpi->powers.size())
					name += "G-mode";
				else
					name += fan_conf->powers[mon->acpi->powers[fan_conf->lastProf->powerStage]];

				for (int i = 0; i < mon->acpi->fans.size(); i++) {
					name += "\n" + GetFanName(i, true);
				}
			}
			strcpy_s(conf->niData.szTip, 127, name.c_str());
			Shell_NotifyIcon(NIM_MODIFY, &conf->niData);
		} break;
		}
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
			UpdateState(false);
			break;
		case ID_TRAYMENU_DIMLIGHTS:
			conf->dimmed = !conf->dimmed;
			UpdateState(false);
			break;
		case ID_TRAYMENU_ENABLEEFFECTS:
			conf->enableEffects = !conf->enableEffects;
			UpdateState(true);
			break;
		case ID_TRAYMENU_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProfSwitch = !conf->enableProfSwitch;
			eve->StartProfiles();
			SelectProfile();
			break;
		case ID_TRAYMENU_RESTORE:
			RestoreApp();
			break;
		case ID_TRAYMENU_PROFILE_SELECTED: {
			SelectProfile(conf->profiles[idx]);
		} break;
		}
	} break;
	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC: {
			// resume from sleep/hibernate
			DebugPrint("Resume from Sleep/hibernate initiated\n");
			fxhl->stateScreen = true; // patch for later StateScreen update
			if (mon)
				mon->Start();
			fxhl->Start();
			eve->ChangeEffects();
			eve->StartProfiles();
			if (conf->updateCheck) {
				needUpdateFeedback = false;
				CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
			}
		} break;
		case PBT_APMPOWERSTATUSCHANGE:
			// ac/batt change
			DebugPrint("Power source change initiated\n");
			eve->ChangePowerState();
			break;
		case PBT_POWERSETTINGCHANGE: {
			POWERBROADCAST_SETTING* sParams = (POWERBROADCAST_SETTING*) lParam;
			fxhl->stateScreen = !conf->offWithScreen || sParams->Data[0];
			if (sParams->PowerSetting == GUID_LIDSWITCH_STATE_CHANGE)
				fxhl->stateScreen = fxhl->stateScreen || GetSystemMetrics(SM_CMONITORS) > 1;
			DebugPrint("Screen state changed to " + to_string(fxhl->stateScreen) + " (source: " +
				(sParams->PowerSetting == GUID_LIDSWITCH_STATE_CHANGE ? "Lid" : "Monitor")
				+ ")\n");
			fxhl->SetState();
		} break;
		case PBT_APMSUSPEND:
			// Sleep initiated.
			DebugPrint("Sleep/hibernate initiated\n");
			conf->Save();
			eve->StopProfiles();
			eve->ChangeEffects(true);
			// need to restore lights if followed screen
			fxhl->stateScreen = true;
			fxhl->SetState();
			fxhl->Refresh(true);
			fxhl->Stop();
			if (mon)
				mon->Stop();
			break;
		}
		break;
	case WM_DEVICECHANGE:
		if (wParam == DBT_DEVNODES_CHANGED) {
			DebugPrint("Device list changed \n");
			vector<AlienFX_SDK::Functions*> devList = conf->afx_dev.AlienFXEnumDevices(mon ? mon->acpi : NULL);
			if (devList.size() != conf->afx_dev.activeDevices) {
				DebugPrint("Active device list changed!\n");
				fxhl->Stop();
				conf->afx_dev.AlienFXApplyDevices(false, devList);
				activeDevice = NULL;
				if (conf->afx_dev.activeDevices && !dDlg) {
					fxhl->Start();
					fxhl->Refresh();
				}
				SetMainTabs();
			}
		}
		break;
	case WM_DISPLAYCHANGE:
		// Monitor configuration changed
	    dxgi_Restart();
		break;
	case WM_ENDSESSION:
		// Shutdown/restart scheduled....
		DebugPrint("Shutdown initiated\n");
		conf->Save();
		delete eve;
		fxhl->Refresh(true);
		fxhl->Stop();
		if (mon)
			delete mon;
		exit(0);
	case WM_HOTKEY:
		if (wParam > 9 && wParam - 10 <= conf->profiles.size()) { // Profile switch
			SelectProfile(wParam == 10 ? conf->FindDefaultProfile() : conf->profiles[wParam - 11]);
			break;
		}
		if (mon && wParam > 29 && wParam - 30 < mon->acpi->powers.size()) { // PowerMode switch
			mon->SetPowerMode((WORD)wParam - 30);
			if (tabSel == TAB_FANS)
				OnSelChanged();
			BlinkNumLock((int)wParam - 29);
			break;
		}
		switch (wParam) {
		case 1: // on/off
			conf->lightsOn = !conf->lightsOn;
			UpdateState(false);
			break;
		case 2: // dim
			conf->dimmed = !conf->dimmed;
			UpdateState(false);
			break;
		case 3: // off-dim-full circle
			if (conf->lightsOn)
				if (conf->stateDimmed) {
					conf->lightsOn = false;
					conf->dimmed = false;
				}
				else
					conf->dimmed = true;
			else
				conf->lightsOn = true;
			UpdateState(false);
			break;
		case 4: // effects
			conf->enableEffects = !conf->enableEffects;
			UpdateState(true);
			break;
		case 5: // profile autoswitch
			eve->StopProfiles();
			conf->enableProfSwitch = !conf->enableProfSwitch;
			eve->StartProfiles();
			SelectProfile();
			break;
		case 6: // G-key for Dell G-series power switch
			if (mon) {
				AlterGMode(NULL);
				if (tabSel == TAB_FANS)
					OnSelChanged();
			}
			break;
		default: return false;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		break;
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &conf->niData);
		conf->Save();
		mDlg = NULL;
		PostQuitMessage(0);
		break;
	default: return false;
	}
	return true;
}


AlienFX_SDK::Afx_colorcode Act2Code(AlienFX_SDK::Afx_action *act) {
	AlienFX_SDK::Afx_colorcode c = { act->b, act->g, act->r };
	return c;
}

AlienFX_SDK::Afx_action *Code2Act(AlienFX_SDK::Afx_colorcode *c) {
	return new AlienFX_SDK::Afx_action({0,0,0,c->r,c->g,c->b});
}

void CColorRefreshProc(LPVOID param) {
	AlienFX_SDK::Afx_action* lastcolor = (AlienFX_SDK::Afx_action*)param;
	if (memcmp(lastcolor, mod, sizeof(AlienFX_SDK::Afx_action))) {
		*lastcolor = *mod;
		if (mmap) fxhl->RefreshOne(mmap);
	}
}

UINT_PTR Lpcchookproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	//case WM_INITDIALOG:
	//	mod = (AlienFX_SDK::Afx_action*)((CHOOSECOLOR*)lParam)->lCustData;
	//	break;
	case WM_CTLCOLOREDIT:
		mod->r = GetDlgItemInt(hDlg, COLOR_RED, NULL, false);
		mod->g = GetDlgItemInt(hDlg, COLOR_GREEN, NULL, false);
		mod->b = GetDlgItemInt(hDlg, COLOR_BLUE, NULL, false);
		break;
	}
	return 0;
}

bool SetColor(HWND ctrl, AlienFX_SDK::Afx_action* map, bool needUpdate = true) {
	CHOOSECOLOR cc{ sizeof(cc), ctrl, NULL, RGB(map->r, map->g, map->b), (LPDWORD)conf->customColors,
		CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR | CC_ENABLEHOOK, NULL/*(LPARAM)map*/, Lpcchookproc };

	bool ret;
	AlienFX_SDK::Afx_action savedColor = *map, lastcolor = *map;
	mod = map;
	ThreadHelper* colorUpdate = NULL;

	if (needUpdate)
		colorUpdate = new ThreadHelper(CColorRefreshProc, &lastcolor, conf->afx_dev.GetGroupById(mmap->group)->have_power ? 1000 : 200);

	if (!(ret = ChooseColor(&cc)))
		(*map) = savedColor;

	if (needUpdate) {
		delete colorUpdate;
		fxhl->Refresh();
	}
	RedrawButton(ctrl, &Act2Code(map));
	return ret;
}

bool SetColor(HWND ctrl, AlienFX_SDK::Afx_colorcode *clr) {
	bool ret;
	AlienFX_SDK::Afx_action* savedColor = Code2Act(clr);
	if (ret = SetColor(ctrl, savedColor, false))
		*clr = Act2Code(savedColor);
	return ret;
}

bool IsLightInGroup(DWORD lgh, AlienFX_SDK::Afx_group* grp) {
	if (grp)
		for (auto pos = grp->lights.begin(); pos < grp->lights.end(); pos++)
			if (pos->lgh == lgh)
				return true;
	return false;
}

bool IsGroupUnused(DWORD gid) {
	for (auto prof = conf->profiles.begin(); prof != conf->profiles.end(); prof++) {
		for (auto ls = (*prof)->lightsets.begin(); ls != (*prof)->lightsets.end(); ls++)
			if (ls->group == gid) {
				return false;
			}
	}
	return true;
}

void RemoveLightFromGroup(AlienFX_SDK::Afx_group* grp, WORD devid, WORD lightid) {
	AlienFX_SDK::Afx_groupLight cur{ devid, lightid };
	for (auto pos = grp->lights.begin(); pos != grp->lights.end(); pos++)
		if (pos->lgh == cur.lgh) {
			if (conf->afx_dev.GetFlags(devid, lightid) & ALIENFX_FLAG_POWER)
				grp->have_power = false;
			grp->lights.erase(pos);
			break;
		}
}

void RemoveLightAndClean(int dPid, int eLid) {
	// delete from all groups...
	for (auto iter = conf->afx_dev.GetGroups()->begin(); iter < conf->afx_dev.GetGroups()->end(); iter++) {
		RemoveLightFromGroup(&(*iter), dPid, eLid);
	}
	// Clean from grids...
	DWORD lcode = MAKELPARAM(dPid, eLid);
	for (auto g = conf->afx_dev.GetGrids()->begin(); g < conf->afx_dev.GetGrids()->end(); g++) {
		for (int ind = 0; ind < g->x * g->y; ind++)
			if (g->grid[ind].lgh == lcode)
				g->grid[ind].lgh = 0;
	}
}


