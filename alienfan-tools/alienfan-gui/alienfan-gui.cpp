// alienfan-gui.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include "alienfan-gui.h"
#include <windowsx.h>
#include <winuser.h>
#include <CommCtrl.h>
#include <string>
#include <wininet.h>
//#include "KDL.h"
#include "alienfan-SDK.h"
#include "ConfigHelper.h"
#include "MonHelper.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"Version.lib")
#pragma comment(lib,"Wininet.lib")

using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

AlienFan_SDK::Control* acpi = NULL;             // ACPI control object
ConfigHelper* fan_conf = NULL;                      // Config...
MonHelper* mon = NULL;                          // Monitoring & changer object

fan_point* lastFanPoint = NULL;
UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));
HWND toolTip = NULL;
HWND fanWindow = NULL;

// Forward declarations of functions included in this code module:
//ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FanCurve(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg = {0};

    fan_conf = new ConfigHelper();
    fan_conf->Load();

    acpi = new AlienFan_SDK::Control();

    if (acpi->IsActivated()) {
        if (acpi->Probe()) {

            // Perform application initialization:
            HWND mDlg;
            if (!(mDlg = InitInstance(hInstance, nCmdShow))) {
                return FALSE;
            }

            // minimize if needed
            if (fan_conf->startMinimized)
                SendMessage(mDlg, WM_SIZE, SIZE_MINIMIZED, 0);

            if (fan_conf->lastProf->powerStage >= 0)
                acpi->SetPower(fan_conf->lastProf->powerStage);

            if (fan_conf->lastProf->GPUPower >= 0)
                acpi->SetGPU(fan_conf->lastProf->GPUPower);

            mon = new MonHelper(mDlg, fanWindow, fan_conf, acpi);

            HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAIN_ACC));

            // Main message loop:
            while ((GetMessage(&msg, 0, 0, 0)) != 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            delete mon;
        } else
            MessageBox(NULL, "Supported hardware not detected!", "Fatal error",
                       MB_OK | MB_ICONSTOP);
    } else {
        MessageBox(NULL, "Can't install driver, check system configuration!", "Fatal error",
                   MB_OK | MB_ICONSTOP);
    }

    delete acpi;
    delete fan_conf;

    return (int) msg.wParam;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND dlg;
    dlg = CreateDialogParam(hInstance,//GetModuleHandle(NULL),         /// instance handle
                            MAKEINTRESOURCE(IDD_MAIN_VIEW),    /// dialog box template
                            NULL,                    /// handle to parent
                            (DLGPROC)WndProc, 0);
    if (!dlg) return NULL;

    SendMessage(dlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFANGUI)));
    SendMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFANGUI), IMAGE_ICON, 16, 16, 0));

    ShowWindow(dlg, nCmdShow);

    return dlg;
}

HWND CreateToolTip(HWND hwndParent, HWND oldTip)
{
    // Create a tooltip.
    if (oldTip) {
        DestroyWindow(oldTip);
    } 
    HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
                                 WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 hwndParent, NULL, hInst, NULL);

    SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    TOOLINFO ti = { 0 };
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hwndParent;
    ti.hinst = hInst;
    ti.lpszText = (LPTSTR)"0";

    GetClientRect(hwndParent, &ti.rect);

    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    return hwndTT;
}

void SetTooltip(HWND tt, int x, int y) {
    TOOLINFO ti = { 0 };
    ti.cbSize = sizeof(ti);
    if (tt) {
        SendMessage(tt, TTM_ENUMTOOLS, 0, (LPARAM)&ti);
        string toolTip = "Temp: " + to_string(x) + ", Boost: " + to_string(y);
        ti.lpszText = (LPTSTR) toolTip.c_str();
        SendMessage(tt, TTM_SETTOOLINFO, 0, (LPARAM)&ti);
    }
}

