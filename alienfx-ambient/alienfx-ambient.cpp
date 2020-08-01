// alienfx-ambient.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "alienfx-ambient.h"
#include <DaramCam.h>
#pragma comment ( lib, "DaramCam.lib" )
#include <DaramCam.MediaFoundationGenerator.h>
#include "CaptureHelper.h"
#include "ConfigHandler.h"
#include <CommCtrl.h>
#include <shellapi.h>

#pragma comment ( lib, "DaramCam.MediaFoundationGenerator.lib" )
#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")
#pragma comment(lib, "ComCtl32.lib")

#define MAX_LOADSTRING 100

CaptureHelper* cap;
ConfigHandler* conf;

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

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

void SetButtonColors(UCHAR* data) {

}

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
    LoadStringW(hInstance, IDC_ALIENFXAMBIENT, szWindowClass, MAX_LOADSTRING);
    //MyRegisterClass(hInstance);

    // Perform application initialization:
    HWND hDlg;
    InitCommonControls();
    conf = new ConfigHandler();
    conf->Load();
    if (!(hDlg=InitInstance (hInstance, nCmdShow)))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALIENFXAMBIENT));

    MSG msg; bool ret;

    cap->Start();

    // Main message loop:
    /*while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }*/
    while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
        //if (ret == -1)
        //    return -1;

        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    cap->Stop();
    conf->Save();
    delete cap;
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
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXAMBIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ALIENFXAMBIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ALIENFXAMBIENT));

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
   hInst = hInstance; // Store instance handle in our global variable

   //HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   //if (!hWnd)
   //{
   //   return FALSE;
   //}

   HWND dlg;
   dlg = CreateDialogParam(hInstance,//GetModuleHandle(NULL),         /// instance handle
    	MAKEINTRESOURCE(IDD_DIALOG_MAIN),    /// dialog box template
    	NULL,                    /// handle to parent
    	(DLGPROC)DialogConfigStatic, 0);
   if (!dlg) return NULL;

   SendMessage(dlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXAMBIENT)));
   SendMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXAMBIENT), IMAGE_ICON, 16, 16, 0));

   cap = new CaptureHelper(dlg, conf);

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

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND dev_list = GetDlgItem(hDlg, IDC_DEVICE);
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
    HWND divider = GetDlgItem(hDlg, IDC_EDIT_DIVIDER);
    mapping* map = NULL;
    unsigned i;

    switch (message)
    {
    case WM_INITDIALOG:
    {
        unsigned numdev = conf->lfx->GetNumDev();
        for (i = 0; i < numdev; i++) {
            deviceinfo* dev = conf->lfx->GetDevInfo(i);
            int pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)((dev->desc)));
            SendMessage(dev_list, LB_SETITEMDATA, pos, (LPARAM)dev->id);
        }
        SendMessage(dev_list, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
        if (numdev > 0) {
            unsigned lights = conf->lfx->GetDevInfo(0)->lights;
            for (i = 0; i < lights; i++) {
                lightinfo* lgh = conf->lfx->GetLightInfo(0, i);
                int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)((lgh->desc)));
                SendMessage(light_list, LB_SETITEMDATA, pos, (LPARAM)lgh->id);
            }
        }
        // divider....
        TCHAR decay[17];
        sprintf_s(decay, 16, "%d", conf->divider);
        SendMessage(divider, WM_SETTEXT, 0, (LPARAM)decay);
        // Mode...
        if (conf->mode) {
            CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_CHECKED);
        }
        else {
            CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_UNCHECKED);
        }
    } break;
    case WM_COMMAND:
    {
        int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
        int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
        lbItem = (int)SendMessage(dev_list, CB_GETCURSEL, 0, 0);
        int did = (int)SendMessage(dev_list, CB_GETITEMDATA, lbItem, 0);
        switch (LOWORD(wParam))
        {
        case IDOK: case IDCANCEL: case IDCLOSE:
        {
            DestroyWindow(hDlg); //EndDialog(hDlg, IDOK);
        } break;
        case IDC_DEVICE: {
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE: {
                unsigned numdev = conf->lfx->GetNumDev();
                if (numdev > 0 && did < numdev) {
                    unsigned lights = conf->lfx->GetDevInfo(did)->lights;
                    SendMessage(light_list, LB_RESETCONTENT, 0, 0);
                    for (i = 0; i < lights; i++) {
                        lightinfo* lgh = conf->lfx->GetLightInfo(did, i);
                        int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(TEXT(lgh->desc)));
                        SendMessage(light_list, LB_SETITEMDATA, pos, (LPARAM)lgh->id);
                    }
                    RedrawWindow(light_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                }
            } break;
            }
        } break;// should reload dev list
        case IDC_LIGHTS: // should reload mappings
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE: {
                // check in config - do we have mappings?
                for (i = 0; i < conf->mappings.size(); i++)
                    if (conf->mappings[i].devid == did && conf->mappings[i].lightid == lid)
                        break;
                if (i < conf->mappings.size()) {
                    map = &conf->mappings[i];
                }
                else {
                    mapping newmap;
                    newmap.devid = did;
                    newmap.lightid = lid;
                    conf->mappings.push_back(newmap);
                    map = &conf->mappings[i];
                }
                // load zones....
                UINT bid = IDC_CHECK1;
                //SendMessage(freq_list, LB_SETSEL, FALSE, -1);
                // clear checks...
                for (i = 0; i < 9; i++) {
                    CheckDlgButton(hDlg, bid + i, BST_UNCHECKED);
                }
                for (int j = 0; j < map->map.size(); j++) {
                    //HWND cBid = GetDlgItem(hDlg, bid + map->map[j]);
                    //SendMessage(cBid, BM_SETSTATE, TRUE, 0);
                    //RedrawWindow(cBid, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                    CheckDlgButton(hDlg, bid+map->map[j], BST_CHECKED);
                }
                //RedrawWindow(freq_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
            } break;
        } break;
        case IDC_BUTTON1: case IDC_BUTTON2: case IDC_BUTTON3: case IDC_BUTTON4: case IDC_BUTTON5: case IDC_BUTTON6: case IDC_BUTTON7:
        case IDC_BUTTON8: case IDC_BUTTON9: {
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                UINT id = LOWORD(wParam) - IDC_BUTTON1;
                UINT bid = IDC_CHECK1 + id;
                for (i = 0; i < conf->mappings.size(); i++)
                    if (conf->mappings[i].devid == did && conf->mappings[i].lightid == lid)
                        break;
                if (i < conf->mappings.size()) {
                    map = &conf->mappings[i];
                    // add mapping
                    std::vector <unsigned char>::iterator Iter = map->map.begin();
                    for (i = 0; i < map->map.size(); i++)
                        if (map->map[i] == id)
                            break;
                        else
                            Iter++;
                    HWND cBid = GetDlgItem(hDlg, bid);
                    if (i == map->map.size()) {
                        // new mapping, add and select
                        map->map.push_back(id);
                        CheckDlgButton(hDlg, bid, BST_CHECKED);
                        //SendMessage(cBid, BM_SETSTATE, TRUE, 0);
                    }
                    else {
                        map->map.erase(Iter);
                        CheckDlgButton(hDlg, bid, BST_UNCHECKED);
                        //SendMessage(cBid, BM_SETSTATE, FALSE, 0);
                    }
                    //RedrawWindow(cBid, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                    // remove selection
                }
            } break;
            }
        } break;
        case IDC_EDIT_DIVIDER:
            switch (HIWORD(wParam)) {
            case EN_UPDATE: {
                TCHAR buffer[17]; buffer[0] = 16; buffer[1] = 0;
                if (SendMessage(divider, EM_GETLINE, 0, (LPARAM)buffer))
                    conf->divider = atoi(buffer) > 0 ? atoi(buffer) : 1;
            } break;
        } break;
        case IDC_RADIO_PRIMARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_CHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_UNCHECKED);
                conf->mode = 0;
                break;
            }
        break;
        case IDC_RADIO_SECONDARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_CHECKED);
                conf->mode = 1;
                break;
            }
            break;
        default: return false;
        }
    } break;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            // go to tray...

            ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
            niData.cbSize = sizeof(NOTIFYICONDATA);
            niData.uID = IDI_ALIENFXAMBIENT;
            niData.uFlags = NIF_ICON | NIF_MESSAGE;
            niData.hIcon =
                (HICON)LoadImage(GetModuleHandle(NULL),
                    MAKEINTRESOURCE(IDI_ALIENFXAMBIENT),
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
            Shell_NotifyIcon(NIM_DELETE, &niData);
            break;
            //case WM_RBUTTONDOWN:
            //case WM_CONTEXTMENU:
            //	ShowContextMenu(hWnd);
        }
        break;
    } break;
    case WM_CLOSE: DestroyWindow(hDlg); break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return false;
    }
    return true;
}