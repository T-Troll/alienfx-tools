// alienfx-mon.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "alienfx-mon.h"
#include <Shlobj.h>
#include <shellapi.h>
#include <windowsx.h>
#include <wininet.h>
#include <string>
#include <Commdlg.h>
#include "ConfigMon.h"
#include "SenMonHelper.h"
using namespace std;

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"Wininet.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND mDlg = 0;
bool needUpdateFeedback = false;
bool isNewVersion = false;
bool runUIUpdate = true;
int selSensor = 0;

UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));

ConfigMon* conf;
SenMonHelper* senmon;

// Forward declarations of functions included in this code module:
HWND                InitInstance(HINSTANCE, int);
BOOL CALLBACK       DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI UpdateMonUI(LPVOID);
HANDLE muiEvent = CreateEvent(NULL, false, false, NULL), uiMonHandle = NULL;

DWORD EvaluteToAdmin() {
    // Evaluation attempt...
    DWORD dwError = ERROR_CANCELLED;
    char szPath[MAX_PATH];
    if (!IsUserAnAdmin()) {
        if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
        {
            // Launch itself as admin
            SHELLEXECUTEINFO sei{ sizeof(sei) };
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
                    //fan_conf->Save();
                    //if (acpi)
                    //    delete acpi;
                    //Shell_NotifyIcon(NIM_DELETE, &conf->niData);
                    EndDialog(mDlg, 1);
                }
                _exit(1);  // Quit itself
            }
        }
        return dwError;
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	conf = new ConfigMon();

	if (conf->eSensors || conf->bSensors)
		EvaluteToAdmin();

	senmon = new SenMonHelper(conf);

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

string GetAppVersion() {

	HRSRC hResInfo = FindResource(hInst, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);

	string res;

	if (hResInfo) {
		DWORD dwSize = SizeofResource(hInst, hResInfo);
		HGLOBAL hResData = LoadResource(hInst, hResInfo);
		if (hResData) {
			LPVOID pRes = LockResource(hResData),
				pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
			if (pResCopy) {
				UINT uLen = 0;
				VS_FIXEDFILEINFO* lpFfi = NULL;

				CopyMemory(pResCopy, pRes, dwSize);

				VerQueryValue(pResCopy, TEXT("\\"), (LPVOID*)&lpFfi, &uLen);

				res = to_string(HIWORD(lpFfi->dwFileVersionMS)) + "."
					+ to_string(LOWORD(lpFfi->dwFileVersionMS)) + "."
					+ to_string(HIWORD(lpFfi->dwFileVersionLS)) + "."
					+ to_string(LOWORD(lpFfi->dwFileVersionLS));

				LocalFree(pResCopy);
			}
			FreeResource(hResData);
		}
	}
	return res;
}

