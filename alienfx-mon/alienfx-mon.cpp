#include <windowsx.h>
#include "resource.h"
#include "ConfigMon.h"
#include "SenMonHelper.h"
#include "Common.h"

using namespace std;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND mDlg = 0;
bool needUpdateFeedback = false;
bool isNewVersion = false;
bool runUIUpdate = true;
DWORD selSensor = 0xffffffff;

int idc_version = IDC_STATIC_VERSION, idc_homepage = IDC_SYSLINK_HOMEPAGE; // for About

AlienFan_SDK::Control* acpi = NULL;

UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));

ConfigMon* conf;
SenMonHelper* senmon;

// Forward declarations of functions included in this code module:
//HWND                InitInstance(HINSTANCE, int);
BOOL CALLBACK       DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

bool IsSensorValid(map<DWORD, SENSOR>::iterator sen) {
	if (sen != conf->active_sensors.end()) {
		SENID sid; sid.sid = sen->first;
		return (!conf->showHidden ^ sen->second.disabled) && ((conf->wSensors && sid.source == 0) ||
			(conf->eSensors && sid.source == 1 && (!conf->bSensors || sid.type == 1)) ||
			(conf->bSensors && sid.source == 2));
	}
	return false;
}

void FindValidSensor() {
	auto sen = conf->active_sensors.begin();
	for (; selSensor != 0xffffffff && sen != conf->active_sensors.end(); sen++)
		if (sen->first == selSensor)
			break;
	bool forward = sen != conf->active_sensors.end();
	while (!IsSensorValid(sen)) {
		if (forward)
			sen++;
		else
			sen--;
		if (sen == conf->active_sensors.end()) {
			//sen--;
			forward = false;
		} else
			if (sen == conf->active_sensors.begin() && !IsSensorValid(sen)) {
				selSensor = 0xffffffff;
				return;
			}
	}
	selSensor = sen->first;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
	//UNREFERENCED_PARAMETER(nCmdShow);

	// Perform application initialization:
	hInst = hInstance;

	ResetDPIScale(lpCmdLine);

	conf = new ConfigMon();

	senmon = new SenMonHelper();

	if (CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN_WINDOW), NULL, (DLGPROC)DialogMain)) {
		SendMessage(mDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ALIENFXMON)));
		SendMessage(mDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_ALIENFXMON), IMAGE_ICON, 16, 16, 0));

		ShowWindow(mDlg, conf->startMinimized ? SW_HIDE : SW_SHOW);

		MSG msg;
		// Main message loop:
		while ((GetMessage(&msg, 0, 0, 0)) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    delete senmon;
	delete conf;

    return 0;
}

