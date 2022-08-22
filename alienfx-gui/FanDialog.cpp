#include "alienfx-gui.h"
#include "EventHandler.h"
#include <powrprof.h>
#include <commctrl.h>

#pragma comment(lib, "PowrProf.lib")

extern void SwitchTab(int);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);

extern EventHandler* eve;
extern AlienFan_SDK::Control* acpi;
extern ConfigFan* fan_conf;
extern MonHelper* mon;
HWND fanWindow = NULL, tipWindow = NULL;
extern HWND toolTip;

int pLid = -1;

extern bool fanMode;

GUID* sch_guid, perfset;
NOTIFYICONDATA* niData;

extern INT_PTR CALLBACK FanCurve(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI CheckFanOverboost(LPVOID lpParam);
extern void ReloadFanView(HWND list);
extern void ReloadPowerList(HWND list);
extern void ReloadTempView(HWND list);
extern void SetCurrentGmode();
extern HANDLE ocStopEvent;

void UpdateFanUI(LPVOID);
ThreadHelper* fanUIUpdate = NULL;

void StartOverboost(HWND hDlg, int fan) {
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_POWER), false);
    CreateThread(NULL, 0, CheckFanOverboost, (LPVOID)&fan, 0, NULL);
    SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Stop Overboost");
}

BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        power_gpu = GetDlgItem(hDlg, IDC_SLIDER_GPU);

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

            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_GMODE), acpi->GetDeviceFlags() & DEV_FLAG_GMODE);
            Button_SetCheck(GetDlgItem(hDlg, IDC_CHECK_GMODE), fan_conf->lastProf->gmode);

            // So open fan control window...
            fanWindow = GetDlgItem(hDlg, IDC_FAN_CURVE);
            SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
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
                StartOverboost(hDlg, -1);
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
                fan_conf->lastProf->powerStage = (WORD)ComboBox_GetItemData(power_list, pLid);
                acpi->SetPower(acpi->powers[fan_conf->lastProf->powerStage]);
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
                    ComboBox_DeleteString(power_list, pLid);
                    ComboBox_InsertString(power_list, pLid, buffer);
                    ComboBox_SetItemData(power_list, pLid, fan_conf->lastProf->powerStage);
                }
                break;
            }
            }
        } break;
        case IDC_FAN_RESET:
        {
            temp_block* cur = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
            if (cur) {
                fan_block* fan = fan_conf->FindFanBlock(cur, fan_conf->lastSelectedFan);
                if (fan) {
                    fan->points.clear();
                    fan->points.push_back({0,0});
                    fan->points.push_back({100,100});
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
            if (mon->oldGmode >= 0)
                acpi->SetGMode(fan_conf->lastProf->gmode);
            break;
        }
    } break;
    case WM_NOTIFY:
        switch (((NMHDR*) lParam)->idFrom) {
        case IDC_FAN_LIST:
            switch (((NMHDR*) lParam)->code) {
            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
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
                if (fanUIUpdate) {
                    delete fanUIUpdate;
                    fanUIUpdate = NULL;
                }
                HWND editC = ListView_GetEditControl(tempList);
                auto pwr = fan_conf->sensors.find((byte)sItem->item.lParam);
                Edit_SetText(editC, (pwr != fan_conf->sensors.end() ? pwr->second : acpi->sensors[(byte)sItem->item.lParam].name).c_str());
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
                    auto pwr = fan_conf->sensors.find((byte)sItem->item.lParam);
                    if (pwr == fan_conf->sensors.end()) {
                        if (strlen(sItem->item.pszText))
                            fan_conf->sensors.emplace((byte)sItem->item.lParam, sItem->item.pszText);
                    }
                    else {
                        if (strlen(sItem->item.pszText))
                            pwr->second = sItem->item.pszText;
                        else
                            fan_conf->sensors.erase(pwr);
                    }
                }
                ReloadTempView(tempList);
                fanUIUpdate = new ThreadHelper(UpdateFanUI, hDlg, 500);
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
    //case WM_HSCROLL:
    //    switch (LOWORD(wParam)) {
    //    case TB_THUMBPOSITION: case TB_ENDTRACK: {
    //        if ((HWND)lParam == power_gpu) {
    //            fan_conf->lastProf->GPUPower = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
    //            acpi->SetGPU(fan_conf->lastProf->GPUPower);
    //        }
    //    } break;
    //    } break;
    case WM_DESTROY:
        if (fanUIUpdate) {
            delete fanUIUpdate;
            fanUIUpdate = NULL;
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
        SetWindowText(GetDlgItem((HWND)lpParam, IDC_BUT_OVER), "Overboost");
        wasBoostMode = false;
    }
    if (mon && IsWindowVisible((HWND)lpParam)) {
        if (!mon->monThread) {
            for (int i = 0; i < acpi->sensors.size(); i++) {
                mon->senValues[i] = acpi->GetTempValue(i);
                if (mon->senValues[i] > mon->maxTemps[i])
                    mon->maxTemps[i] = mon->senValues[i];
            }
            for (int i = 0; i < acpi->fans.size(); i++)
                mon->fanRpm[i] = acpi->GetFanRPM(i);
        }
        //DebugPrint("Fans UI update...\n");
        for (int i = 0; i < acpi->sensors.size(); i++) {
            string name = to_string(mon->senValues[i]) + " (" + to_string(mon->maxTemps[i]) + ")";
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
    }
}