DWORD WINAPI CUpdateCheck(LPVOID lparam) {
	NOTIFYICONDATA* niData = (NOTIFYICONDATA*)lparam;
	HINTERNET session, req;
	char buf[2048];
	DWORD byteRead;
	bool isConnectionFailed = false;
	// Wait connection for a while
	if (!needUpdateFeedback)
		Sleep(10000);
	if (session = InternetOpen("alienfx-tools", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) {
		if (req = InternetOpenUrl(session, "https://api.github.com/repos/t-troll/alienfx-tools/tags?per_page=1",
			NULL, 0, 0, NULL)) {
			if (InternetReadFile(req, buf, 2047, &byteRead)) {
				buf[byteRead] = 0;
				string res = buf;
				size_t pos = res.find("\"name\":"),
					posf = res.find("\"", pos + 8);
				if (pos != string::npos) {
					res = res.substr(pos + 8, posf - pos - 8);
					size_t dotpos = res.find(".", 1 + res.find(".", 1 + res.find(".")));
					if (res.find(".", 1 + res.find(".", 1 + res.find("."))) == string::npos)
						res += ".0";
					if (res != GetAppVersion()) {
						// new version detected!
						niData->uFlags |= NIF_INFO;
						strcpy_s(niData->szInfoTitle, "Update avaliable!");
						strcpy_s(niData->szInfo, ("Latest version is " + res).c_str());
						Shell_NotifyIcon(NIM_MODIFY, niData);
						niData->uFlags &= ~NIF_INFO;
						isNewVersion = true;
					}
				}
			}
			else
				isConnectionFailed = true;
			InternetCloseHandle(req);
		}
		else
			isConnectionFailed = true;
		InternetCloseHandle(session);
	}
	else
		isConnectionFailed = true;
	if (needUpdateFeedback && !isNewVersion) {
		niData->uFlags |= NIF_INFO;
		if (isConnectionFailed) {
			strcpy_s(niData->szInfoTitle, "Update check failed!");
			strcpy_s(niData->szInfo, "Can't connect to GitHub for update check.");
		}
		else {
			strcpy_s(niData->szInfoTitle, "You are up to date!");
			strcpy_s(niData->szInfo, "You are using latest version.");
		}
		Shell_NotifyIcon(NIM_MODIFY, niData);
		niData->uFlags &= ~NIF_INFO;
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {

		Static_SetText(GetDlgItem(hDlg, IDC_STATIC_VERSION), ("Version: " + GetAppVersion()).c_str());

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
				char hurl[MAX_PATH];
				LoadString(hInst, IDS_HOMEPAGE, hurl, MAX_PATH);
				ShellExecute(NULL, "open", hurl, NULL, NULL, SW_SHOWNORMAL);
			} break;
			} break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void RedrawButton(unsigned id, DWORD* clr) {
	RECT rect;
	HBRUSH Brush = NULL;
	HWND tl = GetDlgItem(mDlg, id);
	GetWindowRect(tl, &rect);
	HDC cnt = GetWindowDC(tl);
	rect.bottom -= rect.top;
	rect.right -= rect.left;
	rect.top = rect.left = 0;
	// BGR!
	Brush = CreateSolidBrush(*clr);
	FillRect(cnt, &rect, Brush);
	DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
	DeleteObject(Brush);
	ReleaseDC(tl, cnt);
}

void ReloadSensorView() {
	int pos = 0, rpos = 0;
	HWND list = GetDlgItem(mDlg, IDC_SENSOR_LIST);
	ListView_DeleteAllItems(list);
	ListView_SetExtendedListViewStyle(list, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	if (!ListView_GetColumnWidth(list, 1)) {
		LVCOLUMNA lCol;
		lCol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lCol.cx = 100;
		lCol.iSubItem = 0;
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
		if (!conf->active_sensors[i].disabled) {
			LVITEMA lItem;
			string name = to_string(conf->active_sensors[i].min);
			lItem.mask = LVIF_TEXT | LVIF_PARAM;
			lItem.iItem = pos;
			//lItem.iImage = 0;
			lItem.iSubItem = 0;
			lItem.lParam = i;
			lItem.pszText = (LPSTR)name.c_str();
			if (i == selSensor) {
				lItem.mask |= LVIF_STATE;
				lItem.state = LVIS_SELECTED;
				rpos = ListView_InsertItem(list, &lItem);
				CheckDlgButton(mDlg, IDC_CHECK_INTRAY, conf->active_sensors[i].intray);
				RedrawButton(IDC_BUTTON_COLOR, &conf->active_sensors[i].traycolor);
			} else
				ListView_InsertItem(list, &lItem);
			name = to_string(conf->active_sensors[i].cur);
			ListView_SetItemText(list, pos, 1, (LPSTR)name.c_str());
			name = to_string(conf->active_sensors[i].max);
			ListView_SetItemText(list, pos, 2, (LPSTR)name.c_str());
			ListView_SetItemText(list, pos, 3, (LPSTR)conf->active_sensors[i].name.c_str());
			pos++;
		}
	}
	ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(list, 1, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(list, 2, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(list, 3, LVSCW_AUTOSIZE_USEHEADER);
	ListView_EnsureVisible(list, rpos, false);
	conf->needFullUpdate = false;
}

void RemoveSensors(int src, bool state) {
	//senmon->StopMon();
	for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++)
		if (iter->source == src) {
			//conf->active_sensors.erase(iter);
			//iter = conf->active_sensors.begin();
			iter->disabled = !state;
			//if (iter->intray && iter->niData)
			//	Shell_NotifyIcon(NIM_DELETE, iter->niData);
		}
	conf->needFullUpdate = true;
	//senmon->StartMon();
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
	RedrawButton(id, clr);
	return ret;
}

BOOL CALLBACK DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

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
		CheckDlgButton(hDlg, IDC_STARTW, conf->startWindows);
		CheckDlgButton(hDlg, IDC_STARTM, conf->startMinimized);
		CheckDlgButton(hDlg, IDC_CHECK_UPDATE, conf->updateCheck);
		CheckDlgButton(hDlg, IDC_WSENSORS, conf->wSensors);
		CheckDlgButton(hDlg, IDC_ESENSORS, conf->eSensors);
		CheckDlgButton(hDlg, IDC_BSENSORS, conf->bSensors);

		Edit_SetText(GetDlgItem(hDlg, IDC_REFRESH_TIME), to_string(conf->refreshDelay).c_str());

		ReloadSensorView();

		conf->niData.hWnd = hDlg;
		conf->SetIconState();

		if (Shell_NotifyIcon(NIM_ADD, &conf->niData) && conf->updateCheck) {
			// check update....
			CreateThread(NULL, 0, CUpdateCheck, &conf->niData, 0, NULL);
		}

		// Start UI update thread...
		uiMonHandle = CreateThread(NULL, 0, UpdateMonUI, hDlg, 0, NULL);

	} break;
	case WM_COMMAND:
	{
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
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
			char pathBuffer[2048];
			string shellcomm;
			if (conf->startWindows) {
				GetModuleFileNameA(NULL, pathBuffer, 2047);
				shellcomm = "Register-ScheduledTask -TaskName \"AlienFX-Mon\" -trigger $(New-ScheduledTaskTrigger -Atlogon) -settings $(New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0) -action $(New-ScheduledTaskAction -Execute '"
					+ string(pathBuffer) + "') -force -RunLevel Highest";
				ShellExecute(NULL, "runas", "powershell.exe", shellcomm.c_str(), NULL, SW_HIDE);
			}
			else {
				shellcomm = "/delete /F /TN \"Alienfx-Mon\"";
				ShellExecute(NULL, "runas", "schtasks.exe", shellcomm.c_str(), NULL, SW_HIDE);
			}
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
			if (state)
				EvaluteToAdmin();
			RemoveSensors(1, state);
			break;
		case IDC_BSENSORS:
			conf->bSensors = state;
			if (state)
				EvaluteToAdmin();
			RemoveSensors(2, state);
			senmon->ModifyMon();
			break;
		case IDC_BUTTON_RESET:
			for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++)
				iter->max = iter->min = iter->cur;
			break;
		case IDC_BUTTON_MINIMIZE:
			ShowWindow(hDlg, SW_HIDE);
			break;
		case IDC_BUTTON_COLOR:
			SetColor(IDC_BUTTON_COLOR, &conf->active_sensors[selSensor].traycolor);
			break;
		case IDC_CHECK_INTRAY:
			conf->active_sensors[selSensor].intray = state;
			break;
		}
	} break;
	case WM_APP + 1:
	{
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
			ShowWindow(hDlg, SW_RESTORE);
			SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
			SetWindowPos(hDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
			break;
		case WM_RBUTTONUP: case WM_CONTEXTMENU:
		{
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		} break;
		case NIN_BALLOONUSERCLICK:
		{
			ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools/releases", NULL, NULL, SW_SHOWNORMAL);
		} break;
		}
		break;
	} break;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			// go to tray...
			ShowWindow(hDlg, SW_HIDE);
		}
		break;
	case WM_CLOSE:
		Shell_NotifyIcon(NIM_DELETE, &conf->niData);
		EndDialog(hDlg, IDOK);
		DestroyWindow(hDlg);
		break;
	case WM_DESTROY:
		SetEvent(muiEvent);
		WaitForSingleObject(uiMonHandle, 1000);
		CloseHandle(uiMonHandle);
		// Remove icons from tray...
		for (int i = 0; i < conf->active_sensors.size(); i++)
			if (conf->active_sensors[i].intray && conf->active_sensors[i].niData) {
				Shell_NotifyIcon(NIM_DELETE, conf->active_sensors[i].niData);
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
					CheckDlgButton(mDlg, IDC_CHECK_INTRAY, conf->active_sensors[selSensor].intray);
					RedrawButton(IDC_BUTTON_COLOR, &conf->active_sensors[selSensor].traycolor);
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
				return false;
			} break;
			}
		} break;
		}
		break;
	default: return false;
	}
	return true;
}

