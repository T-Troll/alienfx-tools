// alienfx-gui.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "alienfx-gui.h"
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <Commdlg.h>
#include <shellapi.h>
#include <Shlobj.h>
#include <psapi.h>
#include <ColorDlg.h>
#include <algorithm>
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "..\AlienFX-SDK\AlienFX_SDK\AlienFX_SDK.h"
#include "EventHandler.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")
#pragma comment(lib,"comctl32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

NOTIFYICONDATA niData;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

FXHelper* fxhl;
ConfigHandler* conf;
EventHandler* eve;

HWND mDlg;

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
                conf->Save();
                _exit(1);  // Quit itself
            }
        }
        return dwError;
    }
    return 0;
}

bool DoStopService(bool kind) {
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwStartTime = GetTickCount64();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 10000; // 10-second time-out
    //DWORD dwWaitTime;

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

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALIENFXGUI, szWindowClass, MAX_LOADSTRING);
    //MyRegisterClass(hInstance);

    conf = new ConfigHandler();
    conf->Load();
    fxhl = new FXHelper(conf);

    if (fxhl->GetDevList().size() > 0) {

        conf->wasAWCC = DoStopService(true);

        fxhl->Refresh(true);

        if (conf->esif_temp)
            EvaluteToAdmin();

        eve = new EventHandler(conf, fxhl);

        eve->ChangePowerState();

        // Perform application initialization:
        if (!(mDlg = InitInstance(hInstance, nCmdShow)))
        {
            return FALSE;
        }

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
            if (!TranslateAccelerator(
                mDlg,      // handle to receiving window 
                hAccelTable,        // handle to active accelerator table 
                &msg))         // message data 
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        delete eve;

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
    dlg = CreateDialogParam(hInstance,//GetModuleHandle(NULL),         /// instance handle
        MAKEINTRESOURCE(IDD_MAINWINDOW),    /// dialog box template
        NULL,                    /// handle to parent
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

#define C_PAGES 5

typedef struct tag_dlghdr {
    HWND hwndTab;       // tab control 
    HWND hwndDisplay;   // current child dialog box 
    RECT rcDisplay;     // display rectangle for the tab control 
    DLGTEMPLATE* apRes[C_PAGES];
} DLGHDR;

DLGTEMPLATE* DoLockDlgRes(LPCTSTR lpszResName)
{
    HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);

    // Note that g_hInst is the global instance handle
    HGLOBAL hglb = LoadResource(hInst, hrsrc);
    return (DLGTEMPLATE*)LockResource(hglb);
}

int tabSel = (-1);
bool wasNullTab = false;

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
    //ShowWindow(pHdr->hwndDisplay, SW_SHOW);
    //SetActiveWindow(pHdr->hwndDisplay);
    return;
}

int pRid = -1, pRitem = -1;
bool isProfileEdited = false;

void ReloadProfileList(HWND hDlg) {
    if (hDlg == NULL) hDlg = mDlg;
    HWND tab_list = GetDlgItem(hDlg, IDC_TAB_MAIN),
        profile_list = GetDlgItem(hDlg, IDC_PROFILES);
    SendMessage(profile_list, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < conf->profiles.size(); i++) {
        int pos = (int)SendMessage(profile_list, CB_ADDSTRING, 0, (LPARAM)(conf->profiles[i].name.c_str()));
        SendMessage(profile_list, CB_SETITEMDATA, pos, conf->profiles[i].id);
        if (conf->profiles[i].id == conf->activeProfile) {
            SendMessage(profile_list, CB_SETCURSEL, pos, 0);
            pRid = conf->activeProfile; pRitem = pos;
        }
    }

    EnableWindow(profile_list, !conf->enableProf);

    //OnSelChanged(tab_list);
}

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    HWND tab_list = GetDlgItem(hDlg, IDC_TAB_MAIN),
        profile_list = GetDlgItem(hDlg, IDC_PROFILES);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        RECT rcTab;
        DLGHDR* pHdr = (DLGHDR*) LocalAlloc(LPTR, sizeof(DLGHDR));
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
        pHdr->rcDisplay.right += 1; GetSystemMetrics(SM_CXDLGFRAME);// +1;
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
            //eve->StopEvents();
            Shell_NotifyIcon(NIM_DELETE, &niData);
            EndDialog(hDlg, IDOK);
            DestroyWindow(hDlg);
        } break;
        case IDM_ABOUT: // about dialogue here
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
            fxhl->afx_dev->SaveMappings();
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
                pRitem = pbItem; pRid = prid;
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
            SetWindowPos(hDlg,       // handle to window
                HWND_TOPMOST,  // placement-order handle
                0,     // horizontal position
                0,      // vertical position
                0,  // width
                0, // height
                SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE// window-positioning options
            );
            SetWindowPos(hDlg,       // handle to window
                HWND_NOTOPMOST,  // placement-order handle
                0,     // horizontal position
                0,      // vertical position
                0,  // width
                0, // height
                SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE// window-positioning options
            );
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
            //SetMenuInfo(pMenu, &mi);
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
            //eve->StopEvents();
            Shell_NotifyIcon(NIM_DELETE, &niData);
            EndDialog(hDlg, IDOK);
            DestroyWindow(hDlg);
        } break;
        case ID_TRAYMENU_REFRESH:
            fxhl->RefreshState(true);
            break;
        case ID_TRAYMENU_LIGHTSON:
            conf->stateOn = conf->lightsOn = !conf->lightsOn;
            //fxhl->Refresh(true);
            eve->ToggleEvents();
            break;
        case ID_TRAYMENU_DIMLIGHTS:
            conf->dimmed = !conf->dimmed;
            fxhl->RefreshState();
            break;
        case ID_TRAYMENU_MONITORING:
            conf->enableMon = !conf->enableMon;
            conf->monState = eve->FindProfile(conf->activeProfile)->flags & 0x2 ? 0 : conf->enableMon;
            eve->ToggleEvents();
            break;
        case ID_TRAYMENU_PROFILESWITCH:
            eve->StopProfiles();
            conf->enableProf = !conf->enableProf;
            ReloadProfileList(hDlg);
            eve->StartProfiles();
            break;
        case ID_TRAYMENU_RESTORE:
            ShowWindow(hDlg, SW_RESTORE);
            SetWindowPos(hDlg,       // handle to window
                HWND_TOPMOST,  // placement-order handle
                0,     // horizontal position
                0,      // vertical position
                0,  // width
                0, // height
                SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE// window-positioning options
            );
            SetWindowPos(hDlg,       // handle to window
                HWND_NOTOPMOST,  // placement-order handle
                0,     // horizontal position
                0,      // vertical position
                0,  // width
                0, // height
                SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE// window-positioning options
            );
            //Shell_NotifyIcon(NIM_DELETE, &niData);
            break;
        case ID_TRAYMENU_PROFILE_SELECTED: {
            if (idx < conf->profiles.size() && conf->profiles[idx].id != conf->activeProfile) {
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
            //fxhl->afx_dev->AlienFXEnumDevices(fxhl->afx_dev->vid);
#ifdef _DEBUG
            OutputDebugString("Resume from Sleep/hibernate initiated\n");
#endif
            int cPid = fxhl->afx_dev->GetPID();
            if (cPid != -1) {
                fxhl->afx_dev->AlienFXChangeDevice(cPid);
                eve->ChangePowerState();
                eve->StartEvents();
                eve->StartProfiles();
            }
        } break;
        case PBT_APMPOWERSTATUSCHANGE:
            // ac/batt change
            eve->ChangePowerState();
            break;
        case PBT_POWERSETTINGCHANGE: {
            POWERBROADCAST_SETTING* sParams = (POWERBROADCAST_SETTING*)lParam;
#ifdef _DEBUG
            OutputDebugString("Power state changed\n");
#endif
            if (sParams->PowerSetting == GUID_MONITOR_POWER_ON || sParams->PowerSetting == GUID_CONSOLE_DISPLAY_STATE) {
                eve->ChangeScreenState(sParams->Data[0]);
#ifdef _DEBUG
                OutputDebugString("Monitor state changed\n");
#endif
            }
        } break;
        case PBT_APMSUSPEND:
#ifdef _DEBUG
            OutputDebugString("Sleep/hibernate initiated\n");
#endif
            eve->StopProfiles();
            eve->StopEvents();
            return true;
            break;
        }
        break;
    case WM_QUERYENDSESSION: //case WM_ENDSESSION:
        // Shutdown/restart scheduled....
