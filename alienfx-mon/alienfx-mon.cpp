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
//#pragma comment(lib,"Version.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND mDlg = 0;
bool needUpdateFeedback = false;
bool isNewVersion = false;
bool needRemove = false;
bool runUIUpdate = true;
DWORD selSensor = 0;

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

bool IsSensorValid(map<DWORD, SENSOR>::iterator sen, bool state) {
	if (sen != conf->active_sensors.end()) {
		SENID sid; sid.sid = sen->first;
		return (!state ^ sen->second.disabled) && ((conf->wSensors && sid.source == 0) ||
			(conf->eSensors && sid.source == 1 && (!conf->bSensors || sid.type == 1)) ||
			(conf->bSensors && sid.source == 2));
	}
	return false;
}

void FindValidSensor() {
	auto sen = conf->active_sensors.find(selSensor);
	auto senPlus = sen, senMinus = sen;
	while (senPlus != conf->active_sensors.end() || senMinus != conf->active_sensors.begin()) {
		if (senPlus != conf->active_sensors.end()) {
			if (IsSensorValid(senPlus, conf->showHidden)) {
				selSensor = senPlus->first;
				return;
			}
			else
				senPlus++;
		}
		if (senMinus != conf->active_sensors.begin()) {
			if (IsSensorValid(senMinus, conf->showHidden)) {
				selSensor = senMinus->first;
				return;
			}
			else
				senMinus--;
		}
	}
	if (senMinus != conf->active_sensors.end())
		selSensor = senMinus->first;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
	//UNREFERENCED_PARAMETER(nCmdShow);

	ResetDPIScale(lpCmdLine);

	conf = new ConfigMon();

	if (conf->eSensors || conf->bSensors)
		EvaluteToAdmin();

	senmon = new SenMonHelper();

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
	GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_COLOR), &rect);
	MapWindowPoints(HWND_DESKTOP, mDlg, (LPPOINT)&rect, 2);
	RedrawWindow(mDlg, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void UpdateItemInfo() {
	if (conf->active_sensors.find(selSensor) != conf->active_sensors.end()) {
		CheckDlgButton(mDlg, IDC_CHECK_INTRAY, conf->active_sensors[selSensor].intray);
		CheckDlgButton(mDlg, IDC_CHECK_INVERTED, conf->active_sensors[selSensor].inverse);
		CheckDlgButton(mDlg, IDC_CHECK_ALARM, conf->active_sensors[selSensor].alarm);
		CheckDlgButton(mDlg, IDC_CHECK_DIRECTION, conf->active_sensors[selSensor].direction);
		SetDlgItemInt(mDlg, IDC_ALARM_POINT, conf->active_sensors[selSensor].ap, false);
		RedrawButton(IDC_BUTTON_COLOR, conf->active_sensors[selSensor].traycolor);
	}
}

char* GetSensorName(SENSOR* id) {
	return (LPSTR)(id->name.length() ? id->name : id->sname).c_str();
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
		lCol.pszText = (LPSTR)"Source";
		lCol.iSubItem = 3;
		ListView_InsertColumn(list, 3, &lCol);
		lCol.pszText = (LPSTR)"Name";
		lCol.iSubItem = 4;
		ListView_InsertColumn(list, 4, &lCol);
	}

	SENID sid;
	for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++) {
		sid.sid = i->first;
		if (IsSensorValid(i, conf->showHidden)) {
			string name = i->second.min > NO_SEN_VALUE ? to_string(i->second.min) : "--";
			LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM, pos };
			lItem.lParam = i->first;
			lItem.pszText = (LPSTR)name.c_str();
			ListView_InsertItem(list, &lItem);
			if (i->first == selSensor) {
				ListView_SetItemState(list, pos, LVIS_SELECTED, LVIS_SELECTED);
				rpos = pos;
				UpdateItemInfo();
			}
			name = i->second.min > NO_SEN_VALUE ? to_string(i->second.cur) : "--";
			ListView_SetItemText(list, pos, 1, (LPSTR)name.c_str());
			name = i->second.min > NO_SEN_VALUE ? to_string(i->second.max) : "--";
			ListView_SetItemText(list, pos, 2, (LPSTR)name.c_str());
			switch (sid.source) {
			case 0: name = "W";
				switch (sid.type) {
				case 0: name += "C"; break;
				case 1: name += "R"; break;
				case 2: name += "H"; break;
				case 3: name += "B"; break;
				case 4: name += "G"; break;
				case 5: name += "T"; break;
				}
				break;
			case 1: name = "B";
				switch (sid.type) {
				case 0: name += "T"; break;
				case 1: name += "P"; break;
				}
				break;
			case 2: name = "A";
				switch (sid.type) {
				case 0: name += "T"; break;
				case 1: name += "R"; break;
				case 2: name += "P"; break;
				case 3: name += "B"; break;
				}
				break;
			}
			//name += " " + to_string(sid.id);
			ListView_SetItemText(list, pos, 3, (LPSTR)name.c_str());
			ListView_SetItemText(list, pos, 4, GetSensorName(&i->second));
			pos++;
		}
	}
	RECT cArea;
	GetClientRect(list, &cArea);
	int wd = 0;
	for (int i = 0; i < 4; i++) {
		ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE);
		wd += ListView_GetColumnWidth(list, i);
	}
	ListView_SetColumnWidth(list, 4, cArea.right - wd);
	ListView_EnsureVisible(list, rpos, false);
	conf->needFullUpdate = false;
}

