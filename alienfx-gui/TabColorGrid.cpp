#include "alienfx-gui.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern colorset* FindMapping(int mid);

//extern void SetLightInfo(HWND hDlg);

extern void InitGridButtonZone(HWND);
//extern void RedrawGridButtonZone(HWND);
extern void SetLightGridSize(HWND dlg, int x, int y);
extern void RepaintGrid(HWND hDlg);
void RedrawGridButtonZone(bool recalc = false);
extern bool TranslateClick(HWND hDlg, LPARAM lParam);

extern AlienFX_SDK::lightgrid* mainGrid;
//extern int eLid, dIndex;//, devID;
//extern bool whiteTest;
//extern AlienFX_SDK::Mappings afx_dev;

//extern AlienFX_SDK::mapping* FindCreateMapping();
//extern void SetLightMap(HWND);

extern POINT clkPoint, dragStart;
extern RECT dragZone;
extern DWORD oldClkValue;
//
extern HWND tipH, tipV;
//
extern int gridTabSel, eItem;

HWND cgDlg = NULL;

pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*> colorGrid[30][15];
//
//AlienFX_SDK::mapping* FindCreateMapping() {
//    AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingById(conf->afx_dev.fxdevs[dIndex].dev->GetPID(), eLid);
//    if (!lgh) {
//        // create new mapping
//        lgh = new AlienFX_SDK::mapping({ 0, (WORD)conf->afx_dev.fxdevs[dIndex].dev->GetPID(), (WORD)eLid, 0,
//            "Light " + to_string(eLid + 1) });
//        conf->afx_dev.GetMappings()->push_back(lgh);
//        conf->afx_dev.fxdevs[dIndex].lights.push_back(conf->afx_dev.GetMappings()->back());
//    }
//    return lgh;
//}
//
//void InitGridButtonZone(HWND dlg) {
//    // delete zone buttons...
//    for (DWORD bID = 2000; GetDlgItem(dlg, bID); bID++)
//        DestroyWindow(GetDlgItem(dlg, bID));
//    // Create zone buttons...
//    HWND bblock = GetDlgItem(dlg, IDC_BUTTON_ZONE);
//    RECT bzone;
//    GetClientRect(bblock, &bzone);
//    MapWindowPoints(bblock, dlg, (LPPOINT)&bzone, 1);
//    bzone.right /= mainGrid->x;
//    bzone.bottom /= mainGrid->y;
//    LONGLONG bId = 2000;
//    for (int y = 0; y < mainGrid->y; y++)
//        for (int x = 0; x < mainGrid->x; x++) {
//            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
//                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId,
//                GetModuleHandle(NULL), NULL);
//            bId++;
//        }
//}
//
//void RedrawGridButtonZone(bool recalc = false) {
//    // Now refresh final grids...
//    if (recalc) {
//        for (int x = 0; x < mainGrid->x; x++)
//            for (int y = 0; y < mainGrid->y; y++)
//                colorGrid[x][y] = { NULL, NULL };
//        for (auto cs = conf->activeProfile->lightsets.colors.begin();
//            cs < conf->activeProfile->lightsets.colors.end(); cs++) {
//            for (auto cgrp = cs->groups.begin(); cgrp < cs->groups.end(); cgrp++)
//                for (auto clgh = (*cgrp)->lights.begin(); clgh < (*cgrp)->lights.end(); clgh++) {
//                    for (int x = 0; x < mainGrid->x; x++)
//                        for (int y = 0; y < mainGrid->y; y++)
//                            if (LOWORD(mainGrid->grid[x][y]) == clgh->first &&
//                                HIWORD(mainGrid->grid[x][y]) == clgh->second) {
//                                if (cs->color.size()) {
//                                    colorGrid[x][y] = { &cs->color.front(), &cs->color.back() };
//                                }
//                                if (conf->GetEffect() == 0) {// monitoring
//                                    if (cs->power.active) {
//                                        colorGrid[x][y].first = cs->fromColor ? colorGrid[x][y].first : &cs->power.from;
//                                        colorGrid[x][y].second = &cs->power.to;
//                                    }
//                                    if (cs->perf.active) {
//                                        colorGrid[x][y].first = cs->fromColor ? colorGrid[x][y].first : &cs->perf.from;
//                                        colorGrid[x][y].second = &cs->perf.to;
//                                    }
//                                    if (cs->events.active) {
//                                        colorGrid[x][y].first = cs->fromColor ? colorGrid[x][y].first : &cs->events.from;
//                                        colorGrid[x][y].second = &cs->events.to;
//                                    }
//                                }
//                            }
//                }
//        }
//    }
//    //for (int i = 0; i < mainGrid->x * mainGrid->y; i++)
//    //    RedrawWindow(GetDlgItem(cgDlg, 2000 + i), 0, 0, RDW_INVALIDATE);
//    RedrawWindow(cgDlg, 0, 0, RDW_INVALIDATE);
//}
//
//void SetLightGridSize(HWND dlg, int x, int y) {
//    mainGrid->x = x;
//    mainGrid->y = y;
//    InitGridButtonZone(dlg);
//}

