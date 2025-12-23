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
bool wasAWCC = false;
int idc_version = IDC_STATIC_VERSION, idc_homepage = IDC_SYSLINK_HOMEPAGE; // for About

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
AlienFX_SDK::Afx_action* mod; //  Current user-selected color
AlienFX_SDK::Afx_action lastColor; // last selected color
bool needColorUpdate; // is needed to update group after color change?

HANDLE haveLightFX;

// tooltips
HWND sTip1 = 0, sTip2 = 0, sTip3 = 0;

// Tab selections:
int tabSel = 0;
int tabLightSel = 0;

// Explorer restart event
UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));

// last zone selected
groupset* activeMapping = NULL;

const char* freqNames[] = { "Keep", "60Hz", "90Hz", "120Hz", "144Hz", "165Hz", "240Hz", "300Hz", "360Hz", ""};
const int fvArray[] = { 0, 60, 90, 120, 144, 165, 240, 300, 360 };
const int* freqValues = fvArray;

extern string GetFanName(int ind, bool forTray = false);
extern void AlterGMode(bool, HWND view = NULL);
extern void PowerChangeNotify(bool blink, HWND power_list=NULL);

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
		RegisterHotKey(mDlg, 1, 0, VK_F18);
		if (conf->fanControl)
			RegisterHotKey(mDlg, 2, 0, VK_F17);
		else
			UnregisterHotKey(mDlg, 2);
		RegisterHotKey(mDlg, 3, MOD_CONTROL | MOD_SHIFT, VK_F12);
		RegisterHotKey(mDlg, 4, MOD_CONTROL | MOD_SHIFT, VK_F11);
		RegisterHotKey(mDlg, 5, MOD_CONTROL | MOD_SHIFT, VK_F10);
		RegisterHotKey(mDlg, 6, MOD_CONTROL | MOD_SHIFT, VK_F9);
		// brightness
		RegisterHotKey(mDlg, 7, MOD_CONTROL | MOD_SHIFT, VK_OEM_PLUS);
		RegisterHotKey(mDlg, 8, MOD_CONTROL | MOD_SHIFT, VK_OEM_MINUS);
		for (int i = 0; i < 10; i++)
			RegisterHotKey(mDlg, 10 + i, MOD_CONTROL | MOD_SHIFT, 0x30 + i); // 1,2,3...
		if (mon) {
			for (int i = 0; i < mon->powerSize; i++)
				RegisterHotKey(mDlg, 30 + i, MOD_CONTROL | MOD_ALT, 0x30 + i); // 0,1,2...
		}
	}
	else {
		//unregister global hotkeys...
		UnregisterHotKey(mDlg, 1);
		UnregisterHotKey(mDlg, 2);
		for (int i = 3; i < 9; i++)
			UnregisterHotKey(mDlg, i);
		for (int i = 0; i < 10; i++) {
			UnregisterHotKey(mDlg, 10 + i);
			UnregisterHotKey(mDlg, 30 + i);
		}
	}
}

void OnSelChanged();
void UpdateProfileList();
void SetMainTabs();

void SelectProfile(profile* prof = conf->activeProfile) {
	if (!dDlg) {
		eve->SwitchActiveProfile(prof);
		activeMapping = conf->activeProfile->lightsets.size() ? &conf->activeProfile->lightsets.front() : NULL;
		if (tabSel == TAB_FANS || tabSel == TAB_LIGHTS)
			OnSelChanged();
	}
	UpdateProfileList();
}

void UpdateLightDevices() {
	if (conf->afx_dev.AlienFXEnumDevices(mon ? mon->acpi : NULL)) {
		DebugPrint("Active device list changed!\n");
		if (conf->afx_dev.activeDevices && !dDlg) {
			fxhl->updateAllowed = true;
			for (auto& fxd : conf->afx_dev.fxdevs) {
				if (fxd.arrived) {
					fxhl->UpdateGlobalEffect(&fxd);
					fxhl->QueryCommand(fxd.pid, LightQueryElement({ 0, 2 }));
				}
				else {
					if (!fxd.present && fxd.dev) {
						// Schedule device removal
						fxhl->QueryCommand(fxd.pid, LightQueryElement({ 0, 3 }));
					}
				}
			}
			fxhl->Refresh();
		}
		SetMainTabs();
	}
}