void ModifySensors() {
	if (conf->bSensors || conf->eSensors) {
		conf->Save();
		EvaluteToAdmin();
	}
	conf->paused = true;
	senmon->ModifyMon();
	conf->paused = false;
	CheckDlgButton(mDlg, IDC_BSENSORS, conf->bSensors);
	FindValidSensor();
	conf->needFullUpdate = true;
	//ReloadSensorView();
}

bool SetColor(int id, DWORD* clr) {
	CHOOSECOLOR cc{ sizeof(cc), mDlg };
	bool ret;
	DWORD custColors[16]{ 0 };
	cc.lpCustColors = custColors;
	cc.rgbResult = *clr;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if (ret = ChooseColor(&cc)) {
		*clr = (DWORD)cc.rgbResult;
	}
	RedrawButton(id, *clr);
	return ret;
}

void RestoreWindow(DWORD id) {
	if (id) selSensor = id;
	ShowWindow(mDlg, SW_RESTORE);
	SetForegroundWindow(mDlg);
	RedrawButton(IDC_BUTTON_COLOR, conf->active_sensors[selSensor].traycolor);
	ReloadSensorView();
}

void ResetMinMax() {
	for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++) {
		iter->second.max = iter->second.min = iter->second.cur;
		iter->second.oldCur = NO_SEN_VALUE;
	}
	ReloadSensorView();
}