void ModifyColorDragZone(WORD did, WORD lid, bool clear = false) {
    for (int x = dragZone.left; x <= dragZone.right; x++)
        for (int y = dragZone.top; y <= dragZone.bottom; y++) {
            if (!mainGrid->grid[x][y] && !clear)
                mainGrid->grid[x][y] = MAKELPARAM(did, lid);
            else
                if (clear && mainGrid->grid[x][y] == MAKELPARAM(did, lid))
                    mainGrid->grid[x][y] = 0;
        }
}

//bool TranslateClick(HWND hDlg, LPARAM lParam) {
//    clkPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
//    // check if in control window...
//    RECT gRect;
//    MapWindowPoints(hDlg, HWND_DESKTOP, &clkPoint, 1);
//    GetClientRect(GetDlgItem(hDlg, IDC_BUTTON_ZONE), &gRect);
//    ScreenToClient(GetDlgItem(hDlg, IDC_BUTTON_ZONE), &clkPoint);
//    if (PtInRect(&gRect, clkPoint)) {
//        // start dragging...
//        clkPoint = { clkPoint.x / (gRect.right / mainGrid->x),
//                     clkPoint.y / (gRect.bottom / mainGrid->y) };
//        return true;
//    }
//    else {
//        clkPoint = { -1, -1 };
//        return false;
//    }
//}

