// alienfx-gui.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "alienfx-gui.h"
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <Commdlg.h>
#include <shellapi.h>
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "..\AlienFX-SDK\AlienFX_SDK\AlienFX_SDK.h"
#include "EventHandler.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")

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
BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

FXHelper* fxhl;
ConfigHandler* conf;
EventHandler* eve;

HWND mDlg;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALIENFXGUI, szWindowClass, MAX_LOADSTRING);
    //MyRegisterClass(hInstance);

    conf = new ConfigHandler();
    fxhl = new FXHelper(conf);
    eve = new EventHandler(conf, fxhl);

    conf->Load();

    eve->ChangePowerState();

    //fxhl->Refresh(true);

    if (conf->lightsOn) {
        eve->StartEvents();
    }

    // Perform application initialization:
    if (!(mDlg=InitInstance (hInstance, nCmdShow)))
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
    
    /*while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }*/

    eve->StopEvents();

    conf->Save();

    //fxhl->Refresh(true);

    AlienFX_SDK::Functions::SaveMappings();

    delete eve;
    delete fxhl;
    delete conf;

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
/*ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXGUI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ALIENFXGUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ALIENFXGUI));

    return RegisterClassExW(&wcex);
}*/

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND dlg;
    dlg = CreateDialogParam(hInstance,//GetModuleHandle(NULL),         /// instance handle
        MAKEINTRESOURCE(IDD_MAINWINDOW),    /// dialog box template
        NULL,                    /// handle to parent
        (DLGPROC)DialogConfigStatic, 0);
    if (!dlg) return NULL;

    //cap = new CaptureHelper(dlg, conf, fxhl);

    SendMessage(dlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXGUI)));
    SendMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXGUI), IMAGE_ICON, 16, 16, 0));

    //ShowWindow(hWnd, nCmdShow);
    ShowWindow(dlg, nCmdShow);
    //UpdateWindow(dlg);

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
                CopyMemory(pResCopy, pRes, dwSize);
                FreeResource(hResData);

                VerQueryValue(pResCopy, TEXT("\\"), (LPVOID*)&lpFfi, &uLen);
                char buf[255];

                DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
                DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

                sprintf_s(buf, 255, "Version: %d.%d.%d.%d", HIWORD(dwFileVersionMS), LOWORD(dwFileVersionMS), HIWORD(dwFileVersionLS), LOWORD(dwFileVersionLS));

                Static_SetText(version_text, buf);
                LocalFree(pResCopy);
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

#define C_PAGES 4

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

VOID OnSelChanged(HWND hwndDlg)
{

    // Get the dialog header data.
    DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(
        hwndDlg, GWLP_USERDATA);

    // Get the index of the selected tab.
    tabSel = TabCtrl_GetCurSel(pHdr->hwndTab);

    // Destroy the current child dialog box, if any. 
    if (pHdr->hwndDisplay != NULL)
        DestroyWindow(pHdr->hwndDisplay);
    DLGPROC tdl = NULL;
    switch (tabSel) {
    case 0: tdl = (DLGPROC)TabColorDialog; break;
    case 1: tdl = (DLGPROC)TabEventsDialog; break;
    case 2: tdl = (DLGPROC)TabDevicesDialog; break;
    case 3: tdl = (DLGPROC)TabSettingsDialog; break;
    }
    pHdr->hwndDisplay = CreateDialogIndirect(hInst,
        (DLGTEMPLATE*)pHdr->apRes[tabSel], pHdr->hwndTab, tdl);

    SetWindowPos(pHdr->hwndDisplay, NULL,
        pHdr->rcDisplay.left, pHdr->rcDisplay.top,
        (pHdr->rcDisplay.right - pHdr->rcDisplay.left), //- cxMargin - (GetSystemMetrics(SM_CXDLGFRAME)) + 1,
        (pHdr->rcDisplay.bottom - pHdr->rcDisplay.top), //- /*cyMargin - */(GetSystemMetrics(SM_CYDLGFRAME)) - GetSystemMetrics(SM_CYCAPTION) + 3,
        SWP_SHOWWINDOW);
    ShowWindow(pHdr->hwndDisplay, SW_SHOW);
    return;
}

