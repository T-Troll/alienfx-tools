#include "alienfx-gui.h"
#include "MonHelper.h"
#include "common.h"
#include <powrprof.h>

#pragma comment(lib, "PowrProf.lib")

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);

extern AlienFan_SDK::Control* acpi;
extern ConfigFan* fan_conf;
extern MonHelper* mon;
HWND fanWindow = NULL, tipWindow = NULL;
extern HWND toolTip;

//extern bool fanMode;

GUID* sch_guid, perfset;
extern NOTIFYICONDATA* niData;

extern INT_PTR CALLBACK FanCurve(HWND, UINT, WPARAM, LPARAM);
extern DWORD WINAPI CheckFanOverboost(LPVOID lpParam);
extern DWORD WINAPI CheckFanRPM(LPVOID lpParam);
extern void ReloadFanView(HWND list);
extern void ReloadPowerList(HWND list);
extern void ReloadTempView(HWND list);
extern void TempUIEvent(NMLVDISPINFO* lParam, HWND tempList, HWND fanList);
extern void FanUIEvent(NMLISTVIEW* lParam, HWND fanList);
extern string GetFanName(int ind);
extern HANDLE ocStopEvent;

BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        tempList = GetDlgItem(hDlg, IDC_TEMP_LIST),
        fanList = GetDlgItem(hDlg, IDC_FAN_LIST);

    switch (message) {
    case WM_INITDIALOG:
    {
        niData = &conf->niData;

        // set PerfBoost lists...
        IIDFromString(L"{be337238-0d82-4146-a960-4f3749d470c7}", &perfset);
        PowerGetActiveScheme(NULL, &sch_guid);
        DWORD acMode, dcMode;
        PowerReadACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &acMode);
        PowerReadDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &dcMode);

        vector<string> pModes{ "Off", "Enabled", "Aggressive", "Efficient", "Efficient aggressive" };
        UpdateCombo(GetDlgItem(hDlg, IDC_AC_BOOST), pModes, acMode);
        UpdateCombo(GetDlgItem(hDlg, IDC_DC_BOOST), pModes, dcMode);;

        ReloadPowerList(power_list);
        ReloadTempView(tempList);

        // So open fan control window...
        fanWindow = GetDlgItem(hDlg, IDC_FAN_CURVE);
        SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR)FanCurve);
        toolTip = CreateToolTip(fanWindow, toolTip);
        tipWindow = GetDlgItem(hDlg, IDC_FC_LABEL);

        // Start UI update thread...
        SetTimer(hDlg, 0, 500, NULL);
        SetTimer(fanWindow, 1, 500, NULL);

        //SendMessage(power_gpu, TBM_SETRANGE, true, MAKELPARAM(0, 4));
        //SendMessage(power_gpu, TBM_SETTICFREQ, 1, 0);
        //SendMessage(power_gpu, TBM_SETPOS, true, fan_conf->lastProf->GPUPower);

    } break;
    case WM_COMMAND:
    {
        // Parse the menu selections:
        switch (LOWORD(wParam)) {
        case IDC_AC_BOOST: case IDC_DC_BOOST: {
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
            {
                int cBst = ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam)));
                if (LOWORD(wParam) == IDC_AC_BOOST)
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
                fan_conf->lastProf->gmode = (newMode == acpi->powers.size());
                if (newMode < acpi->powers.size())
                    fan_conf->lastProf->powerStage = newMode;
                mon->SetCurrentMode(newMode);
            } break;
            case CBN_EDITCHANGE:
                if (!fan_conf->lastProf->gmode && !fan_conf->lastProf->powerStage) {
                    char* buffer = new char[MAX_PATH];
                    GetWindowText(power_list, buffer, MAX_PATH);
                    fan_conf->powers[acpi->powers[fan_conf->lastProf->powerStage]] = buffer;
                    ComboBox_DeleteString(power_list, fan_conf->lastProf->powerStage);
                    ComboBox_InsertString(power_list, fan_conf->lastProf->powerStage, buffer);
                    delete buffer;
                }
                break;
            }
        } break;
        case IDC_FAN_RESET:
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
            if (mon->inControl) {
                CreateThread(NULL, 0, CheckFanOverboost, hDlg, 0, NULL);
            }
            else {
                SetEvent(ocStopEvent);
            }
            break;
        case IDC_BUT_RESETBOOST:
            if (mon->inControl)
                fan_conf->boosts[fan_conf->lastSelectedFan] = { 100, (unsigned short)acpi->GetMaxRPM(fan_conf->lastSelectedFan) };
            break;
        }
    } break;
    case WM_APP + 2:
        EnableWindow(power_list, (bool)lParam);
        SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), (bool)lParam ? "Check\n Max. boost" : "Stop check");
        break;
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_FAN_LIST:
            FanUIEvent((NMLISTVIEW*)lParam, fanList);
            break;
        case IDC_TEMP_LIST:
            TempUIEvent((NMLVDISPINFO*)lParam, tempList, fanList);
            break;
        } break;
    case WM_TIMER: {
        if (IsWindowVisible(hDlg) && acpi) {
            if (acpi->DPTFdone) {
                ReloadTempView(tempList);
                acpi->DPTFdone = false;
                return false;
            }
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
    } break;
    case WM_DESTROY:
        LocalFree(sch_guid);
        break;
    }
    return 0;
}