#ifdef _DEBUG
        OutputDebugString("Shutdown initiated\n");
#endif
        eve->StopProfiles();
        eve->StopEvents();
        //fxhl->Refresh(true);
        EndDialog(hDlg, IDOK);
        DestroyWindow(hDlg);
        return true;
        break;
    case WM_HOTKEY:
        switch (wParam) {
        case 1: // on/off
            conf->stateOn = conf->lightsOn = !conf->lightsOn;
            fxhl->RefreshState(true);
            eve->ToggleEvents();
            break;
        case 2: // dim
            conf->dimmed = !conf->dimmed;
            //fxhl->Refresh(true);
            fxhl->RefreshState();
            break;
        case 3: // off-dim-full circle
            if (conf->lightsOn) {
                if (conf->dimmed) {
                    conf->stateOn = conf->lightsOn = !conf->lightsOn;
                    conf->dimmed = !conf->dimmed;
                    eve->StopEvents();
                }
                else {
                    conf->dimmed = !conf->dimmed;
                    //fxhl->Refresh(true);
                    fxhl->RefreshState();
                }
            }
            else {
                conf->stateOn = conf->lightsOn = !conf->lightsOn;
                //fxhl->Refresh(true);
                eve->StartEvents();
            }
            break;
        default: return false;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0); break;
    default: return false;
    }
    return true;
}

void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b) {
    RECT rect;
    HBRUSH Brush = NULL;
    HWND tl = GetDlgItem(hDlg, id);
    GetWindowRect(tl, &rect);
    HDC cnt = GetWindowDC(tl);
    rect.bottom -= rect.top;
    rect.right -= rect.left;
    rect.top = rect.left = 0;
    // BGR!
    Brush = CreateSolidBrush(RGB(r, g, b));
    FillRect(cnt, &rect, Brush);
    DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
    DeleteObject(Brush);
    //RedrawWindow(tl, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
}

AlienFX_SDK::afx_act* mod;
ULONGLONG lastColorCall = 0;
bool runLightsRefresh = false;

