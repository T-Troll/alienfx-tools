#include "alienfx-gui.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern AlienFX_SDK::afx_act* Code2Act(AlienFX_SDK::Colorcode* c);

extern void SetLightInfo(HWND hDlg);
extern void RedrawDevList(HWND hDlg);

extern int eLid, dIndex;//, devID;
extern bool whiteTest;

POINT clkPoint, dragStart;
RECT dragZone;
DWORD oldClkValue;

HWND tipH = NULL, tipV = NULL;

extern HWND cgDlg;

AlienFX_SDK::mapping* FindCreateMapping() {
    AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingById(conf->afx_dev.fxdevs[dIndex].dev->GetPID(), eLid);
    if (!lgh) {
        // create new mapping
        lgh = new AlienFX_SDK::mapping({ 0, (WORD)conf->afx_dev.fxdevs[dIndex].dev->GetPID(), (WORD)eLid, 0,
            "Light " + to_string(eLid + 1) });
        conf->afx_dev.GetMappings()->push_back(lgh);
        conf->afx_dev.fxdevs[dIndex].lights.push_back(conf->afx_dev.GetMappings()->back());
    }
    return lgh;
}

void InitGridButtonZone(HWND dlg) {
    // delete zone buttons...
    for (DWORD bID = 2000; GetDlgItem(dlg, bID); bID++)
        DestroyWindow(GetDlgItem(dlg, bID));
    // Create zone buttons...
    HWND bblock = GetDlgItem(dlg, IDC_BUTTON_ZONE);
    RECT bzone;
    GetClientRect(bblock, &bzone);
    MapWindowPoints(bblock, dlg, (LPPOINT)&bzone, 1);
    bzone.right /= conf->mainGrid->x;
    bzone.bottom /= conf->mainGrid->y;
    LONGLONG bId = 2000;
    for (int y = 0; y < conf->mainGrid->y; y++)
        for (int x = 0; x < conf->mainGrid->x; x++) {
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId,
                GetModuleHandle(NULL), NULL);
            bId++;
        }
}

void RedrawGridButtonZone(bool recalc = false) {
    // Now refresh final grids...
    if (recalc) {
        if (conf->colorGrid)
            delete conf->colorGrid;
        conf->colorGrid = new pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*>[conf->mainGrid->x * conf->mainGrid->y]{};
        for (auto cs = conf->activeProfile->lightsets.begin();
            cs < conf->activeProfile->lightsets.end(); cs++) {
            for (auto clgh = cs->group->lights.begin(); clgh < cs->group->lights.end(); clgh++) {
                for (int x = 0; x < conf->mainGrid->x; x++)
                    for (int y = 0; y < conf->mainGrid->y; y++)
                        if (LOWORD(conf->mainGrid->grid[ind(x,y)]) == clgh->first &&
                            HIWORD(conf->mainGrid->grid[ind(x,y)]) == clgh->second) {
                            if (conf->enableMon)
                                switch (conf->GetEffect()) {
                                case 1: { // monitoring
                                    bool firstOne = true;
                                    for (int i = 0; i < 3; i++)
                                        if (cs->events[i].state) {
                                            if (firstOne) {
                                                conf->colorGrid[ind(x,y)].first = cs->fromColor && cs->color.size() ?
                                                    &cs->color.front() : &cs->events[i].from;
                                                firstOne = false;
                                            }
                                            conf->colorGrid[ind(x, y)].second = &cs->events[i].to;
                                        }
                                } break;
                                case 3: // haptics
                                    if (cs->haptics.size()) {
                                        conf->colorGrid[ind(x, y)] = { Code2Act(&cs->haptics.front().colorfrom), Code2Act(&cs->haptics.back().colorto) };
                                    }
                                    break;
                                }
                            if (!conf->colorGrid[ind(x, y)].first && cs->color.size()) {
                                conf->colorGrid[ind(x, y)] = { &cs->color.front(), &cs->color.back() };
                            }
                        }
            }
        }
    }
    RECT pRect;
    GetWindowRect(GetDlgItem(cgDlg, IDC_BUTTON_ZONE), &pRect);
    MapWindowPoints(HWND_DESKTOP, cgDlg, (LPPOINT)&pRect, 2);
    RedrawWindow(cgDlg, &pRect, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void SetLightGridSize(HWND dlg, int x, int y) {
    if (x != conf->mainGrid->x || y != conf->mainGrid->y) {
        DWORD* newgrid = new DWORD[x * y]{ 0 };
        for (int row = 0; row < min(conf->mainGrid->y, y); row++)
            memcpy(newgrid + row * x,
                conf->mainGrid->grid + row * conf->mainGrid->x /** sizeof(DWORD)*/,
                min(conf->mainGrid->x, x) * sizeof(DWORD));
        delete[] conf->mainGrid->grid;
        conf->mainGrid->grid = newgrid;
        conf->mainGrid->x = x;
        conf->mainGrid->y = y;
        InitGridButtonZone(dlg);
    }
}

void ModifyDragZone(WORD did, WORD lid, bool clear = false) {
    for (int x = dragZone.left; x <= dragZone.right; x++)
        for (int y = dragZone.top; y <= dragZone.bottom; y++) {
            if (!conf->mainGrid->grid[ind(x,y)] && !clear)
                conf->mainGrid->grid[ind(x,y)] = MAKELPARAM(did, lid);
            else
                if (clear && conf->mainGrid->grid[ind(x,y)] == MAKELPARAM(did, lid))
                    conf->mainGrid->grid[ind(x,y)] = 0;
        }
}

bool TranslateClick(HWND hDlg, LPARAM lParam) {
    clkPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    // check if in control window...
    RECT gRect;
    MapWindowPoints(hDlg, HWND_DESKTOP, &clkPoint, 1);
    GetClientRect(GetDlgItem(hDlg, IDC_BUTTON_ZONE), &gRect);
    ScreenToClient(GetDlgItem(hDlg, IDC_BUTTON_ZONE), &clkPoint);
    if (PtInRect(&gRect, clkPoint)) {
        // start dragging...
        clkPoint = { clkPoint.x / (gRect.right / conf->mainGrid->x),
                     clkPoint.y / (gRect.bottom / conf->mainGrid->y) };
        return true;
    }
    else {
        clkPoint = { -1, -1 };
        return false;
    }
}

void RepaintGrid(HWND hDlg) {
    InitGridButtonZone(hDlg);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), TBM_SETPOS, true, conf->mainGrid->x);
    SetSlider(tipH, conf->mainGrid->x);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), TBM_SETPOS, true, conf->mainGrid->y);
    SetSlider(tipV, conf->mainGrid->y);
    RedrawGridButtonZone(true);
}

BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    WORD devID = dIndex < 0 ? 0 : conf->afx_dev.fxdevs[dIndex].desc->devid;

	switch (message) {
	case WM_INITDIALOG:
	{
        cgDlg = hDlg;
        if (!conf->afx_dev.GetGrids()->size()) {
            conf->afx_dev.GetGrids()->push_back({ 0, 20, 8, "Main" });
            conf->afx_dev.GetGrids()->back().grid = new DWORD[20 * 8]{ 0 };
        }
        if (!conf->mainGrid)
            conf->mainGrid = &conf->afx_dev.GetGrids()->front();

        tipH = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), tipH);
        tipV = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), tipV);

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(3, 80));
        //SendMessage(gridX, TBM_SETPOS, true, mainGrid->x);

        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(3, 20));
        //SendMessage(gridY, TBM_SETPOS, true, mainGrid->y);

        RepaintGrid(hDlg);
	} break;
    case WM_COMMAND: {
        switch (LOWORD(wParam))
        {
        case IDC_BUT_CLEARGRID:
            if (eLid >= 0) {
                for (int ind = 0; ind < conf->mainGrid->x * conf->mainGrid->y; ind++)
                    if (conf->mainGrid->grid[ind] == MAKELPARAM(devID, eLid))
                        conf->mainGrid->grid[ind] = 0;
                RedrawGridButtonZone();
            }
            break;
        }
    } break;
    case WM_RBUTTONUP: {
        // remove grid mapping
        if (TranslateClick(hDlg, lParam)) {
            conf->mainGrid->grid[ind(clkPoint.x,clkPoint.y)] = 0;
            RedrawGridButtonZone();
        }
    } break;
    case WM_LBUTTONDOWN: {
        // selection mark
        if (TranslateClick(hDlg, lParam)) {
            oldClkValue = conf->mainGrid->grid[ind(clkPoint.x, clkPoint.y)];
        }
        dragZone = { clkPoint.x, clkPoint.y, clkPoint.x, clkPoint.y };
        dragStart = clkPoint;
    } break;
    case WM_LBUTTONUP:
        // end selection
        if (dragZone.top >= 0) {
            if (dragZone.bottom - dragZone.top || dragZone.right - dragZone.left || !oldClkValue) {
                // just set zone
                ModifyDragZone(devID, eLid);
                FindCreateMapping();
            }
            else {
                // single click at assigned grid
                if (oldClkValue == MAKELPARAM(devID, eLid))
                    conf->mainGrid->grid[ind(dragZone.left,dragZone.top)] = 0;
                else {
                    // change light to old one
                    auto pos = find_if(conf->afx_dev.fxdevs.begin(), conf->afx_dev.fxdevs.end(),
                        [](auto t) {
                            return t.dev->GetPID() == LOWORD(oldClkValue);
                        });
                    if (pos != conf->afx_dev.fxdevs.end() && dIndex != (pos - conf->afx_dev.fxdevs.begin())) {
                        dIndex = (int)(pos - conf->afx_dev.fxdevs.begin());
                        RedrawDevList(GetParent(GetParent(hDlg)));
                    }
                    eLid = HIWORD(oldClkValue);
                    SetLightInfo(GetParent(GetParent(hDlg)));
                }
            }
            SetLightInfo(GetParent(GetParent(hDlg)));
            //RedrawGridButtonZone();
        }
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            // drag
            if (TranslateClick(hDlg, lParam) && dragZone.top >= 0) {
                ModifyDragZone(devID, eLid, true);
                dragZone = { min(clkPoint.x, dragStart.x), min(clkPoint.y, dragStart.y),
                    max(clkPoint.x, dragStart.x), max(clkPoint.y, dragStart.y) };
                ModifyDragZone(devID, eLid);
                RedrawGridButtonZone();
            }
        }
        break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        /*case TB_THUMBPOSITION: */case TB_ENDTRACK:
            if ((HWND)lParam == gridX) {
                SetLightGridSize(hDlg, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), conf->mainGrid->y);
                SetSlider(tipH, conf->mainGrid->x);
            }
            break;
        default:
            if ((HWND)lParam == gridX) {
                SetSlider(tipH, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
        }
        break;
    case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        /*case TB_THUMBPOSITION:*/ case TB_ENDTRACK:
            if ((HWND)lParam == gridY) {
                SetLightGridSize(hDlg, conf->mainGrid->x, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                SetSlider(tipV, conf->mainGrid->y);
            }
            break;
        default:
            if ((HWND)lParam == gridY) {
                SetSlider(tipV, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
        }
        break;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
        if (ditem->CtlID >= 2000) {
            DWORD gridVal = conf->mainGrid->grid[(ditem->CtlID - 2000)];//% mainGrid->x][(ditem->CtlID - 2000) / mainGrid->x];
            WORD idVal = HIWORD(gridVal) << 4;
            HBRUSH Brush = NULL;
            if (gridVal) {
                if (HIWORD(gridVal) == eLid && LOWORD(gridVal) == devID)
                    Brush = CreateSolidBrush(RGB(conf->testColor.r, conf->testColor.g, conf->testColor.b));
                else
                    Brush = CreateSolidBrush(RGB(0xff - (idVal << 1), 0, idVal & 0xff));
            }
            else
                Brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(ditem->hDC, &ditem->rcItem, Brush);
            DeleteObject(Brush);
            if (!gridVal)
                DrawEdge(ditem->hDC, &ditem->rcItem, /*selected ? EDGE_SUNKEN : */EDGE_RAISED, BF_RECT);
            //else
            //    DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_SUNKEN, BF_MONO | BF_RECT);
        }
    } break;
    case WM_DESTROY:
        for (DWORD bID = 2000; GetDlgItem(hDlg, bID); bID++)
            DestroyWindow(GetDlgItem(hDlg, bID));
        break;
	default: return false;
	}
	return true;
}

