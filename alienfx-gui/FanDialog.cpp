#include "alienfx-gui.h"
#include "EventHandler.h"
#include <powrprof.h>
#include <commctrl.h>

#pragma comment(lib, "PowrProf.lib")

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);

extern EventHandler* eve;
extern AlienFan_SDK::Control* acpi;
extern ConfigFan* fan_conf;
extern MonHelper* mon;
HWND fanWindow = NULL, tipWindow = NULL;
extern HWND toolTip;

extern bool fanMode;
extern bool fanUpdateBlock;

GUID* sch_guid, perfset;
NOTIFYICONDATA* niData;

extern INT_PTR CALLBACK FanCurve(HWND, UINT, WPARAM, LPARAM);
extern DWORD WINAPI CheckFanOverboost(LPVOID lpParam);
extern DWORD WINAPI CheckFanRPM(LPVOID lpParam);
extern void ReloadFanView(HWND list);
extern void ReloadPowerList(HWND list);
extern void ReloadTempView(HWND list);
extern HANDLE ocStopEvent;

void UpdateFanUI(LPVOID);
ThreadHelper* fanUIUpdate = NULL;

void StartOverboost(HWND hDlg, int fan, bool type) {
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_POWER), false);
    if (type) {
        CreateThread(NULL, 0, CheckFanOverboost, (LPVOID)fan, 0, NULL);
        SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Stop check");
    } else
        CreateThread(NULL, 0, CheckFanRPM, (LPVOID)fan, 0, NULL);
}

BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        tempList = GetDlgItem(hDlg, IDC_TEMP_LIST);

    switch (message) {
    case WM_INITDIALOG:
    {
        if (mon) {
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
            ReloadTempView(GetDlgItem(hDlg, IDC_TEMP_LIST));
            ReloadFanView(GetDlgItem(hDlg, IDC_FAN_LIST));

            //EnableWindow(g_mode, acpi->GetDeviceFlags() & DEV_FLAG_GMODE);
            //Button_SetCheck(g_mode, fan_conf->lastProf->gmode);

            // So open fan control window...
            fanWindow = GetDlgItem(hDlg, IDC_FAN_CURVE);
            SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR)FanCurve);
            toolTip = CreateToolTip(fanWindow, toolTip);
            tipWindow = GetDlgItem(hDlg, IDC_FC_LABEL);

            // Start UI update thread...
            fanUIUpdate = new ThreadHelper(UpdateFanUI, hDlg, 500);

            //SendMessage(power_gpu, TBM_SETRANGE, true, MAKELPARAM(0, 4));
            //SendMessage(power_gpu, TBM_SETTICFREQ, 1, 0);
            //SendMessage(power_gpu, TBM_SETPOS, true, fan_conf->lastProf->GPUPower);

            if (!fan_conf->obCheck && MessageBox(NULL, "Fan overboost values not defined!\nDo you want to set it now (it will took some minutes)?", "Question",
                    MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                    // ask for boost check
                    EnableWindow(power_list, false);
                    StartOverboost(hDlg, -1, true);
                }
            fan_conf->obCheck = 1;
        }
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
                if (newMode > acpi->powers.size())
                    // G-Mode on
                    mon->SetCurrentGmode(true);
                else {
                    mon->SetCurrentGmode(false);
                    fan_conf->lastProf->powerStage = (WORD)ComboBox_GetCurSel(power_list);
                }
            } break;
            case CBN_EDITCHANGE:
                if (fan_conf->lastProf->powerStage > 0 && fan_conf->lastProf->powerStage < acpi->powers.size()) {
                    char buffer[MAX_PATH];
                    GetWindowText(power_list, buffer, MAX_PATH);
                    fan_conf->powers[acpi->powers[fan_conf->lastProf->powerStage]] = buffer;
                    ComboBox_DeleteString(power_list, fan_conf->lastProf->powerStage);
                    ComboBox_InsertString(power_list, fan_conf->lastProf->powerStage, buffer);
                }
                break;
            }
        } break;
        case IDC_FAN_RESET:
        {
            fan_block* fan = fan_conf->FindFanBlock(fan_conf->lastSelectedFan);
            if (fan && (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you want to clear all fan curves?", "Warning",
                MB_YESNO | MB_ICONWARNING) == IDYES)) {
                fan->sensors.clear();
                ReloadFanView(GetDlgItem(hDlg, IDC_FAN_LIST));
            }
        } break;
        case IDC_MAX_RESET:
            mon->maxTemps = mon->senValues;
            ReloadTempView(GetDlgItem(hDlg, IDC_TEMP_LIST));
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
    } break;
    case WM_NOTIFY:
        switch (((NMHDR*) lParam)->idFrom) {
        case IDC_FAN_LIST:
            if (fanUpdateBlock)
                break;
            switch (((NMHDR*)lParam)->code) {
            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
                if (lPoint->uNewState & LVIS_SELECTED && lPoint->iItem != -1) {
                    // Select other fan....
                    fan_conf->lastSelectedFan = lPoint->iItem;
                    SendMessage(fanWindow, WM_PAINT, 0, 0);
                    break;
                }
                if (lPoint->uNewState & 0x3000) {
                    fan_block* fan = fan_conf->FindFanBlock(lPoint->iItem);
                    switch (lPoint->uNewState & 0x3000) {
                    case 0x1000:
                        if (fan)
                            // Remove sensor
                            for (auto cSen = fan->sensors.begin(); cSen != fan->sensors.end(); cSen++)
                                if (cSen->first == fan_conf->lastSelectedSensor) {
                                    cSen->second.active = false;
                                    break;
                                }
                        break;
                    case 0x2000:
                        // add fan
                        if (!fan) { // add new fan block
                            fan_conf->lastProf->fanControls.push_back({ (short)lPoint->iItem });
                            fan = &fan_conf->lastProf->fanControls.back();
                        }
                        if (fan->sensors.find(fan_conf->lastSelectedSensor) == fan->sensors.end())
                            fan->sensors[fan_conf->lastSelectedSensor] = { true, { {0,0},{100,100} } };
                        else
                            fan->sensors[fan_conf->lastSelectedSensor].active = true;
                        break;
                    }
                    ListView_SetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, LVIS_SELECTED, LVIS_SELECTED);
                }
            } break;
            }
            break;
        case IDC_TEMP_LIST: {
            switch (((NMHDR*)lParam)->code) {
            case LVN_BEGINLABELEDIT: {
                NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
                fanUIUpdate->Stop();
                auto pwr = fan_conf->sensors.find((WORD)sItem->item.lParam);
                Edit_SetText(ListView_GetEditControl(tempList), (pwr != fan_conf->sensors.end() ? pwr->second : acpi->sensors[sItem->item.iItem].name).c_str());
            } break;
            case LVN_ITEMACTIVATE: case NM_RETURN:
            {
                int pos = ListView_GetSelectionMark(tempList);
                RECT rect;
                ListView_GetSubItemRect(tempList, pos, 1, LVIR_BOUNDS, &rect);
                SetWindowPos(ListView_EditLabel(tempList, pos), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
            } break;
            case LVN_ENDLABELEDIT:
            {
                NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
                if (sItem->item.pszText) {
                    if (strlen(sItem->item.pszText))
                        fan_conf->sensors[(WORD)sItem->item.lParam] = sItem->item.pszText;
                    else {
                        auto pwr = fan_conf->sensors.find((WORD)sItem->item.lParam);
                        if (pwr != fan_conf->sensors.end())
                            fan_conf->sensors.erase(pwr);
                    }
                    ReloadTempView(tempList);
                }
                fanUIUpdate->Start();
            } break;
            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
                if (lPoint->uNewState & LVIS_FOCUSED && lPoint->iItem != -1) {
                    // Select other sensor....
                    fan_conf->lastSelectedSensor = (WORD)lPoint->lParam;
                    // Redraw fans
                    ReloadFanView(GetDlgItem(hDlg, IDC_FAN_LIST));
                    SendMessage(fanWindow, WM_PAINT, 0, 0);
                }
            } break;
            }
        } break;
        }
        break;
    case WM_DESTROY:
        if (fanUIUpdate) {
            delete fanUIUpdate;
            LocalFree(sch_guid);
        }
        break;
    }
    return 0;
}