void DrawFan(int oper = 0, int xx=-1, int yy=-1)
{
    if (fanWindow) {
        RECT clirect, graphZone;
        GetClientRect(fanWindow, &clirect);
        graphZone = clirect;
        clirect.right -= 1;
        clirect.bottom -= 1;

        switch (oper) {
        case 2:// tooltip, no redraw
            SetTooltip(toolTip, 100 * (xx - clirect.left) / (clirect.right - clirect.left),
                       100 * (clirect.bottom - yy) / (clirect.bottom - clirect.top));
            return;
        case 1:// show tooltip
            SetTooltip(toolTip, 100 * (xx - clirect.left) / (clirect.right - clirect.left),
                       100 * (clirect.bottom - yy) / (clirect.bottom - clirect.top));
            break;
        }

        HDC hdc_r = GetDC(fanWindow);

        // Double buff...
        HDC hdc = CreateCompatibleDC(hdc_r);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, graphZone.right - graphZone.left, graphZone.bottom - graphZone.top);

        SetBkMode(hdc, TRANSPARENT);

        HGDIOBJ hOld = SelectObject(hdc, hbmMem);

        SetDCPenColor(hdc, RGB(127, 127, 127));
        SelectObject(hdc, GetStockObject(DC_PEN));
        for (int x = 0; x < 11; x++)
            for (int y = 0; y < 11; y++) {
                int cx = x * (clirect.right - clirect.left) / 10 + clirect.left,
                    cy = y * (clirect.bottom - clirect.top) / 10 + clirect.top;
                MoveToEx(hdc, cx, clirect.top, NULL);
                LineTo(hdc, cx, clirect.bottom);
                MoveToEx(hdc, clirect.left, cy, NULL);
                LineTo(hdc, clirect.right, cy);
            }

        if (fan_conf->lastSelectedFan != -1) {
            // curve...
            temp_block* sen = NULL;
            fan_block* fan = NULL;
            if ((sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor)) && (fan = fan_conf->FindFanBlock(sen, fan_conf->lastSelectedFan))) {
                // draw fan curve
                SetDCPenColor(hdc, RGB(0, 255, 0));
                SelectObject(hdc, GetStockObject(DC_PEN));
                // First point
                MoveToEx(hdc, clirect.left, clirect.bottom, NULL);
                for (int i = 0; i < fan->points.size(); i++) {
                    int cx = fan->points[i].temp * (clirect.right - clirect.left) / 100 + clirect.left,
                        cy = (100 - fan->points[i].boost) * (clirect.bottom - clirect.top) / 100 + clirect.top;
                    LineTo(hdc, cx, cy);
                    Ellipse(hdc, cx - 2, cy - 2, cx + 2, cy + 2);
                }
                // Red dot and RPM
                SetDCPenColor(hdc, RGB(255, 0, 0));
                SetDCBrushColor(hdc, RGB(255, 0, 0));
                SelectObject(hdc, GetStockObject(DC_PEN));
                SelectObject(hdc, GetStockObject(DC_BRUSH));
                POINT mark;
                mark.x = acpi->GetTempValue(fan_conf->lastSelectedSensor) * (clirect.right - clirect.left) / 100 + clirect.left;
                mark.y = (100 - acpi->GetFanValue(fan_conf->lastSelectedFan)) * (clirect.bottom - clirect.top) / 100 + clirect.top;
                Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
                string rpmText = "Fan curve (boost: " + to_string(acpi->GetFanValue(fan_conf->lastSelectedFan)) + ")";
                SetWindowText(fanWindow, rpmText.c_str());
            }
        }

        BitBlt(hdc_r, 0, 0, graphZone.right - graphZone.left, graphZone.bottom - graphZone.top, hdc, 0, 0, SRCCOPY);

        // Free-up the off-screen DC
        SelectObject(hdc, hOld);

        DeleteObject(hbmMem);
        DeleteDC(hdc);
        ReleaseDC(fanWindow, hdc_r);
        DeleteDC(hdc_r);
    }
} 

void ReloadFanView(HWND hDlg, int cID) {
    temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
    HWND list = GetDlgItem(hDlg, IDC_FAN_LIST);
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES /*| LVS_EX_AUTOSIZECOLUMNS*/ | LVS_EX_FULLROWSELECT);
    LVCOLUMNA lCol;
    lCol.mask = LVCF_WIDTH;
    lCol.cx = 100;
    lCol.iSubItem = 0;
    ListView_DeleteColumn(list, 0);
    ListView_InsertColumn(list, 0, &lCol);
    for (int i = 0; i < acpi->HowManyFans(); i++) {
        LVITEMA lItem;
        string name = "Fan " + to_string(i + 1) + " (" + to_string(acpi->GetFanRPM(i)) + ")";
        lItem.mask = LVIF_TEXT | LVIF_PARAM;
        lItem.iItem = i;
        lItem.iImage = 0;
        lItem.iSubItem = 0;
        lItem.lParam = i;
        lItem.pszText = (LPSTR) name.c_str();
        if (i == cID) {
            lItem.mask |= LVIF_STATE;
            lItem.state = LVIS_SELECTED;
            DrawFan();
        }
        ListView_InsertItem(list, &lItem);
        if (sen && fan_conf->FindFanBlock(sen, i)) {
            fan_conf->lastSelectedSensor = -1;
            ListView_SetCheckState(list, i, true);
            fan_conf->lastSelectedSensor = sen->sensorIndex;
        }
    }

    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
}


