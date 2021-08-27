#include "alienfx-gui.h"
#include <windowsx.h>

extern UINT newTaskBar;
extern HWND fanWindow;
extern HWND mDlg;

VOID OnSelChanged(HWND hwndDlg);

fan_point* lastFanPoint = NULL;

INT_PTR CALLBACK    FanCurve(HWND, UINT, WPARAM, LPARAM);

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
    HWND curve = fanWindow;

    if (curve) {
        RECT clirect, graphZone;
        GetClientRect(curve, &clirect);
        graphZone = clirect;
        //clirect.left += 1;
        //clirect.top += 1;
        clirect.right -= 1;
        clirect.bottom -= 1;

        switch (oper) {
        case 2:// tooltip, no redraw
            SetTooltip(sTip, 100 * (xx - clirect.left) / (clirect.right - clirect.left),
                       100 * (clirect.bottom - yy) / (clirect.bottom - clirect.top));
            return;
        case 1:// show tooltip
            SetTooltip(sTip, 100 * (xx - clirect.left) / (clirect.right - clirect.left),
                       100 * (clirect.bottom - yy) / (clirect.bottom - clirect.top));
            break;
        }

        HDC hdc_r = GetDC(curve);

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
                SetWindowText(curve, rpmText.c_str());
            }
        }

        BitBlt(hdc_r, 0, 0, graphZone.right - graphZone.left, graphZone.bottom - graphZone.top, hdc, 0, 0, SRCCOPY);

        // Free-up the off-screen DC
        SelectObject(hdc, hOld);

        DeleteObject(hbmMem);
        DeleteDC(hdc);
        ReleaseDC(curve, hdc_r);
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
        string name = "100";
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
    if (message == newTaskBar) {
        // Started/restarted explorer...
        Shell_NotifyIcon(NIM_ADD, &fan_conf->niData);
        CreateThread(NULL, 0, CUpdateCheck, &fan_conf->niData, 0, NULL);
        return true;
    }

    switch (message) {
    case WM_INITDIALOG:
    {
        if (acpi) {

            ReloadPowerList(hDlg, fan_conf->lastProf->powerStage);
            ReloadTempView(hDlg, fan_conf->lastSelectedSensor);
            ReloadFanView(hDlg, fan_conf->lastSelectedFan);

            // So open fan control window...
            RECT cDlg, mwRect;
            GetWindowRect(GetParent(hDlg), &cDlg);
            GetWindowRect(mDlg, &mwRect);
            int wh = cDlg.bottom - cDlg.top;// -2 * GetSystemMetrics(SM_CYBORDER);
            string wName = "Fan curve (boost: " + to_string(acpi->GetFanValue(fan_conf->lastSelectedFan)) + ")";
            fanWindow = CreateWindow("STATIC", wName.c_str(), WS_CAPTION | WS_POPUP,//WS_OVERLAPPED,
                                     mwRect.right, mwRect.top, wh, wh,
                                     hDlg, NULL, hInst, 0);
            SetWindowLongPtr(fanWindow, GWLP_WNDPROC, (LONG_PTR) FanCurve);
            sTip = CreateToolTip(fanWindow, NULL);
            ShowWindow(fanWindow, SW_SHOWNA);
            mon->Stop();
            mon->dlg = hDlg;
            mon->fDlg = fanWindow;
            mon->Start();

            SendMessage(power_gpu, TBM_SETRANGE, true, MAKELPARAM(0, 4));
            SendMessage(power_gpu, TBM_SETTICFREQ, 1, 0);
            SendMessage(power_gpu, TBM_SETPOS, true, fan_conf->lastProf->GPUPower);

        } else {
            HWND tab_list = GetParent(hDlg);
            TabCtrl_SetCurSel(tab_list, 6);
            OnSelChanged(tab_list);
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
                fan_conf->lastProf->powerStage = pid;
                acpi->SetPower(pid);
                fan_conf->Save();
            } break;
            }
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
        }
    } break;
    //case WM_MOVE:
    //{
    //    // Reposition child...
    //    RECT cDlg;
    //    GetWindowRect(GetParent(hDlg), &cDlg);
    //    SetWindowPos(fanWindow, hDlg, cDlg.right, cDlg.top, 0, 0, SWP_NOSIZE | SWP_NOREDRAW | SWP_NOACTIVATE);
    //} break;
    case WM_NOTIFY:
        switch (((NMHDR*) lParam)->idFrom) {
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
                            sen->sensorIndex = (short) fan_conf->lastSelectedSensor;
                            fan_conf->lastProf->fanControls.push_back(*sen);
                            delete sen;
                            sen = &fan_conf->lastProf->fanControls.back();
                        }
                        fan_block cFan = {(short) lPoint->iItem};
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
    case WM_CLOSE: case WM_DESTROY:
        // TODO: close curve window
        DestroyWindow(fanWindow);
        fanWindow = NULL;
        break;
    }
    return 0;
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
        //case WM_COMMAND: {
        //    int i = 0;
        //} break;
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
            // int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
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
            //int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
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
        //case WM_SETCURSOR:
        //{
        //    SetCursor(LoadCursor(hInst, IDC_ARROW));
        ////    //TRACKMOUSEEVENT mEv = {0};
        ////    //mEv.cbSize = sizeof(TRACKMOUSEEVENT);
        ////    //mEv.dwFlags = TME_HOVER | TME_LEAVE;
        ////    //mEv.dwHoverTime = HOVER_DEFAULT;
        ////    //mEv.hwndTrack = hDlg;
        ////    //TrackMouseEvent(&mEv);
        ////    return true;
        //} break;
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_ERASEBKGND:
        return true;
        break;
    default:
        int i = 0;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}
