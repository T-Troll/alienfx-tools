#include "alienfx-gui.h"
#include "EventHandler.h"
#include <powrprof.h>

#pragma comment(lib, "PowrProf.lib")

extern void SwitchTab(int);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);

extern EventHandler* eve;
extern AlienFan_SDK::Control* acpi;
fan_point* lastFanPoint = NULL;
HWND fanWindow = NULL;

int pLid = -1;

GUID* sch_guid, perfset;

INT_PTR CALLBACK FanCurve(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI UpdateFanUI(LPVOID);
HANDLE fuiEvent = CreateEvent(NULL, false, false, NULL), uiFanHandle = NULL;

void SetTooltip(HWND tt, int x, int y) {
    TOOLINFO ti{ 0 };
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
            SetTooltip(sTip1, 100 * (xx - clirect.left) / (clirect.right - clirect.left),
                       100 * (clirect.bottom - yy) / (clirect.bottom - clirect.top));
            return;
        case 1:// show tooltip
            SetTooltip(sTip1, 100 * (xx - clirect.left) / (clirect.right - clirect.left),
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

        if (conf->fan_conf->lastSelectedFan != -1) {
            // curve...
            temp_block *sen = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor);
            fan_block *fan = NULL;
            for (auto senI = conf->fan_conf->lastProf->fanControls.begin();
                 senI < conf->fan_conf->lastProf->fanControls.end(); senI++)
                if (fan = conf->fan_conf->FindFanBlock(&(*senI), conf->fan_conf->lastSelectedFan)) {
                    // draw fan curve
                    if (sen && sen->sensorIndex == senI->sensorIndex)
                        SetDCPenColor(hdc, RGB(0, 255, 0));
                    else
                        SetDCPenColor(hdc, RGB(255, 255, 0));
                    SelectObject(hdc, GetStockObject(DC_PEN));
                    // First point
                    MoveToEx(hdc, clirect.left, clirect.bottom, NULL);
                    for (int i = 0; i < fan->points.size(); i++) {
                        int cx = fan->points[i].temp * (clirect.right - clirect.left) / 100 + clirect.left,
                            cy = (100 - fan->points[i].boost) * (clirect.bottom - clirect.top) / 100 + clirect.top;
                        LineTo(hdc, cx, cy);
                        Ellipse(hdc, cx - 2, cy - 2, cx + 2, cy + 2);
                    }
                }
            // Red dot and RPM
            SetDCPenColor(hdc, RGB(255, 0, 0));
            SetDCBrushColor(hdc, RGB(255, 0, 0));
            SelectObject(hdc, GetStockObject(DC_PEN));
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            POINT mark;
            int percent;
            mark.x = acpi->GetTempValue(conf->fan_conf->lastSelectedSensor) * (clirect.right - clirect.left) / 100 + clirect.left;
            mark.y = (100 - acpi->GetFanValue(conf->fan_conf->lastSelectedFan)) * (clirect.bottom - clirect.top) / 100 + clirect.top;
            Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
            string rpmText = "Boost: " + to_string(acpi->GetFanValue(conf->fan_conf->lastSelectedFan)) +
                ", " + to_string((percent = acpi->GetFanPercent(conf->fan_conf->lastSelectedFan)) > 100 ? 0 : percent < 0 ? 0 : percent) + "%";
            SetWindowText(GetDlgItem(GetParent(fanWindow), IDC_STATIC_BOOST), rpmText.c_str());
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
    temp_block* sen = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor);
    HWND list = GetDlgItem(hDlg, IDC_FAN_LIST);
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    if (!ListView_GetColumnWidth(list, 0)) {
        LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
        ListView_InsertColumn(list, 0, &lCol);
    }
    for (int i = 0; i < acpi->HowManyFans(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM, i};
        string name = "Fan " + to_string(i + 1) + " (" + to_string(acpi->GetFanRPM(i)) + ")";
        lItem.lParam = i;
        lItem.pszText = (LPSTR) name.c_str();
        if (i == cID) {
            lItem.mask |= LVIF_STATE;
            lItem.state = LVIS_SELECTED;
            DrawFan();
        }
        ListView_InsertItem(list, &lItem);
        if (sen && conf->fan_conf->FindFanBlock(sen, i)) {
            //conf->fan_conf->lastSelectedSensor = -1;
            ListView_SetCheckState(list, i, true);
            //conf->fan_conf->lastSelectedSensor = sen->sensorIndex;
        }
    }

    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
}

void ReloadPowerList(HWND hDlg, int id) {
    HWND list = GetDlgItem(hDlg, IDC_COMBO_POWER);
    ComboBox_ResetContent(list);
    for (int i = 0; i < acpi->HowManyPower(); i++) {
        string name;
        if (i) {
            auto pwr = conf->fan_conf->powers.find(acpi->powers[i]);
            name = pwr != conf->fan_conf->powers.end() ? pwr->second : "Level " + to_string(i);
        }
        else
            name = "Manual";
        int pos = ComboBox_AddString(list, (LPARAM)(name.c_str()));
        ComboBox_SetItemData(list, pos, i);
        if (i == id) {
            ComboBox_SetCurSel(list, pos);
            pLid = pos;
        }
    }
}

void ReloadTempView(HWND hDlg, int cID) {
    int rpos = 0;
    HWND list = GetDlgItem(hDlg, IDC_TEMP_LIST);
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    if (!ListView_GetColumnWidth(list, 1)) {
        LVCOLUMNA lCol{ LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ,LVCFMT_LEFT,100,(LPSTR)"Temp",4};
        ListView_InsertColumn(list, 0, &lCol);
        lCol.pszText = (LPSTR)"Name";
        lCol.iSubItem = 1;
        ListView_InsertColumn(list, 1, &lCol);
    }
    for (int i = 0; i < acpi->HowManySensors(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM, i };
        string name = to_string(acpi->GetTempValue(i)) + " (" + to_string(eve->mon->maxTemps[i]) + ")";
        lItem.lParam = i;
        lItem.pszText = (LPSTR) name.c_str();
        if (i == cID) {
            lItem.mask |= LVIF_STATE;
            lItem.state = LVIS_SELECTED;
            rpos = i;
        }
        ListView_InsertItem(list, &lItem);
        ListView_SetItemText(list, i, 1, (LPSTR) acpi->sensors[i].name.c_str());
    }
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(list, 1, LVSCW_AUTOSIZE_USEHEADER);
    ListView_EnsureVisible(list, rpos, false);
}

BOOL CALLBACK TabFanDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND power_list = GetDlgItem(hDlg, IDC_COMBO_POWER),
        power_gpu = GetDlgItem(hDlg, IDC_SLIDER_GPU);

    switch (message) {
    case WM_INITDIALOG:
    {
        if (acpi && acpi->IsActivated()) {

            // set PerfBoost lists...
            HWND boost_ac = GetDlgItem(hDlg, IDC_AC_BOOST),
                boost_dc = GetDlgItem(hDlg, IDC_DC_BOOST);
            IIDFromString(L"{be337238-0d82-4146-a960-4f3749d470c7}", &perfset);
            PowerGetActiveScheme(NULL, &sch_guid);
            DWORD acMode, dcMode;
            PowerReadACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &acMode);
            PowerReadDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, &dcMode);

            char buffer[64];
            LoadString(hInst, IDS_BOOST_OFF, buffer, 64);
            ComboBox_AddString(boost_ac, buffer);
            ComboBox_AddString(boost_dc, buffer);
            LoadString(hInst, IDS_BOOST_ON, buffer, 64);
            ComboBox_AddString(boost_ac, buffer);
            ComboBox_AddString(boost_dc, buffer);
            LoadString(hInst, IDS_BOOST_AGGRESSIVE, buffer, 64);
            ComboBox_AddString(boost_ac, buffer);
            ComboBox_AddString(boost_dc, buffer);
            LoadString(hInst, IDS_BOOST_EFFICIENT, buffer, 64);
            ComboBox_AddString(boost_ac, buffer);
            ComboBox_AddString(boost_dc, buffer);
            LoadString(hInst, IDS_BOOST_EFFAGGR, buffer, 64);
            ComboBox_AddString(boost_ac, buffer);
            ComboBox_AddString(boost_dc, buffer);

            ComboBox_SetCurSel(boost_ac, acMode);
            ComboBox_SetCurSel(boost_dc, dcMode);

            ReloadPowerList(hDlg, conf->fan_conf->lastProf->powerStage);
            ReloadTempView(hDlg, conf->fan_conf->lastSelectedSensor);
            ReloadFanView(hDlg, conf->fan_conf->lastSelectedFan);

            Static_SetText(GetDlgItem(hDlg, IDC_FC_LABEL),
                           ("Fan curve (scale: " + to_string(conf->fan_conf->boosts[conf->fan_conf->lastSelectedFan].maxBoost) + ")").c_str());

            // So open fan control window...
            fanWindow = GetDlgItem(hDlg, IDC_FAN_CURVE);
            SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
            sTip1 = CreateToolTip(fanWindow, NULL);

            // Start UI update thread...
            uiFanHandle = CreateThread(NULL, 0, UpdateFanUI, hDlg, 0, NULL);

            SendMessage(power_gpu, TBM_SETRANGE, true, MAKELPARAM(0, 4));
            SendMessage(power_gpu, TBM_SETTICFREQ, 1, 0);
            SendMessage(power_gpu, TBM_SETPOS, true, conf->fan_conf->lastProf->GPUPower);

        } else {
            SwitchTab(TAB_SETTINGS);
            return false;
        }
        return true;
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
                conf->fan_conf->lastProf->powerStage = pid;
                acpi->SetPower(pid);
                conf->fan_conf->Save();
            } break;
            case CBN_EDITCHANGE:
            {
                char buffer[MAX_PATH];
                GetWindowTextA(power_list, buffer, MAX_PATH);
                if (pLid > 0) {
                    auto ret = conf->fan_conf->powers.emplace(acpi->powers[conf->fan_conf->lastProf->powerStage], buffer);
                    if (!ret.second)
                        // just update...
                        ret.first->second = buffer;
                    conf->fan_conf->Save();
                    ComboBox_DeleteString(power_list, pLid);
                    ComboBox_InsertString(power_list, pLid, buffer);
                    ComboBox_SetItemData(power_list, pLid, conf->fan_conf->lastProf->powerStage);
                }
                break;
            }
            }
        } break;
        case IDC_BUT_RESET:
        {
            temp_block* cur = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor);
            if (cur) {
                fan_block* fan = conf->fan_conf->FindFanBlock(cur, conf->fan_conf->lastSelectedFan);
                if (fan) {
                    fan->points.clear();
                    fan->points.push_back({0,0});
                    fan->points.push_back({100,100});
                    DrawFan();
                }
            }
        } break;
        case IDC_MAX_RESET:
        {
            for (int i = 0; i < acpi->HowManySensors(); i++)
                eve->mon->maxTemps[i] = acpi->GetTempValue(i);
            ReloadTempView(hDlg, conf->fan_conf->lastSelectedSensor);
        } break;
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
                        conf->fan_conf->lastSelectedFan = lPoint->iItem;
                        // Update label...
                        Static_SetText(GetDlgItem(hDlg, IDC_FC_LABEL),
                                       ("Fan curve (scale: " + to_string(conf->fan_conf->boosts[conf->fan_conf->lastSelectedFan].maxBoost) + ")").c_str());
                        // Redraw fans
                        DrawFan();
                    }
                }
                if (lPoint->uNewState & 0x2000) { // checked, 0x1000 - unchecked
                    if (conf->fan_conf->lastSelectedSensor != -1) {
                        temp_block* sen = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor);
                        if (!sen) { // add new sensor block
                            conf->fan_conf->lastProf->fanControls.push_back({ (short)conf->fan_conf->lastSelectedSensor });
                            sen = &conf->fan_conf->lastProf->fanControls.back();
                        }
                        if (!conf->fan_conf->FindFanBlock(sen, lPoint->iItem)) {
                            sen->fans.push_back({ (short)lPoint->iItem,{{0,0},{100,100}} });
                        }
                        DrawFan();
                    }
                }
                if (lPoint->uNewState & 0x1000 && lPoint->uOldState && 0x2000) { // unchecked
                    if (conf->fan_conf->lastSelectedSensor != -1) {
                        temp_block* sen = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor);
                        if (sen) { // remove sensor block
                            for (auto iFan = sen->fans.begin(); iFan < sen->fans.end(); iFan++)
                                if (iFan->fanIndex == lPoint->iItem) {
                                    sen->fans.erase(iFan);
                                    break;
                                }
                            if (!sen->fans.size()) // remove sensor block!
                                for (auto iSen = conf->fan_conf->lastProf->fanControls.begin();
                                     iSen < conf->fan_conf->lastProf->fanControls.end(); iSen++)
                                    if (iSen->sensorIndex == sen->sensorIndex) {
                                        conf->fan_conf->lastProf->fanControls.erase(iSen);
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
                        conf->fan_conf->lastSelectedSensor = lPoint->iItem;
                        // Redraw fans
                        ReloadFanView(hDlg, conf->fan_conf->lastSelectedFan);
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
                conf->fan_conf->lastProf->GPUPower = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                acpi->SetGPU(conf->fan_conf->lastProf->GPUPower);
            }
        } break;
        } break;
    case WM_DESTROY:
        SetEvent(fuiEvent);
        WaitForSingleObject(uiFanHandle, 1000);
        CloseHandle(uiFanHandle);
        LocalFree(sch_guid);
        break;
    }
    return 0;
}

