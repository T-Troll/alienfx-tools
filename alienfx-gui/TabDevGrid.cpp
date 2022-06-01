#include "alienfx-gui.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern AlienFX_SDK::afx_act* Code2Act(AlienFX_SDK::Colorcode* c);
extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void UpdateZoneList(byte flag = 0);

extern void SetLightInfo(HWND hDlg);
extern void RedrawDevList(HWND hDlg);

extern int eLid, dIndex;
extern bool whiteTest;
extern int lastTab, eItem;

extern HWND zsDlg;

POINT clkPoint, dragStart;
RECT dragZone;
DWORD oldClkValue;

HWND tipH = NULL, tipV = NULL;

HWND cgDlg;

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
        for (auto cs = conf->activeProfile->lightsets.begin(); cs < conf->activeProfile->lightsets.end(); cs++) {
            AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(cs->group);
            for (auto clgh = grp->lights.begin(); clgh < grp->lights.end(); clgh++) {
                for (int x = 0; x < conf->mainGrid->x; x++)
                    for (int y = 0; y < conf->mainGrid->y; y++) {
                        int ind = ind(x, y);
                        if (LOWORD(conf->mainGrid->grid[ind]) == clgh->first &&
                            HIWORD(conf->mainGrid->grid[ind]) == clgh->second) {
                            if (conf->enableMon)
                                switch (conf->GetEffect()) {
                                case 1: { // monitoring
                                    for (int i = 0; i < 3; i++)
                                        if (cs->events[i].state) {
                                            if (!conf->colorGrid[ind].first && !cs->fromColor) {
                                                conf->colorGrid[ind].first = &cs->events[i].from;
                                            }
                                            conf->colorGrid[ind].second = &cs->events[i].to;
                                        }
                                } break;
                                case 3: // haptics
                                    if (cs->haptics.size()) {
                                        conf->colorGrid[ind] = { Code2Act(&cs->haptics.front().colorfrom), Code2Act(&cs->haptics.back().colorto) };
                                    }
                                    break;
                                }
                            if (cs->color.size()) {
                                if (!conf->colorGrid[ind].first)
                                    conf->colorGrid[ind].first = &cs->color.front();
                                if (!conf->colorGrid[ind].second)
                                    conf->colorGrid[ind].second = &cs->color.back();
                            }
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
            memcpy(newgrid + row * x, conf->mainGrid->grid + row * conf->mainGrid->x,
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

void ModifyColorDragZone(bool inverse = false, bool clear = false) {
    AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(eItem);
    if (grp) {
        for (int x = dragZone.left; x <= dragZone.right; x++)
            for (int y = dragZone.top; y <= dragZone.bottom; y++)
                if (conf->mainGrid->grid[ind(x, y)]) {
                    auto pos = find_if(grp->lights.begin(), grp->lights.end(),
                        [x, y](auto t) {
                            return t.first == LOWORD(conf->mainGrid->grid[ind(x, y)]) &&
                                t.second == HIWORD(conf->mainGrid->grid[ind(x, y)]);
                        });
                    if (inverse && pos != grp->lights.end()) {
                        grp->lights.erase(pos);
                        continue;
                    }
                    if (!clear && pos == grp->lights.end()) {
                        grp->lights.push_back({ (DWORD)LOWORD(conf->mainGrid->grid[ind(x, y)]), (DWORD)HIWORD(conf->mainGrid->grid[ind(x, y)]) });
                        // Sort!
                        sort(grp->lights.begin(), grp->lights.end(),
                            [](auto t, auto t2) {
                                return t.first == t2.first ? t2.second > t.second : t2.first > t.first;
                            });
                    }
                }
        // now check for power...
        grp->have_power = find_if(grp->lights.begin(), grp->lights.end(),
            [](auto t) {
                return conf->afx_dev.GetFlags(t.first, (WORD)t.second) & ALIENFX_FLAG_POWER;
            }) != grp->lights.end();

            groupset* map = FindMapping(eItem);
            if (map && map->gauge)
                conf->SortGroupGauge(map);
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
        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(3, 20));

        if (lastTab != TAB_DEVICES) {
            EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), false);
            EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), false);
        }

        RepaintGrid(hDlg);
	} break;
    case WM_COMMAND: {
        switch (LOWORD(wParam))
        {
        case IDC_BUT_CLEARGRID:
            if (lastTab == TAB_DEVICES)
                if (eLid >= 0) {
                    for (int ind = 0; ind < conf->mainGrid->x * conf->mainGrid->y; ind++)
                        if (conf->mainGrid->grid[ind] == MAKELPARAM(devID, eLid))
                            conf->mainGrid->grid[ind] = 0;
                    RedrawGridButtonZone();
                }
            else
                if (eItem > 0) {
                    AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(eItem);
                    dragZone = { 0, 0, conf->mainGrid->x - 1, conf->mainGrid->y - 1 };
                    ModifyColorDragZone(true, true);
                    RedrawGridButtonZone();
                    UpdateZoneList();
                }
            break;
        }
    } break;
    case WM_RBUTTONUP: {
        // remove grid mapping
        if (lastTab == TAB_DEVICES && TranslateClick(hDlg, lParam)) {
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
            if (lastTab == TAB_DEVICES) {
                if (dragZone.bottom - dragZone.top || dragZone.right - dragZone.left || !oldClkValue) {
                    // just set zone
                    ModifyDragZone(devID, eLid);
                    FindCreateMapping();
                }
                else {
                    // single click at assigned grid
                    if (oldClkValue == MAKELPARAM(devID, eLid))
                        conf->mainGrid->grid[ind(dragZone.left, dragZone.top)] = 0;
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
            }
            else {
                ModifyColorDragZone(!(dragZone.bottom - dragZone.top || dragZone.right - dragZone.left));
                UpdateZoneList();
                RedrawGridButtonZone();
            }
        }
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            // drag
            if (TranslateClick(hDlg, lParam) && dragZone.top >= 0) {
                if (lastTab == TAB_DEVICES)
                    ModifyDragZone(devID, eLid, true);
                else
                    ModifyColorDragZone(true);
                dragZone = { min(clkPoint.x, dragStart.x), min(clkPoint.y, dragStart.y),
                    max(clkPoint.x, dragStart.x), max(clkPoint.y, dragStart.y) };
                if (lastTab == TAB_DEVICES)
                    ModifyDragZone(devID, eLid);
                else
                    ModifyColorDragZone();
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
            HBRUSH Brush = NULL, Brush2 = NULL;
            DWORD gridVal = conf->mainGrid->grid[(ditem->CtlID - 2000)];
            if (lastTab == TAB_DEVICES) {
                WORD idVal = HIWORD(gridVal) << 4;
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
            }
            else {
                if (gridVal) {
                    pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*> lightcolors = conf->colorGrid[ditem->CtlID - 2000];
                    if (lightcolors.first == NULL) {
                        // not setted
                        Brush = CreateSolidBrush(RGB(0, 0, 0));
                        Brush2 = CreateSolidBrush(RGB(0, 0, 0));
                    }
                    else {
                        // setted
                        Brush = CreateSolidBrush(RGB(lightcolors.first->r, lightcolors.first->g, lightcolors.first->b));
                        Brush2 = CreateSolidBrush(RGB(lightcolors.second->r, lightcolors.second->g, lightcolors.second->b));
                    }
                }
                else {
                    Brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
                    Brush2 = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
                }
                POINT triangle[3] = { {ditem->rcItem.left, ditem->rcItem.bottom},
                    {ditem->rcItem.left, ditem->rcItem.top},
                    {ditem->rcItem.right, ditem->rcItem.top}
                }, triangle2[3] = { {ditem->rcItem.right, ditem->rcItem.top},
                    {ditem->rcItem.right, ditem->rcItem.bottom},
                    {ditem->rcItem.left, ditem->rcItem.bottom}
                };
                SelectObject(ditem->hDC, GetStockObject(NULL_PEN));
                SelectObject(ditem->hDC, Brush);
                Polygon(ditem->hDC, triangle, 3);
                SelectObject(ditem->hDC, Brush2);
                Polygon(ditem->hDC, triangle2, 3);
                DeleteObject(Brush);
                DeleteObject(Brush2);
                if (gridVal)
                {
                    AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(eItem);
                    if (grp && find_if(grp->lights.begin(), grp->lights.end(),
                        [gridVal](auto lgh) {
                            return lgh.first == LOWORD(gridVal) && lgh.second == HIWORD(gridVal);
                        }) != grp->lights.end())
                        DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_SUNKEN, BF_MONO | BF_RECT);
                }
            }
            if (!gridVal)
                DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_RAISED, BF_RECT);
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