int pRid = -1, pRitem = -1;

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
        pHdr->apRes[3] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_SETTINGS));

        TCITEM tie;

        tie.mask = TCIF_TEXT;
        tie.iImage = -1;
        tie.pszText = (LPSTR)"Colors";
        SendMessage(tab_list, TCM_INSERTITEM, 0, (LPARAM)&tie);
        tie.pszText = (LPSTR)"Monitoring";
        SendMessage(tab_list, TCM_INSERTITEM, 1, (LPARAM)&tie);
        tie.pszText = (LPSTR)"Devices and Lights";
        SendMessage(tab_list, TCM_INSERTITEM, 2, (LPARAM)&tie);
        tie.pszText = (LPSTR)"Settings";
        SendMessage(tab_list, TCM_INSERTITEM, 3, (LPARAM)&tie);

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

        OnSelChanged(tab_list);

        if (conf->profiles.size() == 0) {
            std::string dname = "Default";
            int pos = (int)SendMessage(profile_list, CB_ADDSTRING, 0, (LPARAM)(dname.c_str()));
            SendMessage(profile_list, CB_SETITEMDATA, pos, 0);
            SendMessage(profile_list, CB_SETCURSEL, pos, 0);
            pRid = 0; pRitem = pos;
        }
        else {
            for (int i = 0; i < conf->profiles.size(); i++) {
                int pos = (int)SendMessage(profile_list, CB_ADDSTRING, 0, (LPARAM)(conf->profiles[i].name.c_str()));
                SendMessage(profile_list, CB_SETITEMDATA, pos, conf->profiles[i].id);
                if (conf->profiles[i].id == conf->activeProfile) {
                    SendMessage(profile_list, CB_SETCURSEL, pos, 0);
                    pRid = conf->activeProfile; pRitem = pos;
                }
            }
        }

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
        case IDOK: case IDCANCEL: case IDCLOSE: case IDM_EXIT: case ID_TRAYMENU_EXIT:
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
        case IDC_BUTTON_REFRESH: case ID_TRAYMENU_REFRESH:
            fxhl->RefreshState();
            break;
        case ID_TRAYMENU_LIGHTSON:
            conf->lightsOn = !conf->lightsOn;
            if (conf->lightsOn) {
                fxhl->Refresh(true);
                eve->StartEvents();
            }
            else eve->StopEvents();
            break;
        case ID_TRAYMENU_DIMLIGHTS:
            conf->dimmed = !conf->dimmed;
            fxhl->RefreshState();
            break;
        case ID_TRAYMENU_MONITORING:
            eve->StopEvents();
            conf->enableMon = !conf->enableMon;
            eve->StartEvents();
            //fxhl->RefreshState();
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
        case IDC_BUTTON_SAVE:
            AlienFX_SDK::Functions::SaveMappings();
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
        case ID_ACC_SETTINGS:
            TabCtrl_SetCurSel(tab_list, 3);
            OnSelChanged(tab_list);
            break;
        case IDC_PROFILES: {
            int pbItem = (int)SendMessage(profile_list, CB_GETCURSEL, 0, 0);
            int prid = (int)SendMessage(profile_list, CB_GETITEMDATA, pbItem, 0);
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE: {
                for (int i = 0; i < conf->profiles.size(); i++)
                    if (conf->profiles[i].id == prid) {
                        // save current profile mappings...
                        conf->profiles[conf->activeProfile].lightsets = conf->mappings;
                        // load new mappings...
                        conf->mappings = conf->profiles[i].lightsets;
                        conf->activeProfile = prid;
                        // Reload lighs list at colors and events.
                        OnSelChanged(tab_list);
                        pRitem = pbItem; pRid = prid;
                        fxhl->RefreshState();
                    }
            } break;
            case CBN_EDITCHANGE: {
                char* buffer = new char[32767];
                GetWindowTextA(profile_list, buffer, 32767);
                for (int i = 0; i < conf->profiles.size(); i++) {
                    if (conf->profiles[i].id == pRid) {
                        conf->profiles[i].name = buffer;
                        break;
                    }
                }
                SendMessage(profile_list, CB_DELETESTRING, pRitem, 0);
                SendMessage(profile_list, CB_INSERTSTRING, pRitem, (LPARAM)(buffer));
                SendMessage(profile_list, CB_SETITEMDATA, pRitem, (LPARAM)pRid);
                delete buffer;
            } break;
            } 
        } break;
        case IDC_ADDPROFILE: {
            char buf[128]; unsigned vacID = 0;
            profile prof;
            for (int i = 0; i < conf->profiles.size(); i++)
                if (vacID == conf->profiles[i].id) {
                    vacID++; i = 0;
                }
            sprintf_s(buf, 128, "Profile %d", vacID);
            prof.id = vacID;
            prof.name = buf;
            prof.lightsets = conf->mappings;
            conf->profiles.push_back(prof);
            int pos = (int)SendMessage(profile_list, CB_ADDSTRING, 0, (LPARAM)(prof.name.c_str()));
            SendMessage(profile_list, CB_SETITEMDATA, pos, vacID);
            SendMessage(profile_list, CB_SETCURSEL, pos, 0);
            pRid = conf->activeProfile = vacID; pRitem = pos;
        } break;
        case IDC_REMOVEPROFILE: {
            if (conf->profiles.size() > 1 && MessageBox(hDlg, "Do you really want to remove current profile and all settings for it?", "Warning!",
                MB_YESNO | MB_ICONWARNING) == IDYES) { // can't delete last profile!
                for (std::vector <profile>::iterator Iter = conf->profiles.begin(); 
                    Iter != conf->profiles.end(); Iter++)
                    if (Iter->id == pRid) {
                        conf->profiles.erase(Iter);
                        break;
                    }
                // now delete from list and reselect
                SendMessage(profile_list, CB_DELETESTRING, pRitem, 0);
                pRitem = pRitem > 0 ? pRitem - 1 : 0;
                SendMessage(profile_list, CB_SETCURSEL, pRitem, 0);
                pRid = (int)SendMessage(profile_list, CB_GETITEMDATA, pRitem, 0);
                conf->activeProfile = pRid;
                // Reload mappings...
                for (int i = 0; i < conf->profiles.size(); i++)
                    if (conf->profiles[i].id == pRid) {
                        conf->mappings = conf->profiles[i].lightsets;
                        break;
                    }
                OnSelChanged(tab_list);
                fxhl->RefreshState();
            }
        } break;

        } break;
    } break;
    case WM_NOTIFY: {
        NMHDR* event = (NMHDR*)lParam;
        switch (event->idFrom) {
        case IDC_TAB_MAIN: {
            //NMHDR* event = (NMHDR*)lParam;
            int newTab = TabCtrl_GetCurSel(tab_list);
            if (newTab != tabSel) { // selection changed!
                OnSelChanged(tab_list);
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
            GetCursorPos(&lpClickPoint);
            SetForegroundWindow(hDlg);
            if (conf->lightsOn) CheckMenuItem(tMenu, ID_TRAYMENU_LIGHTSON, MF_CHECKED);
            if (conf->dimmed) CheckMenuItem(tMenu, ID_TRAYMENU_DIMLIGHTS, MF_CHECKED);
            if (conf->enableMon) CheckMenuItem(tMenu, ID_TRAYMENU_MONITORING, MF_CHECKED);
            TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
                lpClickPoint.x, lpClickPoint.y, 0, hDlg, NULL);
        } break;
        }
        break;
    } break;
    case WM_POWERBROADCAST:
        switch (wParam) {
        case PBT_APMRESUMEAUTOMATIC: case PBT_APMPOWERSTATUSCHANGE:
                //power status changed
            eve->ChangePowerState();
            //fxhl->RefreshState();
            break;
        case PBT_POWERSETTINGCHANGE:
            POWERBROADCAST_SETTING* sParams = (POWERBROADCAST_SETTING*)lParam;
            if (sParams->PowerSetting == GUID_MONITOR_POWER_ON) {
                eve->ChangeScreenState(sParams->Data[0]);
            }
            break;
        }
        break;
    case WM_HOTKEY:
        switch (wParam) {
        case 1: // on/off
            conf->lightsOn = !conf->lightsOn;
            if (conf->lightsOn) {
                fxhl->Refresh(true);
                eve->StartEvents();
            }
            else eve->StopEvents();
            break;
        case 2: // dim
            conf->dimmed = !conf->dimmed;
            fxhl->RefreshState();
            break;
        case 3: // off-dim-full circle
            if (conf->lightsOn) {
                if (conf->dimmed) {
                    conf->lightsOn = !conf->lightsOn;
                    conf->dimmed = !conf->dimmed;
                    //fxhl->Refresh();
                    eve->StopEvents();
                }
                else {
                    conf->dimmed = !conf->dimmed;
                    fxhl->RefreshState();
                }
            }
            else {
                conf->lightsOn = !conf->lightsOn;
                fxhl->Refresh(true);
                eve->StartEvents();
            }
            break;
        default: return false;
        }
        break;
    //case WM_CLOSE:
    //    DestroyWindow(hDlg); break;
    case WM_DESTROY: PostQuitMessage(0); break;
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

