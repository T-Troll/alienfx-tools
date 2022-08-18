#include <Shlobj.h>
#include <windowsx.h>
#include <string>
#include "resource.h"
#include "ConfigMon.h"
#include "SenMonHelper.h"
#include "ThreadHelper.h"
#include "Common.h"

using namespace std;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND mDlg = 0;
bool needUpdateFeedback = false;
bool isNewVersion = false;
bool needRemove = false;
bool runUIUpdate = true;
int selSensor = 0;

AlienFan_SDK::Control* acpi = NULL;

UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));

ConfigMon* conf;
SenMonHelper* senmon;

// Forward declarations of functions included in this code module:
HWND                InitInstance(HINSTANCE, int);
BOOL CALLBACK       DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void ResetDPIScale();

void UpdateMonUI(LPVOID);
ThreadHelper* muiThread = NULL;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	conf = new ConfigMon();

	if (conf->eSensors || conf->bSensors)
		EvaluteToAdmin();

	// Due to the fact task run delay broken in W10....
	/*if (wstring(lpCmdLine) == L"-d")
		Sleep(7000);*/

	senmon = new SenMonHelper();

	ResetDPIScale();

    // Perform application initialization:
    if (!InitInstance (hInstance, conf->startMinimized ? SW_HIDE : SW_NORMAL))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALIENFXMON));

    MSG msg;
    // Main message loop:
    while ((GetMessage(&msg, 0, 0, 0)) != 0) {
        if (!TranslateAccelerator(mDlg, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	delete senmon;
	delete conf;

    return (int) msg.wParam;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;
	CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN_WINDOW), NULL, (DLGPROC)DialogMain);

	if (mDlg) {

		SendMessage(mDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXMON)));
		SendMessage(mDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXMON), IMAGE_ICON, 16, 16, 0));

		ShowWindow(mDlg, nCmdShow);
	}
	return mDlg;
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

void RedrawButton(unsigned id, DWORD clr) {
	RECT rect;
	HBRUSH Brush = NULL;
	HWND tl = GetDlgItem(mDlg, id);
	GetWindowRect(tl, &rect);
	HDC cnt = GetDC(tl);
	rect.bottom -= rect.top;
	rect.right -= rect.left;
	rect.top = rect.left = 0;
	// BGR!
	Brush = CreateSolidBrush(clr);
	FillRect(cnt, &rect, Brush);
	DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
	DeleteObject(Brush);
	ReleaseDC(tl, cnt);
}

void UpdateItemInfo() {
	CheckDlgButton(mDlg, IDC_CHECK_INTRAY, conf->active_sensors[selSensor].intray);
	CheckDlgButton(mDlg, IDC_CHECK_INVERTED, conf->active_sensors[selSensor].inverse);
	CheckDlgButton(mDlg, IDC_CHECK_ALARM, conf->active_sensors[selSensor].alarm);
	CheckDlgButton(mDlg, IDC_CHECK_DIRECTION, conf->active_sensors[selSensor].direction);
	SetDlgItemInt(mDlg, IDC_ALARM_POINT, conf->active_sensors[selSensor].ap, false);
	RedrawButton(IDC_BUTTON_COLOR, conf->active_sensors[selSensor].traycolor);
}

void ReloadSensorView() {
	int pos = 0, rpos = 0;
	HWND list = GetDlgItem(mDlg, IDC_SENSOR_LIST);
	ListView_DeleteAllItems(list);
	ListView_SetExtendedListViewStyle(list, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	if (!ListView_GetColumnWidth(list, 0)) {
		LVCOLUMNA lCol{ LVCF_TEXT | LVCF_SUBITEM };
		lCol.pszText = (LPSTR)"Min";
		ListView_InsertColumn(list, 0, &lCol);
		lCol.pszText = (LPSTR)"Cur";
		lCol.iSubItem = 1;
		ListView_InsertColumn(list, 1, &lCol);
		lCol.pszText = (LPSTR)"Max";
		lCol.iSubItem = 2;
		ListView_InsertColumn(list, 2, &lCol);
		lCol.pszText = (LPSTR)"Name";
		lCol.iSubItem = 3;
		ListView_InsertColumn(list, 3, &lCol);
	}

	for (int i = 0; i < conf->active_sensors.size(); i++) {
		if (!conf->showHidden ^ conf->active_sensors[i].disabled) {
			string name = conf->active_sensors[i].min > NO_SEN_VALUE ? to_string(conf->active_sensors[i].min) : "--";
			LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM, pos };
			lItem.lParam = i;
			lItem.pszText = (LPSTR)name.c_str();
			ListView_InsertItem(list, &lItem);
			if (i == selSensor) {
				ListView_SetItemState(list, pos, LVIS_SELECTED, LVIS_SELECTED);
				rpos = pos;
				UpdateItemInfo();
			}
			name = conf->active_sensors[i].min > NO_SEN_VALUE ? to_string(conf->active_sensors[i].cur) : "--";
			ListView_SetItemText(list, pos, 1, (LPSTR)name.c_str());
			name = conf->active_sensors[i].min > NO_SEN_VALUE ? to_string(conf->active_sensors[i].max) : "--";
			ListView_SetItemText(list, pos, 2, (LPSTR)name.c_str());
			ListView_SetItemText(list, pos, 3, (LPSTR)conf->active_sensors[i].name.c_str());
			pos++;
		}
	}
	RECT cArea;
	GetClientRect(list, &cArea);
	int wd = 0;
	for (int i = 0; i < 3; i++) {
		ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE);
		wd += ListView_GetColumnWidth(list, i);
	}
	ListView_SetColumnWidth(list, 3, cArea.right - wd);
	ListView_EnsureVisible(list, rpos, false);
	conf->needFullUpdate = false;
}