void ReloadPowerList(HWND hDlg, int id) {
    HWND list = GetDlgItem(hDlg, IDC_COMBO_POWER);
    ComboBox_ResetContent(list);
    for (int i = 0; i < acpi->HowManyPower(); i++) {
        string name;
        if (i)
            name = "Level " + to_string(i);
        else
            name = "Manual";
        int pos = ComboBox_AddString(list, (LPARAM)(name.c_str()));
        ComboBox_SetItemData(list, pos, i);
        if (i == id)
            ComboBox_SetCurSel(list, pos);
    }
}

void ReloadTempView(HWND hDlg, int cID) {
    int rpos = 0;
    HWND list = GetDlgItem(hDlg, IDC_TEMP_LIST);
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    LVCOLUMNA lCol;
    lCol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lCol.cx = 100;
    lCol.iSubItem = 0;
    lCol.pszText = (LPSTR) "T";
    //ListView_DeleteColumn(list, 0);
    ListView_InsertColumn(list, 0, &lCol);
    lCol.pszText = (LPSTR) "Name";
    lCol.iSubItem = 1;
    ListView_InsertColumn(list, 1, &lCol);
    for (int i = 0; i < acpi->HowManySensors(); i++) {
        LVITEMA lItem;
        string name = to_string(acpi->GetTempValue(i));
        lItem.mask = LVIF_TEXT | LVIF_PARAM;
        lItem.iItem = i;
        lItem.iImage = 0;
        lItem.iSubItem = 0;
        lItem.lParam = i;
        lItem.pszText = (LPSTR) name.c_str();
        if (i == cID) {
            lItem.mask |= LVIF_STATE;
            lItem.state = LVIS_SELECTED;
            // TODO: check selected fans here
            rpos = i;
        }
        ListView_InsertItem(list, &lItem);
        //lItem.pszText = (LPSTR) acpi->sensors[i].name.c_str();
        ListView_SetItemText(list, i, 1, (LPSTR) acpi->sensors[i].name.c_str());
    }
    //RECT csize;
    //GetClientRect(profile_list, &csize);
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);// csize.right - csize.left - 1);
    ListView_SetColumnWidth(list, 1, LVSCW_AUTOSIZE_USEHEADER);
    ListView_EnsureVisible(list, rpos, false);
    //return rpos;
}

string GetAppVersion();