BOOL CALLBACK DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	if (message == newTaskBar) {
		// Started/restarted explorer...
		AddTrayIcon(&conf->niData, conf->updateCheck);
		for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++) {
			if (i->second.intray)
				i->second.oldCur = NO_SEN_VALUE;
		}
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

		conf->niData.hWnd = hDlg;

		AddTrayIcon(&conf->niData, conf->updateCheck);

		// Start UI update thread...
		muiThread = new ThreadHelper(UpdateMonUI, hDlg, conf->refreshDelay, THREAD_PRIORITY_ABOVE_NORMAL);

		if (conf->active_sensors.size())
			selSensor = conf->active_sensors.begin()->first;
		FindValidSensor();
		ReloadSensorView();
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
				ListView_GetSubItemRect(senList, ListView_GetSelectionMark(senList), 4, LVIR_LABEL, &rect);
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
			ModifySensors();
			break;
		case IDC_ESENSORS:
			conf->eSensors = state;
			ModifySensors();
			break;
		case IDC_BSENSORS:
			conf->bSensors = state;
			ModifySensors();
			break;
		case IDC_BUTTON_RESET:
			ResetMinMax();
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
		if  (((DRAWITEMSTRUCT*)lParam)->CtlID && conf->active_sensors.find(selSensor) != conf->active_sensors.end()) {
			DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
			HBRUSH Brush = CreateSolidBrush(conf->active_sensors[selSensor].traycolor);
			FillRect(ditem->hDC, &ditem->rcItem, Brush);
			DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_RAISED, BF_RECT);
			DeleteObject(Brush);
		}
		break;
	case WM_APP + 1:
	{
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
			RestoreWindow((DWORD)wParam);
			break;
		case WM_RBUTTONUP: case WM_CONTEXTMENU:
		{
			POINT lpClickPoint;
			HMENU tMenu = LoadMenuA(hInst, MAKEINTRESOURCEA(IDC_MON_TRAY));
			tMenu = GetSubMenu(tMenu, 0);
			MENUINFO mi{ sizeof(MENUINFO), MIM_STYLE, MNS_NOTIFYBYPOS };
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
		switch (GetMenuItemID((HMENU)lParam, (int)wParam)) {
		case ID__EXIT:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;
		case ID__RESTORE:
			RestoreWindow(0);
			break;
		case ID__PAUSE:
			conf->paused = !conf->paused;
			break;
		case ID__RESETMIN:
			ResetMinMax();
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
		for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++)
			if (i->second.niData) {
				Shell_NotifyIcon(NIM_DELETE, i->second.niData);
				if (i->second.niData->hIcon)
					DestroyIcon(i->second.niData->hIcon);
				delete i->second.niData;
			}
		PostQuitMessage(0);
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_SENSOR_LIST: {
			HWND senList = ((NMHDR*)lParam)->hwndFrom;
			switch (((NMHDR*)lParam)->code) {
			case LVN_BEGINLABELEDIT: {
				runUIUpdate = false;
				Edit_SetText(ListView_GetEditControl(senList), GetSensorName(&conf->active_sensors[selSensor]));
				//RECT rect;
				//int pos = ListView_GetSelectionMark(senList);
				//ListView_GetSubItemRect(senList, pos, 4, LVIR_LABEL, &rect);
				//SetWindowPos(ListView_EditLabel(senList, pos), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
			} break;
			case LVN_ITEMACTIVATE: case NM_RETURN:// case NM_CLICK:
			{
				runUIUpdate = false;
				int pos = ListView_GetSelectionMark(senList);
				RECT rect;
				ListView_GetSubItemRect(senList, pos, 4, LVIR_LABEL, &rect);
				SetWindowPos(ListView_EditLabel(senList, pos), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
			} break;
			case LVN_ITEMCHANGING:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uNewState & LVIS_FOCUSED && lPoint->iItem != -1) {
					if (selSensor != (DWORD)lPoint->lParam) {
						selSensor = (DWORD)lPoint->lParam;
						UpdateItemInfo();
					}
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				if (sItem->item.pszText) {
					conf->active_sensors[selSensor].name = sItem->item.pszText;
					ListView_SetItemText(senList, sItem->item.iItem, 4, GetSensorName(&conf->active_sensors[selSensor]));
				}
				runUIUpdate = true;
			} break;
			}
		} break;
		}
		break;
	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMRESUMEAUTOMATIC: {
			// resume from sleep/hibernate
			for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++)
				if (i->second.intray) i->second.oldCur = NO_SEN_VALUE;
			conf->needFullUpdate = true;
			if (conf->updateCheck)
				CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		} break;
		//case PBT_APMSUSPEND:
		//	break;
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

	sprintf_s(niData->szTip, 128, "%s\nMin: %d\nCur: %d\nMax: %d", GetSensorName(sen), sen->min, sen->cur, sen->max);

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
	bool visible = IsWindowVisible(mDlg);
	if (!conf->paused) {
		senmon->UpdateSensors();
		if (visible) {
			if (conf->needFullUpdate)
				ReloadSensorView();
		}
		else
			conf->needFullUpdate = true;
		int pos = 0;
		for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++) {
			// Update UI...
			if (visible && runUIUpdate) {
				if (IsSensorValid(i, conf->showHidden)) {
					if (i->second.cur != i->second.oldCur && i->second.min != NO_SEN_VALUE) {
						string name = to_string(i->second.min);
						ListView_SetItemText(list, pos, 0, (LPSTR)name.c_str());
						name = to_string(i->second.cur);
						ListView_SetItemText(list, pos, 1, (LPSTR)name.c_str());
						name = to_string(i->second.max);
						ListView_SetItemText(list, pos, 2, (LPSTR)name.c_str());
					}
					pos++;
				}
			}
			// Update tray icons...
			if (IsSensorValid(i, false) && i->second.intray) {
				if (!i->second.niData) {
					// add tray icon
					i->second.niData = new NOTIFYICONDATA({ sizeof(NOTIFYICONDATA), mDlg, (DWORD)(i->first),
						NIF_ICON | NIF_TIP | NIF_MESSAGE, WM_APP + 1 });
					i->second.oldCur = NO_SEN_VALUE;
				}
				if (i->second.cur != i->second.oldCur && i->second.min != NO_SEN_VALUE) {
					UpdateTrayData(&i->second, 0);
					if (!Shell_NotifyIcon(NIM_MODIFY, i->second.niData))
						Shell_NotifyIcon(NIM_ADD, i->second.niData);
					i->second.oldCur = i->second.cur;
				}
			}
			else {
				// remove tray icon
				if (i->second.niData) {
					Shell_NotifyIcon(NIM_DELETE, i->second.niData);
					DestroyIcon(i->second.niData->hIcon);
					delete i->second.niData;
					i->second.niData = NULL;
					//i->second.oldCur = NO_SEN_VALUE;
				}
			}
		}
		if (visible && runUIUpdate) {
			// resize
			RECT cArea;
			GetClientRect(list, &cArea);
			int wd = 0;
			for (int i = 0; i < 4; i++) {
				ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE);
				wd += ListView_GetColumnWidth(list, i);
			}
			ListView_SetColumnWidth(list, 4, cArea.right - wd);
		}
	}
}
