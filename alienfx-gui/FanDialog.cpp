#include "alienfx-gui.h"
#include "EventHandler.h"
#include <powrprof.h>

#pragma comment(lib, "PowrProf.lib")

extern void SwitchTab(int);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void UpdateCombo(HWND ctrl, vector<string> items, int sel = 0, vector<int> val = {});

extern EventHandler* eve;
extern AlienFan_SDK::Control* acpi;
ConfigFan* fan_conf = NULL;
MonHelper* mon = NULL;
HWND fanWindow = NULL, tipWindow = NULL;
extern HWND toolTip;

int pLid = -1;

extern bool fanMode;

GUID* sch_guid, perfset;

extern INT_PTR CALLBACK FanCurve(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI CheckFanOverboost(LPVOID lpParam);
extern void ReloadFanView(HWND list, int cID);
extern void ReloadPowerList(HWND list, int id);
extern void ReloadTempView(HWND list, int cID);
extern HANDLE ocStopEvent;

void UpdateFanUI(LPVOID);
ThreadHelper* fanUIUpdate = NULL;

BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        power_gpu = GetDlgItem(hDlg, IDC_SLIDER_GPU);

    switch (message) {
    case WM_INITDIALOG:
    {
        if (eve->mon) {
            fan_conf = conf->fan_conf;
            mon = eve->mon;

            // set PerfBoost lists...
            HWND boost_ac = GetDlgItem(hDlg, IDC_AC_BOOST),
                boost_dc = GetDlgItem(hDlg, IDC_DC_BOOST);
            IIDFromString(L"{be337238-0d82-4146-a960-4f3749d470c7}", &perfset);
            PowerGetActiveScheme(NULL, &sch_guid);
            DWORD acMode, dcMode;
            PowerReadACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &acMode);
            PowerReadDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &dcMode);

            vector<string> pModes{ "Off", "Enabled", "Aggressive", "Efficient", "Efficient aggressive" };
            UpdateCombo(boost_ac, pModes, acMode);
            UpdateCombo(boost_dc, pModes, dcMode);;

            ReloadPowerList(GetDlgItem(hDlg, IDC_COMBO_POWER), fan_conf->lastProf->powerStage);
            ReloadTempView(GetDlgItem(hDlg, IDC_TEMP_LIST), fan_conf->lastSelectedSensor);
            ReloadFanView(GetDlgItem(hDlg, IDC_FAN_LIST), fan_conf->lastSelectedFan);

            // So open fan control window...
            fanWindow = GetDlgItem(hDlg, IDC_FAN_CURVE);
            SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
            toolTip = CreateToolTip(fanWindow, toolTip);
            tipWindow = GetDlgItem(hDlg, IDC_FC_LABEL);

            // Start UI update thread...
            fanUIUpdate = new ThreadHelper(UpdateFanUI, hDlg);
            //uiFanHandle = CreateThread(NULL, 0, UpdateFanUI, hDlg, 0, NULL);

            SendMessage(power_gpu, TBM_SETRANGE, true, MAKELPARAM(0, 4));
            SendMessage(power_gpu, TBM_SETTICFREQ, 1, 0);
            SendMessage(power_gpu, TBM_SETPOS, true, fan_conf->lastProf->GPUPower);

            if (!fan_conf->obCheck && MessageBox(NULL, "Fan overboost values not defined!\nDo you want to set it now (it will took some minutes)?", "Question",
                MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                // ask for boost check
                EnableWindow(power_list, false);
                CreateThread(NULL, 0, CheckFanOverboost, (LPVOID)(-1), 0, NULL);
                SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Stop Overboost");
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
        case IDC_COMBO_POWER:
        {
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
            {
                pLid = ComboBox_GetCurSel(power_list);
                int pid = (int)ComboBox_GetItemData(power_list, pLid);
                fan_conf->lastProf->powerStage = pid;
                acpi->SetPower(pid);
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
            ReloadTempView(GetDlgItem(hDlg, IDC_TEMP_LIST), fan_conf->lastSelectedSensor);
            break;
        case IDC_BUT_OVER:
            if (fanMode) {
                EnableWindow(power_list, false);
                CreateThread(NULL, 0, CheckFanOverboost, (LPVOID)fan_conf->lastSelectedFan, 0, NULL);
                SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Stop Overboost");
            }
            else {
                SetEvent(ocStopEvent);
                SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), "Overboost");
            }
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
                if (lPoint->uNewState & LVIS_FOCUSED) {
                    // Select other item...
                    if (lPoint->iItem != -1) {
                        // Select other fan....
                        fan_conf->lastSelectedFan = lPoint->iItem;

                        // Redraw fans
                        SendMessage(fanWindow, WM_PAINT, 0, 0);
                    }
                }
                if (lPoint->uNewState & 0x2000) { // checked, 0x1000 - unchecked
                    if (fan_conf->lastSelectedSensor != -1) {
                        temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
                        if (!sen) { // add new sensor block
                            fan_conf->lastProf->fanControls.push_back({ (short)fan_conf->lastSelectedSensor });
                            sen = &fan_conf->lastProf->fanControls.back();
                        }
                        if (!fan_conf->FindFanBlock(sen, lPoint->iItem)) {
                            sen->fans.push_back({ (short)lPoint->iItem,{{0,0},{100,100}} });
                        }
                        SendMessage(fanWindow, WM_PAINT, 0, 0);
                    }
                } else
                    if (lPoint->uNewState & 0x1000 && lPoint->uOldState & 0x2000) { // unchecked
                        if (fan_conf->lastSelectedSensor != -1) {
                            temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
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
                            SendMessage(fanWindow, WM_PAINT, 0, 0);
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
                        ReloadFanView(GetDlgItem(hDlg, IDC_FAN_LIST), fan_conf->lastSelectedFan);
                        SendMessage(fanWindow, WM_PAINT, 0, 0);
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
    if (fanMode) {
        EnableWindow(power_list, true);
        SetWindowText(GetDlgItem((HWND)lpParam, IDC_BUT_OVER), "Overboost");
    }
    if (eve->mon && IsWindowVisible((HWND)lpParam)) {
        //DebugPrint("Fans UI update...\n");
        for (int i = 0; i < acpi->HowManySensors(); i++) {
            string name = to_string(acpi->GetTempValue(i)) + " (" + to_string(eve->mon->maxTemps[i]) + ")";
            ListView_SetItemText(tempList, i, 0, (LPSTR)name.c_str());
        }
        for (int i = 0; i < acpi->HowManyFans(); i++) {
            string name = "Fan " + to_string(i + 1) + " (" + to_string(acpi->GetFanRPM(i) /*eve->mon->fanValues[i]*/) + ")";
            ListView_SetItemText(fanList, i, 0, (LPSTR)name.c_str());
        }
        SendMessage(fanWindow, WM_PAINT, 0, 0);
    }
}