void UpdateFanUI(LPVOID lpParam) {
    HWND tempList = GetDlgItem((HWND)lpParam, IDC_TEMP_LIST),
        fanList = GetDlgItem((HWND)lpParam, IDC_FAN_LIST),
        power_list = GetDlgItem((HWND)lpParam, IDC_COMBO_POWER);
    static bool wasBoostMode = false;
    if (!fanMode) wasBoostMode = true;
    if (fanMode && wasBoostMode) {
        EnableWindow(power_list, true);
        SetWindowText(GetDlgItem((HWND)lpParam, IDC_BUT_OVER), "Check\n Max. boost");
        wasBoostMode = false;
    }
    if (mon && IsWindowVisible((HWND)lpParam)) {
        eve->monInUse.lock();
        if (!mon->monThread) {
            for (int i = 0; i < acpi->sensors.size(); i++) {
                mon->senValues[acpi->sensors[i].sid] = acpi->GetTempValue(i);
                mon->maxTemps[acpi->sensors[i].sid] = max(mon->senValues[acpi->sensors[i].sid], mon->maxTemps[acpi->sensors[i].sid]);
            }
            for (int i = 0; i < acpi->fans.size(); i++)
                mon->fanRpm[i] = acpi->GetFanRPM(i);
        }
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
            string name = "Fan " + to_string(i + 1) + " (" + to_string(mon->fanRpm[i]) + ")";
            ListView_SetItemText(fanList, i, 0, (LPSTR)name.c_str());
        }
        SendMessage(fanWindow, WM_PAINT, 0, 0);
        eve->monInUse.unlock();
    }
}