void PauseSystem() {
	conf->Save();
	eve->StopProfiles();
	eve->ChangeEffects(true);
	fxhl->ClearAndRefresh(true);
	fxhl->Stop();
	if (mon)
		mon->Stop();
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

	//if (conf->activeProfile->flags & PROF_FANS)
	//	fan_conf->lastProf = (fan_profile*)conf->activeProfile->fansets;

	if (conf->esif_temp || conf->fanControl || conf->awcc_disable)
		if (EvaluteToAdmin()) {
			wasAWCC = DoStopAWCC(conf->awcc_disable, true);
			DetectFans();
		}
		else {
			conf->fanControl = conf->esif_temp = false;
		}

	fxhl = new FXHelper();
	UpdateLightDevices();
	eve = new EventHandler();

	if (CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINWINDOW), NULL, (DLGPROC)MainDialog)) {

		SendMessage(mDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXGUI)));
		SendMessage(mDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_ALIENFXGUI), IMAGE_ICON, 16, 16, 0));

		SetHotkeys();
		// Power notifications...
		RegisterPowerSettingNotification(mDlg, &GUID_MONITOR_POWER_ON, 0);
		RegisterPowerSettingNotification(mDlg, &GUID_LIDSWITCH_STATE_CHANGE, 0);

		ShowWindow(mDlg, conf->startMinimized ? SW_HIDE : SW_SHOW);

		if (!conf->startMinimized)
			SelectProfile();

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

	PauseSystem();
	delete fxhl;
	delete eve;

	DoStopAWCC(wasAWCC, false);

	if (mon) {
		delete mon;
	}

	delete conf;

	return 0;
}

void RedrawButton(HWND ctrl, DWORD act) {
	RECT rect;
	HBRUSH Brush = CreateSolidBrush(act & 0xff000000 ? GetSysColor(COLOR_BTNFACE) : act);
	GetClientRect(ctrl, &rect);
	HDC cnt = GetWindowDC(ctrl);
	FillRect(cnt, &rect, Brush);
	DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
	DeleteObject(Brush);
	ReleaseDC(ctrl, cnt);
}

void ResizeTab(HWND tab) {
	RECT dRect, rcDisplay;
	HWND frame = GetParent(tab);
	GetWindowRect(tab, &dRect);
	GetWindowRect(frame, &rcDisplay);
	//POINT newSize{ dRect.right + 4/*+ 3 * GetSystemMetrics(SM_CXBORDER)*/, 
	//	dRect.bottom + GetSystemMetrics(SM_CYSMSIZE) + 1/*+ GetSystemMetrics(SM_CYBORDER)*/ };
	POINT delta = { (dRect.right + 4 - dRect.left) - (rcDisplay.right - rcDisplay.left), (
		dRect.bottom + GetSystemMetrics(SM_CYSMSIZE) + 1 - dRect.top) - (rcDisplay.bottom -rcDisplay.top) };
	if (delta.x || delta.y) {
		GetWindowRect(GetParent(frame), &rcDisplay);
		SetWindowPos(GetParent(frame), NULL, 0, 0, rcDisplay.right - rcDisplay.left + delta.x, rcDisplay.bottom - rcDisplay.top + delta.y, SWP_NOZORDER | SWP_NOMOVE);
	}
	SetWindowPos(tab, NULL,
		1/*GetSystemMetrics(SM_CXBORDER)*/, GetSystemMetrics(SM_CYSMSIZE) - 1/*- GetSystemMetrics(SM_CYBORDER)*/,
		dRect.right - dRect.left,
		dRect.bottom - dRect.top,
		SWP_SHOWWINDOW | SWP_NOZORDER);
}