void RemoveSensors(int src, bool state) {
	for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++)
		if (iter->source == src) {
			iter->disabled = !state;
			iter->oldCur = NO_SEN_VALUE;
		}
	conf->needFullUpdate = true;
}

bool SetColor(int id, DWORD* clr) {
	CHOOSECOLOR cc{ sizeof(cc), mDlg };
	bool ret;
	DWORD custColors[16]{ 0 };
	// Initialize CHOOSECOLOR
	cc.lpCustColors = custColors;
	cc.rgbResult = *clr;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ret = ChooseColor(&cc)) {
		*clr = (DWORD)cc.rgbResult;
	}
	RedrawButton(id, *clr);
	return ret;
}

void RestoreWindow(int id) {
	if (id) selSensor = (id & 0xff) - 1;
	ShowWindow(mDlg, SW_RESTORE);
	SetForegroundWindow(mDlg);
	RedrawButton(IDC_BUTTON_COLOR, conf->active_sensors[selSensor].traycolor);
	ReloadSensorView();
}

void FindValidSensor() {
	for (; selSensor < conf->active_sensors.size() &&
		!(!conf->showHidden ^ conf->active_sensors[selSensor].disabled); selSensor++);
	if (selSensor == conf->active_sensors.size()) {
		for (selSensor--; selSensor > 0 && !(!conf->showHidden ^ conf->active_sensors[selSensor].disabled);	selSensor--);
	}
	if (selSensor < 0) selSensor = 0;
}