#include <ColorDlg.h>
Colorcode* mod;

UINT_PTR Lpcchookproc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    DRAWITEMSTRUCT* item = 0;
    //Colorcode* mod;
    //HWND r = GetDlgItem(hDlg, 706);
    UINT r = 0, g = 0, b = 0;

    switch (message)
    {
    case WM_INITDIALOG:
        mod = (Colorcode*)((CHOOSECOLOR*)lParam)->lCustData;
        break;
    case WM_COMMAND:
        break;
    case WM_CTLCOLOREDIT:
        r = GetDlgItemInt(hDlg, COLOR_RED, NULL, false);
        g = GetDlgItemInt(hDlg, COLOR_GREEN, NULL, false);
        b = GetDlgItemInt(hDlg, COLOR_BLUE, NULL, false);
        if (r != mod->cs.red || g != mod->cs.green || b != mod->cs.blue) {
            mod->cs.red = r;
            mod->cs.green = g;
            mod->cs.blue = b;
            // update lights....
            fxhl->RefreshState();
        }
        break;
    }
    return 0;
}

bool SetColor(HWND hDlg, int id, Colorcode* map) {
    CHOOSECOLOR cc;                 // common dialog box structure 
    bool ret;

    unsigned savedColor = map->ci;

    // Initialize CHOOSECOLOR 
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hDlg;
    cc.lpfnHook = Lpcchookproc;
    cc.lCustData = (LPARAM) map;
    cc.lpCustColors = (LPDWORD)conf->customColors;
    cc.rgbResult = RGB(map->cs.red, map->cs.green, map->cs.blue);
    cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR | CC_ENABLEHOOK;

    if (!(ret = ChooseColor(&cc)))
    {
        map->ci = savedColor;
        RedrawButton(hDlg, id, map->cs.red, map->cs.green, map->cs.blue);
    }
    return ret;
}