void OnSelChanged()
{
	if (IsWindowVisible(mDlg)) {
		HWND hwndDlg = GetDlgItem(mDlg, IDC_TAB_MAIN);
		// Get the dialog header data.
		DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

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
}

void UpdateProfileList() {
	if (IsWindowVisible(mDlg)) {
		//activeMapping = NULL;
		HWND profile_list = GetDlgItem(mDlg, IDC_PROFILES);
		EnableWindow(GetDlgItem(mDlg, IDC_PROFILE_EFFECTS), conf->enableEffects);
		CheckDlgButton(mDlg, IDC_PROFILE_EFFECTS, conf->activeProfile->effmode);
		ComboBox_ResetContent(profile_list);
		int id;
		for (profile* prof : conf->profiles) {
			id = ComboBox_AddString(profile_list, prof->name.c_str());
			ComboBox_SetItemData(profile_list, id, prof->id);
			if (prof->id == conf->activeProfile->id)
				ComboBox_SetCurSel(profile_list, id);
		}
		//DebugPrint("Profile list reloaded.\n");
	}
}

void UpdateState(bool checkMode = false) {
	if (!dDlg) {
		if (checkMode)
			eve->ChangeEffectMode();
		else
			fxhl->SetState();
		if (tabSel == TAB_SETTINGS || tabSel == TAB_LIGHTS)
			OnSelChanged();
	}
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

void CreateTabControl(HWND parent, int tabsize, const char* names[], const DWORD* resID, const DLGPROC* func) {

	ClearOldTabs(parent);

	DLGHDR* pHdr = new DLGHDR{ 0 };
	SetWindowLongPtr(parent, GWLP_USERDATA, (LONG_PTR)pHdr);

	pHdr->apRes = new DLGTEMPLATE*[tabsize];
	pHdr->apProc = new DLGPROC[tabsize];

	TCITEM tie{ TCIF_TEXT | TCIF_PARAM };

	for (int i = 0; i < tabsize; i++) {
		if (tabsize < 5)
			switch (i) { // check disabled tabs
			case 0: if (!conf->afx_dev.activeDevices) continue;
				break;
			case 1: if (!mon) continue;
			}
		pHdr->apRes[i] = (DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(resID[i]), RT_DIALOG)));
		tie.pszText = (LPSTR)names[i]/*.c_str()*/;
		tie.lParam = i;
		pHdr->apProc[i] = func[i];
		int pos = TabCtrl_InsertItem(parent, i, (LPARAM)&tie);
		if ((tabsize < 5 && i == tabSel) || (tabsize > 4 && i == tabLightSel))
			TabCtrl_SetCurSel(parent, pos);
	}
}

const char* mainTabs[] = { "Lights", "Fans and Power", "Profiles", "Settings" };
const DWORD resTabs[] = { IDD_DIALOG_LIGHTS, IDD_DIALOG_FAN, IDD_DIALOG_PROFILES, IDD_DIALOG_SETTINGS };
const DLGPROC procTabs[] = { (DLGPROC)TabLightsDialog, (DLGPROC)TabFanDialog, (DLGPROC)TabProfilesDialog, (DLGPROC)TabSettingsDialog };