DWORD WINAPI CUpdateCheck(LPVOID lparam) {
    NOTIFYICONDATA* niData = (NOTIFYICONDATA*) lparam;
    HINTERNET session, req;
    char buf[2048];
    DWORD byteRead;
    if (session = InternetOpen("alienfx-tools", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) {
        if (req = InternetOpenUrl(session, "https://api.github.com/repos/t-troll/alienfan-tools/tags?per_page=1",
                                  NULL, 0, 0, NULL)) {
            if (InternetReadFile(req, buf, 2047, &byteRead)) {
                buf[byteRead] = 0;
                string res = buf;
                size_t pos = res.find("\"name\":"), 
                    posf = res.find("\"", pos + 8);
                res = res.substr(pos + 8, posf - pos - 8);
                size_t dotpos = res.find(".", 1+ res.find(".", 1 + res.find(".")));
                if (res.find(".", 1+ res.find(".", 1 + res.find("."))) == string::npos)
                    res += ".0";
                if (res != GetAppVersion()) {
                    // new version detected!
                    niData->uFlags |= NIF_INFO;
                    strcpy_s(niData->szInfoTitle, "Update avaliable!");
                    strcpy_s(niData->szInfo, ("Version " + res + " released at GitHub.").c_str());
                    Shell_NotifyIcon(NIM_MODIFY, niData);
                    niData->uFlags &= ~NIF_INFO;
                }
            }
            InternetCloseHandle(req);
        }
        InternetCloseHandle(session);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        power_gpu = GetDlgItem(hDlg, IDC_SLIDER_GPU);
    if (message == newTaskBar) {
        // Started/restarted explorer...
        Shell_NotifyIcon(NIM_ADD, &fan_conf->niData);
        CreateThread(NULL, 0, CUpdateCheck, &fan_conf->niData, 0, NULL);
        return true;
    }

    switch (message)
    {
    case WM_INITDIALOG:
    {
        fan_conf->niData.hWnd = hDlg;
        Shell_NotifyIcon(NIM_ADD, &fan_conf->niData);
        CreateThread(NULL, 0, CUpdateCheck, &fan_conf->niData, 0, NULL);

        ReloadPowerList(hDlg, fan_conf->lastProf->powerStage);
        ReloadTempView(hDlg, fan_conf->lastSelectedSensor);
        ReloadFanView(hDlg, fan_conf->lastSelectedFan);

        // So open fan control window...
        RECT cDlg;
        GetWindowRect(hDlg, &cDlg);
        int wh = cDlg.bottom - cDlg.top;// -2 * GetSystemMetrics(SM_CYBORDER);
        fanWindow = CreateWindow("STATIC", "Fan curve", WS_CAPTION | WS_POPUP,//WS_OVERLAPPED,
                                        cDlg.right, cDlg.top, wh, wh, 
                                        hDlg, NULL, hInst, 0);
        SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
        toolTip = CreateToolTip(fanWindow, NULL);
        if (fan_conf->startMinimized) {
            ShowWindow(fanWindow, SW_HIDE);
        } else {
            ShowWindow(fanWindow, SW_SHOWNA);
        }

        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTWITHWINDOWS, fan_conf->startWithWindows ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTMINIMIZED, fan_conf->startMinimized ? MF_CHECKED : MF_UNCHECKED);

        SendMessage(power_gpu, TBM_SETRANGE, true, MAKELPARAM(0, 4));
        SendMessage(power_gpu, TBM_SETTICFREQ, 1, 0);
        SendMessage(power_gpu, TBM_SETPOS, true, fan_conf->lastProf->GPUPower);

        return true; 
    } break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDC_COMBO_POWER: {
                int pItem = ComboBox_GetCurSel(power_list);
                int pid = (int)ComboBox_GetItemData(power_list, pItem);
                switch (HIWORD(wParam))
                {
                case CBN_SELCHANGE: {
                    fan_conf->lastProf->powerStage = pid;
                    acpi->SetPower(pid);
                    fan_conf->Save();
                } break;
                }
            } break;
            case IDC_BUT_MINIMIZE:
                ShowWindow(fanWindow, SW_HIDE);
                ShowWindow(hDlg, SW_HIDE);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
                break;
            case IDC_BUT_CLOSE: case IDM_EXIT:
                SendMessage(hDlg, WM_CLOSE, 0, 0);
                break;
            case IDM_SETTINGS_STARTWITHWINDOWS:
            {
                fan_conf->startWithWindows = !fan_conf->startWithWindows;
                CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTWITHWINDOWS, fan_conf->startWithWindows ? MF_CHECKED : MF_UNCHECKED);
                char pathBuffer[MAX_PATH];
                string shellcomm;
                if (fan_conf->startWithWindows) {
                    GetModuleFileName(NULL, pathBuffer, MAX_PATH);
                    shellcomm = "Register-ScheduledTask -TaskName \"AlienFan-GUI\" -trigger $(New-ScheduledTaskTrigger -Atlogon) -settings $(New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -ExecutionTimeLimit 0) -action $(New-ScheduledTaskAction -Execute '"
                        + string(pathBuffer) + "') -force -RunLevel Highest";
                    ShellExecute(NULL, "runas", "powershell.exe", shellcomm.c_str(), NULL, SW_HIDE);
                } else {
                    shellcomm = "/delete /F /TN \"AlienFan-GUI\"";
                    ShellExecute(NULL, "runas", "schtasks.exe", shellcomm.c_str(), NULL, SW_HIDE);
                }
                fan_conf->Save();
            } break;
            case IDM_SETTINGS_STARTMINIMIZED:
            {
                fan_conf->startMinimized = !fan_conf->startMinimized;
                CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTMINIMIZED, fan_conf->startMinimized ? MF_CHECKED : MF_UNCHECKED);
                fan_conf->Save();
            } break;
            case IDC_BUT_RESET:
            {
                temp_block* cur = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
                if (cur) {
                    fan_block* fan = fan_conf->FindFanBlock(cur, fan_conf->lastSelectedFan);
                    if (fan) {
                        fan->points.clear();
                        fan->points.push_back({0,0});
                        fan->points.push_back({100,0});
                        DrawFan();
                    }
                }
            } break;
            default:
                return DefWindowProc(hDlg, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            // go to tray...
            ShowWindow(hDlg, SW_HIDE);
        }
        break;
    case WM_MOVE:
    {
        // Reposition child...
        RECT cDlg;
        GetWindowRect(hDlg, &cDlg);
        SetWindowPos(fanWindow, hDlg, cDlg.right, cDlg.top, 0, 0, SWP_NOSIZE | SWP_NOREDRAW | SWP_NOACTIVATE);
    } break;
    case WM_APP + 1: {
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
            ShowWindow(fanWindow, SW_RESTORE);
            DrawFan();
            ShowWindow(hDlg, SW_RESTORE);
            SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
            SetWindowPos(hDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
            break;
        case WM_RBUTTONUP: case WM_CONTEXTMENU: {
            SendMessage(hDlg, WM_CLOSE, 0, 0);
        } break;
        case NIN_BALLOONUSERCLICK:
        {
            ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfan-tools/releases", NULL, NULL, SW_SHOWNORMAL);
        } break;
        }
        break;
    } break;
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_FAN_LIST:
            switch (((NMHDR*) lParam)->code) {
            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
                //if (lPoint->uNewState & LVIS_STATEIMAGEMASK) {
                //    int i = 0;
                //}
                if (lPoint->uNewState & LVIS_FOCUSED) {
                    // Select other item...
                    if (lPoint->iItem != -1) {
                        // Select other fan....
                        fan_conf->lastSelectedFan = lPoint->iItem;
                        // Redraw fans
                        DrawFan();
                    }
                }
                if (lPoint->uNewState & 0x2000) { // checked, 0x1000 - unchecked
                    if (fan_conf->lastSelectedSensor != -1) {
                        temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
                        if (!sen) { // add new sensor block
                            sen = new temp_block;
                            sen->sensorIndex = (short)fan_conf->lastSelectedSensor;
                            fan_conf->lastProf->fanControls.push_back(*sen);
                            delete sen;
                            sen = &fan_conf->lastProf->fanControls.back();
                        }
                        fan_block cFan = {(short)lPoint->iItem};
                        cFan.points.push_back({0,0});
                        cFan.points.push_back({100,0});
                        sen->fans.push_back(cFan);
                        DrawFan();
                    }
                }
                if (lPoint->uNewState & 0x1000 && lPoint->uOldState && 0x2000) { // unchecked
                    if (fan_conf->lastSelectedSensor != -1) {
                        temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
                        if (sen) { // remove sensor block
                            for (vector<fan_block>::iterator iFan = sen->fans.begin();
                                 iFan < sen->fans.end(); iFan++)
                                if (iFan->fanIndex == lPoint->iItem) {
                                    sen->fans.erase(iFan);
                                    break;
                                }
                            if (!sen->fans.size()) // remove sensor block!
                                for (vector<temp_block>::iterator iSen = fan_conf->lastProf->fanControls.begin();
                                     iSen < fan_conf->lastProf->fanControls.end(); iSen++)
                                    if (iSen->sensorIndex == sen->sensorIndex) {
                                        fan_conf->lastProf->fanControls.erase(iSen);
                                        break;
                                    }
                        }
                        DrawFan();
                    }
                }
            } break;
            }
            break;
        case IDC_TEMP_LIST:
            switch (((NMHDR*) lParam)->code) {
            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
                if (lPoint->uNewState & LVIS_FOCUSED) {
                    // Select other item...
                    if (lPoint->iItem != -1) {
                        // Select other fan....
                        fan_conf->lastSelectedSensor = lPoint->iItem;
                        // Redraw fans
                        ReloadFanView(hDlg, fan_conf->lastSelectedFan);
                        DrawFan();
                    }
                }
            } break;
            }
            break;
        }
        break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK: {
            if ((HWND)lParam == power_gpu) {
                fan_conf->lastProf->GPUPower = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                acpi->SetGPU(fan_conf->lastProf->GPUPower);
            }
         } break;
        } break;
    case WM_CLOSE:
        EndDialog(hDlg, IDOK);
        mon->Stop();
        Shell_NotifyIcon(NIM_DELETE, &fan_conf->niData);
        DestroyWindow(hDlg);
        break;
    case WM_DESTROY:
        fan_conf->Save();
        PostQuitMessage(0);
        break;
    }
    return 0;
}