BOOL CALLBACK TabColorGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    //WORD devID = dIndex < 0 ? 0 : conf->afx_dev.fxdevs[dIndex].desc->devid;

	switch (message) {
	case WM_INITDIALOG:
	{
        cgDlg = hDlg;
        if (!conf->afx_dev.GetGrids()->size())
            conf->afx_dev.GetGrids()->push_back({ 0, 20, 8, "Main" });
        if (!mainGrid)
            mainGrid = &conf->afx_dev.GetGrids()->front();

        tipH = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), tipH);
        tipV = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), tipV);

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(3, 22));
        //SendMessage(gridX, TBM_SETPOS, true, mainGrid->x);


        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(3, 10));
        //SendMessage(gridY, TBM_SETPOS, true, mainGrid->y);

        RepaintGrid(hDlg);
	} break;
    case WM_RBUTTONUP: {
        // remove grid mapping
        if (TranslateClick(hDlg, lParam)) {
            mainGrid->grid[clkPoint.x][clkPoint.y] = 0;
            RedrawGridButtonZone();
        }
    } break;
    case WM_LBUTTONDOWN: {
        // selection mark
        if (TranslateClick(hDlg, lParam)) {
            oldClkValue = mainGrid->grid[clkPoint.x][clkPoint.y];
        }
        dragZone = { clkPoint.x, clkPoint.y, clkPoint.x, clkPoint.y };
        dragStart = clkPoint;
    } break;
    case WM_LBUTTONUP:
        // end selection
        if (dragZone.top >= 0) {
            if (dragZone.bottom - dragZone.top || dragZone.right - dragZone.left || !oldClkValue) {
                // just set zone
                // ModifyColorDragZone(devID, eLid);
                //FindCreateMapping();
            }
            else {
                // single click at assigned grid
                //if (oldClkValue == MAKELPARAM(devID, eLid))
                //    mainGrid->grid[dragZone.left][dragZone.top] = 0;
                //else {
                //    // change light to old one
                //    eLid = HIWORD(oldClkValue);
                //    auto pos = find_if(conf->afx_dev.fxdevs.begin(), conf->afx_dev.fxdevs.end(),
                //        [](auto t) {
                //            return t.dev->GetPID() == LOWORD(oldClkValue);
                //        });
                //    if (pos != conf->afx_dev.fxdevs.end())
                //        dIndex = (int)(pos - conf->afx_dev.fxdevs.begin());
                //}
            }
            // SetLightInfo(GetParent(GetParent(hDlg)));
            RedrawGridButtonZone();
        }
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            // drag
            if (TranslateClick(hDlg, lParam) && dragZone.top >= 0) {
                //ModifyColorDragZone(devID, eLid, true);
                dragZone = { min(clkPoint.x, dragStart.x), min(clkPoint.y, dragStart.y),
                    max(clkPoint.x, dragStart.x), max(clkPoint.y, dragStart.y) };
                //ModifyColorDragZone(devID, eLid);
                RedrawGridButtonZone();
            }
        }
        break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == gridX) {
                SetLightGridSize(hDlg, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), mainGrid->y);
                SetSlider(tipH, mainGrid->x);
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
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == gridY) {
                SetLightGridSize(hDlg, mainGrid->x, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                SetSlider(tipH, mainGrid->y);
            }
            break;
        default:
            if ((HWND)lParam == gridY) {
                SetSlider(tipH, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
        }
        break;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
        if (ditem->CtlID >= 2000) {
            DWORD gridVal = mainGrid->grid[(ditem->CtlID - 2000) % mainGrid->x][(ditem->CtlID - 2000) / mainGrid->x];
            HBRUSH Brush = NULL, Brush2 = NULL;
            if (gridVal) {
                pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*> lightcolors = colorGrid[(ditem->CtlID - 2000) % mainGrid->x][(ditem->CtlID - 2000) / mainGrid->x];
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
            //FillRect(ditem->hDC, &ditem->rcItem, Brush);
            DeleteObject(Brush);
            DeleteObject(Brush2);
            if (!gridVal)
                DrawEdge(ditem->hDC, &ditem->rcItem, /*selected ? EDGE_SUNKEN : */EDGE_RAISED, BF_RECT);
            else {
                colorset* mmap = FindMapping(eItem);
                if (mmap) {
                    if (find_if(mmap->groups.begin(), mmap->groups.end(),
                        [gridVal](auto tgrp) {
                            return find_if(tgrp->lights.begin(), tgrp->lights.end(),
                                [gridVal](auto lgh) {
                                    return lgh.first == LOWORD(gridVal) && lgh.second == HIWORD(gridVal);
                                }) != tgrp->lights.end();
                        }) != mmap->groups.end())
                        DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_SUNKEN, BF_MONO | BF_RECT);
                }
            }
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

//void CreateGridBlock(HWND gridTab) {
//
//    RECT rcDisplay;
//    TCITEM tie{ TCIF_TEXT };
//
//    GetClientRect(gridTab, &rcDisplay);
//
//    rcDisplay.left = GetSystemMetrics(SM_CXBORDER);
//    rcDisplay.top = GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER);
//    rcDisplay.right -= 2 * GetSystemMetrics(SM_CXBORDER) + 1;
//    rcDisplay.bottom -= GetSystemMetrics(SM_CYBORDER) + 1;
//
//    for (int i = 0; i < conf->afx_dev.GetGrids()->size(); i++) {
//        tie.pszText = (char*)conf->afx_dev.GetGrids()->at(i).name.c_str();
//        TabCtrl_InsertItem(gridTab, i, (LPARAM)&tie);
//    }
//
//    // Special tabs for add/remove
//    tie.pszText = LPSTR("+");
//    TabCtrl_InsertItem(gridTab, conf->afx_dev.GetGrids()->size(), (LPARAM)&tie);
//    tie.pszText = LPSTR("-");
//    TabCtrl_InsertItem(gridTab, conf->afx_dev.GetGrids()->size() + 1, (LPARAM)&tie);
//
//    TabCtrl_SetMinTabWidth(gridTab, 10);
//
//    HWND hwndDisplay = CreateDialogIndirect(GetModuleHandle(NULL),
//        (DLGTEMPLATE*)LockResource(LoadResource(GetModuleHandle(NULL), FindResource(NULL, MAKEINTRESOURCE(IDD_GRIDBLOCK), RT_DIALOG))),
//        gridTab, (DLGPROC)TabGrid);
//
//    SetWindowLongPtr(gridTab, GWLP_USERDATA, (LONG_PTR)hwndDisplay);
//
//    SetWindowPos(hwndDisplay, NULL,
//        rcDisplay.left, rcDisplay.top,
//        rcDisplay.right - rcDisplay.left,
//        rcDisplay.bottom - rcDisplay.top,
//        SWP_SHOWWINDOW);
//}
//
//void OnGridSelChanged(HWND hwndDlg)
//{
//    // Get the dialog header data.
//    HWND pHdr = (HWND)GetWindowLongPtr(
//        hwndDlg, GWLP_USERDATA);
//
//    // Get the index of the selected tab.
//    gridTabSel = TabCtrl_GetCurSel(hwndDlg);
//    mainGrid = &conf->afx_dev.GetGrids()->at(gridTabSel);
//
//    // Repaint.
//    RepaintGrid(pHdr);
//}
