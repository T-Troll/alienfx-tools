#include <windows.h>
#include <powrprof.h>
#include <string>
#include "Resource.h"
#include "alienfan-SDK.h"
#include "ConfigFan.h"
#include "MonHelper.h"
#include "common.h"
#include <windowsx.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib,"PowrProf.lib")

using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance

extern AlienFan_SDK::Control* acpi;             // ACPI control object
ConfigFan* fan_conf = NULL;                     // Config...
MonHelper* mon = NULL;                          // Monitoring object

UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));
HWND mDlg = NULL, fanWindow = NULL, tipWindow = NULL;

const vector<string> pModes{ "Off", "Enabled", "Aggressive", "Efficient", "Efficient aggressive" };

extern HWND toolTip;
extern string GetFanName(int ind);

GUID* sch_guid, perfset;

bool wasBoostMode = false;

NOTIFYICONDATA niDataFC{ sizeof(NOTIFYICONDATA), 0, IDI_ALIENFANGUI, NIF_ICON | NIF_MESSAGE | NIF_TIP, WM_APP + 1,
        (HICON)LoadImage(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDI_ALIENFANGUI),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR) };
extern NOTIFYICONDATA* niData;

bool isNewVersion = false;
bool needUpdateFeedback = false;
bool needRemove = false;

// Forward declarations of functions included in this code module:
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    FanDialog(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FanCurve(HWND, UINT, WPARAM, LPARAM);

extern void ReloadFanView(HWND list);
extern void ReloadPowerList(HWND list);
extern void ReloadTempView(HWND list);
extern void TempUIEvent(NMLVDISPINFO* lParam, HWND tempList, HWND fanList);
extern void FanUIEvent(NMLISTVIEW* lParam, HWND fanList);
HWND CreateToolTip(HWND hwndParent, HWND oldTip);

extern bool fanMode;
extern HANDLE ocStopEvent;
extern DWORD WINAPI CheckFanOverboost(LPVOID lpParam);
extern DWORD WINAPI CheckFanRPM(LPVOID lpParam);

void SetHotkeys() {
    if (fan_conf->keyShortcuts) {
        //power mode hotkeys
        for (int i = 0; i < acpi->powers.size(); i++)
            RegisterHotKey(mDlg, 20 + i, MOD_CONTROL | MOD_ALT, 0x30 + i);
        RegisterHotKey(mDlg, 6, 0, VK_F17);
    }
    else
    {
        UnregisterHotKey(mDlg, 6);
        for (int i = 0; i < acpi->powers.size(); i++)
            UnregisterHotKey(mDlg, 20 + i);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    //UNREFERENCED_PARAMETER(lpCmdLine);
    //UNREFERENCED_PARAMETER(nCmdShow);

    ResetDPIScale(lpCmdLine);

    MSG msg{0};

    fan_conf = new ConfigFan();
    fan_conf->wasAWCC = DoStopService(fan_conf->awcc_disable, true);
    mon = new MonHelper();

    if (mon->monThread) {
        fan_conf->lastSelectedSensor = acpi->sensors.front().sid;
        Shell_NotifyIcon(NIM_DELETE, &niDataFC);

        hInst = hInstance;

        if (mDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN_VIEW), NULL, (DLGPROC)FanDialog)) {

            SendMessage(mDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ALIENFANGUI)));
            SendMessage(mDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFANGUI), IMAGE_ICON, 16, 16, 0));

            SetHotkeys();

            ShowWindow(mDlg, fan_conf->startMinimized ? SW_HIDE : SW_SHOW);
            ShowWindow(fanWindow, fan_conf->startMinimized ? SW_HIDE : SW_SHOWNA);

            // Main message loop:
            while ((GetMessage(&msg, 0, 0, 0)) != 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    else {
        ShowNotification(&niDataFC, "Error", "Compatible hardware not found!", false);
        WindowsStartSet(fan_conf->startWithWindows = false, "AlienFan-GUI");
        Sleep(5000);
    }
    delete mon;
    DoStopService(fan_conf->wasAWCC, false);
    Shell_NotifyIcon(NIM_DELETE, &niDataFC);

    delete fan_conf;
    return 0;
}