#define DRAG_ZONE 2

INT_PTR CALLBACK FanCurve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    fan_block* cFan = conf->fan_conf->FindFanBlock(conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor), conf->fan_conf->lastSelectedFan);
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
            for (auto fPi = cFan->points.begin(); fPi < cFan->points.end(); fPi++) {
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
            for (auto fPi = cFan->points.begin(); fPi < cFan->points.end() - 1; fPi++)
                if (fPi->temp > (fPi + 1)->temp) {
                    fan_point t = *fPi;
                    *fPi = *(fPi + 1);
                    *(fPi + 1) = t;
                }
            cFan->points.front().temp = 0;
            cFan->points.back().temp = 100;
            DrawFan();
            //conf->fan_conf->Save();
        }
        SetFocus(GetParent(hDlg));
    } break;
    case WM_RBUTTONUP: {
        // remove point from curve...
        if (cFan && cFan->points.size() > 2) {
            // check and remove point
            int temp = (100 * (GET_X_LPARAM(lParam) - cArea.left)) / (cArea.right - cArea.left),
                boost = (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / (cArea.bottom - cArea.top);
            for (auto fPi = cFan->points.begin() + 1; fPi < cFan->points.end() - 1; fPi++)
                if (fPi->temp - DRAG_ZONE <= temp && fPi->temp + DRAG_ZONE >= temp ) {
                    // Remove this element...
                    cFan->points.erase(fPi);
                    break;
                }
            DrawFan();
            conf->fan_conf->Save();
        }
        SetFocus(GetParent(hDlg));
    } break;
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_ERASEBKGND:
        return true;
        break;
    }
    return false;
}

DWORD WINAPI UpdateFanUI(LPVOID lpParam) {
    HWND tempList = GetDlgItem((HWND)lpParam, IDC_TEMP_LIST),
        fanList = GetDlgItem((HWND)lpParam, IDC_FAN_LIST),
        power_list = GetDlgItem((HWND)lpParam, IDC_COMBO_POWER);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
    while (WaitForSingleObject(fuiEvent, 250) == WAIT_TIMEOUT) {
        if (eve->mon && IsWindowVisible((HWND)lpParam)) {
            //DebugPrint("Fans UI update...\n");
            for (int i = 0; i < eve->mon->acpi->HowManySensors(); i++) {
                string name = to_string(eve->mon->senValues[i]) + " (" + to_string(eve->mon->maxTemps[i]) + ")";
                ListView_SetItemText(tempList, i, 0, (LPSTR)name.c_str());
            }
            for (int i = 0; i < eve->mon->acpi->HowManyFans(); i++) {
                string name = "Fan " + to_string(i + 1) + " (" + to_string(eve->mon->fanValues[i]) + ")";
                ListView_SetItemText(fanList, i, 0, (LPSTR)name.c_str());
            }
            SendMessage(fanWindow, WM_PAINT, 0, 0);
        }
    }
    return 0;
}