DWORD CColorRefreshProc(LPVOID param) {
    AlienFX_SDK::afx_act last;
    ULONGLONG cColorCall = 0;
    lastColorCall = GetTickCount64();
    last.r = mod->r;
    last.g = mod->g;
    last.b = mod->b;
    while (runLightsRefresh) {
        if (last.r != mod->r || last.g != mod->g || last.b != mod->b) {
            cColorCall = GetTickCount64();
            if (cColorCall - lastColorCall > 350) {
                // set colors...
                last.r = mod->r;
                last.g = mod->g;
                last.b = mod->b;
                fxhl->RefreshState();
                lastColorCall = GetTickCount64();
            }
        }
        else
            Sleep(100);
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
    //HWND r = GetDlgItem(hDlg, 706);
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

bool SetColor(HWND hDlg, int id, AlienFX_SDK::afx_act* map) {
    CHOOSECOLOR cc;                 // common dialog box structure 
    bool ret;

    AlienFX_SDK::afx_act savedColor = *map;
    DWORD crThreadID;
    HANDLE crRefresh;

    // Initialize CHOOSECOLOR 
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hDlg;
    cc.lpfnHook = Lpcchookproc;
    cc.lCustData = (LPARAM) map;
    cc.lpCustColors = (LPDWORD)conf->customColors;
    cc.rgbResult = RGB(map->r, map->g, map->b);
    cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR | CC_ENABLEHOOK;

    mod = map;
    runLightsRefresh = true;
    crRefresh = CreateThread(
        NULL,              // default security
        0,                 // default stack size
        CColorRefreshProc,        // name of the thread function
        hDlg,
        0,                 // default startup flags
        &crThreadID);

    if (!(ret = ChooseColor(&cc)))
    {
        map->r = savedColor.r;
        map->g = savedColor.g;
        map->b = savedColor.b;
        RedrawButton(hDlg, id, map->r, map->g, map->b);
    }
    runLightsRefresh = false;
    return ret;
}

lightset* FindMapping(int did, int lid)
{
    lightset *map = NULL;
    //mapping* mmap = NULL;
    for (int i = 0; i < conf->active_set.size(); i++)
        if (conf->active_set[i].devid == did && conf->active_set[i].lightid == lid) {
            map = &conf->active_set[i];
            break;
        }
    return map;
}

int effID = -1, eItem = -1;

void RebuildEffectList(HWND eff_list, lightset* mmap) {
    // Populate effects list...
    ListView_DeleteAllItems(eff_list);
    HIMAGELIST hSmall;
    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        ILC_COLOR32/*ILC_MASK*/, 1, 1);
    for (int i = 0; i < mmap->eve[0].map.size(); i++) {
        UINT* picData = (UINT*)malloc(GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON) * sizeof(UINT));
        for (int j = 0; j < GetSystemMetrics(SM_CXSMICON) * GetSystemMetrics(SM_CYSMICON); j++)
            picData[j] = RGB(mmap->eve[0].map[i].b, mmap->eve[0].map[i].g, mmap->eve[0].map[i].r);
        HBITMAP colorBox = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
            1, 32, picData);
        free(picData);
        ImageList_Add(hSmall, colorBox, NULL);
    }
    ListView_SetImageList(eff_list, hSmall, LVSIL_SMALL);
    for (int i = 0; i < mmap->eve[0].map.size(); i++) {
        LVITEMA lItem; char efName[100];
        lItem.mask = LVIF_TEXT | LVIF_IMAGE;
        lItem.iItem = i;
        lItem.iImage = i;
        lItem.iSubItem = 0;
        switch (mmap->eve[0].map[i].type) {
        case AlienFX_SDK::AlienFX_A_Color:
            LoadString(hInst, IDS_TYPE_COLOR, efName, 100);
            break;
        case AlienFX_SDK::AlienFX_A_Pulse:
            LoadString(hInst, IDS_TYPE_PULSE, efName, 100);
            break;
        case AlienFX_SDK::AlienFX_A_Morph:
            LoadString(hInst, IDS_TYPE_MORPH, efName, 100);
            break;
        case AlienFX_SDK::AlienFX_A_Spectrum:
            LoadString(hInst, IDS_TYPE_SPECTRUM, efName, 100);
            break;
        case AlienFX_SDK::AlienFX_A_Breathing:
            LoadString(hInst, IDS_TYPE_BREATH, efName, 100);
            break;
        case AlienFX_SDK::AlienFX_A_Rainbow:
            LoadString(hInst, IDS_TYPE_RAINBOW, efName, 100);
            break;
        }
        lItem.pszText = efName;
        ListView_InsertItem(eff_list, &lItem);
    }
    ListView_SetItemState(eff_list, effID, LVIS_SELECTED, LVIS_SELECTED);
}

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    int pid = fxhl->afx_dev->GetPID();
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
        eff_list = GetDlgItem(hDlg, IDC_EFFECTS_LIST),
        s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
        l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
        type_c1 = GetDlgItem(hDlg, IDC_TYPE1);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        //int pid = fxhl->afx_dev->GetPID();
        size_t lights = fxhl->afx_dev->GetMappings()->size();
        bool noLights = true;
        for (int i = 0; i < lights; i++) {
            AlienFX_SDK::mapping lgh = fxhl->afx_dev->GetMappings()->at(i);
            if (lgh.devid == pid) {
                int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
                SendMessage(light_list, LB_SETITEMDATA, pos, lgh.lightid);
                noLights = false;
            }
        }
        if (noLights) {// no lights, switch to setup
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
        // Restore selection....
        if (eItem != (-1)) {
            SendMessage(light_list, LB_SETCURSEL, eItem, 0);
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
            lightset* mmap = FindMapping(pid, (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0));
            if (mmap != NULL)
                RebuildEffectList(eff_list, mmap);
        }
    } break;
    case WM_COMMAND: {
        int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0),
          lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0),
          ceItem = (int)SendMessage(eff_list, LB_GETCURSEL, 0, 0),
          ceid = (int)SendMessage(eff_list, LB_GETITEMDATA, ceItem, 0),
          lType1 = (int)SendMessage(type_c1, CB_GETCURSEL, 0, 0);
        lightset* mmap = FindMapping(pid, lid);
        switch (LOWORD(wParam))
        {
        case IDC_LIGHTS: // should reload mappings
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                eItem = lbItem;
                if (mmap == NULL) {
                    lightset newmap;
                    AlienFX_SDK::afx_act act;
                    newmap.devid = pid;
                    newmap.lightid = lid;
                    newmap.eve[0].fs.b.flags = 1;
                    newmap.eve[0].map.push_back(act);
                    if (fxhl->afx_dev->GetFlags(pid, lid))
                        newmap.eve[0].map.push_back(act);
                    newmap.eve[1].map.push_back(act);
                    newmap.eve[1].map.push_back(act);
                    newmap.eve[2].map.push_back(act);
                    newmap.eve[2].map.push_back(act);
                    newmap.eve[3].map.push_back(act);
                    newmap.eve[3].map.push_back(act);
                    newmap.eve[3].fs.b.cut = 90;
                    conf->active_set.push_back(newmap);
                    std::sort(conf->active_set.begin(), conf->active_set.end(), ConfigHandler::sortMappings);
                    mmap = FindMapping(pid, lid);
                }
                // Populate effects list...
                effID = 0;
                RebuildEffectList(eff_list, mmap);
                // Enable or disable controls
                bool flag = !fxhl->afx_dev->GetFlags(pid, lid);
                if (!flag && mmap->eve[0].map.size() < 2) {
                    AlienFX_SDK::afx_act act;
                    mmap->eve[0].map.push_back(act);
                }
                EnableWindow(type_c1, flag);
                EnableWindow(s1_slider, flag);
                EnableWindow(l1_slider, flag);
                // Set data
                SendMessage(type_c1, CB_SETCURSEL, mmap->eve[0].map[0].type, 0);
                RedrawButton(hDlg, IDC_BUTTON_C1, mmap->eve[0].map[0].r, mmap->eve[0].map[0].g, mmap->eve[0].map[0].b);
                SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[0].tempo);
                SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[0].time);
                break;
            } break;
        case IDC_TYPE1:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                if (mmap != NULL) {
                    mmap->eve[0].map[effID].type = lType1;
                    RebuildEffectList(eff_list, mmap);
                    fxhl->RefreshState(true);
                }
            }
            break;
        case IDC_BUTTON_C1:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (mmap != NULL) {
                    SetColor(hDlg, IDC_BUTTON_C1, &mmap->eve[0].map[effID]);
                    RebuildEffectList(eff_list, mmap);
                    fxhl->RefreshState();
                }
            } break;
            } break;
        case IDC_BUT_ADD_EFFECT:
            if (HIWORD(wParam) == BN_CLICKED && mmap != NULL && !fxhl->afx_dev->GetFlags(pid, lid) && mmap->eve[0].map.size() < 9) {
                AlienFX_SDK::afx_act act = mmap->eve[0].map.back();
                mmap->eve[0].map.push_back(act);
                RebuildEffectList(eff_list, mmap);
                fxhl->RefreshState(true);
            }
            break;
        case IDC_BUTT_REMOVE_EFFECT:
            if (HIWORD(wParam) == BN_CLICKED && mmap != NULL && !fxhl->afx_dev->GetFlags(pid, lid) && mmap->eve[0].map.size() > 1) {
                mmap->eve[0].map.pop_back();
                if (effID == mmap->eve[0].map.size()) {
                    effID--;
                    bool flag = !fxhl->afx_dev->GetFlags(pid, lid);
                    EnableWindow(type_c1, flag);
                    EnableWindow(s1_slider, flag);
                    EnableWindow(l1_slider, flag);
                    // Set data
                    SendMessage(type_c1, CB_SETCURSEL, mmap->eve[0].map[effID].type, 0);
                    RedrawButton(hDlg, IDC_BUTTON_C1, mmap->eve[0].map[effID].r, mmap->eve[0].map[effID].g, mmap->eve[0].map[effID].b);
                    SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].tempo);
                    SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].time);
                }
                RebuildEffectList(eff_list, mmap);
                fxhl->RefreshState(true);
            }
            break;
        case IDC_BUTTON_SETALL:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (mmap != NULL &&
                    MessageBox(hDlg, "Do you really want to set all lights for current device to this settings?", "Warning!",
                        MB_YESNO | MB_ICONWARNING) == IDYES) {
                    for (int i = 0; i < conf->active_set.size(); i++)
                        if (conf->active_set[i].devid == pid && !fxhl->afx_dev->GetFlags(pid, conf->active_set[i].lightid)) {
                            conf->active_set[i].eve[0] = mmap->eve[0];
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
            int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
            int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
            lightset* mmap = FindMapping(pid, lid);
            if (mmap != NULL) {
                if ((HWND)lParam == s1_slider) {
                    mmap->eve[0].map[effID].tempo = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == l1_slider) {
                    mmap->eve[0].map[effID].time = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                fxhl->Refresh();
            }
        break;
    } break;
    case WM_DRAWITEM:
        switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
        case IDC_BUTTON_C1: //case IDC_BUTTON_C2:
            AlienFX_SDK::afx_act c;
            if (eItem != -1) {
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0);
                lightset* map = FindMapping(pid, lid);
                if (map != NULL) {
                    switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
                    case IDC_BUTTON_C1: c = map->eve[0].map[effID]; break;
                    }
                }
            }
            RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.r, c.g, c.b);
            break;
        }
        break;
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_EFFECTS_LIST:
            //int code = ((NMHDR*)lParam)->code;
            if (((NMHDR*)lParam)->code == NM_CLICK) {
                NMITEMACTIVATE* sItem = (NMITEMACTIVATE*)lParam;
                // Select other color....
                effID = sItem->iItem;
                int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
                lightset* mmap = FindMapping(pid, lid);
                if (mmap != NULL) {
                    bool flag = !fxhl->afx_dev->GetFlags(pid, lid);
                    EnableWindow(type_c1, flag);
                    EnableWindow(s1_slider, flag);
                    EnableWindow(l1_slider, flag);
                    // Set data
                    SendMessage(type_c1, CB_SETCURSEL, mmap->eve[0].map[effID].type, 0);
                    RedrawButton(hDlg, IDC_BUTTON_C1, mmap->eve[0].map[effID].r, mmap->eve[0].map[effID].g, mmap->eve[0].map[effID].b);
                    SendMessage(s1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].tempo);
                    SendMessage(l1_slider, TBM_SETPOS, true, mmap->eve[0].map[effID].time);
                }
            }
            break;
        }
        break;
    default: return false;
    }
    return true;
}