void SetMainTabs() {
	CreateTabControl(GetDlgItem(mDlg, IDC_TAB_MAIN), 4, mainTabs, resTabs, procTabs);
	OnSelChanged();
}

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
		while (!conf->SetIconState(conf->updateCheck))
			Sleep(100);
		SetMainTabs();
	} break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_BUT_PREV: case IDC_BUT_NEXT: case IDC_BUT_LAST: case IDC_BUT_FIRST:
			if (dDlg)
				SendMessage(dDlg, message, wParam, lParam);
			break;
		case IDM_EXIT: case IDC_BUTTON_MINIMIZE:
			//eve->notInDestroy = false;
			DestroyWindow(hDlg);
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
		case IDC_BUTTON_REFRESH:
			fxhl->ClearAndRefresh();
			break;
		case IDC_BUTTON_SAVE:
			conf->afx_dev.SaveMappings();
			conf->Save();
			fxhl->ClearAndRefresh(true);
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
				SelectProfile(conf->FindProfile((int)ComboBox_GetItemData(profile_list, ComboBox_GetCurSel(profile_list))));
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
				DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(tab_list, GWLP_USERDATA);
				DestroyWindow(pHdr->hwndDisplay);
				pHdr->hwndDisplay = NULL;
			}
			else
				if (!(pos->flags & SWP_NOSIZE) && (pos->cx || pos->cy)) {
					RECT oldRect;
					GetWindowRect(mDlg, &oldRect);
					if (pos->cx > 0 && pos->cy > 0) {
						int deltax = pos->cx - oldRect.right + oldRect.left,
							deltay = pos->cy - oldRect.bottom + oldRect.top;
						if (deltax || deltay) {
							RECT probe;
							GetWindowRect(tab_list, &probe);
							int width = probe.right - probe.left + deltax;
							// bottom border - 3CU, make pixels
							SetWindowPos(tab_list, NULL, 0, 0, width,
								probe.bottom - probe.top + deltay, SWP_NOZORDER | SWP_NOMOVE);
							GetWindowRect(GetDlgItem(mDlg, IDC_PROFILES), &probe);
							SetWindowPos(GetDlgItem(mDlg, IDC_PROFILES), NULL, 0, 0, width, probe.bottom - probe.top, SWP_NOOWNERZORDER | SWP_NOMOVE);
							GetWindowRect(GetDlgItem(mDlg, IDC_PROFILE_EFFECTS), &probe);
							POINT topLine{ probe.left + deltax, probe.top };
							ScreenToClient(mDlg, &topLine);
							SetWindowPos(GetDlgItem(mDlg, IDC_PROFILE_EFFECTS), NULL, topLine.x, topLine.y, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
							GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_REFRESH), &probe);
							POINT botLine{ probe.left, probe.top + deltay };
							ScreenToClient(mDlg, &botLine);
							SetWindowPos(GetDlgItem(mDlg, IDC_BUTTON_REFRESH), NULL, botLine.x, botLine.y, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
							GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_MINIMIZE), &probe);
							POINT mLine{ deltax + probe.left, probe.top };
							ScreenToClient(mDlg, &mLine);
							SetWindowPos(GetDlgItem(mDlg, IDC_BUTTON_MINIMIZE), NULL, mLine.x, botLine.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
							GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_SAVE), &probe);
							mLine = { deltax + probe.left, probe.top };
							ScreenToClient(mDlg, &mLine);
							SetWindowPos(GetDlgItem(mDlg, IDC_BUTTON_SAVE), NULL, mLine.x, botLine.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
						}
					}
				}
		return false;
	} break;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			ShowWindow(hDlg, SW_HIDE);
		break;
	case WM_APP + 1: {
		switch (LOWORD(lParam)) {
		case WM_LBUTTONDBLCLK: case WM_LBUTTONUP:
			RestoreApp();
			break;
		case WM_RBUTTONUP: case NIN_KEYSELECT: case WM_CONTEXTMENU:
		{
			POINT lpClickPoint;
			HMENU cMenu = LoadMenu(hInst, MAKEINTRESOURCEA(IDR_MENU_TRAY)),
			tMenu = GetSubMenu(cMenu, 0);
			MENUINFO mi{ sizeof(MENUINFO), MIM_STYLE, MNS_NOTIFYBYPOS /*| MNS_MODELESS | MNS_AUTODISMISS*/};
			SetMenuInfo(tMenu, &mi);
			HMENU pMenu;
			// add profiles...
			pMenu = CreatePopupMenu();

			for (auto& i : conf->profiles) {
				AppendMenu(pMenu, MF_STRING | (i->id == conf->activeProfile->id ? MF_CHECKED : MF_UNCHECKED),
					i->id, (LPSTR)i->name.c_str());
			}

			ModifyMenu(tMenu, ID_TRAYMENU_PROFILES, MF_ENABLED | MF_BYCOMMAND | MF_STRING | MF_POPUP, (UINT_PTR)pMenu, ("Profiles (" + 
				conf->activeProfile->name + ")").c_str());

			CheckMenuItem(tMenu, ID_TRAYMENU_ENABLEEFFECTS, conf->enableEffects ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_LIGHTSON, conf->lightsOn ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_DIMLIGHTS, conf->dimmed ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(tMenu, ID_TRAYMENU_PROFILESWITCH, conf->enableProfSwitch ? MF_CHECKED : MF_UNCHECKED);

			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(hDlg);
			TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
				lpClickPoint.x, lpClickPoint.y - 20, 0, hDlg, NULL);
			PostMessage(hDlg, WM_NULL, 0, 0);
		} break;
		case NIN_BALLOONTIMEOUT:
			if (isNewVersion) 
				isNewVersion = false;
			else
			{
				Shell_NotifyIcon(NIM_DELETE, &conf->niData);
				Shell_NotifyIcon(NIM_ADD, &conf->niData);
			}
			break;
		case NIN_BALLOONUSERCLICK:
			if (isNewVersion) {
				ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools/releases", NULL, NULL, SW_SHOWNORMAL);
				isNewVersion = false;
			}
			break;
		case WM_MOUSEMOVE: {
			string name = conf->activeProfile->name;
			if (conf->stateEffects) {
				name.append(string("\nEffects: ") + (eve->sysmon ? "M" : "m")
					+ (eve->capt ? "A" : "a")
					+ (eve->audio ? "H" : "h")
					+ (eve->grid ? "G" : "g"));
			}
			if (mon) {
				name.append(string("\n") + (fan_conf->lastProf->gmodeStage ? "G-mode" : *fan_conf->GetPowerName(mon->acpi->powers[fan_conf->lastProf->powerStage])));
				for (int i = 0; i < mon->fansize; i++) {
					name.append("\n" + GetFanName(i, true));
				}
			}
			//conf->niData.szTip[127] = 0;
			strcpy_s(conf->niData.szTip, min(128, name.length() + 1), name.c_str());
			//conf->niData.uFlags = NIF_TIP | NIF_SHOWTIP;
			Shell_NotifyIcon(NIM_MODIFY, &conf->niData);
		} break;
		}
	} break;
	case WM_MENUCOMMAND: {
		int idx = GetMenuItemID((HMENU)lParam, LOWORD(wParam));
		switch (idx) {
		case ID_TRAYMENU_EXIT:
			DestroyWindow(hDlg);
			break;
		case ID_TRAYMENU_REFRESH:
			fxhl->ClearAndRefresh();
			break;
		case ID_TRAYMENU_LIGHTSON:
			conf->lightsOn = !conf->lightsOn;
			UpdateState();
			break;
		case ID_TRAYMENU_DIMLIGHTS:
			conf->dimmed = !conf->dimmed;
			UpdateState();
			break;
		case ID_TRAYMENU_ENABLEEFFECTS:
			conf->enableEffects = !conf->enableEffects;
			UpdateState(true);
			break;
		case ID_TRAYMENU_PROFILESWITCH:
			eve->ToggleProfiles();
			SelectProfile();
			break;
		case ID_TRAYMENU_RESTORE:
			RestoreApp();
			break;
		default: {
			// profile selected
			SelectProfile(conf->FindProfile(idx));
		}
		}
	} break;
	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC: {
			// resume from sleep/hibernate
			DebugPrint("Resume from Sleep/hibernate initiated\n");
			if (mon)
				mon->Start();
			fxhl->Start();
			conf->SetIconState(conf->updateCheck);
			eve->StartProfiles();
		} break;
		case PBT_APMPOWERSTATUSCHANGE:
			// ac/batt change
			DebugPrint("Power source change initiated\n");
			eve->ChangePowerState();
			if (tabSel == TAB_FANS)
				OnSelChanged();
			break;
		case PBT_POWERSETTINGCHANGE: {
			if (conf->offWithScreen &&
				((((POWERBROADCAST_SETTING*)lParam)->PowerSetting == GUID_LIDSWITCH_STATE_CHANGE && GetSystemMetrics(SM_CMONITORS) == 1) ||
				((POWERBROADCAST_SETTING*)lParam)->PowerSetting == GUID_CONSOLE_DISPLAY_STATE ||
				((POWERBROADCAST_SETTING*)lParam)->PowerSetting == GUID_SESSION_DISPLAY_STATUS ||
				((POWERBROADCAST_SETTING*)lParam)->PowerSetting == GUID_MONITOR_POWER_ON)) {
				// react state change state
				fxhl->stateScreen = ((POWERBROADCAST_SETTING*)lParam)->Data[0] > 0;
				fxhl->stateDim = ((POWERBROADCAST_SETTING*)lParam)->Data[0] == 2;
				DebugPrint("Screen state changed to " + to_string(((POWERBROADCAST_SETTING*)lParam)->Data[0]) + "\n");
				eve->ChangeEffectMode();
			}
		} break;
		case PBT_APMSUSPEND:
			DebugPrint("Sleep/hibernate initiated\n");
			PauseSystem();
			break;
		}
		break;
	case WM_SETTINGCHANGE: {
		// Get accent color
		DWORD acc = conf->GetAccentColor();
		if (acc != conf->accentColor) {
			conf->accentColor = acc;
			fxhl->Refresh();
		}
		} break;
	case WM_DEVICECHANGE: {
		if (wParam == DBT_DEVNODES_CHANGED) {
			DebugPrint("Device list changed \n");
			UpdateLightDevices();
		}
	} break;
	case WM_DISPLAYCHANGE:
		// Monitor configuration changed
		DebugPrint("Display config changed!\n");
		dxgi_Restart();
		//ChangeDisplaySettings() - ToDo - change freqs using it.
		break;
	case WM_ENDSESSION:
		// Shutdown/restart scheduled....
		DebugPrint("Shutdown initiated\n");
		PauseSystem();
		break;
	case WM_HOTKEY:
		if (wParam > 9 && wParam - 11 <= conf->profiles.size()) { // Profile switch
			SelectProfile(wParam == 10 ? conf->FindDefaultProfile() : conf->profiles[wParam - 11]);
			break;
		}
		if (mon && wParam > 29 && wParam - 30 < mon->powerSize) { // PowerMode switch
			mon->SetPowerMode((WORD)wParam - 30);
			PowerChangeNotify(conf->keyShortcuts);
			if (tabSel == TAB_FANS)
				OnSelChanged();
			break;
		}
		switch (wParam) {
		case 3: // on/off
			conf->lightsOn = !conf->lightsOn;
			UpdateState();
			break;
		case 4: // dim
			conf->dimmed = !conf->dimmed;
			UpdateState();
			break;
		case 1: // off-dim-full circle
			if (conf->lightsOn)
				if (conf->stateDimmed) {
					conf->lightsOn = false;
					conf->dimmed = false;
				}
				else
					conf->dimmed = true;
			else
				conf->lightsOn = true;
			UpdateState();
			break;
		case 5: // effects
			conf->enableEffects = !conf->enableEffects;
			UpdateState(true);
			break;
		case 6: // profile autoswitch
			eve->ToggleProfiles();
			SelectProfile();
			break;
		case 2: // G-key for Dell G-series power switch
			if (mon) {
				AlterGMode(fan_conf->keyShortcuts);
				if (tabSel == TAB_FANS)
					OnSelChanged();
			}
			break;
		case 7: case 8: // Brightness up/down
			if (conf->stateOn) {
				DWORD& bright = conf->stateDimmed ? conf->dimmingPower : conf->fullPower;
				switch (wParam) {
				case 7: bright = min(bright + 0x10, 0xff); break;
				case 8: bright = max((int)bright - 0x10, 0);
				}
				UpdateState();
			}
			break;
		default: return false;
		}
		break;
	case WM_CLOSE:
		SendMessage(hDlg, WM_SIZE, SIZE_MINIMIZED, 0);
		break;
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &conf->niData);
		PostQuitMessage(0);
		break;
	default: return false;
	}
	return true;
}