BOOL CALLBACK DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	if (message == newTaskBar) {
		// Started/restarted explorer...
		Shell_NotifyIcon(NIM_ADD, &conf->niData);
		if (conf->updateCheck)
			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		for (auto i = conf->active_sensors.begin(); i < conf->active_sensors.end(); i++) {
			if (i->intray)
				i->oldCur = NO_SEN_VALUE;
		}
		return true;
	}

	switch (message)
	{
	case WM_INITDIALOG:
	{
		mDlg = hDlg;
		CheckDlgButton(hDlg, IDC_STARTW, conf->startWindows);
		CheckDlgButton(hDlg, IDC_STARTM, conf->startMinimized);
		CheckDlgButton(hDlg, IDC_CHECK_UPDATE, conf->updateCheck);
		CheckDlgButton(hDlg, IDC_WSENSORS, conf->wSensors);
		CheckDlgButton(hDlg, IDC_ESENSORS, conf->eSensors);
		CheckDlgButton(hDlg, IDC_BSENSORS, conf->bSensors);

		Edit_SetText(GetDlgItem(hDlg, IDC_REFRESH_TIME), to_string(conf->refreshDelay).c_str());

		FindValidSensor();
		ReloadSensorView();

		conf->niData.hWnd = hDlg;

		if (Shell_NotifyIcon(NIM_ADD, &conf->niData) && conf->updateCheck) {
			// check update....
			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		}

		// Start UI update thread...
		muiThread = new ThreadHelper(UpdateMonUI, hDlg, conf->refreshDelay);

	} break;
	case WM_COMMAND:
	{
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDOK: {
			HWND senList = GetDlgItem(hDlg, IDC_SENSOR_LIST);
			HWND editC = ListView_GetEditControl(senList);
			if (editC) {
				RECT rect;
				ListView_GetSubItemRect(senList, ListView_GetSelectionMark(senList), 3, LVIR_LABEL, &rect);
				SetWindowPos(editC, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
			}
		} break;
		case IDCLOSE: case IDC_CLOSE: case IDM_EXIT:
		{
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		} break;
		case IDC_STARTM:
			conf->startMinimized = state;
			break;
		case IDC_STARTW:
		{
			conf->startWindows = state;
			if (WindowsStartSet(state, "AlienFX-Mon"))
				conf->Save();
		} break;
		case IDC_CHECK_UPDATE:
			conf->updateCheck = state;
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
			break;
		case IDC_WSENSORS:
			conf->wSensors = state;
			RemoveSensors(0, state);
			break;
		case IDC_ESENSORS:
			conf->eSensors = state;
			RemoveSensors(1, state);
			if (state) {
				conf->Save();
				EvaluteToAdmin();
			}
			break;
		case IDC_BSENSORS:
			conf->bSensors = state;
			RemoveSensors(2, state);
			if (state) {
				conf->Save();
				EvaluteToAdmin();
			}
			senmon->ModifyMon();
			CheckDlgButton(hDlg, IDC_BSENSORS, conf->bSensors);
			break;
		case IDC_BUTTON_RESET:
			for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++) {
				iter->max = iter->min = iter->cur;
				iter->oldCur = NO_SEN_VALUE;
			}
			ReloadSensorView();
			break;
		case IDC_BUTTON_MINIMIZE:
			ShowWindow(hDlg, SW_HIDE);
			break;
		case IDC_BUTTON_COLOR:
			SetColor(IDC_BUTTON_COLOR, &conf->active_sensors[selSensor].traycolor);
			conf->active_sensors[selSensor].oldCur = NO_SEN_VALUE;
			break;
		case IDC_CHECK_INTRAY:
			conf->active_sensors[selSensor].intray = state;
			break;
		case IDC_CHECK_INVERTED:
			conf->active_sensors[selSensor].inverse = state;
			conf->active_sensors[selSensor].oldCur = NO_SEN_VALUE;
			break;
		case IDC_CHECK_ALARM:
			conf->active_sensors[selSensor].alarm = state;
			conf->active_sensors[selSensor].ap = GetDlgItemInt(hDlg, IDC_ALARM_POINT, NULL, false);
			break;
		case IDC_CHECK_DIRECTION:
			conf->active_sensors[selSensor].direction = state;
			break;
		case IDC_BUT_HIDE:
			conf->active_sensors[selSensor].disabled = !conf->showHidden;
			FindValidSensor();
			ReloadSensorView();
			break;
		case IDC_BUT_RESET:
			conf->active_sensors[selSensor].max = conf->active_sensors[selSensor].min = conf->active_sensors[selSensor].cur;
			conf->active_sensors[selSensor].oldCur = NO_SEN_VALUE;
			break;
		case IDC_SHOW_HIDDEN:
			conf->showHidden = state;
			SetDlgItemText(hDlg, IDC_BUT_HIDE, state ? "Unhide" : "Hide");
			selSensor = 0;
			FindValidSensor();
			ReloadSensorView();
			break;
		case IDC_REFRESH_TIME:
			if (HIWORD(wParam) == EN_CHANGE) {
				conf->refreshDelay = GetDlgItemInt(hDlg, IDC_REFRESH_TIME, NULL, false);
				if (muiThread)
					muiThread->delay = conf->refreshDelay;
			}
			break;
		case IDC_ALARM_POINT:
			if (HIWORD(wParam) == EN_CHANGE) {
				conf->active_sensors[selSensor].ap = GetDlgItemInt(hDlg, IDC_ALARM_POINT, NULL, false);
			}
			break;
		}
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_COLOR:
		{
			if (selSensor < conf->active_sensors.size())
				RedrawButton(IDC_BUTTON_COLOR, conf->active_sensors[selSensor].traycolor);
		} break;
		}
		break;
	case WM_APP + 1:
	{
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
			RestoreWindow((int)wParam);
			break;
		case WM_RBUTTONUP: case WM_CONTEXTMENU:
		{
			POINT lpClickPoint;
			HMENU tMenu = LoadMenuA(hInst, MAKEINTRESOURCEA(IDC_MON_TRAY));
			tMenu = GetSubMenu(tMenu, 0);
			MENUINFO mi{ sizeof(mi), MIM_STYLE, MNS_NOTIFYBYPOS };
			SetMenuInfo(tMenu, &mi);
			MENUITEMINFO mInfo{ sizeof(MENUITEMINFO), MIIM_STRING | MIIM_ID };
			CheckMenuItem(tMenu, ID__PAUSE, conf->paused ? MF_CHECKED : MF_UNCHECKED);
			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(hDlg);
			TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
				lpClickPoint.x, lpClickPoint.y, 0, hDlg, NULL);
		} break;
		case NIN_BALLOONUSERCLICK:
		{
			if (isNewVersion) {
				ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools/releases", NULL, NULL, SW_SHOWNORMAL);
				isNewVersion = false;
			} else
				RestoreWindow(0);
		} break;
		case NIN_BALLOONHIDE: case NIN_BALLOONTIMEOUT:
			if (!isNewVersion && needRemove) {
				Shell_NotifyIcon(NIM_DELETE, &conf->niData);
				Shell_NotifyIcon(NIM_ADD, &conf->niData);
				needRemove = false;
			}
			else
				isNewVersion = false;
			break;
		}
		break;
	} break;
	case WM_MENUCOMMAND: {
		int idx = LOWORD(wParam);
		switch (GetMenuItemID((HMENU)lParam, idx)) {
		case ID__EXIT:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;
		case ID__RESTORE:
			RestoreWindow((int)wParam);
			break;
		case ID__PAUSE:
			conf->paused = !conf->paused;
			if (conf->paused)
				senmon->StopMon();
			else
				senmon->StartMon();
			break;
		case ID__RESETMIN:
			for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++)
				iter->max = iter->min = iter->cur;
			ReloadSensorView();
			break;
		}
	} break;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			// go to tray...
			ShowWindow(hDlg, SW_HIDE);
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg, IDOK);
		DestroyWindow(hDlg);
		break;
	case WM_DESTROY:
		delete muiThread;
		// Remove icons from tray...
		Shell_NotifyIcon(NIM_DELETE, &conf->niData);
		for (auto i = conf->active_sensors.begin(); i < conf->active_sensors.end(); i++)
			if (i->niData) {
				Shell_NotifyIcon(NIM_DELETE, i->niData);
				if (i->niData->hIcon)
					DestroyIcon(i->niData->hIcon);
				delete i->niData;
			}
		PostQuitMessage(0);
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_SENSOR_LIST: {
			HWND senList = GetDlgItem(hDlg, (int)((NMHDR*)lParam)->idFrom);
			switch (((NMHDR*)lParam)->code) {
			case LVN_BEGINLABELEDIT: {
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				HWND editC = ListView_GetEditControl(senList);
				Edit_SetText(editC, conf->active_sensors[sItem->item.lParam].name.c_str());
			} break;
			case LVN_ITEMACTIVATE:
			{
				runUIUpdate = false;
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*)lParam;
				HWND editC = ListView_EditLabel(senList, sItem->iItem);
				RECT rect;
				ListView_GetSubItemRect(senList, sItem->iItem, 3, LVIR_LABEL, &rect);
				SetWindowPos(editC, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
			} break;
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uNewState & LVIS_FOCUSED) {
					selSensor = (int)lPoint->lParam;
					UpdateItemInfo();
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				if (sItem->item.pszText) {
					conf->active_sensors[selSensor].name = sItem->item.pszText;
					ListView_SetItemText(senList, sItem->item.iItem, 3, sItem->item.pszText);
				}
				runUIUpdate = true;
				//return false;
			} break;
			}
		} break;
		}
		break;
	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC: {
			// resume from sleep/hibernate
			//DebugPrint("Resume from Sleep/hibernate initiated\n");
			senmon->StartMon();
			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		} break;
		case PBT_APMSUSPEND:
			// Sleep initiated.
			//DebugPrint("Sleep/hibernate initiated\n");
			senmon->StopMon();
			break;
		}
		break;
	case WM_ENDSESSION:
		conf->Save();
		return 0;
	default: return false;
	}
	return true;
}