void RedrawButton(unsigned id, DWORD clr) {
	RECT rect;
	GetWindowRect(GetDlgItem(mDlg, IDC_BUTTON_COLOR), &rect);
	MapWindowPoints(HWND_DESKTOP, mDlg, (LPPOINT)&rect, 2);
	RedrawWindow(mDlg, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void UpdateItemInfo() {
	auto sen = &conf->active_sensors[selSensor];
	CheckDlgButton(mDlg, IDC_CHECK_INTRAY, sen->intray);
	CheckDlgButton(mDlg, IDC_CHECK_INVERTED, sen->inverse);
	CheckDlgButton(mDlg, IDC_CHECK_ALARM, sen->alarm);
	CheckDlgButton(mDlg, IDC_CHECK_DIRECTION, sen->direction);
	CheckDlgButton(mDlg, IDC_CHECK_HIGHLIGHT, sen->highlight);
	SetDlgItemInt(mDlg, IDC_ALARM_POINT, sen->ap, false);
	RedrawButton(IDC_BUTTON_COLOR, sen->traycolor);
}

char* GetSensorName(SENSOR* id) {
	return (LPSTR)(id->name.length() ? id->name : id->sname).c_str();
}

const char* uiColumns[] = { "Min", "Cur", "Max", "Source", "Name" };
const char sid0[] = { 'C', 'R', 'H', 'B', 'G', 'T' };

void SetNumValue(HWND list, int pos, int col, int val) {
	string name = val > NO_SEN_VALUE ? to_string(val) : "--";
	ListView_SetItemText(list, pos, col, (LPSTR)name.c_str());
}

void ReloadSensorView() {
	int pos = 0, rpos = 0;
	HWND list = GetDlgItem(mDlg, IDC_SENSOR_LIST);
	ListView_DeleteAllItems(list);
	ListView_SetExtendedListViewStyle(list, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	if (!ListView_GetColumnWidth(list, 0)) {
		LVCOLUMNA lCol{ LVCF_TEXT | LVCF_SUBITEM };
		for (int i = 0; i < 5; i++) {
			lCol.iSubItem = i;
			lCol.pszText = (LPSTR)uiColumns[i];
			ListView_InsertColumn(list, i, &lCol);
		}
	}
	SENID sid;
	FindValidSensor();
	for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++) {
		sid.sid = i->first;
		SENSOR* sen = &i->second;
		if (IsSensorValid(i)) {
			LVITEMA lItem{ /*LVIF_TEXT | */LVIF_PARAM | LVIF_STATE, pos };
			lItem.lParam = sid.sid;
			if (sid.sid == selSensor) {
				lItem.state = LVIS_SELECTED;
				rpos = pos;
				UpdateItemInfo();
			}
			ListView_InsertItem(list, &lItem);
			SetNumValue(list, pos, 0, sen->min);
			SetNumValue(list, pos, 1, sen->cur);
			SetNumValue(list, pos, 2, sen->max);
			string name;
			switch (sid.source) {
			case 0: name = string("W") + sid0[sid.type];
				break;
			case 1: name = "I";
				switch (sid.type) {
				case 0: name += "T"; break;
				case 1: name += "P"; break;
				}
				break;
			case 2: name = "A";
				switch (sid.type) {
				case 0: name += "T"; break;
				default: name += "F"; break;
				}
				break;
			}
			ListView_SetItemText(list, pos, 3, (LPSTR)name.c_str());
			ListView_SetItemText(list, pos, 4, GetSensorName(sen));
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

void RemoveTrayIcons() {
	// Remove icons from tray...
	for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++)
		if (i->second.niData) {
			Shell_NotifyIcon(NIM_DELETE, i->second.niData);
			if (i->second.niData->hIcon)
				DestroyIcon(i->second.niData->hIcon);
			delete i->second.niData;
			i->second.niData = NULL;
		}
}

void ResetTraySensors() {
	for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++) {
		if (i->second.intray)
			i->second.cur = NO_SEN_VALUE;
	}
	SendMessage(mDlg, WM_TIMER, 0, 0);
}

void ModifySensors() {
	conf->paused = true;
	RemoveTrayIcons();
	conf->Save();
	senmon->ModifyMon();
	CheckDlgButton(mDlg, IDC_ESENSORS, conf->eSensors);
	CheckDlgButton(mDlg, IDC_BSENSORS, conf->bSensors);
	ResetTraySensors();
	conf->paused = false;
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
	if (id != 0xffffffff) selSensor = id;
	ShowWindow(mDlg, SW_RESTORE);
	SetForegroundWindow(mDlg);
	RedrawButton(IDC_BUTTON_COLOR, conf->active_sensors[selSensor].traycolor);
	SendMessage(mDlg, WM_TIMER, 0, 0);
}

void ResetMinMax(DWORD id = 0xffffffff) {
	for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++)
		if (id == 0xffffffff || id == iter->first) {
			iter->second.max = iter->second.min = iter->second.cur;
			iter->second.cur = NO_SEN_VALUE;
		}
	SendMessage(mDlg, WM_TIMER, 0, 0);
}

void UpdateTrayData(SENSOR* sen) {

	NOTIFYICONDATA* niData = sen->niData;
	char val[5];

	if (sen->cur != 100)
		sprintf_s(val, "  %2d", sen->cur > 100 ? sen->cur / 100 : sen->cur);
	else
		sprintf_s(val, "00");

	sprintf_s(niData->szTip, 128, "%s\nMin: %d\nCur: %d\nMax: %d", GetSensorName(sen), sen->min, sen->cur, sen->max);

	RECT clip{ 0,0,32,32 };

	HDC hdc = GetDC(NULL), hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 32, 32),
		hBitmapMask = hBitmap;
	HFONT hFont = CreateFont(32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Microsoft Sans Serif"));

	DWORD finalColor = sen->highlight && sen->alarming ? 0xff : sen->traycolor;

	SelectObject(hdcMem, hFont);
	SelectObject(hdcMem, hBitmap);

	SetTextColor(hdcMem, sen->inverse ?
		finalColor != 0xffffff ? finalColor : 0
		: finalColor ? 0 : 0xffffff);

	if (!sen->inverse) {
		SetBkMode(hdcMem, OPAQUE);
		SetBkColor(hdcMem, finalColor);
	}

	DrawText(hdcMem, val, -1, &clip, DT_RIGHT | DT_SINGLELINE /*| DT_VCENTER*/ | DT_NOCLIP);

	if (sen->inverse) {
		hBitmapMask = CreateCompatibleBitmap(hdc, 32, 32);
		SelectObject(hdcMem, hBitmapMask);
		SetTextColor(hdcMem, finalColor);
		SetBkMode(hdcMem, TRANSPARENT);
		DrawText(hdcMem, val, -1, &clip, DT_RIGHT | DT_SINGLELINE /*| DT_VCENTER*/ | DT_NOCLIP);
	}

	ICONINFO iconInfo{ true, 0, 0, hBitmap, hBitmapMask };
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

SENSOR* FindSensor() {
	for (auto sen = conf->active_sensors.begin(); sen != conf->active_sensors.end(); sen++)
		if (sen->first == selSensor)
			return &sen->second;
	return nullptr;
}

BOOL CALLBACK DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND senList = GetDlgItem(hDlg, IDC_SENSOR_LIST);
	SENSOR* sen = FindSensor();

	if (message == newTaskBar) {
		// Started/restarted explorer...
		AddTrayIcon(&conf->niData, conf->updateCheck);
		ResetTraySensors();
		SendMessage(hDlg, WM_TIMER, 0, 0);
	}

	switch (message)
	{
	case WM_INITDIALOG:
	{
		conf->niData.hWnd = mDlg = hDlg;
		CheckDlgButton(hDlg, IDC_STARTW, conf->startWindows);
		CheckDlgButton(hDlg, IDC_STARTM, conf->startMinimized);
		CheckDlgButton(hDlg, IDC_CHECK_UPDATE, conf->updateCheck);
		CheckDlgButton(hDlg, IDC_WSENSORS, conf->wSensors);
		CheckDlgButton(hDlg, IDC_ESENSORS, conf->eSensors);
		CheckDlgButton(hDlg, IDC_BSENSORS, conf->bSensors);

		SetDlgItemInt(hDlg, IDC_REFRESH_TIME, conf->refreshDelay, false);

		AddTrayIcon(&conf->niData, conf->updateCheck);
		senmon->UpdateSensors();
		ReloadSensorView();
		SendMessage(hDlg, WM_TIMER, 0, 0);
	} break;
	case WM_COMMAND:
	{
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDOK: {
			RECT rect;
			ListView_GetSubItemRect(senList, ListView_GetSelectionMark(senList), 4, LVIR_LABEL, &rect);
			SetWindowPos(ListView_GetEditControl(senList), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
		} break;
		case IDC_CLOSE: case IDM_EXIT:
		{
			DestroyWindow(hDlg);
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
			if (sen) {
				SetColor(IDC_BUTTON_COLOR, &sen->traycolor);
				sen->cur = NO_SEN_VALUE;
			}
			break;
		case IDC_CHECK_INTRAY:
			if (sen)
				sen->intray = state;
			break;
		case IDC_CHECK_INVERTED:
			if (sen) {
				sen->inverse = state;
				sen->cur = NO_SEN_VALUE;
			}
			break;
		case IDC_CHECK_ALARM:
			if (sen) {
				sen->alarm = state;
				sen->ap = GetDlgItemInt(hDlg, IDC_ALARM_POINT, NULL, false);
			}
			break;
		case IDC_CHECK_DIRECTION:
			if (sen)
				sen->direction = state;
			break;
		case IDC_CHECK_HIGHLIGHT:
			if (sen)
				sen->highlight = state;
			break;
		case IDC_BUT_HIDE:
			if (sen) {
				sen->disabled = !conf->showHidden;
				ReloadSensorView();
			}
			break;
		case IDC_BUT_RESET:
			if (sen)
				ResetMinMax(selSensor);
			break;
		case IDC_SHOW_HIDDEN:
			conf->showHidden = state;
			SetDlgItemText(hDlg, IDC_BUT_HIDE, state ? "Unhide" : "Hide");
			conf->selection[!state] = selSensor;
			selSensor = conf->selection[state];
			ReloadSensorView();
			break;
		case IDC_REFRESH_TIME:
			if (HIWORD(wParam) == EN_CHANGE) {
				conf->refreshDelay = GetDlgItemInt(hDlg, IDC_REFRESH_TIME, NULL, false);
				SetTimer(hDlg, 0, conf->refreshDelay, NULL);
			}
			break;
		case IDC_ALARM_POINT:
			if (HIWORD(wParam) == EN_CHANGE && sen) {
				sen->ap = GetDlgItemInt(hDlg, IDC_ALARM_POINT, NULL, false);
			}
			break;
		}
	} break;
	case WM_DRAWITEM:
		if  (((DRAWITEMSTRUCT*)lParam)->CtlID && sen) {
			DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
			HBRUSH Brush = CreateSolidBrush(sen->traycolor);
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
			MENUINFO mi{ sizeof(MENUINFO), MIM_STYLE | MIM_HELPID, MNS_NOTIFYBYPOS /*| MNS_AUTODISMISS*/ };
			mi.dwContextHelpID = (DWORD)wParam;
			SetMenuInfo(tMenu, &mi);
			MENUITEMINFO mInfo{ sizeof(MENUITEMINFO), MIIM_STRING | MIIM_ID };
			CheckMenuItem(tMenu, ID__PAUSE, conf->paused ? MF_CHECKED : MF_UNCHECKED);
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
		}
		break;
	} break;
	case WM_MENUCOMMAND: {
		switch (GetMenuItemID((HMENU)lParam, (int)wParam)) {
		case ID__EXIT:
			DestroyWindow(hDlg);
			break;
		case ID__RESTORE:
			RestoreWindow(GetMenuContextHelpId((HMENU)lParam));
			break;
		case ID__PAUSE:
			conf->paused = !conf->paused;
			break;
		case ID__RESETMIN:
			ResetMinMax(GetMenuContextHelpId((HMENU)lParam));
			break;
		}
	} break;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			ShowWindow(hDlg, SW_HIDE);
		}
		break;
	case WM_CLOSE:
		ShowWindow(hDlg, SW_HIDE);
		break;
	case WM_DESTROY:
		KillTimer(hDlg, 0);
		RemoveTrayIcons();
		Shell_NotifyIcon(NIM_DELETE, &conf->niData);
		PostQuitMessage(0);
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_SENSOR_LIST: {
			HWND senList = ((NMHDR*)lParam)->hwndFrom;
			switch (((NMHDR*)lParam)->code) {
			case LVN_BEGINLABELEDIT: {
				runUIUpdate = false;
				Edit_SetText(ListView_GetEditControl(senList), GetSensorName(sen));
			} break;
			case LVN_ITEMACTIVATE: case NM_RETURN:
			{
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
					sen->name = sItem->item.pszText;
					ListView_SetItemText(senList, sItem->item.iItem, 4, GetSensorName(sen));
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
			ResetTraySensors();
			conf->needFullUpdate = true;
			if (conf->updateCheck)
				CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		} break;
		}
		break;
	case WM_ENDSESSION:
		conf->Save();
		return 0;
	case WM_TIMER: {
		bool visible = IsWindowVisible(hDlg) && runUIUpdate;
		if (!conf->paused) {
			senmon->UpdateSensors();
			if (visible) {
				if (conf->needFullUpdate) {
					ReloadSensorView();
				}
			}
			else
				conf->needFullUpdate = true;
			int pos = 0;
			for (auto i = conf->active_sensors.begin(); i != conf->active_sensors.end(); i++) {
				// Update UI...
				if (IsSensorValid(i)) {
					SENSOR* sen = &i->second;
					if (visible) {
						if (sen->changed) {
							SetNumValue(senList, pos, 0, sen->min);
							SetNumValue(senList, pos, 1, sen->cur);
							SetNumValue(senList, pos, 2, sen->max);
						}
						pos++;
					}
					// Update tray icons...
					if (sen->intray && sen->cur != NO_SEN_VALUE) {
						if (!sen->niData) {
							// add tray icon
							sen->niData = new NOTIFYICONDATA({ sizeof(NOTIFYICONDATA), hDlg, (DWORD)(i->first),
								NIF_ICON | NIF_TIP | NIF_MESSAGE, WM_APP + 1 });
							sen->changed = true;
						}
						if (sen->changed) {
							UpdateTrayData(sen);
							if (!Shell_NotifyIcon(NIM_MODIFY, sen->niData))
								Shell_NotifyIcon(NIM_ADD, sen->niData);
						}
					}
					else {
						// remove tray icon
						if (sen->niData) {
							Shell_NotifyIcon(NIM_DELETE, sen->niData);
							DestroyIcon(sen->niData->hIcon);
							delete sen->niData;
							sen->niData = NULL;
							// Unresponsive icon workaround
							Shell_NotifyIcon(NIM_MODIFY, &conf->niData);
						}
					}
				}
			}
			if (visible) {
				// resize
				RECT cArea;
				GetClientRect(senList, &cArea);
				int wd = 0;
				for (int i = 0; i < 4; i++) {
					ListView_SetColumnWidth(senList, i, LVSCW_AUTOSIZE);
					wd += ListView_GetColumnWidth(senList, i);
				}
				ListView_SetColumnWidth(senList, 4, cArea.right - wd);
			}
		}
	} break;
	default: return false;
	}
	return true;
}