DWORD MakeRGB(AlienFX_SDK::Afx_colorcode c) {
	return RGB(c.r, c.g, c.b);
}

DWORD MakeRGB(AlienFX_SDK::Afx_action* act) {
	return RGB(act->r, act->g, act->b);
}

AlienFX_SDK::Afx_colorcode Act2Code(AlienFX_SDK::Afx_action *act) {
	return AlienFX_SDK::Afx_colorcode({ act->b, act->g, act->r });
}

AlienFX_SDK::Afx_action Code2Act(AlienFX_SDK::Afx_colorcode c) {
	return AlienFX_SDK::Afx_action({ 0,0,0,c.r,c.g,c.b });
}

UINT_PTR Lpcchookproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_CTLCOLOREDIT)
	{
		mod->r = GetDlgItemInt(hDlg, COLOR_RED, NULL, false);
		mod->g = GetDlgItemInt(hDlg, COLOR_GREEN, NULL, false);
		mod->b = GetDlgItemInt(hDlg, COLOR_BLUE, NULL, false);
		if (needColorUpdate && activeMapping && memcmp(&lastColor, mod, sizeof(AlienFX_SDK::Afx_action))) {
			memcpy(&lastColor, mod, sizeof(AlienFX_SDK::Afx_action));
			fxhl->RefreshZone(activeMapping);
		}
	}
	return 0;
}

