#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include "resource.h"
#include "CaptureHelper.h"
#include "ConfigAmbient.h"
#include "FXHelper.h"
#include "AlienFX_SDK.h"
#include "toolkit.h"

//#define MAX_LOADSTRING 100
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")

FXHelper* fxhl;
CaptureHelper* cap;
ConfigAmbient* conf;

BOOL CALLBACK AmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Global Variables:
//WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
NOTIFYICONDATA niData;

// Forward declarations of functions included in this code module:
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

    // Perform application initialization:
    HWND hDlg;

    conf = new ConfigAmbient();
    fxhl = new FXHelper(conf);

    if (!(hDlg = InitInstance(hInstance, nCmdShow)))
    {
        return FALSE;
    }

    cap = new CaptureHelper(hDlg, conf, fxhl);

    if (cap->isDirty) {
        // no capture device detected!
        MessageBox(NULL, "Can't capture you screen! Do you have DirectX installed?", "Error",
            MB_OK | MB_ICONSTOP);
    }
    else {

        MSG msg;

        RegisterPowerSettingNotification(hDlg, &GUID_MONITOR_POWER_ON, 0);

        // Main message loop:
        while ((GetMessage(&msg, 0, 0, 0)) != 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }

    fxhl->ChangeState();
    fxhl->FadeToBlack();
    delete cap;
    delete fxhl;
    delete conf;

    return 1;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND dlg;
   dlg = CreateDialogParam(hInstance,//GetModuleHandle(NULL),         /// instance handle
    	MAKEINTRESOURCE(IDD_DIALOG_MAIN),    /// dialog box template
    	NULL,                    /// handle to parent
    	(DLGPROC)AmbientDialog, 0);
   if (!dlg) return NULL;

   SendMessage(dlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXAMBIENT)));
   SendMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXAMBIENT), IMAGE_ICON, 16, 16, 0));

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
        HWND version_text = GetDlgItem(hDlg, IDC_STATIC_VERSION);
        Static_SetText(version_text, ("Version: " + GetAppVersion()).c_str());

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_NOTIFY:
        switch (LOWORD(wParam)) {
        case IDC_SYSLINK_HOMEPAGE:
            switch (((LPNMHDR)lParam)->code)
            {

            case NM_CLICK:
            case NM_RETURN:
                ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools", NULL, NULL, SW_SHOWNORMAL);
                break;
            } break;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

mapping* FindMapping(int lid) {
    if (lid != -1) {
        if (lid > 0xffff) {
            // group
            for (int i = 0; i < conf->mappings.size(); i++)
                if (conf->mappings[i].devid == 0 && conf->mappings[i].lightid == lid) {
                    return &conf->mappings[i];
                }
        } else {
            // mapping
            AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(lid);
            for (int i = 0; i < conf->mappings.size(); i++)
                if (conf->mappings[i].devid == lgh.devid && conf->mappings[i].lightid == lgh.lightid)
                    return &conf->mappings[i];
        }
    }
    return NULL;
}

HWND sTip = 0, lTip = 0;

BOOL CALLBACK AmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
    HWND brSlider = GetDlgItem(hDlg, IDC_SLIDER_BR);
    HWND divSlider = GetDlgItem(hDlg, IDC_SLIDER_DIV);
    mapping* map = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
    {

        UpdateLightList<FXHelper>(light_list, fxhl, 3);

        // Mode...
        if (conf->mode) {
            CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_CHECKED);
        }
        else {
            CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_UNCHECKED);
        }
        CheckDlgButton(hDlg, IDC_CHECK_GAMMA, conf->gammaCorrection ? BST_CHECKED : BST_UNCHECKED);
        
        SendMessage(brSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(brSlider, TBM_SETPOS, true, conf->shift);
        SendMessage(brSlider, TBM_SETTICFREQ, 16, 0);

        SendMessage(divSlider, TBM_SETRANGE, true, MAKELPARAM(0, 32));
        SendMessage(divSlider, TBM_SETPOS, true, 32-conf->divider);
        SendMessage(divSlider, TBM_SETTICFREQ, 2, 0);

        sTip = CreateToolTip(brSlider, sTip);
        lTip = CreateToolTip(divSlider, lTip);
        SetSlider(sTip, conf->shift);
        SetSlider(lTip, 32-conf->divider);

        // tray icon
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
        // check update....
        CreateThread(NULL, 0, CUpdateCheck, &niData, 0, NULL);
    } break;
    case WM_COMMAND:
    {
        int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
        int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
        switch (LOWORD(wParam))
        {
        case IDOK: case IDCANCEL: case IDCLOSE: case IDM_EXIT:
        {
            DestroyWindow(hDlg); 
        } break;
        case IDM_ABOUT: // about dialogue here
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
            break;
        case IDC_LIGHTS: // should reload mappings
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE: {
                // check in config - do we have mappings?
                map = FindMapping(lid);
                if (map == NULL) {
                    mapping newmap;
                    if (lid > 0xffff) {
                        // group
                        newmap.devid = 0;
                        newmap.lightid = lid;
                    } else {
                        // light
                        AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(lid);
                        newmap.devid = lgh.devid;
                        newmap.lightid = lgh.lightid;
                    }
                    conf->mappings.push_back(newmap);
                    std::sort(conf->mappings.begin(), conf->mappings.end(), ConfigAmbient::sortMappings);
                    map = FindMapping(lid);
                }
                if (map) {
                    // load zones....
                    UINT bid = IDC_CHECK1;
                    // clear checks...
                    for (int i = 0; i < 12; i++) {
                        CheckDlgButton(hDlg, bid + i, BST_UNCHECKED);
                    }
                    for (int j = 0; j < map->map.size(); j++) {
                        CheckDlgButton(hDlg, bid + map->map[j], BST_CHECKED);
                    }
                }
            } break;
        } break;
        case IDC_BUTTON1: case IDC_BUTTON2: case IDC_BUTTON3: case IDC_BUTTON4: case IDC_BUTTON5: case IDC_BUTTON6: case IDC_BUTTON7:
        case IDC_BUTTON8: case IDC_BUTTON9: case IDC_BUTTON10: case IDC_BUTTON11: case IDC_BUTTON12: {
            switch (HIWORD(wParam))
            {
            case BN_CLICKED: {
                UINT id = LOWORD(wParam) - IDC_BUTTON1;
                UINT bid = IDC_CHECK1 + id;
                if (lid >= 0) {
                    map = FindMapping(lid);
                    if (!map) {
                        mapping newmap;
                        if (lid > 0xffff) {
                            // group
                            newmap.devid = 0;
                            newmap.lightid = lid;
                        } else {
                            // light
                            AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(lid);
                            newmap.devid = lgh.devid;
                            newmap.lightid = lgh.lightid;
                        }
                        conf->mappings.push_back(newmap);
                        std::sort(conf->mappings.begin(), conf->mappings.end(), ConfigAmbient::sortMappings);
                        map = FindMapping(lid);
                    }
                    // add mapping
                    vector <unsigned char>::iterator Iter = map->map.begin();
                    int i = 0;
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
                    } else {
                        map->map.erase(Iter);
                        if (!map->map.size()) {
                            // delete mapping!
                            vector<mapping>::iterator mIter;
                            for (mIter = conf->mappings.begin(); mIter != conf->mappings.end(); mIter++)
                                if (mIter->devid == map->devid && mIter->lightid == map->lightid) {
                                    conf->mappings.erase(mIter);
                                    break;
                                }
                        }
                        CheckDlgButton(hDlg, bid, BST_UNCHECKED);
                    }
                }
            } break;
            }
        } break;
        case IDC_RADIO_PRIMARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_CHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_UNCHECKED);
                conf->mode = 0;
                cap->Restart();
                break;
            }
        break;
        case IDC_RADIO_SECONDARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_CHECKED);
                conf->mode = 1;
                cap->Restart();
                break;
            }
            break;
        case IDC_CHECK_GAMMA:
            conf->gammaCorrection = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
            break;
        case IDC_BUTTON_MIN:
            // go to tray...
            ShowWindow(hDlg, SW_HIDE);
            break;
        case IDC_BUTTON_RESET:
            cap->Stop();
            fxhl->FillDevs(true, false);
            UpdateLightList<FXHelper>(light_list, fxhl, 3);
            cap->Restart();
            break;
        default: return false;
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
            SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
            SetWindowPos(hDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
            break;
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                DestroyWindow(hDlg);
                break;
        }
        break;
    } break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == brSlider) {
                conf->shift = (DWORD) SendMessage(brSlider, TBM_GETPOS, 0, 0);
                SetSlider(sTip, conf->shift);
                fxhl->ChangeState();
            } else 
                if ((HWND)lParam == divSlider) {
                    conf->divider = 32 - (DWORD)SendMessage(divSlider, TBM_GETPOS, 0, 0);
                    SetSlider(lTip, 32 - conf->divider);
                    fxhl->ChangeState();
                }
            break;
        default: 
            if ((HWND)lParam == brSlider) {
                SetSlider(sTip, (int)SendMessage(brSlider, TBM_GETPOS, 0, 0));
            }
            else
                if ((HWND)lParam == divSlider) {
                    SetSlider(lTip, (int)SendMessage(divSlider, TBM_GETPOS, 0, 0));
                }
        }
        break;
    case WM_PAINT:
        if (lParam != NULL) {
            // repaint buttons from lparam
            RECT rect;
            HBRUSH Brush = NULL;
            UCHAR* imgui = (UCHAR*)lParam;
            for (int i = 0; i < 12; i++) {
                HWND tl = GetDlgItem(hDlg, IDC_BUTTON1 + i);
                HWND cBid = GetDlgItem(hDlg, IDC_CHECK1 + i);
                GetWindowRect(tl, &rect);
                HDC cnt = GetWindowDC(tl);
                rect.bottom -= rect.top;
                rect.right -= rect.left;
                rect.top = rect.left = 0;
                // BGR!
                Brush = CreateSolidBrush(RGB(imgui[i * 3 + 2], imgui[i * 3 + 1], imgui[i * 3]));
                FillRect(cnt, &rect, Brush);
                DeleteObject(Brush);
                UINT state = IsDlgButtonChecked(hDlg, IDC_CHECK1 + i);
                if ((state & BST_CHECKED))
                    DrawEdge(cnt, &rect, EDGE_SUNKEN, BF_RECT);
                else
                    DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
                RedrawWindow(cBid, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                ReleaseDC(tl, cnt);
            }
        }
        return false;
        break;
    case WM_CLOSE: DestroyWindow(hDlg); break;
    case WM_DESTROY: 
        Shell_NotifyIcon(NIM_DELETE, &niData); 
        PostQuitMessage(0); 
        break;
    case WM_POWERBROADCAST:
        switch (wParam) {
        case PBT_APMRESUMEAUTOMATIC: case PBT_APMPOWERSTATUSCHANGE:
            if (wParam == PBT_APMRESUMEAUTOMATIC) {
                //resumed from sleep
                cap->Restart();
            }
            break;
        case PBT_POWERSETTINGCHANGE:
            POWERBROADCAST_SETTING* sParams = (POWERBROADCAST_SETTING*)lParam;
            if (sParams->PowerSetting == GUID_MONITOR_POWER_ON) {
                if (sParams->Data[0] == 0) {
                    cap->Stop();
                    fxhl->FadeToBlack();
                } else
                    cap->Restart();
            }
            if (sParams->PowerSetting == GUID_SESSION_DISPLAY_STATUS) {
                cap->Restart();
            }
            break;
        }
        break;
    case WM_DISPLAYCHANGE:
        // Monitor configuration changed
        cap->Restart();
        break;
    default: return false;
    }
    return true;
}