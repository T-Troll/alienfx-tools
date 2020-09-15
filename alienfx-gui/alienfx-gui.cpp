// alienfx-gui.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "alienfx-gui.h"
#include <CommCtrl.h>

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
    HWND hDlg;
    // Perform application initialization:
    if (!(hDlg=InitInstance (hInstance, nCmdShow)))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALIENFXGUI));

    MSG msg;
    // Main message loop:
    while ((GetMessage(&msg, 0, 0, 0)) != 0) {
        if (!IsDialogMessage(hDlg, &msg)) {
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

BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    HWND tab_list = GetDlgItem(hDlg, IDC_TAB_MAIN);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        TCITEM tie;

        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)L"Colors";
        tie.iImage = -1;
        SendMessage(tab_list, TCM_INSERTITEM, 0, (LPARAM)&tie);
        
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
    case WM_CLOSE: //cap->Stop(); 
        DestroyWindow(hDlg); break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return false;
    }
    return true;
}