bool SetColor(HWND ctrl, AlienFX_SDK::Afx_action* map, bool needUpdate = true) {
	CHOOSECOLOR cc{ sizeof(cc), ctrl, NULL, MakeRGB(map), (LPDWORD)conf->customColors,
		CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR | CC_ENABLEHOOK, NULL, Lpcchookproc };
	// Let's change CC 15 to accent color
	conf->customColors[15] = conf->accentColor;

	bool ret;
	AlienFX_SDK::Afx_action savedColor = *map;
	lastColor = savedColor;
	mod = map;
	needColorUpdate = needUpdate;

	if (!(ret = ChooseColor(&cc))) {
		(*map) = savedColor;
		if (needUpdate && activeMapping)
			fxhl->RefreshZone(activeMapping);
	}

	RedrawButton(ctrl, MakeRGB(map));
	return ret;
}

bool SetColor(HWND ctrl, AlienFX_SDK::Afx_colorcode& clr) {
	bool ret;
	AlienFX_SDK::Afx_action savedColor = Code2Act(clr);
	if (ret = SetColor(ctrl, &savedColor, false))
		clr = Act2Code(&savedColor);
	return ret;
}

AlienFX_SDK::Afx_group* FindCreateMappingGroup() {
	if (!activeMapping) {
		int eItem = 0x10000;
		while (conf->afx_dev.GetGroupById(eItem))
			eItem++;
		conf->activeProfile->lightsets.push_back({ eItem });
		activeMapping = &conf->activeProfile->lightsets.back();
	}
	return conf->FindCreateGroup(activeMapping->group);
}

bool OpenFileOrDir(string& resname, bool doNotStrip) {
	BROWSEINFO bri{ 0 };
	char dname[MAX_PATH]{ "" }, fname[MAX_PATH]{ "" };
	bri.pszDisplayName = dname;
	bri.ulFlags = BIF_BROWSEINCLUDEFILES | BIF_DONTGOBELOWDOMAIN | BIF_NONEWFOLDERBUTTON | BIF_NOTRANSLATETARGETS /*| BIF_RETURNFSANCESTORS*/;
	CoInitializeEx(nullptr, COINIT::COINIT_MULTITHREADED);
	LPITEMIDLIST pIDL = SHBrowseForFolder(&bri);
	bool isOk = pIDL != NULL;
	if (isOk) {
		SHGetPathFromIDList(pIDL, fname);
		CoTaskMemFree(pIDL);
		resname = string(fname);
		if (resname.length() - resname.find_last_of('.') < 5) {
			resname = doNotStrip ? resname : dname;
		}
		else {
			resname += '\\';
		}
	}
	CoUninitialize();
	return isOk;
}