mapping* FindMapping(int did, int lid, int index)
{
    lightset *map = NULL;
    mapping* mmap = NULL;
    for (int i = 0; i < conf->mappings.size(); i++)
        if (conf->mappings[i].devid == did && conf->mappings[i].lightid == lid) {
            map = &conf->mappings[i];
            //mmap = &map->eve[0].map;
            break;
        }
    if (map != NULL) {
        mmap = &map->eve[index].map;
        mmap->lightset = (void*)map;
    }
    return mmap;
}

int eLid = (-1), eDid = (-1), eItem = (-1), dItem = (-1);

BOOL CALLBACK TabColorDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    int pid = AlienFX_SDK::Functions::GetPID();
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
        s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
        s2_slider = GetDlgItem(hDlg, IDC_SPEED2),
        l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
        l2_slider = GetDlgItem(hDlg, IDC_LENGTH2),
        type_c1 = GetDlgItem(hDlg, IDC_TYPE1),
        type_c2 = GetDlgItem(hDlg, IDC_TYPE2);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        //int pid = AlienFX_SDK::Functions::GetPID();
        size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
        bool noLights = true;
        for (int i = 0; i < lights; i++) {
            AlienFX_SDK::mapping lgh = AlienFX_SDK::Functions::GetMappings()->at(i);
            if (lgh.devid == pid) {
                int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
                SendMessage(light_list, LB_SETITEMDATA, pos, lgh.lightid);
                noLights = false;
            }
        }
        if (noLights) {// no lights, switch to setup
            HWND tab_list = GetParent(hDlg);
            TabCtrl_SetCurSel(tab_list, 2);
            EndDialog(hDlg, IDOK);
            DestroyWindow(hDlg);
            OnSelChanged(tab_list);
            return true;
        }
        // Set types list...
        char buffer[100];
        LoadString(hInst, IDS_TYPE_COLOR, buffer, 100);
        SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
        SendMessage(type_c2, CB_ADDSTRING, 0, (LPARAM)buffer);
        LoadString(hInst, IDS_TYPE_PULSE, buffer, 100);
        SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
        SendMessage(type_c2, CB_ADDSTRING, 0, (LPARAM)buffer);
        LoadString(hInst, IDS_TYPE_MORPH, buffer, 100);
        SendMessage(type_c1, CB_ADDSTRING, 0, (LPARAM)buffer);
        SendMessage(type_c2, CB_ADDSTRING, 0, (LPARAM)buffer);
        SendMessage(type_c1, CB_SETCURSEL, 0, 0);
        SendMessage(type_c2, CB_SETCURSEL, 0, 0);
        // now sliders...
        SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(l1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(l2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        //TBM_SETTICFREQ
        SendMessage(s1_slider, TBM_SETTICFREQ, 32, 0);
        SendMessage(s2_slider, TBM_SETTICFREQ, 32, 0);
        SendMessage(l1_slider, TBM_SETTICFREQ, 32, 0);
        SendMessage(l2_slider, TBM_SETTICFREQ, 32, 0);
        if (eItem != (-1)) {
            SendMessage(light_list, LB_SETCURSEL, eItem, 0);
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
        }
    } break;
    case WM_COMMAND: {
        int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
        int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
        int lType1 = (int)SendMessage(type_c1, CB_GETCURSEL, 0, 0);
        int lType2 = (int)SendMessage(type_c2, CB_GETCURSEL, 0, 0);
        mapping* mmap = FindMapping(pid, lid, 0);
        // BLOCK FOR COLORS
        switch (LOWORD(wParam))
        {
        case IDC_LIGHTS: // should reload mappings
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                eItem = lbItem;
                if (mmap == NULL) {
                    lightset newmap;
                    newmap.devid = pid;
                    newmap.lightid = lid;
                    newmap.eve[0].fs.b.flags = 1;
                    newmap.eve[0].map.c1.ci = newmap.eve[0].map.c2.ci = 0;
                    newmap.eve[3].fs.b.cut = 90;
                    conf->mappings.push_back(newmap);
                    mmap = &conf->mappings.back().eve[0].map;
                    mmap->lightset = &conf->mappings.back();
                }
                // Enable or disable controls
                bool flag = !AlienFX_SDK::Functions::GetFlags(pid, lid);
                EnableWindow(type_c1, flag);
                EnableWindow(type_c2, flag);
                EnableWindow(s1_slider, flag);
                EnableWindow(s2_slider, flag);
                EnableWindow(l1_slider, flag);
                EnableWindow(l2_slider, flag);
                // Set data
                SendMessage(type_c1, CB_SETCURSEL, mmap->mode, 0);
                SendMessage(type_c2, CB_SETCURSEL, mmap->mode2, 0);
                RedrawButton(hDlg, IDC_BUTTON_C1, mmap->c1.cs.red, mmap->c1.cs.green, mmap->c1.cs.blue);
                RedrawButton(hDlg, IDC_BUTTON_C2, mmap->c2.cs.red, mmap->c2.cs.green, mmap->c2.cs.blue);
                SendMessage(s1_slider, TBM_SETPOS, true, mmap->speed1);
                SendMessage(s2_slider, TBM_SETPOS, true, mmap->speed2);
                SendMessage(l1_slider, TBM_SETPOS, true, mmap->length1);
                SendMessage(l2_slider, TBM_SETPOS, true, mmap->length2);
                break;
            } break;
        case IDC_TYPE1:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                if (mmap != NULL)
                    mmap->mode = lType1;
            }
            break;
        case IDC_TYPE2:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                if (mmap != NULL)
                    mmap->mode2 = lType2;
            }
            break;
        case IDC_BUTTON_C1:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (mmap != NULL) {
                    SetColor(hDlg, IDC_BUTTON_C1, &mmap->c1);
                }
            } break;
            } break;
        case IDC_BUTTON_C2:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (mmap != NULL) {
                    SetColor(hDlg, IDC_BUTTON_C2, &mmap->c2);
                }
            } break;
            } break;
        case IDC_BUTTON_SETALL:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (mmap != NULL &&
                    MessageBox(hDlg, "Do you really want to set all lights for current device to this settings?", "Warning!",
                        MB_YESNO | MB_ICONWARNING) == IDYES) {
                    for (int i = 0; i < conf->mappings.size(); i++)
                        if (conf->mappings[i].devid == pid) {
                            conf->mappings[i].eve[0] = ((lightset*)mmap->lightset)->eve[0];
                        }
                }
            } break;
            } break;   
        default: return false;
        }
        if (mmap != NULL)
            fxhl->Refresh();
    } break;
    case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBTRACK: case TB_ENDTRACK:
            int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
            int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
            mapping* mmap = FindMapping(pid, lid, 0);
            if (mmap != NULL) {
                if ((HWND)lParam == s1_slider) {
                    mmap->speed1 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == s2_slider) {
                    mmap->speed2 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == l1_slider) {
                    mmap->length1 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == l2_slider) {
                    mmap->length2 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                fxhl->Refresh();
            }
        break;
    } break;
    case WM_DRAWITEM:
        switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
        case IDC_BUTTON_C1: case IDC_BUTTON_C2:
            Colorcode c; c.ci = 0;
            if (eItem != -1) {
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0);
                mapping* map = FindMapping(pid, lid, 0);
                if (map != NULL) {
                    switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
                    case IDC_BUTTON_C1: c = map->c1; break;
                    case IDC_BUTTON_C2: c = map->c2; break;
                    }
                }
            }
            RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.cs.red, c.cs.green, c.cs.blue);
            break;
        }
        break;
    default: return false;
    }
    return true;
}

