// alienfx-gui.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "alienfx-gui.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <Commdlg.h>
#include "ConfigHandler.h"
#include "FXHelper.h"
#include "..\AlienFX-SDK\AlienFX_SDK\AlienFX_SDK.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK TabDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

FXHelper* fxhl;
ConfigHandler* conf;

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

    // Perform application initialization:
    if (!(mDlg=InitInstance (hInstance, nCmdShow)))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALIENFXGUI));

    MSG msg;
    // Main message loop:
    while ((GetMessage(&msg, 0, 0, 0)) != 0) {
        //if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        //}
    }
    
    /*while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }*/

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

    pHdr->hwndDisplay = CreateDialogIndirect(hInst,
        (DLGTEMPLATE*)pHdr->apRes[tabSel], pHdr->hwndTab, (DLGPROC)TabDialog);

    SetWindowPos(pHdr->hwndDisplay, NULL,
        pHdr->rcDisplay.left, pHdr->rcDisplay.top,
        (pHdr->rcDisplay.right - pHdr->rcDisplay.left) - cxMargin - (2 * GetSystemMetrics(SM_CXDLGFRAME)),
        (pHdr->rcDisplay.bottom - pHdr->rcDisplay.top) - cyMargin - (2 * GetSystemMetrics(SM_CYDLGFRAME)) - GetSystemMetrics(SM_CYCAPTION),
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
        int cxMargin = LOWORD(dwDlgBase) / 4;
        int cyMargin = HIWORD(dwDlgBase) / 8;
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
        case IDOK: case IDCANCEL: case IDCLOSE:
        {
            //cap->Stop();
            DestroyWindow(hDlg); //EndDialog(hDlg, IDOK);
        } break;
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
    //RedrawWindow(hDlg, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
}