void UpdateTrayData(SENSOR* sen, byte index) {

	NOTIFYICONDATA* niData = sen->niData;
	char val[3];

	if (sen->cur != 100)
		sprintf_s(val, "%2d", sen->cur > 100 ? sen->cur / 100 : sen->cur);
	else
		sprintf_s(val, "00");

	sprintf_s(niData->szTip, 128, "%s\nMin: %d\nCur: %d\nMax: %d", sen->name.c_str(), sen->min, sen->cur, sen->max);

	RECT clip{ 0,0,32,32 };

	HDC hdc = GetDC(NULL), hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 32, 32),
		hBitmapMask = sen->inverse ? CreateCompatibleBitmap(hdc, 32, 32) : hBitmap;
	HFONT hFont = CreateFont(32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Microsoft Sans Serif"));
	HBRUSH brush = CreateSolidBrush(sen->inverse ? 0xffffff : sen->traycolor);

	SelectObject(hdcMem, hFont);
	SelectObject(hdcMem, hBitmap);

	SetTextColor(hdcMem, sen->inverse ?
		sen->traycolor != 0xffffff ? sen->traycolor : 0
		: sen->traycolor ? 0 : 0xffffff);

	SetBkColor(hdcMem, sen->inverse ? 0xffffff : sen->traycolor);
	SetBkMode(hdcMem, OPAQUE);
	FillRect(hdcMem, &clip, brush);
	DrawText(hdcMem, val, -1, &clip, DT_RIGHT | DT_SINGLELINE /*| DT_VCENTER*/ | DT_NOCLIP);

	if (sen->inverse) {
		SelectObject(hdcMem, hBitmapMask);
		SetTextColor(hdcMem, sen->traycolor);
		SetBkColor(hdcMem, 0x0);
		SetBkMode(hdcMem, TRANSPARENT);
		DrawText(hdcMem, val, -1, &clip, DT_RIGHT | DT_SINGLELINE /*| DT_VCENTER*/ | DT_NOCLIP);
	}

	DeleteObject(brush);

	ICONINFO iconInfo{true, 0, 0, hBitmap, hBitmapMask };
	HICON hIcon = CreateIconIndirect(&iconInfo);

	if (niData->hIcon) {
		DestroyIcon(niData->hIcon);
	}

	niData->hIcon = hIcon;

	DeleteObject(hFont);
	DeleteObject(hBitmap);
	if (sen->inverse)
		DeleteObject(hBitmapMask);
	DeleteDC(hdcMem);
	ReleaseDC(mDlg, hdc);
}