BOOL CALLBACK TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int pid = fxhl->afx_dev->GetPID();
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS_E),
        mode_light = GetDlgItem(hDlg, IDC_CHECK_NOEVENT),
        mode_power = GetDlgItem(hDlg, IDC_CHECK_POWER),
        mode_perf = GetDlgItem(hDlg, IDC_CHECK_PERF),
        mode_status = GetDlgItem(hDlg, IDC_CHECK_STATUS),
        list_counter = GetDlgItem(hDlg, IDC_COUNTERLIST),
        list_status = GetDlgItem(hDlg, IDC_STATUSLIST),
        s1_slider = GetDlgItem(hDlg, IDC_MINPVALUE),
        s2_slider = GetDlgItem(hDlg, IDC_CUTLEVEL);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        size_t lights = fxhl->afx_dev->GetMappings()->size();
        bool noLights = true;
        for (int i = 0; i < lights; i++) {
            AlienFX_SDK::mapping lgh = fxhl->afx_dev->GetMappings()->at(i);
            if (lgh.devid == pid) {
                int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
                SendMessage(light_list, LB_SETITEMDATA, pos, lgh.lightid);
                noLights = false;
            }
        }
        if (noLights) {// no lights, switch to setup
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
        // Set sliders
        SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 99));
        SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 99));
        //TBM_SETTICFREQ
        SendMessage(s1_slider, TBM_SETTICFREQ, 10, 0);
        SendMessage(s2_slider, TBM_SETTICFREQ, 10, 0);

        if (eItem != (-1)) {
            SendMessage(light_list, LB_SETCURSEL, eItem, 0);
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS_E, LBN_SELCHANGE), (LPARAM)hDlg);
        }
    } break;
    case WM_COMMAND: {
        int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0),
            lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0),
            countid = (int)SendMessage(list_counter, CB_GETCURSEL, 0, 0),
            statusid = (int)SendMessage(list_status, CB_GETCURSEL, 0, 0);
        lightset* map = FindMapping(pid, lid);
        switch (LOWORD(wParam))
        {
        case IDC_LIGHTS_E: // should reload mappings
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                eItem = lbItem;
                if (map == NULL) {
                    lightset newmap;
                    AlienFX_SDK::afx_act act;
                    newmap.devid = pid;
                    newmap.lightid = lid;
                    newmap.eve[0].fs.b.flags = 0;
                    newmap.eve[0].map.push_back(act);
                    newmap.eve[1].map.push_back(act);
                    newmap.eve[1].map.push_back(act);
                    newmap.eve[2].map.push_back(act);
                    newmap.eve[2].map.push_back(act);
                    newmap.eve[3].map.push_back(act);
                    newmap.eve[3].map.push_back(act);
                    newmap.eve[3].fs.b.cut = 90;
                    conf->active_set.push_back(newmap);
                    std::sort(conf->active_set.begin(), conf->active_set.end(), ConfigHandler::sortMappings);
                    map = FindMapping(pid, lid);
                }

                for (int i = 0; i < 4; i++) {
                    CheckDlgButton(hDlg, IDC_CHECK_NOEVENT + i, map->eve[i].fs.b.flags ? BST_CHECKED : BST_UNCHECKED);

                    switch (i) {
                    case 3: { // checkbox + slider
                        CheckDlgButton(hDlg, IDC_STATUS_BLINK, map->eve[i].fs.b.proc ? BST_CHECKED : BST_UNCHECKED);
                        SendMessage(s2_slider, TBM_SETPOS, true, map->eve[i].fs.b.cut);
                    } break;
                    case 2: { //slider
                        SendMessage(s1_slider, TBM_SETPOS, true, map->eve[i].fs.b.cut);
                    } break;
                    }
                    if (i > 0) {
                        RedrawButton(hDlg, IDC_BUTTON_CM1 + i - 1, map->eve[i].map[0].r,
                            map->eve[i].map[0].g, map->eve[i].map[0].b);
                        RedrawButton(hDlg, IDC_BUTTON_CM4 + i - 1, map->eve[i].map[1].r,
                            map->eve[i].map[1].g, map->eve[i].map[1].b);
                        }
                }
                SendMessage(list_counter, CB_SETCURSEL, map->eve[2].source, 0);
                SendMessage(list_status, CB_SETCURSEL, map->eve[3].source, 0);
                RedrawWindow(list_counter, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                RedrawWindow(list_status, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                break;
            } break;
        case IDC_CHECK_NOEVENT: case IDC_CHECK_PERF: case IDC_CHECK_POWER: case IDC_CHECK_STATUS: {
            int eid = LOWORD(wParam) - IDC_CHECK_NOEVENT;
            if (map != NULL) {
                map->eve[eid].fs.b.flags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
                fxhl->RefreshMon();
            }
        } break;
        case IDC_STATUS_BLINK:
            if (map != NULL)
                map->eve[3].fs.b.proc = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            break;
        case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: {
            int eid = LOWORD(wParam) - IDC_BUTTON_CM1 + 1;
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                if (map != NULL) {
                    SetColor(hDlg, LOWORD(wParam), &map->eve[eid].map[0]);
                }
                break;
            }
        } break;
        case IDC_BUTTON_CM4: case IDC_BUTTON_CM5: case IDC_BUTTON_CM6: {
            int eid = LOWORD(wParam) - IDC_BUTTON_CM4 + 1;
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                if (map != NULL) {
                    SetColor(hDlg, LOWORD(wParam), &map->eve[eid].map[1]);
                }
                break;
            }
        } break;
        case IDC_COUNTERLIST:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
                if (map != NULL) {
                    map->eve[2].source = countid;
                    fxhl->RefreshMon();
                }
                break;
            }
            break;
        case IDC_STATUSLIST:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
                if (map != NULL) {
                    map->eve[3].source = statusid;
                    fxhl->RefreshMon();
                }
                break;
            }
            break;
        }
        //if (map != NULL)
        //   fxhl->Refresh();
    } break;
    case WM_DRAWITEM:
        switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
        case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: {
            int cid = ((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM1 + 1;
            AlienFX_SDK::afx_act c;
            if (eItem != -1) {
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0);
                lightset* map = FindMapping(pid, lid);
                if (map != NULL)
                    c = map->eve[cid].map[0];
            }
            RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.r, c.g, c.b);
        } break;
        case IDC_BUTTON_CM4: case IDC_BUTTON_CM5: case IDC_BUTTON_CM6: {
            int cid = ((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM4 + 1;
            AlienFX_SDK::afx_act c;
            if (eItem != -1) {
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0);
                lightset* map = FindMapping(pid, lid);
                if (map != NULL)
                    c = map->eve[cid].map[1];
            }
            RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.r, c.g, c.b);
        } break;
        }
        break;
    default: return false;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBTRACK: case TB_ENDTRACK:
            int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
            int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
            lightset* map = FindMapping(pid, lid);
            if (map != NULL) {
                if ((HWND)lParam == s1_slider) {
                    map->eve[2].fs.b.cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == s2_slider) {
                    map->eve[3].fs.b.cut = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                //fxhl->RefreshMon();
            }
            break;
        } break;
    }
    return true;
}