void SetLightMode(HWND hDlg, int mode, mapping* map) {
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

BOOL CALLBACK TabDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    mapping* map = NULL;
    unsigned i;
    int pid = AlienFX_SDK::Functions::GetPID();
    HWND light_list = NULL;
    HWND s1_slider = NULL,
        s2_slider = NULL,
        l1_slider = NULL,
        l2_slider = NULL;
    switch (tabSel) {
    case 0:
        light_list = GetDlgItem(hDlg, IDC_LIGHTS);
        s1_slider = GetDlgItem(hDlg, IDC_SPEED1);
        s2_slider = GetDlgItem(hDlg, IDC_SPEED2);
        l1_slider = GetDlgItem(hDlg, IDC_LENGTH1);
        l2_slider = GetDlgItem(hDlg, IDC_LENGTH2);
        break;
    }

    switch (message)
    {
    case WM_INITDIALOG:
    {
        //int pid = AlienFX_SDK::Functions::GetPID();
        size_t lights = AlienFX_SDK::Functions::GetMappings()->size();
        switch (tabSel) {
        case 0: // Colors
            for (i = 0; i < lights; i++) {
                AlienFX_SDK::mapping lgh = AlienFX_SDK::Functions::GetMappings()->at(i);
                if (lgh.devid == pid) {
                    int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(lgh.name.c_str()));
                    SendMessage(light_list, LB_SETITEMDATA, pos, (LPARAM)TEXT(lgh.lightid));
                }
            }
            RedrawButton(hDlg, IDC_BUTTON_C1, 0, 0, 0);
            RedrawButton(hDlg, IDC_BUTTON_C2, 0, 0, 0);
            SendMessage(s1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
            SendMessage(s2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
            SendMessage(l1_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
            SendMessage(l2_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
            break;
        case 1: // events
            break;
        case 2: // settings
            break;
        }
    } break;
    case WM_COMMAND: {
        int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
        int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
        for (i = 0; i < conf->mappings.size(); i++)
            if (conf->mappings[i].devid == pid && conf->mappings[i].lightid == lid)
                break;
        switch (LOWORD(wParam))
        {
        case IDC_LIGHTS: // should reload mappings
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                if (i < conf->mappings.size())
                    map = &conf->mappings[i];
                else {
                    mapping newmap;
                    newmap.devid = pid;
                    newmap.lightid = lid;
                    conf->mappings.push_back(newmap);
                    map = &conf->mappings[i];
                }
                SetLightMode(hDlg, map->mode, map);
                RedrawButton(hDlg, IDC_BUTTON_C1, map->c1.cs.red, map->c1.cs.green, map->c1.cs.blue);
                RedrawButton(hDlg, IDC_BUTTON_C2, map->c2.cs.red, map->c2.cs.green, map->c2.cs.blue);
                SendMessage(s1_slider, TBM_SETPOS, true, map->speed1);
                SendMessage(s2_slider, TBM_SETPOS, true, map->speed2);
                SendMessage(l1_slider, TBM_SETPOS, true, map->length1);
                SendMessage(l2_slider, TBM_SETPOS, true, map->length2);
                break;
            } break;
        case IDC_BUTTON_C1:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (i < conf->mappings.size()) {
                    map = &conf->mappings[i];
                    CHOOSECOLOR cc;                 // common dialog box structure 
                    static COLORREF acrCustClr[16]; // array of custom colors 

                    // Initialize CHOOSECOLOR 
                    ZeroMemory(&cc, sizeof(cc));
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = mDlg;
                    cc.lpCustColors = (LPDWORD)acrCustClr;
                    cc.rgbResult = RGB(map->c1.cs.red, map->c1.cs.green, map->c1.cs.blue);
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc) == TRUE)
                    {
                        //BYTE br = map->colorfrom.cs.brightness;
                        map->c1.cs.red = cc.rgbResult & 0xff;
                        map->c1.cs.green = cc.rgbResult >> 8 & 0xff;
                        map->c1.cs.blue = cc.rgbResult >> 16 & 0xff;
                        //map->c1.cs.brightness = br;
                        //unsigned clrmap = MAKEIPADDRESS(map->colorfrom.cs.red, map->colorfrom.cs.green, map->colorfrom.cs.blue, map->colorfrom.cs.brightness);
                 //       SendMessage(from_color, IPM_SETADDRESS, 0, clrmap);
                 //       RedrawWindow(from_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                        RedrawButton(hDlg, IDC_BUTTON_C1, cc.rgbResult & 0xff, cc.rgbResult >> 8 & 0xff, cc.rgbResult >> 16 & 0xff);
                        //rgbCurrent = cc.rgbResult;
                    }
                }
            } break;
            } break;
        case IDC_BUTTON_C2:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                if (i < conf->mappings.size()) {
                    map = &conf->mappings[i];
                    CHOOSECOLOR cc;                 // common dialog box structure 
                    static COLORREF acrCustClr[16]; // array of custom colors 
                    //HWND hwnd;                      // owner window
                    //HBRUSH hbrush;                  // brush handle

                    // Initialize CHOOSECOLOR 
                    ZeroMemory(&cc, sizeof(cc));
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = mDlg;
                    cc.lpCustColors = (LPDWORD)acrCustClr;
                    cc.rgbResult = RGB(map->c2.cs.red, map->c2.cs.green, map->c2.cs.blue);
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc) == TRUE)
                    {
                        map->c2.cs.red = cc.rgbResult & 0xff;
                        map->c2.cs.green = cc.rgbResult >> 8 & 0xff;
                        map->c2.cs.blue = cc.rgbResult >> 16 & 0xff;
                        //unsigned clrmap = MAKEIPADDRESS(map->colorto.cs.red, map->colorto.cs.green, map->colorto.cs.blue, map->colorto.cs.brightness);
                 //       SendMessage(to_color, IPM_SETADDRESS, 0, clrmap);
                 //       RedrawWindow(to_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                        RedrawButton(hDlg, IDC_BUTTON_C2, cc.rgbResult & 0xff, cc.rgbResult >> 8 & 0xff, cc.rgbResult >> 16 & 0xff);
                        //rgbCurrent = cc.rgbResult;
                    }
                }
            } break;
            } break;
        case IDC_RADIO_COLOR: case IDC_RADIO_PULSE: case IDC_RADIO_MORPH:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                if (i < conf->mappings.size()) {
                    map = &conf->mappings[i];
                    int mid = LOWORD(wParam) - IDC_RADIO_COLOR;
                    SetLightMode(hDlg, mid, map);
                }
                else
                    SetLightMode(hDlg, 3, NULL);
                break;
            }
            break;
        default: return false;
        }
        fxhl->UpdateLight(i);
    } break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBTRACK: case TB_ENDTRACK:
            int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
            int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
            for (i = 0; i < conf->mappings.size(); i++)
                if (conf->mappings[i].devid == pid && conf->mappings[i].lightid == lid)
                    break;
            if (i < conf->mappings.size()) {
                map = &conf->mappings[i];
                if ((HWND)lParam == s1_slider) {
                    map->speed1 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == s2_slider) {
                    map->speed2 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == l1_slider) {
                    map->length1 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                if ((HWND)lParam == l2_slider) {
                    map->length2 = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                }
                fxhl->UpdateLight(i);
            }
            //if (i < conf->mappings.size())
            //    map->speed1 = (DWORD)SendMessage(s1_slider, TBM_GETPOS, 0, 0);
            break;
        }
        break;
    default: return false;
    }
    return true;
}