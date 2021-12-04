#include "alienfx-gui.h"
#include "EventHandler.h"

extern void SwitchTab(int);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);

extern EventHandler* eve;
extern AlienFan_SDK::Control* acpi;
fan_point* lastFanPoint = NULL;
HWND fanWindow = NULL;

INT_PTR CALLBACK FanCurve(HWND, UINT, WPARAM, LPARAM);

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
            temp_block* sen = NULL;
            fan_block* fan = NULL;
            if ((sen = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor)) && 
                (fan = conf->fan_conf->FindFanBlock(sen, conf->fan_conf->lastSelectedFan))) {
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
                int percent;
                mark.x = acpi->GetTempValue(conf->fan_conf->lastSelectedSensor) * (clirect.right - clirect.left) / 100 + clirect.left;
                mark.y = (100 - acpi->GetFanValue(conf->fan_conf->lastSelectedFan)) * (clirect.bottom - clirect.top) / 100 + clirect.top;
                Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
                string rpmText = "Boost: " + to_string(acpi->GetFanValue(conf->fan_conf->lastSelectedFan)) +
                    ", " + to_string((percent = acpi->GetFanPercent(conf->fan_conf->lastSelectedFan)) > 100 ? 0 : percent < 0 ? 0 : percent) + "%";
                SetWindowText(GetDlgItem(GetParent(fanWindow), IDC_STATIC_BOOST), rpmText.c_str());
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
    temp_block* sen = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor);
    HWND list = GetDlgItem(hDlg, IDC_FAN_LIST);
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
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
        if (sen && conf->fan_conf->FindFanBlock(sen, i)) {
            conf->fan_conf->lastSelectedSensor = -1;
            ListView_SetCheckState(list, i, true);
            conf->fan_conf->lastSelectedSensor = sen->sensorIndex;
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

            ReloadPowerList(hDlg, conf->fan_conf->lastProf->powerStage);
            ReloadTempView(hDlg, conf->fan_conf->lastSelectedSensor);
            ReloadFanView(hDlg, conf->fan_conf->lastSelectedFan);

            // So open fan control window...
            fanWindow = GetDlgItem(hDlg, IDC_FAN_CURVE);
            SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
            sTip1 = CreateToolTip(fanWindow, NULL);

            eve->mon->Stop();
            eve->mon->dlg = hDlg;
            eve->mon->fDlg = fanWindow;
            eve->mon->Start();

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
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDC_COMBO_POWER:
        {
            int pItem = ComboBox_GetCurSel(power_list);
            int pid = (int) ComboBox_GetItemData(power_list, pItem);
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
            {
                conf->fan_conf->lastProf->powerStage = pid;
                acpi->SetPower(pid);
                conf->fan_conf->Save();
            } break;
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
                        // Redraw fans
                        DrawFan();
                    }
                }
                if (lPoint->uNewState & 0x2000) { // checked, 0x1000 - unchecked
                    if (conf->fan_conf->lastSelectedSensor != -1) {
                        temp_block* sen = conf->fan_conf->FindSensor(conf->fan_conf->lastSelectedSensor);
                        if (!sen) { // add new sensor block
                            sen = new temp_block;
                            sen->sensorIndex = (short) conf->fan_conf->lastSelectedSensor;
                            conf->fan_conf->lastProf->fanControls.push_back(*sen);
                            delete sen;
                            sen = &conf->fan_conf->lastProf->fanControls.back();
                        }
                        sen->fans.push_back({(short) lPoint->iItem,{{0,0},{100,100}}});
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
    case WM_CLOSE: case WM_DESTROY:
        // Close curve window
        fanWindow = NULL;
        if (acpi)
            eve->mon->fDlg = NULL;
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
            conf->fan_conf->Save();
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