int UpdateLightList(HWND light_list, int pid) {

    int pos = -1;
    size_t lights = fxhl->afx_dev->GetMappings()->size();
    SendMessage(light_list, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < lights; i++) {
        AlienFX_SDK::mapping lgh = fxhl->afx_dev->GetMappings()->at(i);
        if (lgh.devid == pid) {
            pos = (int)SendMessage(light_list, CB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
            SendMessage(light_list, CB_SETITEMDATA, pos, lgh.lightid);
        }
    }
    RedrawWindow(light_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
    return pos;
}

int eLid = (-1), eDid = (-1), lItem = -1, dItem = (-1);

BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    lightset* map = NULL;
    unsigned i;
    int pid = fxhl->afx_dev->GetPID();
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS_S),
        dev_list = GetDlgItem(hDlg, IDC_DEVICES),
        light_id = GetDlgItem(hDlg, IDC_LIGHTID);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        size_t lights = fxhl->afx_dev->GetMappings()->size();
        size_t numdev = fxhl->GetDevList().size();
        int cpid = (-1), pos = (-1);
        for (i = 0; i < numdev; i++) {
            cpid = fxhl->GetDevList().at(i);
            int j;
            for (j = 0; j < fxhl->afx_dev->GetDevices()->size(); j++) {
                if (cpid == fxhl->afx_dev->GetDevices()->at(j).devid) {
                    std::string dname = fxhl->afx_dev->GetDevices()->at(i).name;
                    pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(dname.c_str()));
                    SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)cpid);
                    break;
                }
            }
            if (j == fxhl->afx_dev->GetDevices()->size()) {
                // no name
                char devName[256];
                sprintf_s(devName, 255, "Device #%X", cpid);
                pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(devName));
                SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)cpid);
            }
            if (cpid == pid) {
                // select this device.
                SendMessage(dev_list, CB_SETCURSEL, pos, (LPARAM)0);
                eDid = pid; dItem = pos;
            }
        }

        UpdateLightList(light_list, pid);
        BYTE status = fxhl->afx_dev->AlienfxGetDeviceStatus();
        if (status && status != 0xff)
            SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Ok");
        else
            SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Error");
        eLid = -1; lItem = -1;

    } break;
    case WM_COMMAND: {
        int lbItem = (int)SendMessage(light_list, CB_GETCURSEL, 0, 0);
        int lid = (int)SendMessage(light_list, CB_GETITEMDATA, lbItem, 0);
        int dbItem = (int)SendMessage(dev_list, CB_GETCURSEL, 0, 0);
        int did = (int)SendMessage(dev_list, CB_GETITEMDATA, dbItem, 0);
        switch (LOWORD(wParam))
        {
        case IDC_DEVICES: {
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE: {
                fxhl->afx_dev->AlienFXChangeDevice(did);
                conf->lastActive = did;
                UpdateLightList(light_list, did);
                if (fxhl->afx_dev->AlienfxGetDeviceStatus())
                    SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Ok");
                else
                    SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Error");
                eLid = -1; lItem = -1; eDid = did; dItem = dbItem;
            } break;
            case CBN_EDITCHANGE:
                char buffer[256];
                GetWindowTextA(dev_list, buffer, 256);
                for (i = 0; i < fxhl->afx_dev->GetDevices()->size(); i++) {
                    if (fxhl->afx_dev->GetDevices()->at(i).devid == eDid) {
                        fxhl->afx_dev->GetDevices()->at(i).name = buffer;
                        break;
                    }
                }
                if (i == fxhl->afx_dev->GetDevices()->size()) {
                    AlienFX_SDK::devmap nDev;
                    nDev.devid = eDid;
                    nDev.name = buffer;
                    fxhl->afx_dev->GetDevices()->push_back(nDev);
                    fxhl->afx_dev->SaveMappings();
                }
                SendMessage(dev_list, CB_DELETESTRING, dItem, 0);
                SendMessage(dev_list, CB_INSERTSTRING, dItem, (LPARAM)(buffer));
                SendMessage(dev_list, CB_SETITEMDATA, dItem, (LPARAM)eDid);
                break;
            }
        } break;
        case IDC_LIGHTS_S:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE: {
                SetDlgItemInt(hDlg, IDC_LIGHTID, lid, false);
                CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev->GetFlags(did, lid) ? BST_CHECKED : BST_UNCHECKED);
                eve->StopEvents();
                // highlight to check....
                fxhl->TestLight(lid);
                eLid = lid; lItem = lbItem;
            } break;
            case CBN_EDITCHANGE:
                char buffer[256];
                GetWindowTextA(light_list, buffer, 256);
                for (i = 0; i < fxhl->afx_dev->GetMappings()->size(); i++) {
                    if (fxhl->afx_dev->GetMappings()->at(i).devid == eDid &&
                        fxhl->afx_dev->GetMappings()->at(i).lightid == eLid) {
                        fxhl->afx_dev->GetMappings()->at(i).name = buffer;
                        fxhl->afx_dev->SaveMappings();
                        SendMessage(light_list, CB_DELETESTRING, lItem , 0);
                        SendMessage(light_list, CB_INSERTSTRING, lItem, (LPARAM)buffer);
                        SendMessage(light_list, CB_SETITEMDATA, lItem, eLid);
                        break;
                    }
                }
                break;
            case CBN_KILLFOCUS:
                eve->StartEvents();
                break;
            }
            break;
        case IDC_BUTTON_ADDL: {
            char buffer[255];
            int cid = GetDlgItemInt(hDlg, IDC_LIGHTID, NULL, false);
            // let's check if we have the same ID, need to use max+1 in this case
            unsigned maxID = 0; bool haveID = false;
            size_t lights = fxhl->afx_dev->GetMappings()->size();
            for (int i = 0; i < lights; i++) {
                AlienFX_SDK::mapping lgh = fxhl->afx_dev->GetMappings()->at(i);
                if (lgh.devid == eDid) {
                    if (lgh.lightid > maxID)
                        maxID = lgh.lightid;
                    if (lgh.lightid == cid) haveID = true;
                }
            }
            if (haveID) cid = maxID + 1;
            AlienFX_SDK::mapping dev;
            dev.devid = eDid;
            dev.lightid = cid;
            sprintf_s(buffer, 255, "Light #%d", cid);
            dev.name = buffer;
            fxhl->afx_dev->GetMappings()->push_back(dev);
            fxhl->afx_dev->SaveMappings();
            UpdateLightList(light_list, eDid);
        } break;
        case IDC_BUTTON_REML:
            if (MessageBox(hDlg, "Do you really want to remove current light name and all it's settings from all profiles?", "Warning!",
                MB_YESNO | MB_ICONWARNING) == IDYES) {
                // store profile...
                conf->profiles[conf->activeProfile].lightsets = conf->active_set;
                // delete from all profiles...
                for (std::vector <profile>::iterator Iter = conf->profiles.begin();
                    Iter != conf->profiles.end(); Iter++) {
                    // erase mappings
                    for (std::vector <lightset>::iterator mIter = Iter->lightsets.begin();
                        mIter != Iter->lightsets.end(); mIter++)
                        if (mIter->devid == did && mIter->lightid == lid) {
                            Iter->lightsets.erase(mIter);
                            break;
                        }
                }
                // reset active mappings
                conf->active_set = conf->profiles[conf->activeProfile].lightsets;
                std::vector <AlienFX_SDK::mapping>* mapps = fxhl->afx_dev->GetMappings();
                for (std::vector <AlienFX_SDK::mapping>::iterator Iter = mapps->begin();
                    Iter != mapps->end(); Iter++)
                    if (Iter->devid == eDid && Iter->lightid == lid) {
                        fxhl->afx_dev->GetMappings()->erase(Iter);
                        break;
                    }
                fxhl->afx_dev->SaveMappings();
                conf->Save();
                UpdateLightList(light_list, eDid);
            }
            break;
        case IDC_BUTTON_RESETCOLOR:
            if (MessageBox(hDlg, "Do you really want to remove current light control settings from all profiles?", "Warning!",
                MB_YESNO | MB_ICONWARNING) == IDYES) {
                // store profile...
                conf->profiles[conf->activeProfile].lightsets = conf->active_set;
                // delete from all profiles...
                for (std::vector <profile>::iterator Iter = conf->profiles.begin();
                    Iter != conf->profiles.end(); Iter++) {
                    // erase mappings
                    for (std::vector <lightset>::iterator mIter = Iter->lightsets.begin();
                        mIter != Iter->lightsets.end(); mIter++)
                        if (mIter->devid == did && mIter->lightid == lid) {
                            Iter->lightsets.erase(mIter);
                            break;
                        }
                }
                // reset active mappings
                conf->active_set = conf->profiles[conf->activeProfile].lightsets;
            }
            break;
        case IDC_BUTTON_TESTCOLOR: {
            AlienFX_SDK::afx_act c;
            c.r = conf->testColor.cs.red;
            c.g = conf->testColor.cs.green;
            c.b = conf->testColor.cs.blue;
            SetColor(hDlg, IDC_BUTTON_TESTCOLOR, &c);
            conf->testColor.cs.red = c.r;
            conf->testColor.cs.green = c.g;
            conf->testColor.cs.blue = c.b;
            if (lid != -1) {
                eve->StopEvents();
                SetFocus(light_list);
                fxhl->TestLight(lid);
            }
        } break;
        case IDC_ISPOWERBUTTON:
            if (lid != -1) {
                unsigned flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
                fxhl->afx_dev->SetFlags(did, lid, flags);
                // remove power button config from chip if unchecked
                if (!flags) {
                    fxhl->ResetPower();
                    MessageBox(hDlg, "Hardware Power button disabled, you may need to reset light system!", "Warning!",
                        MB_OK);
                }
                fxhl->Refresh(true);
            }
            break;
        case IDC_BUTTON_DEVRESET: {
            if (pid != -1) {
                eve->StopEvents();
                fxhl->afx_dev->AlienFXChangeDevice(pid);
                //conf->mappings = conf->profiles[conf->activeProfile].lightsets;
                BYTE status = fxhl->afx_dev->AlienfxGetDeviceStatus();
                if (status && status != 0xff)
                    SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Ok");
                else
                    SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Error");
                eve->StartEvents();
            }
        } break;
        default: return false;
        }
    } break;
    case WM_DRAWITEM:
        switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
        case IDC_BUTTON_TESTCOLOR:
            RedrawButton(hDlg, IDC_BUTTON_TESTCOLOR, conf->testColor.cs.red, conf->testColor.cs.green, conf->testColor.cs.blue);
            break;
        } break;
    default: return false;
    }
    return true;
}