BOOL TabEventsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int pid = AlienFX_SDK::Functions::GetPID();
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
        size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
        bool noLights = true;
        for (int i = 0; i < lights; i++) {
            AlienFX_SDK::mapping lgh = AlienFX_SDK::Functions::GetMappings()->at(i);
            if (lgh.devid == pid) {
                int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
                SendMessage(light_list, LB_SETITEMDATA, pos, lgh.lightid);
                noLights = false;
            }
        }
        if (noLights) {// no lights, switch to setup
            HWND tab_list = GetParent(hDlg);
            TabCtrl_SetCurSel(tab_list, 2);
            EndDialog(hDlg, IDOK);
            DestroyWindow(hDlg);
            OnSelChanged(tab_list);
            return true;
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
        mapping* mmap = FindMapping(pid, lid, 1);
        lightset* map = NULL;
        if (mmap != NULL)
            map = (lightset*)mmap->lightset;
        switch (LOWORD(wParam))
        {
        case IDC_LIGHTS_E: // should reload mappings
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                eItem = lbItem;
                if (mmap == NULL) {
                    lightset newmap;
                    newmap.devid = pid;
                    newmap.lightid = lid;
                    newmap.eve[0].fs.b.flags = 0;
                    newmap.eve[0].map.c1.ci = newmap.eve[0].map.c2.ci = 0;
                    newmap.eve[3].fs.b.cut = 90;
                    conf->mappings.push_back(newmap);
                    mmap = &conf->mappings.back().eve[0].map;
                    mmap->lightset = map = &conf->mappings.back();
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
                        RedrawButton(hDlg, IDC_BUTTON_CM1 + i - 1, map->eve[i].map.c1.cs.red,
                            map->eve[i].map.c1.cs.green, map->eve[i].map.c1.cs.blue);
                        RedrawButton(hDlg, IDC_BUTTON_CM4 + i - 1, map->eve[i].map.c2.cs.red,
                            map->eve[i].map.c2.cs.green, map->eve[i].map.c2.cs.blue);
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
            if (map != NULL)
                map->eve[eid].fs.b.flags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
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
                    SetColor(hDlg, LOWORD(wParam), &map->eve[eid].map.c1);
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
                    SetColor(hDlg, LOWORD(wParam), &map->eve[eid].map.c2);
                }
                break;
            }
        } break;
        case IDC_COUNTERLIST:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
                if (mmap != NULL) {
                    map->eve[2].source = countid;
                }
                break;
            }
            break;
        case IDC_STATUSLIST:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
                if (mmap != NULL) {
                    map->eve[3].source = statusid;
                }
                break;
            }
            break;
        }
        if (map != NULL)
           fxhl->Refresh();
    } break;
    case WM_DRAWITEM:
        switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
        case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: {
            int cid = ((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM1 + 1;
            Colorcode c; c.ci = 0;
            if (eItem != -1) {
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0);
                mapping* map = FindMapping(pid, lid, cid);
                if (map != NULL)
                    c = map->c1;
            }
            RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.cs.red, c.cs.green, c.cs.blue);
        } break;
        case IDC_BUTTON_CM4: case IDC_BUTTON_CM5: case IDC_BUTTON_CM6: {
            int cid = ((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM4 + 1;
            Colorcode c; c.ci = 0;
            if (eItem != -1) {
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0);
                mapping* map = FindMapping(pid, lid, cid);
                if (map != NULL)
                    c = map->c2;
            }
            RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.cs.red, c.cs.green, c.cs.blue);
        } break;
        }
        break;
    default: return false;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBTRACK: case TB_ENDTRACK:
            int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
            int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
            mapping* mmap = FindMapping(pid, lid, 0);
            lightset* map = NULL;
            if (mmap != NULL)
                map = (lightset*)mmap->lightset;
            if (mmap != NULL) {
                if ((HWND)lParam == s1_slider) {
                    map->eve[2].fs.b.cut = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == s2_slider) {
                    map->eve[3].fs.b.cut = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                fxhl->Refresh();
            }
            break;
        } break;
    }
    return true;
}