void UpdateTrayData(SENSOR* sen) {
	sprintf_s(sen->niData->szTip, 127, "%s\nMin: %d\nCur: %d\nMax: %d", sen->name.c_str(), sen->min, sen->cur, sen->max);

	string val = to_string(sen->cur > 100 ? sen->cur / 100 : sen->cur == 100 ? 0 : sen->cur);

	HDC hdc, hdcMem;
	HBITMAP hBitmap = NULL;
	//HBITMAP hOldBitMap = NULL;
	//HBITMAP hBitmapMask = NULL;
	ICONINFO iconInfo;
	HFONT hFont;
	HICON hIcon;

	hdc = GetDC(mDlg);
	hdcMem = CreateCompatibleDC(hdc);
	hBitmap = CreateCompatibleBitmap(hdc, 32, 32);
	//hBitmapMask = CreateCompatibleBitmap(hdc, 32, 32);
	ReleaseDC(mDlg, hdc);
	/*hOldBitMap = (HBITMAP)*/SelectObject(hdcMem, hBitmap);
	SetTextColor(hdcMem, 0x000000); // 0x00bbggrr
	SetBkColor(hdcMem, sen->traycolor); // color here
	SetBkMode(hdcMem, OPAQUE);// TRANSPARENT);
	//PatBlt(hdcMem, 0, 0, 32, 32, WHITENESS);

	// Draw percentage
	hFont = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Microsoft Sans Serif"));
	hFont = (HFONT)SelectObject(hdcMem, hFont);
	TextOut(hdcMem, 1, 1, val.c_str(), (int)val.length());

	//SelectObject(hdc, hOldBitMap);
	//hOldBitMap = NULL;

	iconInfo.fIcon = TRUE;
	//iconInfo.xHotspot = 0;
	//iconInfo.yHotspot = 0;
	iconInfo.hbmMask = hBitmap;// hBitmapMask;
	iconInfo.hbmColor = hBitmap;

	hIcon = CreateIconIndirect(&iconInfo);

	sen->niData->hIcon = hIcon;

	DeleteObject(SelectObject(hdcMem, hFont));
	DeleteDC(hdcMem);
	DeleteDC(hdc);
	DeleteObject(hBitmap);
	//DeleteObject(hBitmapMask);
}

