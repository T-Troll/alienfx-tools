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

    if (conf->lightsOn) {
        eve->StopEvents();
        fxhl->Refresh();
    }
    conf->Save();

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
ATOM MyRegisterClass(HINSTANCE hInstance)
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
}

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

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

#define C_PAGES 3 

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
    //RECT rcTab;
    DWORD dwDlgBase = GetDialogBaseUnits();
    int cxMargin = LOWORD(dwDlgBase) / 4;
    int cyMargin = HIWORD(dwDlgBase) / 8;

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
    case 2: tdl = (DLGPROC)TabSettingsDialog; break;
    }
    pHdr->hwndDisplay = CreateDialogIndirect(hInst,
        (DLGTEMPLATE*)pHdr->apRes[tabSel], pHdr->hwndTab, tdl);

    SetWindowPos(pHdr->hwndDisplay, NULL,
        pHdr->rcDisplay.left, pHdr->rcDisplay.top,
        (pHdr->rcDisplay.right - pHdr->rcDisplay.left) - cxMargin - (GetSystemMetrics(SM_CXDLGFRAME)) + 1,
        (pHdr->rcDisplay.bottom - pHdr->rcDisplay.top) - /*cyMargin - */(GetSystemMetrics(SM_CYDLGFRAME)) - GetSystemMetrics(SM_CYCAPTION) + 3,
        SWP_SHOWWINDOW);
    ShowWindow(pHdr->hwndDisplay, SW_SHOW);
    return;
}

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    HWND tab_list = GetDlgItem(hDlg, IDC_TAB_MAIN);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        RECT rcTab;
        DWORD dwDlgBase = GetDialogBaseUnits();
        int cxMargin = -(GetSystemMetrics(SM_CXDLGFRAME)); // LOWORD(dwDlgBase) / 4;
        int cyMargin = -1; // GetSystemMetrics(SM_CYDLGFRAME); // HIWORD(dwDlgBase) / 8;
        DLGHDR* pHdr = (DLGHDR*)LocalAlloc(LPTR, sizeof(DLGHDR));
        SetWindowLongPtr(tab_list, GWLP_USERDATA, (LONG_PTR)pHdr);
        
        pHdr->hwndTab = tab_list;

        pHdr->apRes[0] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_COLORS));
        pHdr->apRes[1] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_EVENTS));
        pHdr->apRes[2] = DoLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG_SETTINGS));

        TCITEM tie;

        tie.mask = TCIF_TEXT;
        tie.iImage = -1;
        tie.pszText = (LPSTR)TEXT("Colors");
        SendMessage(tab_list, TCM_INSERTITEM, 0, (LPARAM)&tie);
        tie.pszText = (LPSTR)TEXT("Events");
        SendMessage(tab_list, TCM_INSERTITEM, 1, (LPARAM)&tie);
        tie.pszText = (LPSTR)TEXT("Settings");
        SendMessage(tab_list, TCM_INSERTITEM, 2, (LPARAM)&tie);

        //SetRectEmpty(&rcTab);
        /*rcTab.right = pHdr->apRes[0]->cx;
        rcTab.bottom = pHdr->apRes[0]->cy;
        for (i = 0; i < C_PAGES; i++)
        {
            if (pHdr->apRes[i]->cx > rcTab.right)
                rcTab.right = pHdr->apRes[i]->cx;
            if (pHdr->apRes[i]->cy > rcTab.bottom)
                rcTab.bottom = pHdr->apRes[i]->cy;
        }*/

        GetClientRect(pHdr->hwndTab, &rcTab);
        MapDialogRect(pHdr->hwndTab, &rcTab);

        // Calculate how large to make the tab control, so 
        // the display area can accommodate all the child dialog boxes. 
        TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);
        OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

        // Calculate the display rectangle. 
        CopyRect(&pHdr->rcDisplay, &rcTab);
        TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

        OnSelChanged(tab_list);
        
    } break;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK: case IDCANCEL: case IDCLOSE: case IDM_EXIT: case ID_TRAYMENU_EXIT:
        {
            //cap->Stop();
            Shell_NotifyIcon(NIM_DELETE, &niData);
            DestroyWindow(hDlg); //EndDialog(hDlg, IDOK);
        } break;
        case IDM_ABOUT: // about dialogue here
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
            fxhl->Refresh();
            break;
        case ID_TRAYMENU_LIGHTSON: case ID_ACC_ONOFF:
            conf->lightsOn = !conf->lightsOn;
            fxhl->Refresh();
            if (conf->lightsOn) eve->StartEvents();
            else eve->StopEvents();
            break;
        case ID_TRAYMENU_DIMLIGHTS: case ID_ACC_DIM:
            conf->dimmed = !conf->dimmed;
            fxhl->Refresh();
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
            Shell_NotifyIcon(NIM_DELETE, &niData);
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
        case ID_ACC_SETTINGS:
            TabCtrl_SetCurSel(tab_list, 2);
            OnSelChanged(tab_list);
            break;
        }
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
        } break;
    case WM_APP + 1: {
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
        //case WM_LBUTTONUP:
            ShowWindow(hDlg, SW_RESTORE);
            SetWindowPos(hDlg,       // handle to window
                HWND_TOPMOST,  // placement-order handle
                0,     // horizontal position
                0,      // vertical position
                0,  // width
                0, // height
                SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE// window-positioning options
            );
            Shell_NotifyIcon(NIM_DELETE, &niData);
            break;
            //case WM_RBUTTONDOWN:
        case WM_RBUTTONUP: case WM_CONTEXTMENU: {
            POINT lpClickPoint;
            HMENU tMenu = LoadMenu(hInst, MAKEINTRESOURCEA(IDR_MENU_TRAY));
            tMenu = GetSubMenu(tMenu, 0);
            GetCursorPos(&lpClickPoint);
            SetForegroundWindow(hDlg);
            if (conf->lightsOn) CheckMenuItem(tMenu, ID_TRAYMENU_LIGHTSON, MF_CHECKED);
            if (conf->dimmed) CheckMenuItem(tMenu, ID_TRAYMENU_DIMLIGHTS, MF_CHECKED);
            TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
                lpClickPoint.x, lpClickPoint.y, 0, hDlg, NULL);
        } break;
        }
        break;
    } break;
    case WM_POWERBROADCAST:
        switch (wParam) {
        case PBT_APMRESUMEAUTOMATIC:
                //resumed from sleep
            eve->ChangePowerState();
            fxhl->Refresh();
            break;
        case PBT_APMPOWERSTATUSCHANGE:
            // bat/ac change
            eve->ChangePowerState();
            fxhl->Refresh();
        }
        break;
    case WM_HOTKEY:
        switch (wParam) {
        case 1: // on/off
            conf->lightsOn = !conf->lightsOn;
            fxhl->Refresh();
            if (conf->lightsOn) eve->StartEvents();
            else eve->StopEvents();
            break;
        case 2: // dim
            conf->dimmed = !conf->dimmed;
            fxhl->Refresh();
            break;
        case 3: // off-dim-full circle
            if (conf->lightsOn) {
                if (conf->dimmed) {
                    conf->lightsOn = !conf->lightsOn;
                    conf->dimmed = !conf->dimmed;
                    fxhl->Refresh();
                    eve->StopEvents();
                }
                else {
                    conf->dimmed = !conf->dimmed;
                    fxhl->Refresh();
                }
            }
            else {
                conf->lightsOn = !conf->lightsOn;
                fxhl->Refresh();
                eve->StartEvents();
            }
            break;
        }
        break;
    case WM_CLOSE: //cap->Stop(); 
        DestroyWindow(hDlg); break;
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

