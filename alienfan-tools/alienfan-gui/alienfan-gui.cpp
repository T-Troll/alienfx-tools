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
#pragma comment(lib,"Version.lib")
#pragma comment(lib, "PowrProf.lib")

using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

AlienFan_SDK::Control* acpi = NULL;             // ACPI control object
ConfigFan* fan_conf = NULL;                     // Config...
MonHelper* mon = NULL;                          // Monitoring object

UINT newTaskBar = RegisterWindowMessage(TEXT("TaskbarCreated"));
HWND mDlg = NULL, fanWindow = NULL, tipWindow = NULL;

static vector<string> pModes{ "Off", "Enabled", "Aggressive", "Efficient", "Efficient aggressive" };

extern HWND toolTip;

int pLid = -1;

GUID* sch_guid, perfset;

NOTIFYICONDATA* niData = NULL;

bool isNewVersion = false;
bool needUpdateFeedback = false;
bool needRemove = false;

void UpdateFanUI(LPVOID);
ThreadHelper* fanThread;

// Forward declarations of functions included in this code module:
//ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    FanDialog(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    FanCurve(HWND, UINT, WPARAM, LPARAM);

extern void ResetDPIScale();

extern void ReloadFanView(HWND list);
extern void ReloadPowerList(HWND list);
extern void ReloadTempView(HWND list);
HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetCurrentGmode();

extern bool fanMode;
extern HANDLE ocStopEvent;
DWORD WINAPI CheckFanOverboost(LPVOID lpParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    MSG msg{0};

    fan_conf = new ConfigFan();

    ResetDPIScale();

    NOTIFYICONDATA fanIcon{ sizeof(NOTIFYICONDATA), 0, IDI_ALIENFANGUI, NIF_ICON | NIF_MESSAGE | NIF_TIP, WM_APP + 1,
        (HICON)LoadImage(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDI_ALIENFANGUI),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR) };

    niData = &fanIcon;

    Shell_NotifyIcon(NIM_ADD, niData);

    acpi = new AlienFan_SDK::Control();

    if (acpi->Probe()) {
        Shell_NotifyIcon(NIM_DELETE, niData);
        fan_conf->SetBoosts(acpi);

        mon = new MonHelper(fan_conf);

        if (!(mDlg = InitInstance(hInstance, fan_conf->startMinimized ? SW_HIDE : SW_NORMAL))) {
            return FALSE;
        }

        //power mode hotkeys
        for (int i = 0; i < 6; i++)
            RegisterHotKey(mDlg, 20 + i, MOD_CONTROL | MOD_ALT, 0x30 + i);
        RegisterHotKey(mDlg, 6, 0, VK_F17);

        HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAIN_ACC));

        // Main message loop:
        while ((GetMessage(&msg, 0, 0, 0)) != 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        delete mon;
    }
    else {
        ShowNotification(niData, "Error", "Compatible hardware not found!", false);
    }

    if (!acpi->GetDeviceFlags()) {
        WindowsStartSet(fan_conf->startWithWindows = false, "AlienFan-GUI");
        Sleep(5000);
    }
    Shell_NotifyIcon(NIM_DELETE, niData);
    delete acpi;
    delete fan_conf;

    return 0;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND dlg;
    dlg = CreateDialogParam(hInstance,//GetModuleHandle(NULL),         /// instance handle
                            MAKEINTRESOURCE(IDD_MAIN_VIEW),    /// dialog box template
                            NULL,                    /// handle to parent
                            (DLGPROC)FanDialog, 0);
    if (!dlg) return NULL;

    SendMessage(dlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFANGUI)));
    SendMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFANGUI), IMAGE_ICON, 16, 16, 0));

    ShowWindow(dlg, nCmdShow);

    return dlg;
}

void StartOverboost(HWND hDlg, int fan) {
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_POWER), false);
    CreateThread(NULL, 0, CheckFanOverboost, (LPVOID)fan, 0, NULL);
    SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Stop Overboost");
}