int pCid = -1, pCitem = -1;

int ReloadProfileCombo(HWND hDlg, int cID) {
    int rpos = 0;
    HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
        profile_list = GetDlgItem(hDlg, IDC_COMBO_PROFILES);
    SendMessage(profile_list, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < conf->profiles.size(); i++) {
        int pos = (int)SendMessage(profile_list, CB_ADDSTRING, 0, (LPARAM)(conf->profiles[i].name.c_str()));
        SendMessage(profile_list, CB_SETITEMDATA, pos, conf->profiles[i].id);
        if (conf->profiles[i].id == cID) {
            SendMessage(profile_list, CB_SETCURSEL, pos, 0);
            CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, conf->profiles[i].flags & 0x1 ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHECK_NOMON, conf->profiles[i].flags & 0x2 ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, conf->profiles[i].flags & 0x4 ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, conf->profiles[i].flags & 0x8 ? BST_CHECKED : BST_UNCHECKED);
            SendMessage(app_list, LB_RESETCONTENT, 0, 0);
            SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM)(conf->profiles[i].triggerapp.c_str()));
            rpos = pos;
        }
    }
    return rpos;
}

BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
        profile_list = GetDlgItem(hDlg, IDC_COMBO_PROFILES);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        pCitem = ReloadProfileCombo(hDlg, conf->activeProfile);
        pCid = conf->activeProfile;
    } break;
    case WM_COMMAND:
    {
        int pbItem = (int)SendMessage(profile_list, CB_GETCURSEL, 0, 0);
        int prid = (int)SendMessage(profile_list, CB_GETITEMDATA, pbItem, 0);
        profile* prof = eve->FindProfile(prid);
        
        switch (LOWORD(wParam))
        {
        case IDC_COMBO_PROFILES: {
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE: {
                if (prof != NULL) {
                    CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof->flags & 0x1 ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_NOMON, prof->flags & 0x2 ? BST_CHECKED : BST_UNCHECKED);
                    CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof->flags & 0x4 ? BST_CHECKED : BST_UNCHECKED);
                    SendMessage(app_list, LB_RESETCONTENT, 0, 0);
                    SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM)(prof->triggerapp.c_str()));
                    pCid = prid; pCitem = pbItem;
                }
            } break;
            case CBN_EDITCHANGE:
                prof = eve->FindProfile(pCid);
                if (prof != NULL) {
                    char* buffer = (char *) calloc(32767, sizeof(char));
                    GetWindowTextA(profile_list, buffer, 32767);
                    prof->name = buffer;
                    SendMessage(profile_list, CB_DELETESTRING, pCitem, 0);
                    SendMessage(profile_list, CB_INSERTSTRING, pCitem, (LPARAM)(prof->name.c_str()));
                    SendMessage(profile_list, CB_SETITEMDATA, pCitem, pCid);
                    if (pCid == conf->activeProfile)
                        ReloadProfileList(NULL);
                    free(buffer);
                }
                break;
            case CBN_KILLFOCUS:
                ReloadProfileList(NULL);
                break;
            } break;
        } break;
        case IDC_ADDPROFILE: {
            char buf[128]; unsigned vacID = 0;
            profile prof;
            for (int i = 0; i < conf->profiles.size(); i++)
                if (vacID == conf->profiles[i].id) {
                    vacID++; i = -1;
                }
            sprintf_s(buf, 128, "Profile %d", vacID);
            prof.id = vacID;
            prof.name = buf;
            prof.lightsets = conf->active_set;
            conf->profiles.push_back(prof);
            pCitem = ReloadProfileCombo(hDlg, vacID);
            pCid = vacID; 
            ReloadProfileList(NULL);
        } break;
        case IDC_REMOVEPROFILE: {
            profile* prof = eve->FindProfile(pCid);
            if (prof != NULL && !(prof->flags & 0x1) && conf->profiles.size() > 1 && 
                MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning!",
                MB_YESNO | MB_ICONWARNING) == IDYES) { // can't delete last profile!
                for (std::vector <profile>::iterator Iter = conf->profiles.begin();
                    Iter != conf->profiles.end(); Iter++)
                    if (Iter->id == pCid) {
                        conf->profiles.erase(Iter);
                        break;
                    }
                // now check about profile switch needed
                if (conf->activeProfile == pCid) {
                    // switch to default profile..
                    eve->SwitchActiveProfile(conf->defaultProfile);
                    pCitem = ReloadProfileCombo(hDlg, conf->activeProfile);
                }
                else {
                    pCitem = ReloadProfileCombo(hDlg, conf->activeProfile);
                    pCid = conf->activeProfile;
                }
                ReloadProfileList(NULL);
            }
        } break;
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
                char appName[MAX_PATH];
                ZeroMemory(&fstruct, sizeof(OPENFILENAMEA));
                fstruct.lStructSize = sizeof(OPENFILENAMEA);
                fstruct.hwndOwner = hDlg;
                fstruct.lpstrFile = appName;
                fstruct.lpstrFile[0] = 0;
                fstruct.nMaxFile = 511;
                fstruct.lpstrFilter = "Applications\0*.exe\0\0";
                fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES;
                if (GetOpenFileNameA(&fstruct)) {
                    prof->triggerapp = appName;
                    SendMessage(app_list, LB_RESETCONTENT, 0, 0);
                    SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM)(prof->triggerapp.c_str()));
                }
            }
        } break;
        case IDC_CHECK_DEFPROFILE:
            if (prof != NULL) {
                bool nflags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
                if (nflags) {
                    profile* old_def = eve->FindProfile(conf->defaultProfile);
                    if (old_def != NULL)
                        old_def->flags = old_def->flags & 0xfe;
                    prof->flags = prof->flags | 0x1;
                    conf->defaultProfile = prof->id;
                }
                else
                    CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_CHECKED);
            }
            break;
        case IDC_CHECK_NOMON:
            if (prof != NULL) {
                prof->flags = (prof->flags & 0xfd) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 1;
                if (prof->id == conf->activeProfile) {
                    DWORD oldState = conf->monState;
                    conf->monState = prof->flags & 0x2 ? 0 : conf->enableMon;
                    if (oldState != conf->monState)
                        eve->ToggleEvents();
                }
            }
            break;
        case IDC_CHECK_PROFDIM:
            if (prof != NULL) {
                prof->flags = (prof->flags & 0xfb) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 2;
                if (prof->id == conf->activeProfile) {
                    conf->stateDimmed = prof->flags & 0x4;
                    fxhl->RefreshState();
                }
            }
            break;
        case IDC_CHECK_FOREGROUND:
            if (prof != NULL) {
                prof->flags = (prof->flags & 0xf7) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 3;
            }
            break;
        }
    } break;
    default: return false;
    }
    return true;
}

BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    //int pid = fxhl->afx_dev->GetPID();
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
        if (conf->offWithScreen) CheckDlgButton(hDlg, IDC_BATTSCREENOFF, BST_CHECKED);
        if (conf->enableMon) CheckDlgButton(hDlg, IDC_BATTMONITOR, BST_CHECKED);
        if (conf->lightsOn) CheckDlgButton(hDlg, IDC_CHECK_LON, BST_CHECKED);
        if (conf->dimmed) CheckDlgButton(hDlg, IDC_CHECK_DIM, BST_CHECKED);
        if (conf->gammaCorrection) CheckDlgButton(hDlg, IDC_CHECK_GAMMA, BST_CHECKED);
        if (conf->offPowerButton) CheckDlgButton(hDlg, IDC_OFFPOWERBUTTON, BST_CHECKED);
        if (conf->enableProf) CheckDlgButton(hDlg, IDC_BATTPROFILE, BST_CHECKED);
        if (conf->awcc_disable) CheckDlgButton(hDlg, IDC_AWCC, BST_CHECKED);
        if (conf->esif_temp) CheckDlgButton(hDlg, IDC_ESIFTEMP, BST_CHECKED);
        SendMessage(dim_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(dim_slider, TBM_SETTICFREQ, 16, 0);
        SendMessage(dim_slider, TBM_SETPOS, true, conf->dimmingPower);
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
        case IDC_BATTSCREENOFF:
            conf->offWithScreen = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            break;
        case IDC_BATTMONITOR:
            conf->enableMon = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            conf->monState = eve->FindProfile(conf->activeProfile)->flags & 0x2 ? 0 : conf->enableMon;
            eve->ToggleEvents();
            break;
        case IDC_BATTPROFILE:
            eve->StopProfiles();
            conf->enableProf = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            ReloadProfileList(NULL);
            eve->StartProfiles();
            break;
        case IDC_CHECK_LON:
            conf->stateOn = conf->lightsOn = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            fxhl->Refresh(true);
            eve->ToggleEvents();
            break;
        case IDC_CHECK_DIM:
            conf->dimmed = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            fxhl->RefreshState();
            break;
        case IDC_CHECK_GAMMA:
            conf->gammaCorrection = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            fxhl->RefreshState();
            break;
        case IDC_OFFPOWERBUTTON:
            conf->offPowerButton = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            fxhl->RefreshState();
            break;
        case IDC_AWCC:
            conf->awcc_disable = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            if (!conf->awcc_disable) {
                //conf->block_power = true;
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
                }
                fxhl->RefreshState();
            } break;
        } break;
    default: return false;
    }
    return true;
}