void SetLightMode(HWND hDlg, int num, int mode, mapping* map) {
    if (num == 0) {
        if (map != NULL) map->mode = mode;
        CheckDlgButton(hDlg, IDC_RADIO_COLOR, BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_PULSE, BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_MORPH, BST_UNCHECKED);
        switch (mode) {
        case 0: CheckDlgButton(hDlg, IDC_RADIO_COLOR, BST_CHECKED); break;
        case 1: CheckDlgButton(hDlg, IDC_RADIO_PULSE, BST_CHECKED); break;
        case 2: CheckDlgButton(hDlg, IDC_RADIO_MORPH, BST_CHECKED); break;
        }
    }
    else {
        if (map != NULL) map->mode2 = mode;
        CheckDlgButton(hDlg, IDC_RADIO_COLOR2, BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_PULSE2, BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_MORPH2, BST_UNCHECKED);
        switch (mode) {
        case 0: CheckDlgButton(hDlg, IDC_RADIO_COLOR2, BST_CHECKED); break;
        case 1: CheckDlgButton(hDlg, IDC_RADIO_PULSE2, BST_CHECKED); break;
        case 2: CheckDlgButton(hDlg, IDC_RADIO_MORPH2, BST_CHECKED); break;
        }
    }
}

bool SetColor(HWND hDlg, int id, BYTE* r, BYTE* g, BYTE* b) {
    CHOOSECOLOR cc;                 // common dialog box structure 
    static COLORREF acrCustClr[16]; // array of custom colors 
    bool ret;
    // Initialize CHOOSECOLOR 
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hDlg;
    cc.lpCustColors = (LPDWORD)acrCustClr;
    cc.rgbResult = RGB(*r, *g, *b);
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ret = ChooseColor(&cc))
    {
        *r = cc.rgbResult & 0xff;
        *g = cc.rgbResult >> 8 & 0xff;
        *b = cc.rgbResult >> 16 & 0xff;
        RedrawButton(hDlg, id, *r, *g, *b);
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
    //lightset* map = NULL;
    //mapping* mmap = NULL;
    //unsigned i;
    int pid = AlienFX_SDK::Functions::GetPID();
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
        s1_slider = GetDlgItem(hDlg, IDC_SPEED1),
        s2_slider = GetDlgItem(hDlg, IDC_SPEED2),
        l1_slider = GetDlgItem(hDlg, IDC_LENGTH1),
        l2_slider = GetDlgItem(hDlg, IDC_LENGTH2);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        //int pid = AlienFX_SDK::Functions::GetPID();
        size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
 
        for (int i = 0; i < lights; i++) {
            AlienFX_SDK::mapping lgh = AlienFX_SDK::Functions::GetMappings()->at(i);
            if (lgh.devid == pid) {
                int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
                SendMessage(light_list, LB_SETITEMDATA, pos, lgh.lightid);
            }
        }
        SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(l1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(l2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        if (eItem != (-1)) {
            SendMessage(light_list, LB_SETCURSEL, eItem, 0);
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
        }
    } break;
    case WM_COMMAND: {
        int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
        int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
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
                    newmap.eve[0].flags = 1;
                    newmap.eve[0].map.c1.ci = newmap.eve[0].map.c2.ci = 0;
                    conf->mappings.push_back(newmap);
                    mmap = &conf->mappings.back().eve[0].map;
                    mmap->lightset = &conf->mappings.back();
                }
                SetLightMode(hDlg, 0, mmap->mode, NULL);
                SetLightMode(hDlg, 1, mmap->mode2, NULL);
                RedrawButton(hDlg, IDC_BUTTON_C1, mmap->c1.cs.red, mmap->c1.cs.green, mmap->c1.cs.blue);
                RedrawButton(hDlg, IDC_BUTTON_C2, mmap->c2.cs.red, mmap->c2.cs.green, mmap->c2.cs.blue);
                SendMessage(s1_slider, TBM_SETPOS, true, mmap->speed1);
                SendMessage(s2_slider, TBM_SETPOS, true, mmap->speed2);
                SendMessage(l1_slider, TBM_SETPOS, true, mmap->length1);
                SendMessage(l2_slider, TBM_SETPOS, true, mmap->length2);
                break;
            } break;
        case IDC_BUTTON_C1:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (mmap != NULL) {
                    SetColor(hDlg, IDC_BUTTON_C1, &mmap->c1.cs.red, &mmap->c1.cs.green, &mmap->c1.cs.blue);
                }
            } break;
            } break;
        case IDC_BUTTON_C2:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (mmap != NULL) {
                    SetColor(hDlg, IDC_BUTTON_C2, &mmap->c2.cs.red, &mmap->c2.cs.green, &mmap->c2.cs.blue);
                }
            } break;
            } break;
        case IDC_RADIO_COLOR: case IDC_RADIO_PULSE: case IDC_RADIO_MORPH:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                if (mmap != NULL) {
                    int mid = LOWORD(wParam) - IDC_RADIO_COLOR;
                    SetLightMode(hDlg, 0, mid, mmap);
                }
                else
                    SetLightMode(hDlg, 0, 3, NULL);
                break;
            }
            break;
        case IDC_RADIO_COLOR2: case IDC_RADIO_PULSE2: case IDC_RADIO_MORPH2:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                if (mmap != NULL) {
                    int mid = LOWORD(wParam) - IDC_RADIO_COLOR2;
                    SetLightMode(hDlg, 1, mid, mmap);
                }
                else
                    SetLightMode(hDlg, 1, 3, NULL);
                break;
            }
            break;       
        default: return false;
        }
        if (mmap != NULL)
            fxhl->Refresh();
    } break;
    case WM_HSCROLL:
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
    list_status = GetDlgItem(hDlg, IDC_STATUSLIST);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
        for (int i = 0; i < lights; i++) {
            AlienFX_SDK::mapping lgh = AlienFX_SDK::Functions::GetMappings()->at(i);
            if (lgh.devid == pid) {
                int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
                SendMessage(light_list, LB_SETITEMDATA, pos, lgh.lightid);
            }
        }

        // Set counter list...
        std::string name = "CPU load";
        SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        name = "RAM load";
        SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        name = "Disk(s) load";
        SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        name = "GPU load";
        SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        name = "Network load";
        SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        name = "Max. Temperature";
        SendMessage(list_counter, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        //SendMessage(list_counter, CB_SETMINVISIBLE, 5, 0);
        // Set indicator list
        name = "HDD activity";
        SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        name = "Network activity";
        SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));
        name = "System is hot";
        SendMessage(list_status, CB_ADDSTRING, 0, (LPARAM)(name.c_str()));

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
                    newmap.eve[0].flags = 0;
                    newmap.eve[0].map.c1.ci = newmap.eve[0].map.c2.ci = 0;
                    conf->mappings.push_back(newmap);
                    mmap = &conf->mappings.back().eve[0].map;
                    mmap->lightset = map = &conf->mappings.back();
                }
                //RedrawButton(hDlg, IDC_BUTTON_C1, mmap->c1.cs.red, mmap->c1.cs.green, mmap->c1.cs.blue);
                //SendMessage(s1_slider, TBM_SETPOS, true, mmap->speed1);
                for (int i = 0; i < 4; i++) {
                    if (map->eve[i].flags)
                        CheckDlgButton(hDlg, IDC_CHECK_NOEVENT + i, BST_CHECKED);
                    else
                        CheckDlgButton(hDlg, IDC_CHECK_NOEVENT + i, BST_UNCHECKED);
                    if (i > 0)
                        RedrawButton(hDlg, IDC_BUTTON_CM1 + i - 1, map->eve[i].map.c2.cs.red, 
                            map->eve[i].map.c2.cs.green, map->eve[i].map.c2.cs.blue);
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
                map->eve[eid].flags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
        } break;
        case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3: {
            int eid = LOWORD(wParam) - IDC_BUTTON_CM1 + 1;
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                if (map != NULL) {
                    SetColor(hDlg, LOWORD(wParam), &map->eve[eid].map.c2.cs.red,
                        &map->eve[eid].map.c2.cs.green, &map->eve[eid].map.c2.cs.blue);
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
        case IDC_BUTTON_CM1: case IDC_BUTTON_CM2: case IDC_BUTTON_CM3:
            int cid = ((DRAWITEMSTRUCT*)lParam)->CtlID - IDC_BUTTON_CM1 + 1;
            Colorcode c; c.ci = 0;
            if (eItem != -1) {
                int lid = (int)SendMessage(light_list, LB_GETITEMDATA, eItem, 0);
                mapping* map = FindMapping(pid, lid, cid);
                if (map != NULL)
                    c = map->c2;
            }
            RedrawButton(hDlg, ((DRAWITEMSTRUCT*)lParam)->CtlID, c.cs.red, c.cs.green, c.cs.blue);
            break;
        }
        break;
    default: return false;
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

BOOL TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    lightset* map = NULL;
    mapping* mmap = NULL;
    unsigned i;
    int pid = AlienFX_SDK::Functions::GetPID();
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS_S),
        dev_list = GetDlgItem(hDlg, IDC_DEVICES),
        dim_slider = GetDlgItem(hDlg, IDC_SLIDER_DIMMING),
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
            }
        }

        UpdateLightList(light_list, pid);
        eItem = -1;
        // system settings...
        if (conf->startWindows) CheckDlgButton(hDlg, IDC_STARTW, BST_CHECKED);
        if (conf->startMinimized) CheckDlgButton(hDlg, IDC_STARTM, BST_CHECKED);
        if (conf->autoRefresh) CheckDlgButton(hDlg, IDC_AUTOREFRESH, BST_CHECKED);
        if (conf->dimmedBatt) CheckDlgButton(hDlg, IDC_BATTDIM, BST_CHECKED);
        SendMessage(dim_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(dim_slider, TBM_SETPOS, true, conf->dimmingPower);
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
                eItem = -1;
            } break;
            case CBN_EDITCHANGE:
                char buffer[256];
                ComboBox_GetText(dev_list, buffer, 256);
                if (did != (-1)) {
                    eDid = did; dItem = dbItem;
                }
                for (i = 0; i < AlienFX_SDK::Functions::GetDevices()->size(); i++) {
                    if (AlienFX_SDK::Functions::GetDevices()->at(i).devid == eDid) {
                        AlienFX_SDK::Functions::GetDevices()->at(i).name = buffer;
                        break;
                    }
                }
                if (i == AlienFX_SDK::Functions::GetDevices()->size() && eDid != -1) {
                    // not found, need to add...
                    AlienFX_SDK::devmap dev;
                    dev.devid = did;
                    dev.name = buffer;
                    AlienFX_SDK::Functions::GetDevices()->push_back(dev);
                }
                SendMessage(dev_list, CB_DELETESTRING, dItem, 0);
                SendMessage(dev_list, CB_INSERTSTRING, dItem, (LPARAM)(buffer));
                SendMessage(dev_list, CB_SETITEMDATA, dItem, (LPARAM)eDid);
                break;
            }
        } break;// should reload dev list
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
                // highlight to check....
                fxhl->TestLight(lid);
            } break;
            case CBN_EDITCHANGE:
                char buffer[256];
                nEdited = true;
                if (lid != (-1)) eLid = lid;
                ComboBox_GetText(light_list, buffer, 256);
                for (i = 0; i < AlienFX_SDK::Functions::GetMappings()->size(); i++) {
                    if (AlienFX_SDK::Functions::GetMappings()->at(i).devid == did &&
                        AlienFX_SDK::Functions::GetMappings()->at(i).lightid == eLid) {
                        AlienFX_SDK::Functions::GetMappings()->at(i).name = buffer;
                        break;
                    }
                }
                /*if (i == AlienFX_SDK::Functions::GetMappings()->size() && eLid != -1) {
                    // not found, need to add...
                    AlienFX_SDK::mapping dev;
                    dev.devid = did;
                    dev.lightid = lid;
                    dev.name = buffer;
                    AlienFX_SDK::Functions::GetMappings()->push_back(dev);
                }*/
                break;
            case CBN_KILLFOCUS:
                fxhl->Refresh();
                if (nEdited) {
                    UpdateLightList(light_list, did);
                    SendMessage(light_list, CB_SETCURSEL, lbItem, 0);
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
            //std::vector <AlienFX_SDK::mapping>::iterator Iter = AlienFX_SDK::Functions::GetMappings()->begin();
            for (std::vector <AlienFX_SDK::mapping>::iterator Iter = AlienFX_SDK::Functions::GetMappings()->begin();
                Iter != AlienFX_SDK::Functions::GetMappings()->end(); Iter++) {
                if (Iter->devid == did && Iter->lightid == lid) {
                    AlienFX_SDK::Functions::GetMappings()->erase(Iter);
                    // erase mappings!
                    for (std::vector <lightset>::iterator mIter = conf->mappings.begin();
                        mIter != conf->mappings.end(); mIter++)
                        if (mIter->devid == did && mIter->lightid == lid) {
                            conf->mappings.erase(mIter);
                            break;
                        }
                    break;
                }
            }
            UpdateLightList(light_list, did);
            break;
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
                fxhl->Refresh();
            } break;
        } break;
    default: return false;
    }
    return true;
}