string GetAppVersion() {

    HRSRC hResInfo;
    DWORD dwSize;
    HGLOBAL hResData;
    LPVOID pRes, pResCopy;
    UINT uLen;
    VS_FIXEDFILEINFO* lpFfi;

    std::string res = "";

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

                DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
                DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

                res = std::to_string(HIWORD(dwFileVersionMS)) + "."
                    + std::to_string(LOWORD(dwFileVersionMS)) + "."
                    + std::to_string(HIWORD(dwFileVersionLS)) + "."
                    + std::to_string(LOWORD(dwFileVersionLS));

                LocalFree(pResCopy);
            }
            FreeResource(hResData);
        }
    }
    return res;
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
                ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfan-tools", NULL, NULL, SW_SHOWNORMAL);
                break;
            } break;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

#define DRAG_ZONE 2

INT_PTR CALLBACK FanCurve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    fan_block* cFan = fan_conf->FindFanBlock(fan_conf->FindSensor(fan_conf->lastSelectedSensor), fan_conf->lastSelectedFan);
    RECT cArea;
    GetClientRect(hDlg, &cArea);
    cArea.right -= 1;
    cArea.bottom -= 1;

    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);      
        DrawFan();
        EndPaint(hDlg, &ps);
        return true;
    } break;
    case WM_MOUSEMOVE: { 
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

        if (lastFanPoint && wParam & MK_LBUTTON) {
            int temp = (100 * (GET_X_LPARAM(lParam) - cArea.left)) / (cArea.right - cArea.left),
                boost = (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / (cArea.bottom - cArea.top);
            lastFanPoint->temp = temp < 0 ? 0 : temp > 100 ? 100 : temp;
            lastFanPoint->boost = boost < 0 ? 0 : boost > 100 ? 100 : boost;
            DrawFan(1, x, y);
        } else
            DrawFan(2, x, y);
    } break;
    case WM_LBUTTONDOWN:
    {
        SetCapture(hDlg);
        if (cFan) {
            // check and add point
            int temp = (100 * (GET_X_LPARAM(lParam) - cArea.left)) / (cArea.right - cArea.left),
                boost = (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / (cArea.bottom - cArea.top);
            for (vector<fan_point>::iterator fPi = cFan->points.begin();
                 fPi < cFan->points.end(); fPi++) {
                if (fPi->temp - DRAG_ZONE <= temp && fPi->temp + DRAG_ZONE >= temp) {
                    // Starting drag'n'drop...
                    lastFanPoint = &(*fPi);
                    break;
                }
                if (fPi->temp > temp) {
                    // Insert point here...
                    lastFanPoint = &(*cFan->points.insert(fPi, {(short)temp, (short)boost}));
                    break;
                }
            }
            DrawFan();
        }
    } break;
    case WM_LBUTTONUP: {
        ReleaseCapture();
        // re-sort array.
        if (cFan) {
            for (vector<fan_point>::iterator fPi = cFan->points.begin();
                 fPi < cFan->points.end() - 1; fPi++)
                if (fPi->temp > (fPi + 1)->temp) {
                    fan_point t = *fPi;
                    *fPi = *(fPi + 1);
                    *(fPi + 1) = t;
                }
            cFan->points.front().temp = 0;
            cFan->points.back().temp = 100;
            DrawFan();
            fan_conf->Save();
        }
        SetFocus(GetParent(hDlg));
    } break;
    case WM_RBUTTONUP: {
        // remove point from curve...
        if (cFan && cFan->points.size() > 2) {
            // check and remove point
            int temp = (100 * (GET_X_LPARAM(lParam) - cArea.left)) / (cArea.right - cArea.left),
                boost = (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / (cArea.bottom - cArea.top);
            for (vector<fan_point>::iterator fPi = cFan->points.begin() + 1;
                 fPi < cFan->points.end() - 1; fPi++)
                if (fPi->temp - DRAG_ZONE <= temp && fPi->temp + DRAG_ZONE >= temp ) {
                    // Remove this element...
                    cFan->points.erase(fPi);
                    break;
                }
            DrawFan();
            fan_conf->Save();
        }
        SetFocus(GetParent(hDlg));
    } break;
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_ERASEBKGND:
        return true;
        break;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}