void StartOverboost(HWND hDlg, int fan, bool type) {
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_POWER), false);
    if (type) {
        CreateThread(NULL, 0, CheckFanOverboost, (LPVOID)(ULONG64)fan, 0, NULL);
        SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Stop check");
    }
    else
        CreateThread(NULL, 0, CheckFanRPM, (LPVOID)(ULONG64)fan, 0, NULL);
    wasBoostMode = true;
}

void RestoreApp() {
    ShowWindow(mDlg, SW_RESTORE);
    ShowWindow(fanWindow, SW_RESTORE);
    SendMessage(fanWindow, WM_PAINT, 0, 0);
    SetForegroundWindow(mDlg);
}

LRESULT CALLBACK FanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        tempList = GetDlgItem(hDlg, IDC_TEMP_LIST),
        fanList = GetDlgItem(hDlg, IDC_FAN_LIST);

    if (message == newTaskBar) {
        // Started/restarted explorer...
        AddTrayIcon(&niDataFC, fan_conf->updateCheck);
    }

    switch (message) {
    case WM_INITDIALOG:
    {
        niDataFC.hWnd = hDlg;
        niData = &niDataFC;
        AddTrayIcon(&niDataFC, fan_conf->updateCheck);

        // set PerfBoost lists...
        IIDFromString(L"{be337238-0d82-4146-a960-4f3749d470c7}", &perfset);
        PowerGetActiveScheme(NULL, &sch_guid);
        DWORD acMode, dcMode;
        PowerReadACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &acMode);
        PowerReadDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &dcMode);

        UpdateCombo(GetDlgItem(hDlg, IDC_AC_BOOST), pModes, acMode);
        UpdateCombo(GetDlgItem(hDlg, IDC_DC_BOOST), pModes, dcMode);;

        // So open fan control window...
        RECT cDlg;
        GetWindowRect(hDlg, &cDlg);
        int wh = cDlg.bottom - cDlg.top;// -2 * GetSystemMetrics(SM_CYBORDER);
        tipWindow = fanWindow = CreateWindow("STATIC", "Fan curve", WS_CAPTION | WS_POPUP | WS_SIZEBOX,//WS_OVERLAPPED,
                                 cDlg.right, cDlg.top, wh, wh,
                                 hDlg, NULL, hInst, 0);
        SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
        toolTip = CreateToolTip(fanWindow, NULL);

        ReloadPowerList(power_list);
        ReloadTempView(tempList);

        SetTimer(hDlg, 0, 500, NULL);
        SetTimer(fanWindow, 1, 500, NULL);

        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTWITHWINDOWS, fan_conf->startWithWindows ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTMINIMIZED, fan_conf->startMinimized ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_UPDATE, fan_conf->updateCheck ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(GetMenu(hDlg), IDM_DISABLEAWCC, fan_conf->awcc_disable ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_KEYBOARDSHORTCUTS, fan_conf->keyShortcuts ? MF_CHECKED : MF_UNCHECKED);

        if (!fan_conf->obCheck && MessageBox(NULL, "Do you want to set max. boost now (it will took some minutes)?", "Fan settings",
            MB_YESNO | MB_ICONQUESTION) == IDYES) {
            // ask for boost check
            StartOverboost(hDlg, -1, true);
        }
        fan_conf->obCheck = 1;

        return true;
    } break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        switch (wmId) {
        case IDC_AC_BOOST: case IDC_DC_BOOST: {
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
            {
                int cBst = ComboBox_GetCurSel(GetDlgItem(hDlg, wmId));
                if (wmId == IDC_AC_BOOST)
                    PowerWriteACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, cBst);
                else
                    PowerWriteDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, cBst);
                PowerSetActiveScheme(NULL, sch_guid);
            } break;
            }
        } break;
        case IDOK: {
            RECT rect;
            ListView_GetSubItemRect(tempList, ListView_GetSelectionMark(tempList), 1, LVIR_LABEL, &rect);
            SetWindowPos(ListView_GetEditControl(tempList), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
        } break;
        case IDC_COMBO_POWER:
        {
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
            {
                int newMode = ComboBox_GetCurSel(power_list);
                // G-Mode check
                mon->SetCurrentGmode(!(newMode < acpi->powers.size()));
                if (newMode < acpi->powers.size())
                    fan_conf->lastProf->powerStage = newMode;
            } break;
            case CBN_EDITCHANGE:
                if (!fan_conf->lastProf->gmode && fan_conf->lastProf->powerStage > 0) {
                    char buffer[MAX_PATH];
                    GetWindowText(power_list, buffer, MAX_PATH);
                    fan_conf->powers[acpi->powers[fan_conf->lastProf->powerStage]] = buffer;
                    ComboBox_DeleteString(power_list, fan_conf->lastProf->powerStage);
                    ComboBox_InsertString(power_list, fan_conf->lastProf->powerStage, buffer);
                }
                break;
            }
        } break;
        case IDC_BUT_MINIMIZE:
            SendMessage(hDlg, SW_SHOW, SIZE_MINIMIZED, 0);
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
            break;
        case IDC_BUT_CLOSE: case IDM_EXIT:
            SendMessage(hDlg, WM_CLOSE, 0, 0);
            break;
        case IDM_SAVE:
            fan_conf->Save();
            ShowNotification(&niDataFC, "Configuration saved!", "Configuration saved successfully.", true);
            break;
        case IDM_SETTINGS_STARTWITHWINDOWS:
        {
            fan_conf->startWithWindows = !fan_conf->startWithWindows;
            CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTWITHWINDOWS, fan_conf->startWithWindows ? MF_CHECKED : MF_UNCHECKED);
            WindowsStartSet(fan_conf->startWithWindows, "AlienFan-GUI");
        } break;
        case IDM_SETTINGS_STARTMINIMIZED:
        {
            fan_conf->startMinimized = !fan_conf->startMinimized;
            CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTMINIMIZED, fan_conf->startMinimized ? MF_CHECKED : MF_UNCHECKED);
        } break;
        case IDM_SETTINGS_UPDATE: {
            fan_conf->updateCheck = !fan_conf->updateCheck;
            CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_UPDATE, fan_conf->updateCheck ? MF_CHECKED : MF_UNCHECKED);
            if (fan_conf->updateCheck)
                CreateThread(NULL, 0, CUpdateCheck, &niDataFC, 0, NULL);
        } break;
        case IDM_SETTINGS_KEYBOARDSHORTCUTS:
            fan_conf->keyShortcuts = !fan_conf->keyShortcuts;
            SetHotkeys();
            CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_KEYBOARDSHORTCUTS, fan_conf->keyShortcuts ? MF_CHECKED : MF_UNCHECKED);
            break;
        case IDM_SETTINGS_DISABLEAWCC: {
            fan_conf->awcc_disable = !fan_conf->awcc_disable;
            CheckMenuItem(GetMenu(hDlg), IDM_DISABLEAWCC, fan_conf->updateCheck ? MF_CHECKED : MF_UNCHECKED);
            fan_conf->wasAWCC = DoStopService((bool)fan_conf->awcc_disable != fan_conf->wasAWCC, fan_conf->wasAWCC);
        } break;
        case IDC_BUT_RESET:
        {
            if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you want to clear all fan curves?", "Warning",
                MB_YESNO | MB_ICONWARNING) == IDYES) {
                fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].clear();
                ReloadFanView(fanList);
            }
        } break;
        case IDC_MAX_RESET:
            mon->maxTemps = mon->senValues;
            ReloadTempView(tempList);
            break;
        case IDC_BUT_OVER:
            if (fanMode) {
                StartOverboost(hDlg, fan_conf->lastSelectedFan, true);
            }
            else {
                SetEvent(ocStopEvent);
            }
            break;
        case IDC_BUT_MAXRPM:
            if (fanMode)
                StartOverboost(hDlg, fan_conf->lastSelectedFan, false);
            break;
        }
    }
    break;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            // go to tray...
            ShowWindow(fanWindow, SW_HIDE);
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
    case WM_APP + 1:
    {
        switch (lParam) {
        case WM_RBUTTONUP: case WM_CONTEXTMENU:
        {
            POINT lpClickPoint;
            HMENU tMenu = LoadMenu(hInst, MAKEINTRESOURCEA(IDR_TRAYMENU));
            tMenu = GetSubMenu(tMenu, 0);
            MENUINFO mi{ sizeof(MENUINFO), MIM_STYLE, MNS_NOTIFYBYPOS };
            SetMenuInfo(tMenu, &mi);
            MENUITEMINFO mInfo{ sizeof(MENUITEMINFO), MIIM_STRING | MIIM_ID | MIIM_STATE };
            HMENU pMenu;
            pMenu = CreatePopupMenu();
            mInfo.wID = ID_TRAYMENU_POWER_SELECTED;
            for (int i = 0; i < acpi->powers.size(); i++) {
                mInfo.dwTypeData = (LPSTR)fan_conf->powers.find(acpi->powers[i])->second.c_str();
                mInfo.fState = i == fan_conf->lastProf->powerStage ? MF_CHECKED : MF_UNCHECKED;
                InsertMenuItem(pMenu, i, false, &mInfo);
            }
            ModifyMenu(tMenu, ID_MENU_POWER, MF_BYCOMMAND | MF_STRING | MF_POPUP, (UINT_PTR)pMenu, ("Power mode - " +
                fan_conf->powers.find(acpi->powers[fan_conf->lastProf->powerStage])->second).c_str());
            EnableMenuItem(tMenu, ID_MENU_GMODE, acpi->GetDeviceFlags() & DEV_FLAG_GMODE ? MF_ENABLED : MF_DISABLED);
            CheckMenuItem(tMenu, ID_MENU_GMODE, fan_conf->lastProf->gmode ? MF_CHECKED : MF_UNCHECKED);

            GetCursorPos(&lpClickPoint);
            SetForegroundWindow(hDlg);
            int res = TrackPopupMenu(tMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
                lpClickPoint.x, lpClickPoint.y, 0, hDlg, NULL);
            int i = 0;
        } break;
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
            RestoreApp();
            break;
        case NIN_BALLOONHIDE: case NIN_BALLOONTIMEOUT:
            if (needRemove) {
                Shell_NotifyIcon(NIM_DELETE, niData);
                Shell_NotifyIcon(NIM_ADD, niData);
            }
            isNewVersion = false;
            break;
        case NIN_BALLOONUSERCLICK:
        {
            if (isNewVersion) {
                ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools/releases", NULL, NULL, SW_SHOWNORMAL);
                isNewVersion = false;
            }
        } break;
        }
        break;
    } break;
    case WM_HOTKEY: {
        if (wParam > 19 && wParam - 20 < acpi->powers.size()) {
            fan_conf->lastProf->powerStage = (WORD)wParam - 20;
            ComboBox_SetCurSel(power_list, fan_conf->lastProf->powerStage);
        }
        switch (wParam) {
        case 6: // G-key for Dell G-series power switch
            mon->SetCurrentGmode(!fan_conf->lastProf->gmode);
            ComboBox_SetCurSel(power_list, fan_conf->lastProf->gmode ? acpi->powers.size() : fan_conf->lastProf->powerStage);
            break;
        }
    } break;
    case WM_MENUCOMMAND: {
        int idx = LOWORD(wParam);
        switch (GetMenuItemID((HMENU)lParam, idx)) {
        case ID_MENU_EXIT:
            SendMessage(hDlg, WM_CLOSE, 0, 0);
            return true;
        case ID_MENU_RESTORE:
            RestoreApp();
            break;
        case ID_MENU_GMODE:
            mon->SetCurrentGmode(!fan_conf->lastProf->gmode);
            ComboBox_SetCurSel(power_list, fan_conf->lastProf->gmode ? acpi->powers.size() : fan_conf->lastProf->powerStage);
            break;
        case ID_TRAYMENU_POWER_SELECTED:
            fan_conf->lastProf->powerStage = idx;
            ComboBox_SetCurSel(power_list, fan_conf->lastProf->powerStage);
            break;
        }
    } break;
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_FAN_LIST:
            FanUIEvent((NMLISTVIEW*)lParam, fanList);
            break;
        case IDC_TEMP_LIST:
            TempUIEvent((NMLVDISPINFO*)lParam, tempList, fanList);
            break;
        } break;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;
    case WM_DESTROY:
        fan_conf->Save();
        LocalFree(sch_guid);
        PostQuitMessage(0);
        break;
    case WM_ENDSESSION:
        // Shutdown/restart scheduled....
        fan_conf->Save();
        delete mon;
        LocalFree(sch_guid);
        return 0;
    case WM_POWERBROADCAST:
        switch (wParam) {
        case PBT_APMRESUMEAUTOMATIC:
            //mon->Start();
            mon = new MonHelper();
            if (fan_conf->updateCheck) {
                needUpdateFeedback = false;
                CreateThread(NULL, 0, CUpdateCheck, &niDataFC, 0, NULL);
            }
            break;
        case PBT_APMSUSPEND:
            // Sleep initiated.
            //mon->Stop();
            delete mon;
            mon = NULL;
            fan_conf->Save();
            break;
        }
        break;
    case WM_TIMER: {
        if (fanMode && wasBoostMode) {
            EnableWindow(power_list, true);
            SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Check\n Max. boost");
            wasBoostMode = false;
        }
        if (mon) {
            if (!mon->monThread) {
                for (int i = 0; i < acpi->sensors.size(); i++) {
                    mon->senValues[acpi->sensors[i].sid] = acpi->GetTempValue(i);
                    mon->maxTemps[acpi->sensors[i].sid] = max(mon->senValues[acpi->sensors[i].sid], mon->maxTemps[acpi->sensors[i].sid]);
                }
                for (int i = 0; i < acpi->fans.size(); i++)
                    mon->fanRpm[i] = acpi->GetFanRPM(i);
            }
            if (IsWindowVisible(hDlg)) {
                //DebugPrint("Fans UI update...\n");
                for (int i = 0; i < acpi->sensors.size(); i++) {
                    string name = to_string(mon->senValues[acpi->sensors[i].sid]) + " (" + to_string(mon->maxTemps[acpi->sensors[i].sid]) + ")";
                    ListView_SetItemText(tempList, i, 0, (LPSTR)name.c_str());
                }
                RECT cArea;
                GetClientRect(tempList, &cArea);
                ListView_SetColumnWidth(tempList, 0, LVSCW_AUTOSIZE);
                ListView_SetColumnWidth(tempList, 1, cArea.right - ListView_GetColumnWidth(tempList, 0));
                for (int i = 0; i < acpi->fans.size(); i++) {
                    string name = GetFanName(i);
                    ListView_SetItemText(fanList, i, 0, (LPSTR)name.c_str());
                }
            }
            string name = "Power mode: ";
            if (fan_conf->lastProf->gmode)
                name += "G-mode";
            else
                name += fan_conf->powers[acpi->powers[fan_conf->lastProf->powerStage]];

            for (int i = 0; i < acpi->fans.size(); i++) {
                name += "\n" + GetFanName(i);
            }
            strcpy_s(niDataFC.szTip, 127, name.c_str());
            Shell_NotifyIcon(NIM_MODIFY, &niDataFC);
        }
    } break;
    default: return false;
    }
    return true;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
        SetDlgItemText(hDlg, IDC_STATIC_VERSION, ("Version: " + GetAppVersion()).c_str());
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
                ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools", NULL, NULL, SW_SHOWNORMAL);
            } break;
            } break;
        }
        break;
    }
    return (INT_PTR)FALSE;
}