LRESULT CALLBACK FanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        power_gpu = GetDlgItem(hDlg, IDC_SLIDER_GPU);

    if (message == newTaskBar) {
        // Started/restarted explorer...
        Shell_NotifyIcon(NIM_ADD, niData);
        if (fan_conf->updateCheck)
            CreateThread(NULL, 0, CUpdateCheck, niData, 0, NULL);
        return true;
    }

    switch (message) {
    case WM_INITDIALOG:
    {
        niData->hWnd = hDlg;
        if (Shell_NotifyIcon(NIM_ADD, niData) && fan_conf->updateCheck)
            CreateThread(NULL, 0, CUpdateCheck, niData, 0, NULL);

        // set PerfBoost lists...
        HWND boost_ac = GetDlgItem(hDlg, IDC_AC_BOOST),
             boost_dc = GetDlgItem(hDlg, IDC_DC_BOOST);
        IIDFromString(L"{be337238-0d82-4146-a960-4f3749d470c7}", &perfset);
        PowerGetActiveScheme(NULL, &sch_guid);
        DWORD acMode, dcMode;
        PowerReadACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &acMode);
        PowerReadDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &dcMode);

        UpdateCombo(boost_ac, pModes, acMode);
        UpdateCombo(boost_dc, pModes, dcMode);;

        // So open fan control window...
        RECT cDlg;
        GetWindowRect(hDlg, &cDlg);
        int wh = cDlg.bottom - cDlg.top;// -2 * GetSystemMetrics(SM_CYBORDER);
        fanWindow = CreateWindow("STATIC", "Fan curve", WS_CAPTION | WS_POPUP,//WS_OVERLAPPED,
                                 cDlg.right, cDlg.top, wh, wh,
                                 hDlg, NULL, hInst, 0);
        SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
        toolTip = CreateToolTip(fanWindow, NULL);
        tipWindow = fanWindow;
        if (fan_conf->startMinimized) {
            ShowWindow(fanWindow, SW_HIDE);
        } else {
            ShowWindow(fanWindow, SW_SHOWNA);
        }

        ReloadPowerList(GetDlgItem(hDlg, IDC_COMBO_POWER));
        ReloadTempView(GetDlgItem(hDlg, IDC_TEMP_LIST));
        ReloadFanView(GetDlgItem(hDlg, IDC_FAN_LIST));

        fanThread = new ThreadHelper(UpdateFanUI, hDlg, 500);

        if (mon->oldGmode >= 0)
            Button_SetCheck(GetDlgItem(hDlg, IDC_CHECK_GMODE), fan_conf->lastProf->gmode);
        else
            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_GMODE), false);

        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTWITHWINDOWS, fan_conf->startWithWindows ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTMINIMIZED, fan_conf->startMinimized ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_UPDATE, fan_conf->updateCheck ? MF_CHECKED : MF_UNCHECKED);

        SendMessage(power_gpu, TBM_SETRANGE, true, MAKELPARAM(0, 4));
        SendMessage(power_gpu, TBM_SETTICFREQ, 1, 0);
        SendMessage(power_gpu, TBM_SETPOS, true, fan_conf->lastProf->GPUPower);

        if (!fan_conf->obCheck && MessageBox(NULL, "Fan overboost values not defined!\nDo you want to set it now (it will took some minutes)?", "Question",
            MB_YESNO | MB_ICONINFORMATION) == IDYES) {
            // ask for boost check
            StartOverboost(hDlg, -1);
        }
        fan_conf->obCheck = 1;

        return true;
    } break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
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
            HWND senList = GetDlgItem(hDlg, IDC_TEMP_LIST), editC = ListView_GetEditControl(senList);
            if (editC) {
                RECT rect;
                ListView_GetSubItemRect(senList, ListView_GetSelectionMark(senList), 1, LVIR_LABEL, &rect);
                SetWindowPos(editC, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
            }
        } break;
        case IDC_COMBO_POWER:
        {
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
            {
                pLid = ComboBox_GetCurSel(power_list);
                //int pid = (int)ComboBox_GetItemData(power_list, pLid);
                fan_conf->lastProf->powerStage = (WORD)ComboBox_GetItemData(power_list, pLid);
                acpi->SetPower(fan_conf->lastProf->powerStage);
                fan_conf->Save();
            } break;
            case CBN_EDITCHANGE:
            {
                char buffer[MAX_PATH];
                GetWindowTextA(power_list, buffer, MAX_PATH);
                if (pLid > 0) {
                    auto ret = fan_conf->powers.emplace(acpi->powers[fan_conf->lastProf->powerStage], buffer);
                    if (!ret.second)
                        // just update...
                        ret.first->second = buffer;
                    fan_conf->Save();
                    ComboBox_DeleteString(power_list, pLid);
                    ComboBox_InsertString(power_list, pLid, buffer);
                    ComboBox_SetItemData(power_list, pLid, fan_conf->lastProf->powerStage);
                }
                break;
            }
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
            if (WindowsStartSet(fan_conf->startWithWindows, "AlienFan-GUI"))
                fan_conf->Save();
        } break;
        case IDM_SETTINGS_STARTMINIMIZED:
        {
            fan_conf->startMinimized = !fan_conf->startMinimized;
            CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_STARTMINIMIZED, fan_conf->startMinimized ? MF_CHECKED : MF_UNCHECKED);
            fan_conf->Save();
        } break;
        case IDM_SETTINGS_UPDATE: {
            fan_conf->updateCheck = !fan_conf->updateCheck;
            CheckMenuItem(GetMenu(hDlg), IDM_SETTINGS_UPDATE, fan_conf->updateCheck ? MF_CHECKED : MF_UNCHECKED);
            fan_conf->Save();
            if (fan_conf->updateCheck)
                CreateThread(NULL, 0, CUpdateCheck, niData, 0, NULL);
        } break;
        case IDC_BUT_RESET:
        {
            temp_block *cur = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
            if (cur) {
                fan_block *fan = fan_conf->FindFanBlock(cur, fan_conf->lastSelectedFan);
                if (fan) {
                    fan->points = {{0,0},{100,100}};
                    SendMessage(fanWindow, WM_PAINT, 0, 0);
                }
            }
        } break;
        case IDC_MAX_RESET:
            mon->maxTemps = mon->senValues;
            ReloadTempView(GetDlgItem(hDlg, IDC_TEMP_LIST));
            break;
        case IDC_BUT_OVER:
            if (fanMode) {
                StartOverboost(hDlg, fan_conf->lastSelectedFan);
            }
            else {
                SetEvent(ocStopEvent);
                SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Overboost");
            }
            break;
        case IDC_CHECK_GMODE:
            fan_conf->lastProf->gmode = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
            SetCurrentGmode();
            break;
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
    case WM_APP + 1:
    {
        switch (lParam) {
        case WM_RBUTTONUP:
            SendMessage(hDlg, WM_CLOSE, 0, 0);
            break;
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP: {
            ShowWindow(fanWindow, SW_RESTORE);
            SendMessage(fanWindow, WM_PAINT, 0, 0);
            ShowWindow(mDlg, SW_RESTORE);
            SetForegroundWindow(mDlg);
            HWND temp_list = GetDlgItem(hDlg, IDC_TEMP_LIST);
            ReloadTempView(temp_list);
        } break;
        case NIN_BALLOONHIDE: case NIN_BALLOONTIMEOUT:
            if (!isNewVersion && needRemove) {
                Shell_NotifyIcon(NIM_DELETE, niData);
                Shell_NotifyIcon(NIM_ADD, niData);
                needRemove = false;
            }
            else
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
        if (wParam > 19 && wParam < 26 && acpi && wParam - 20 < acpi->HowManyPower()) {
            fan_conf->lastProf->powerStage = (WORD)wParam - 20;
            acpi->SetPower(fan_conf->lastProf->powerStage);
            ReloadPowerList(GetDlgItem(hDlg, IDC_COMBO_POWER));
        }
        switch (wParam) {
        case 6: // G-key for Dell G-series power switch
            fan_conf->lastProf->gmode = !fan_conf->lastProf->gmode;
            SetCurrentGmode();
            Button_SetCheck(GetDlgItem(hDlg, IDC_CHECK_GMODE), fan_conf->lastProf->gmode);
            break;
        }
    } break;
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_FAN_LIST:
            switch (((NMHDR*)lParam)->code) {
            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
                if (lPoint->uNewState & LVIS_SELECTED && lPoint->iItem != -1) {
                    // Select other fan....
                    fan_conf->lastSelectedFan = lPoint->iItem;
                    // Redraw fans
                    SendMessage(fanWindow, WM_PAINT, 0, 0);
                    break;
                }
                if (fan_conf->lastSelectedSensor != -1 && lPoint->uNewState & 0x3000) {
                    temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
                    switch (lPoint->uNewState & 0x3000) {
                    case 0x1000:
                        // Remove fan
                        if (sen) { // remove sensor block
                            for (auto iFan = sen->fans.begin(); iFan < sen->fans.end(); iFan++)
                                if (iFan->fanIndex == lPoint->iItem) {
                                    sen->fans.erase(iFan);
                                    break;
                                }
                            if (!sen->fans.size()) // remove sensor block!
                                for (auto iSen = fan_conf->lastProf->fanControls.begin();
                                    iSen < fan_conf->lastProf->fanControls.end(); iSen++)
                                    if (iSen->sensorIndex == sen->sensorIndex) {
                                        fan_conf->lastProf->fanControls.erase(iSen);
                                        break;
                                    }
                        }
                        break;
                    case 0x2000:
                        // add fan
                        if (!sen) { // add new sensor block
                            fan_conf->lastProf->fanControls.push_back({ (short)fan_conf->lastSelectedSensor });
                            sen = &fan_conf->lastProf->fanControls.back();
                        }
                        if (!fan_conf->FindFanBlock(sen, lPoint->iItem)) {
                            sen->fans.push_back({ (short)lPoint->iItem,{{0,0},{100,100}} });
                        }
                        break;
                    }
                    ListView_SetItemState(((NMHDR*)lParam)->hwndFrom, -1, 0, LVIS_SELECTED);
                    ListView_SetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, LVIS_SELECTED, LVIS_SELECTED);
                }
            } break;
            }
            break;
        case IDC_TEMP_LIST: {
            HWND tempList = GetDlgItem(hDlg, IDC_TEMP_LIST);
            switch (((NMHDR*)lParam)->code) {
            case LVN_BEGINLABELEDIT: {
                NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
                HWND editC = ListView_GetEditControl(tempList);
                auto pwr = fan_conf->sensors.find(sItem->item.lParam);
                Edit_SetText(editC, (pwr != fan_conf->sensors.end() ? pwr->second : acpi->sensors[sItem->item.lParam].name).c_str());
            } break;
            case LVN_ITEMACTIVATE:
            {
                NMITEMACTIVATE* sItem = (NMITEMACTIVATE*)lParam;
                if (fanThread) {
                    delete fanThread;
                    fanThread = NULL;
                }
                HWND editC = ListView_EditLabel(tempList, sItem->iItem);
                RECT rect;
                ListView_GetSubItemRect(tempList, sItem->iItem, 1, LVIR_LABEL, &rect);
                SetWindowPos(editC, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
            } break;
            case LVN_ENDLABELEDIT:
            {
                NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
                if (sItem->item.pszText) {
                    auto pwr = fan_conf->sensors.find(sItem->item.lParam);
                    if (pwr == fan_conf->sensors.end())
                        fan_conf->sensors.emplace((byte)sItem->item.lParam, sItem->item.pszText);
                    else
                        pwr->second = sItem->item.pszText;
                    ListView_SetItemText(tempList, sItem->item.iItem, 1, sItem->item.pszText);
                }
                fanThread = new ThreadHelper(UpdateFanUI, hDlg);
            } break;
            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
                if (lPoint->uNewState & LVIS_FOCUSED) {
                    // Select other item...
                    if (lPoint->iItem != -1) {
                        // Select other fan....
                        fan_conf->lastSelectedSensor = lPoint->iItem;
                        // Redraw fans
                        ReloadFanView(GetDlgItem(hDlg, IDC_FAN_LIST));
                        SendMessage(fanWindow, WM_PAINT, 0, 0);
                    }
                }
            } break;
            }
        } break;
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
        DestroyWindow(hDlg);
        break;
    case WM_DESTROY:
        delete fanThread;
        LocalFree(sch_guid);
        PostQuitMessage(0);
        break;
    case WM_ENDSESSION:
        // Shutdown/restart scheduled....
        fan_conf->Save();
        delete fanThread;
        delete mon;
        LocalFree(sch_guid);
        return 0;
    case WM_POWERBROADCAST:
        switch (wParam) {
        case PBT_APMRESUMEAUTOMATIC:
            mon = new MonHelper(fan_conf);
            fanThread = new ThreadHelper(UpdateFanUI, hDlg, 500);
            if (fan_conf->updateCheck) {
                needUpdateFeedback = false;
                CreateThread(NULL, 0, CUpdateCheck, niData, 0, NULL);
            }
            break;
        case PBT_APMSUSPEND:
            // Sleep initiated.
            delete fanThread;
            delete mon;
            break;
        }
    }
    return 0;
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