DWORD WINAPI UpdateMonUI(LPVOID lpParam) {
	HWND list = GetDlgItem(mDlg, IDC_SENSOR_LIST);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	while (WaitForSingleObject(muiEvent, conf->refreshDelay) == WAIT_TIMEOUT) {
		if (IsWindowVisible(mDlg) && runUIUpdate) {
			//DebugPrint("Fans UI update...\n");
			if (conf->needFullUpdate)
				ReloadSensorView();
			else {
				int pos = 0;
				for (auto iter = conf->active_sensors.begin(); iter != conf->active_sensors.end(); iter++) {
					if (!iter->disabled) {
						string name = to_string(iter->min);
						ListView_SetItemText(list, pos, 0, (LPSTR)name.c_str());
						name = to_string(iter->cur);
						ListView_SetItemText(list, pos, 1, (LPSTR)name.c_str());
						name = to_string(iter->max);
						ListView_SetItemText(list, pos, 2, (LPSTR)name.c_str());
						//ListView_SetItemText(list, i, 3, (LPSTR)iter->name.c_str());
						pos++;
					}
				}
			}
		}
		// Trayworks...
		for (int i = 0; i < conf->active_sensors.size(); i++) {
			if (conf->active_sensors[i].intray && !conf->active_sensors[i].disabled) {
				if (conf->active_sensors[i].niData) {
					// update tray counter
					UpdateTrayData(&conf->active_sensors[i]);
					Shell_NotifyIcon(NIM_MODIFY, conf->active_sensors[i].niData);
				}
				else {
					// add tray counter
					conf->active_sensors[i].niData = new NOTIFYICONDATA({ sizeof(NOTIFYICONDATA), mDlg, (unsigned)i+1,
						NIF_ICON | NIF_TIP, WM_APP + 1});
					UpdateTrayData(&conf->active_sensors[i]);
					Shell_NotifyIcon(NIM_ADD, conf->active_sensors[i].niData);
				}
			}
			else {
				// check remove from tray
				if (conf->active_sensors[i].niData) {
					Shell_NotifyIcon(NIM_DELETE, conf->active_sensors[i].niData);
					delete conf->active_sensors[i].niData;
					conf->active_sensors[i].niData = NULL;
				}
			}
		}
	}
	return 0;
}