bool nEdited = false;

int UpdateLightList(HWND light_list, int pid) {

    int pos = -1;
    size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
    SendMessage(light_list, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < lights; i++) {
        AlienFX_SDK::mapping lgh = AlienFX_SDK::Functions::GetMappings()->at(i);
        if (lgh.devid == pid) {
            pos = (int)SendMessage(light_list, CB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
            SendMessage(light_list, CB_SETITEMDATA, pos, lgh.lightid);
        }
    }
    RedrawWindow(light_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
    return pos;
}

BOOL TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    lightset* map = NULL;
    mapping* mmap = NULL;
    unsigned i;
    int pid = AlienFX_SDK::Functions::GetPID();
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS_S),
        dev_list = GetDlgItem(hDlg, IDC_DEVICES),
        //dim_slider = GetDlgItem(hDlg, IDC_SLIDER_DIMMING),
        light_id = GetDlgItem(hDlg, IDC_LIGHTID);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
        size_t numdev = fxhl->GetDevList().size();
        int cpid = (-1), pos = (-1);
        for (i = 0; i < numdev; i++) {
            cpid = fxhl->GetDevList().at(i);
            int j;
            for (j = 0; j < AlienFX_SDK::Functions::GetDevices()->size(); j++) {
                if (cpid == AlienFX_SDK::Functions::GetDevices()->at(j).devid) {
                    std::string dname = AlienFX_SDK::Functions::GetDevices()->at(i).name;
                    pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(dname.c_str()));
                    SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)cpid);
                    break;
                }
            }
            if (j == AlienFX_SDK::Functions::GetDevices()->size()) {
                // no name
                char devName[256];
                sprintf_s(devName, 255, "Device #%X", cpid);
                pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(devName));
                SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)pid);
            }
            if (cpid == pid) {
                // select this device.
                SendMessage(dev_list, CB_SETCURSEL, pos, (LPARAM)0);
                eDid = pid; dItem = pos;
            }
        }

        UpdateLightList(light_list, pid);
        eItem = -1;

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
                AlienFX_SDK::Functions::AlienFXChangeDevice(did);
                UpdateLightList(light_list, did);
                eItem = -1; eDid = did; dItem = dbItem;
            } break;
            case CBN_EDITCHANGE:
                char buffer[256];
                GetWindowTextA(dev_list, buffer, 256);
                for (i = 0; i < AlienFX_SDK::Functions::GetDevices()->size(); i++) {
                    if (AlienFX_SDK::Functions::GetDevices()->at(i).devid == eDid) {
                        AlienFX_SDK::Functions::GetDevices()->at(i).name = buffer;
                        break;
                    }
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
                if (nEdited) {
                    UpdateLightList(light_list, did);
                    SendMessage(light_list, CB_SETCURSEL, lbItem, 0);
                    nEdited = false;
                }
                SetDlgItemInt(hDlg, IDC_LIGHTID, lid, false);
                if (AlienFX_SDK::Functions::GetFlags(pid, lid))
                    CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_CHECKED);
                else
                    CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
                eve->StopEvents();
                // highlight to check....
                fxhl->TestLight(lid);
                eLid = lid;
            } break;
            case CBN_EDITCHANGE:
                char buffer[256];
                nEdited = true;
                if (lid != (-1)) eLid = lid;
                GetWindowTextA(light_list, buffer, 256);
                for (i = 0; i < AlienFX_SDK::Functions::GetMappings()->size(); i++) {
                    if (AlienFX_SDK::Functions::GetMappings()->at(i).devid == did &&
                        AlienFX_SDK::Functions::GetMappings()->at(i).lightid == eLid) {
                        AlienFX_SDK::Functions::GetMappings()->at(i).name = buffer;
                        //UpdateLightList(light_list, did);
                        break;
                    }
                }
                break;
            case CBN_KILLFOCUS:
                fxhl->Refresh();
                eve->StartEvents();
                if (nEdited) {
                    UpdateLightList(light_list, did);
                    //SendMessage(light_list, CB_SETCURSEL, lbItem, 0);
                    nEdited = false;
                }
                eLid = (-1);
                break;
            }
            break;
        case IDC_BUTTON_ADDL: {
            char buffer[255];
            int cid = GetDlgItemInt(hDlg, IDC_LIGHTID, NULL, false);
            // let's check if we have the same ID, need to use max+1 in this case
            unsigned maxID = 0; bool haveID = false;
            size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
            for (int i = 0; i < lights; i++) {
                AlienFX_SDK::mapping lgh = AlienFX_SDK::Functions::GetMappings()->at(i);
                if (lgh.devid == did) {
                    if (lgh.lightid > maxID)
                        maxID = lgh.lightid;
                    if (lgh.lightid == cid) haveID = true;
                }
            }
            if (haveID) cid = maxID + 1;
            AlienFX_SDK::mapping dev;
            dev.devid = did;
            dev.lightid = cid;
            sprintf_s(buffer, 255, "Light #%d", cid);
            dev.name = buffer;
            AlienFX_SDK::Functions::GetMappings()->push_back(dev);
            UpdateLightList(light_list, did);
        } break;
        case IDC_BUTTON_REML:
            if (MessageBox(hDlg, "Do you really want to remove current light name and all it's settings from all profiles?", "Warning!",
                MB_YESNO | MB_ICONWARNING) == IDYES) {
                // store profile...
                conf->profiles[conf->activeProfile].lightsets = conf->mappings;
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
                conf->mappings = conf->profiles[conf->activeProfile].lightsets;
                std::vector <AlienFX_SDK::mapping>* mapps = AlienFX_SDK::Functions::GetMappings();
                for (std::vector <AlienFX_SDK::mapping>::iterator Iter = mapps->begin();
                    Iter != mapps->end(); Iter++)
                    if (Iter->devid == did && Iter->lightid == lid) {
                        AlienFX_SDK::Functions::GetMappings()->erase(Iter);
                        break;
                    }
                UpdateLightList(light_list, did);
            }
            break;
        case IDC_BUTTON_TESTCOLOR: {
            SetColor(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor);
            if (lid != -1) {
                eve->StopEvents();
                SetFocus(light_list);
                fxhl->TestLight(lid);
            }
        } break;
        case IDC_ISPOWERBUTTON:
            if (lid != -1) {
                unsigned flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
                AlienFX_SDK::Functions::SetFlags(did, lid, flags);
            }
            break;
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

BOOL TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    //int pid = AlienFX_SDK::Functions::GetPID();
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
        if (conf->offPowerButton) CheckDlgButton(hDlg, IDC_OFFPOWERBUTTON, BST_CHECKED);
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
            eve->StopEvents();
            conf->enableMon = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            eve->StartEvents();
            break;
        case IDC_CHECK_LON:
            eve->StopEvents();
            conf->stateOn = conf->lightsOn = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            eve->StartEvents();
            break;
        case IDC_CHECK_DIM:
            conf->dimmed = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            fxhl->RefreshState();
            break;
        case IDC_OFFPOWERBUTTON:
            conf->offPowerButton = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            fxhl->RefreshState();
            break;
        default: return false;
        }
    } break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBTRACK: case TB_ENDTRACK: {
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