void UpdateFanUI(LPVOID lpParam) {
    HWND tempList = GetDlgItem((HWND)lpParam, IDC_TEMP_LIST),
        fanList = GetDlgItem((HWND)lpParam, IDC_FAN_LIST),
        power_list = GetDlgItem((HWND)lpParam, IDC_COMBO_POWER);
    static bool wasBoostMode = false;
    if (!fanMode) wasBoostMode = true;
    if (fanMode && wasBoostMode) {
        EnableWindow(power_list, true);
        SetWindowText(GetDlgItem((HWND)lpParam, IDC_BUT_OVER), "Overboost");
        wasBoostMode = false;
    }
    if (mon) {
        if (!mon->monThread) {
            for (int i = 0; i < acpi->HowManySensors(); i++) {
                mon->senValues[i] = acpi->GetTempValue(i);
                if (mon->senValues[i] > mon->maxTemps[i])
                    mon->maxTemps[i] = mon->senValues[i];
            }
            for (int i = 0; i < acpi->HowManyFans(); i++)
                mon->fanRpm[i] = acpi->GetFanRPM(i);
        }
        if (IsWindowVisible((HWND)lpParam)) {
            //DebugPrint("Fans UI update...\n");
            for (int i = 0; i < acpi->HowManySensors(); i++) {
                string name = to_string(mon->senValues[i]) + " (" + to_string(mon->maxTemps[i]) + ")";
                ListView_SetItemText(tempList, i, 0, (LPSTR)name.c_str());
            }
            RECT cArea;
            GetClientRect(tempList, &cArea);
            ListView_SetColumnWidth(tempList, 0, LVSCW_AUTOSIZE);
            ListView_SetColumnWidth(tempList, 1, cArea.right - ListView_GetColumnWidth(tempList, 0));
            for (int i = 0; i < acpi->HowManyFans(); i++) {
                string name = "Fan " + to_string(i + 1) + " (" + to_string(mon->fanRpm[i]) + ")";
                ListView_SetItemText(fanList, i, 0, (LPSTR)name.c_str());
            }
            SendMessage(fanWindow, WM_PAINT, 0, 0);
        }
        string name = "Power mode: ";
        if (fan_conf->lastProf->powerStage) {
            auto pwr = fan_conf->powers.find(acpi->powers[fan_conf->lastProf->powerStage]);
            name += (pwr != fan_conf->powers.end() ? pwr->second : "Level " + to_string(fan_conf->lastProf->powerStage));
        }
        else
            name += "Manual";
        for (int i = 0; i < acpi->HowManyFans(); i++) {
            name += "\nFan " + to_string(i + 1) + ": " + to_string(mon->fanRpm[i]) + " RPM";
        }
        strcpy_s(niData->szTip, 128, name.c_str());
        Shell_NotifyIcon(NIM_MODIFY, niData);
    }
}

