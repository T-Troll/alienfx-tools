#include "alienfx-gui.h"
#include "MonHelper.h"
#include "common.h"
#include <powrprof.h>

#pragma comment(lib, "PowrProf.lib")

extern ConfigFan* fan_conf;
extern MonHelper* mon;
HWND fanWindow = NULL, tipWindow = NULL;

GUID* sch_guid, perfset;
extern NOTIFYICONDATA* niData;

extern INT_PTR CALLBACK FanCurve(HWND, UINT, WPARAM, LPARAM);
extern DWORD WINAPI CheckFanOverboost(LPVOID lpParam);
extern void ReloadFanView(HWND list);
extern void ReloadPowerList(HWND list);
extern void ReloadTempView(HWND list);
extern void TempUIEvent(NMLVDISPINFO* lParam, HWND fanList);
extern void FanUIEvent(NMLISTVIEW* lParam, HWND fanList, HWND tempList);
extern string GetFanName(int ind, bool forTray = false);
extern HANDLE ocStopEvent;
extern void DrawFan();

const string pModes[] = { "Off", "Enabled", "Aggressive", "Efficient", "Efficient aggressive", "" };

BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        tcc_slider = GetDlgItem(hDlg, IDC_SLIDER_TCC),
        xmp_slider = GetDlgItem(hDlg, IDC_SLIDER_XMP),
        tempList = GetDlgItem(hDlg, IDC_TEMP_LIST),
        fanList = GetDlgItem(hDlg, IDC_FAN_LIST),
        tcc_edit = GetDlgItem(hDlg, IDC_EDIT_TCC);

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

        //vector<string> pModes{ "Off", "Enabled", "Aggressive", "Efficient", "Efficient aggressive" };
        UpdateCombo(GetDlgItem(hDlg, IDC_AC_BOOST), pModes, acMode);
        UpdateCombo(GetDlgItem(hDlg, IDC_DC_BOOST), pModes, dcMode);;

        ReloadPowerList(power_list);
        ReloadFanView(fanList);

        // So open fan control window...
        fanWindow = GetDlgItem(hDlg, IDC_FAN_CURVE);
        SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR)FanCurve);
        tipWindow = GetDlgItem(hDlg, IDC_FC_LABEL);

        // Set SystemID
        SetDlgItemText(hDlg, IDC_FC_ID, ("ID: " + to_string(mon->systemID)).c_str());

        // Start UI update thread...
        SetTimer(hDlg, 0, fan_conf->pollingRate, NULL);

        // OC block
        EnableWindow(tcc_slider, fan_conf->ocEnable && mon->acpi->isTcc);
        EnableWindow(tcc_edit, fan_conf->ocEnable && mon->acpi->isTcc);
        if (fan_conf->ocEnable && mon->acpi->isTcc) {
            SendMessage(tcc_slider, TBM_SETRANGE, true, MAKELPARAM(mon->acpi->maxTCC - mon->acpi->maxOffset, mon->acpi->maxTCC));
            sTip1 = CreateToolTip(tcc_slider, sTip1);
            SetSlider(sTip1, fan_conf->lastProf->currentTCC);
            SendMessage(tcc_slider, TBM_SETPOS, true, fan_conf->lastProf->currentTCC);

            // Set edit box value to match slider
			SetDlgItemInt(hDlg, IDC_EDIT_TCC, fan_conf->lastProf->currentTCC, FALSE);
        }
        EnableWindow(xmp_slider, fan_conf->ocEnable && mon->acpi->isXMP);
        if (fan_conf->ocEnable && mon->acpi->isXMP) {
            SendMessage(xmp_slider, TBM_SETRANGE, true, MAKELPARAM(0, 2));
            sTip2 = CreateToolTip(xmp_slider, sTip2);
            SetSlider(sTip2, fan_conf->lastProf->memoryXMP);
            SendMessage(xmp_slider, TBM_SETPOS, true, fan_conf->lastProf->memoryXMP);
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
                mon->SetPowerMode(ComboBox_GetCurSel(power_list));
            } break;
            case CBN_EDITCHANGE:
                if (mon->powerMode < mon->powerSize && mon->powerMode) {
                    char buffer[MAX_PATH];
                    GetWindowText(power_list, buffer, MAX_PATH);
                    fan_conf->powers[mon->acpi->powers[mon->powerMode]] = buffer;
                    ComboBox_DeleteString(power_list, mon->powerMode);
                    ComboBox_InsertString(power_list, mon->powerMode, buffer);
                }
                break;
            }
        } break;
        case IDC_FAN_RESET:
        {
            if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you want to clear all fan curves?", "Warning",
                MB_YESNO | MB_ICONWARNING) == IDYES) {
                fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].clear();
                ReloadTempView(tempList);
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
                fan_conf->boosts[fan_conf->lastSelectedFan].maxBoost = 100;
            break;
        }
        case IDC_EDIT_TCC:
            switch (HIWORD(wParam)) {
            case EN_CHANGE: {
                // Doesn't need to check success, 0 will be out of range and not changed
                int val = GetDlgItemInt(hDlg, IDC_EDIT_TCC, NULL, FALSE);
                // Did it in range?
                if (val && val == max(min(val, mon->acpi->maxTCC), mon->acpi->maxTCC - mon->acpi->maxOffset)) {
                    // Set slider and value
                    SendMessage(tcc_slider, TBM_SETPOS, TRUE, fan_conf->lastProf->currentTCC = val);
                    SetSlider(sTip1, val);
                    mon->SetOC();
                }
            } break;
            case EN_KILLFOCUS:
                // Just set resulted value
                SetDlgItemInt(hDlg, IDC_EDIT_TCC, fan_conf->lastProf->currentTCC, FALSE);
                break;
            } break;
    } break;
    case WM_APP + 2:
        EnableWindow(power_list, (bool)lParam);
        SetWindowText(GetDlgItem(hDlg, IDC_BUT_OVER), (bool)lParam ? "Check\n Max. boost" : "Stop check");
        break;
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_FAN_LIST:
            FanUIEvent((NMLISTVIEW*)lParam, fanList, tempList);
            break;
        case IDC_TEMP_LIST:
            TempUIEvent((NMLVDISPINFO*)lParam, tempList);
            break;
        } break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBTRACK: case TB_ENDTRACK: {
            if ((HWND)lParam == tcc_slider) {
                fan_conf->lastProf->currentTCC = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                SetSlider(sTip1, fan_conf->lastProf->currentTCC);
                // Update edit box
				SetDlgItemInt(hDlg, IDC_EDIT_TCC, fan_conf->lastProf->currentTCC, FALSE);
                mon->SetOC();
            }
            if ((HWND)lParam == xmp_slider) {
                fan_conf->lastProf->memoryXMP = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                SetSlider(sTip2, fan_conf->lastProf->memoryXMP);
                mon->SetOC();
            }
        } break;
        } break;
    case WM_TIMER:
        if (mon->modified) {
            for (int i = 0; i < mon->sensorSize; i++) {
                WORD sid = mon->acpi->sensors[i].sid;
                string name = to_string(mon->senValues[sid]) + " (" + to_string(mon->maxTemps[sid]) + ")";
                ListView_SetItemText(tempList, i, 0, (LPSTR)name.c_str());
                name = fan_conf->GetSensorName(&mon->acpi->sensors[i]);
                ListView_SetItemText(tempList, i, 1, (LPSTR)name.c_str());
            }
            RECT cArea;
            GetClientRect(tempList, &cArea);
            ListView_SetColumnWidth(tempList, 0, LVSCW_AUTOSIZE);
            ListView_SetColumnWidth(tempList, 1, cArea.right - ListView_GetColumnWidth(tempList, 0));
        }
        for (int i = 0; i < mon->fansize; i++) {
            string name = GetFanName(i);
            ListView_SetItemText(fanList, i, 0, (LPSTR)name.c_str());
        }
        DrawFan();
        break;
    case WM_DESTROY:
        LocalFree(sch_guid);
        break;
    }
    return 0;
}