void CreateGridBlock(HWND gridTab, DLGPROC func, bool needAddDel) {

    RECT rcDisplay;
    TCITEM tie{ TCIF_TEXT };

    GetClientRect(gridTab, &rcDisplay);

    rcDisplay.left = GetSystemMetrics(SM_CXBORDER);
    rcDisplay.top = GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER);
    rcDisplay.right -= 2 * GetSystemMetrics(SM_CXBORDER) + 1;
    rcDisplay.bottom -= GetSystemMetrics(SM_CYBORDER) + 1;

    for (int i = 0; i < conf->afx_dev.GetGrids()->size(); i++) {
        tie.pszText = (char*)conf->afx_dev.GetGrids()->at(i).name.c_str();
        TabCtrl_InsertItem(gridTab, i, (LPARAM)&tie);
    }

    if (needAddDel) {
        // Special tabs for add/remove
        tie.pszText = LPSTR("+");
        TabCtrl_InsertItem(gridTab, conf->afx_dev.GetGrids()->size(), (LPARAM)&tie);
        tie.pszText = LPSTR("-");
        TabCtrl_InsertItem(gridTab, conf->afx_dev.GetGrids()->size() + 1, (LPARAM)&tie);
    }

    TabCtrl_SetMinTabWidth(gridTab, 10);

    HWND hwndDisplay = CreateDialogIndirect(GetModuleHandle(NULL),
        (DLGTEMPLATE*)LockResource(LoadResource(GetModuleHandle(NULL), FindResource(NULL, MAKEINTRESOURCE(IDD_GRIDBLOCK), RT_DIALOG))),
        gridTab, func);

    SetWindowLongPtr(gridTab, GWLP_USERDATA, (LONG_PTR)hwndDisplay);

    SetWindowPos(hwndDisplay, NULL,
        rcDisplay.left, rcDisplay.top,
        rcDisplay.right - rcDisplay.left,
        rcDisplay.bottom - rcDisplay.top,
        SWP_SHOWWINDOW);
}

void OnGridSelChanged(HWND hwndDlg)
{
    // Get the dialog header data.
    HWND pHdr = (HWND)GetWindowLongPtr(
        hwndDlg, GWLP_USERDATA);

    // Get the index of the selected tab.
    conf->gridTabSel = TabCtrl_GetCurSel(hwndDlg);
    if (conf->gridTabSel < 0) conf->gridTabSel = 0;
    conf->mainGrid = &conf->afx_dev.GetGrids()->at(conf->gridTabSel);

    // Repaint.
    RepaintGrid(pHdr);
}