void UpdateMonUI(LPVOID lpParam) {
	HWND list = GetDlgItem(mDlg, IDC_SENSOR_LIST);
	if (IsWindowVisible(mDlg) && runUIUpdate) {
		//DebugPrint("Fans UI update...\n");
		if (conf->needFullUpdate) {
			ReloadSensorView();
		}
		else {
			if (!conf->showHidden) {
				int pos = 0;
				for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++) {
					if (!iter->disabled) {
						if (iter->min != NO_SEN_VALUE && iter->cur != iter->oldCur) {
							string name = to_string(iter->min);
							ListView_SetItemText(list, pos, 0, (LPSTR)name.c_str());
							name = to_string(iter->cur);
							ListView_SetItemText(list, pos, 1, (LPSTR)name.c_str());
							name = to_string(iter->max);
							ListView_SetItemText(list, pos, 2, (LPSTR)name.c_str());
							//ListView_SetItemText(list, i, 3, (LPSTR)iter->name.c_str());
						}
						pos++;
					}
				}
			}
		}
	}
	else
		conf->needFullUpdate = true;
	// Tray works...
	for (int i = 0; i < conf->active_sensors.size(); i++) {
		if (conf->active_sensors[i].intray && !conf->active_sensors[i].disabled && conf->active_sensors[i].min > NO_SEN_VALUE) {
			if (conf->active_sensors[i].cur != conf->active_sensors[i].oldCur) {
				if (!conf->active_sensors[i].niData) {
					// add tray icon
					conf->active_sensors[i].niData = new NOTIFYICONDATA({ sizeof(NOTIFYICONDATA), mDlg, (unsigned)i + 1,
						NIF_ICON | NIF_TIP | NIF_MESSAGE, WM_APP + 1 });
				}
				UpdateTrayData(&conf->active_sensors[i], 0);
				if (!Shell_NotifyIcon(NIM_MODIFY, conf->active_sensors[i].niData))
					Shell_NotifyIcon(NIM_ADD, conf->active_sensors[i].niData);
				//else {
				//	// add new tray icon
				//	conf->active_sensors[i].niData = new NOTIFYICONDATA({ sizeof(NOTIFYICONDATA), mDlg, (unsigned)i + 1,
				//		NIF_ICON | NIF_TIP | NIF_MESSAGE, WM_APP + 1 });
				//	if (!Shell_NotifyIcon(NIM_ADD, conf->active_sensors[i].niData)) {
				//		DestroyIcon(conf->active_sensors[i].niData->hIcon);
				//		delete conf->active_sensors[i].niData;
				//		conf->active_sensors[i].niData = NULL;
				//	}
				//}
			}
		}
		else {
			// remove tray icon
			if (conf->active_sensors[i].niData) {
				Shell_NotifyIcon(NIM_DELETE, conf->active_sensors[i].niData);
				DestroyIcon(conf->active_sensors[i].niData->hIcon);
				delete conf->active_sensors[i].niData;
				conf->active_sensors[i].niData = NULL;
			}
		}
		conf->active_sensors[i].oldCur = conf->active_sensors[i].cur;
	}
}
