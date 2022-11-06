#include "alienfx-gui.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern AlienFX_SDK::afx_act* Code2Act(AlienFX_SDK::Colorcode* c);
extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void UpdateZoneList();
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::group* grp);

extern void SetLightInfo();
extern void RedrawDevList();

extern int eLid, dIndex;
extern bool whiteTest;
extern int tabLightSel, eItem;

extern HWND zsDlg;

POINT clkPoint, dragStart;
RECT dragZone, buttonZone;
DWORD oldClkValue;

HWND tipH = NULL, tipV = NULL;

HWND cgDlg;

int minGridX, minGridY, maxGridX, maxGridY;

extern BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

AlienFX_SDK::mapping* FindCreateMapping() {
    AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingByDev(&conf->afx_dev.fxdevs[dIndex], eLid);
    if (!lgh) {
        // create new mapping
        conf->afx_dev.fxdevs[dIndex].lights.push_back({ (WORD)eLid, 0, "Light " + to_string(eLid + 1) });
        conf->afx_dev.activeLights++;
        lgh = &conf->afx_dev.fxdevs[dIndex].lights.back();
        // for rgb keyboard, check key...
        switch (conf->afx_dev.fxdevs[dIndex].dev->GetVersion()) {
        case 5: case 8: case 9:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_KEY), NULL, (DLGPROC)KeyPressDialog);
            break;
        }
    }
    return lgh;
}

void InitGridButtonZone() {
    // init sizes
    minGridX = conf->mainGrid->x; minGridY = conf->mainGrid->y;
    maxGridX = maxGridY = -1;
    // delete zone buttons...
    for (DWORD bID = 2000; GetDlgItem(cgDlg, bID); bID++)
        DestroyWindow(GetDlgItem(cgDlg, bID));
    // Create zone buttons...
    HWND bblock = GetDlgItem(cgDlg, IDC_BUTTON_ZONE);
    GetClientRect(bblock, &buttonZone);
    RECT bzone = buttonZone;
    MapWindowPoints(bblock, cgDlg, (LPPOINT)&bzone, 1);
    bzone.right /= conf->mainGrid->x;
    bzone.bottom /= conf->mainGrid->y;
    LONGLONG bId = 2000;
    for (int y = 0; y < conf->mainGrid->y; y++)
        for (int x = 0; x < conf->mainGrid->x; x++) {
            int xx = x+1;
            if (conf->mainGrid->grid[ind(x, y)]) {
                for (; conf->mainGrid->grid[ind(x, y)] == conf->mainGrid->grid[ind(xx, y)]; xx++);
                // now ajust sizes
                minGridX = min(minGridX, x);
                minGridY = min(minGridY, y);
                maxGridX = max(maxGridX, x);
                maxGridY = max(maxGridY, y);
            }
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, (xx-x) * bzone.right, bzone.bottom, cgDlg, (HMENU)bId,
                GetModuleHandle(NULL), NULL);
            SetWindowLongPtr(btn, GWLP_USERDATA, (LONG_PTR)(y * conf->mainGrid->x + x));
            bId++;
            x = xx - 1;
        }
}

void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false) {
    // Now refresh final grids...
    RECT full{ 0, 0, conf->mainGrid->x, conf->mainGrid->y };
    if (what) {
        full = *what;
    }
    if (recalc) {
        if (conf->colorGrid)
            delete conf->colorGrid;
        conf->colorGrid = new pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*>[conf->mainGrid->x * conf->mainGrid->y]{};
        for (auto cs = conf->activeProfile->lightsets.rbegin(); cs < conf->activeProfile->lightsets.rend(); cs++) {
            AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(cs->group);
            if (grp)
                //for (auto clgh = grp->lights.begin(); clgh < grp->lights.end(); clgh++) {
                    for (int x = full.left; x < full.right; x++)
                        for (int y = full.top; y < full.bottom; y++) {
                            int ind = ind(x, y);
                            if (IsLightInGroup(conf->mainGrid->grid[ind], grp)) {
                                if (conf->enableMon)
                                    switch (conf->GetEffect()) {
                                    case 1: { // monitoring
                                        for (auto i = cs->events.begin(); i != cs->events.end(); i++) {
                                                if (!conf->colorGrid[ind].first && !cs->fromColor) {
                                                    conf->colorGrid[ind].first = &i->from;
                                                }
                                                conf->colorGrid[ind].second = &i->to;
                                            }
                                    } break;
                                    case 3: // haptics
                                        if (cs->haptics.size()) {
                                            conf->colorGrid[ind] = { Code2Act(&cs->haptics.front().colorfrom), Code2Act(&cs->haptics.back().colorto) };
                                        }
                                        break;
                                    case 4: // grid effects
                                        if (cs->effect.trigger) {
                                            conf->colorGrid[ind] = { Code2Act(&cs->effect.from), Code2Act(&cs->effect.to) };
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
                //}
        }
    }
    RECT pRect;// = buttonZone;
    GetWindowRect(GetDlgItem(cgDlg, IDC_BUTTON_ZONE), &pRect);
    MapWindowPoints(HWND_DESKTOP, cgDlg, (LPPOINT)&pRect, 2);
    int sizeX = (pRect.right - pRect.left) / conf->mainGrid->x,
        sizeY = (pRect.bottom - pRect.top) / conf->mainGrid->y;
    pRect.right = pRect.left + full.right * sizeX;
    pRect.bottom = pRect.top + full.bottom * sizeY;
    pRect.left += sizeX * full.left;
    pRect.top += sizeY * full.top;
    RedrawWindow(cgDlg, &pRect, 0, RDW_INVALIDATE | /*RDW_UPDATENOW |*/ RDW_ALLCHILDREN);
}

void SetLightGridSize(int x, int y) {
    int minX = 0, minY = 0;
    if (x <= maxGridX) {
        // cut from left
        minX = min(minGridX, maxGridX - x + 1);
        x = max(x, maxGridX - minGridX + 1);
    }
    if (y <= maxGridY) {
        // cut from top
        minY = min(minGridY, maxGridY - y + 1);
        y = max(y, maxGridY - minGridY + 1);
    }
    DWORD* newgrid = new DWORD[x * y]{ 0 };
    for (int row = 0; row < min(conf->mainGrid->y, y); row++)
        memcpy(newgrid + row * x, conf->mainGrid->grid + (row + minY) * conf->mainGrid->x + minX,
            min(conf->mainGrid->x, x) * sizeof(DWORD));
    delete[] conf->mainGrid->grid;
    conf->mainGrid->grid = newgrid;
    conf->mainGrid->x = x;
    conf->mainGrid->y = y;
    InitGridButtonZone();
}

void ModifyDragZone(WORD did, WORD lid) {
    for (int x = dragZone.left; x < dragZone.right; x++)
        for (int y = dragZone.top; y < dragZone.bottom; y++) {
            if (conf->mainGrid->grid[ind(x, y)] == MAKELPARAM(did, lid))
                conf->mainGrid->grid[ind(x, y)] = 0;
            else
                if (!conf->mainGrid->grid[ind(x,y)])
                    conf->mainGrid->grid[ind(x,y)] = MAKELPARAM(did, lid);
        }
}

void ModifyColorDragZone(bool clear = false) {
    AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(eItem);
    vector<DWORD> markRemove, markAdd;
    if (grp) {
        for (int x = dragZone.left; x < dragZone.right; x++)
            for (int y = dragZone.top; y < dragZone.bottom; y++) {
                int ind = ind(x, y);
                if (conf->mainGrid->grid[ind]) {
                    if (IsLightInGroup(conf->mainGrid->grid[ind], grp))
                        markRemove.push_back(conf->mainGrid->grid[ind]);
                    else
                        if (!clear)
                            markAdd.push_back(conf->mainGrid->grid[ind]);
                }
            }
        // now clear by remove list and add new...
        for (auto tr = markRemove.begin(); tr < markRemove.end(); tr++) {
            for (auto pos = grp->lights.begin(); pos < grp->lights.end(); pos++)
                if (*pos == *tr) {
                    grp->lights.erase(pos);
                    break;
                }
        }
        for (auto tr = markAdd.begin(); tr < markAdd.end(); tr++) {
            if (!IsLightInGroup(*tr, grp))
                grp->lights.push_back(*tr);
        }
        // now check for power...
        grp->have_power = find_if(grp->lights.begin(), grp->lights.end(),
            [](auto t) {
                return conf->afx_dev.GetFlags(LOWORD(t), HIWORD(t)) & ALIENFX_FLAG_POWER;
            }) != grp->lights.end();

        conf->SortGroupGauge(grp->gid);
    }
}

bool TranslateClick(LPARAM lParam) {
    clkPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    // check if in control window...
    RECT gRect;
    MapWindowPoints(cgDlg, HWND_DESKTOP, &clkPoint, 1);
    GetClientRect(GetDlgItem(cgDlg, IDC_BUTTON_ZONE), &gRect);
    ScreenToClient(GetDlgItem(cgDlg, IDC_BUTTON_ZONE), &clkPoint);
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

void RepaintGrid() {
    InitGridButtonZone();
    SendMessage(GetDlgItem(cgDlg, IDC_SLIDER_HSCALE), TBM_SETPOS, true, conf->mainGrid->x);
    SetSlider(tipH, conf->mainGrid->x);
    SendMessage(GetDlgItem(cgDlg, IDC_SLIDER_VSCALE), TBM_SETPOS, true, conf->mainGrid->y);
    SetSlider(tipV, conf->mainGrid->y);
    RedrawGridButtonZone(NULL, true);
}

BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    WORD devID = dIndex < 0 ? 0 : conf->afx_dev.fxdevs[dIndex].pid;

	switch (message) {
	case WM_INITDIALOG:
	{
        cgDlg = hDlg;

        tipH = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), tipH);
        tipV = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), tipV);

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(3, 80));
        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(3, 20));

        if (tabLightSel != TAB_DEVICES) {
            EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), false);
            EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), false);
        }

        RepaintGrid();
	} break;
    case WM_PAINT:
        cgDlg = hDlg;
        return false;
    case WM_COMMAND: {
        switch (LOWORD(wParam))
        {
        case IDC_BUT_CLEARGRID:
            if (tabLightSel == TAB_DEVICES) {
                if (eLid >= 0) {
                    for (int ind = 0; ind < conf->mainGrid->x * conf->mainGrid->y; ind++)
                        if (conf->mainGrid->grid[ind] == MAKELPARAM(devID, eLid))
                            conf->mainGrid->grid[ind] = 0;
                }
            }
            else
                if (eItem > 0) {
                    conf->afx_dev.GetGroupById(eItem)->lights.clear();
                    UpdateZoneList();
                }
            RedrawGridButtonZone();
            break;
        }
    } break;
    case WM_RBUTTONUP: {
        // remove grid mapping
        if (tabLightSel == TAB_DEVICES && dragZone.top >=0) {
            RECT oldDragZone = dragZone;
            for (int x = dragZone.left; x < dragZone.right; x++)
                for (int y = dragZone.top; y < dragZone.bottom; y++)
                    conf->mainGrid->grid[ind(x, y)] = 0;
            dragZone = { -1,-1,-1,-1 };
            InitGridButtonZone();
        }
    } break;
    case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: {
        // selection mark
        TranslateClick(lParam);
        dragZone = { clkPoint.x, clkPoint.y, clkPoint.x + 1, clkPoint.y + 1};
        dragStart = clkPoint;
        if (clkPoint.x >= 0)
            RedrawGridButtonZone(&dragZone);
    } break;
    case WM_LBUTTONUP:
        // end selection
        if (dragZone.top >= 0) {
            if (tabLightSel == TAB_DEVICES) {
                DWORD oldLightValue = conf->mainGrid->grid[ind(dragZone.left, dragZone.top)];
                if (dragZone.bottom - dragZone.top == 1 && dragZone.right - dragZone.left == 1 &&
                   oldLightValue && oldLightValue != MAKELPARAM(devID, eLid)) {
                    // switch light
                    dragZone = { -1,-1,-1,-1 };
                    if (LOWORD(oldLightValue) != devID) {
                        // Switch device, if possible
                        for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++)
                            if (conf->afx_dev.fxdevs[i].dev && conf->afx_dev.fxdevs[i].pid == LOWORD(oldLightValue)) {
                                dIndex = i;
                                RedrawDevList();
                            }
                    }
                    eLid = HIWORD(oldLightValue);
                    InitGridButtonZone();
                    SetLightInfo();
                } else {
                    ModifyDragZone(devID, eLid);
                    InitGridButtonZone();
                    dragZone = { -1,-1,-1,-1 };
                    FindCreateMapping();
                    SetLightInfo();
                }
            }
            else {
                ModifyColorDragZone();
                dragZone = { -1,-1,-1,-1 };
                UpdateZoneList();
                RedrawGridButtonZone(NULL, true);
                fxhl->Refresh();
            }
        }
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON || wParam & MK_RBUTTON) {
            // drag
            if (TranslateClick(lParam) && dragZone.top >= 0) {
                RECT oldDragZone = dragZone;
                dragZone = { min(clkPoint.x, dragStart.x), min(clkPoint.y, dragStart.y),
                    max(clkPoint.x + 1, dragStart.x + 1), max(clkPoint.y + 1, dragStart.y + 1) };
                if (dragZone.top != oldDragZone.top || dragZone.left != oldDragZone.left
                    || dragZone.bottom != oldDragZone.bottom || dragZone.right != oldDragZone.right) {
                    RedrawGridButtonZone(&oldDragZone);
                    RedrawGridButtonZone(&dragZone);
                }
            }
        }
        break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        /*case TB_THUMBPOSITION: */case TB_ENDTRACK:
            if ((HWND)lParam == gridX) {
                SetLightGridSize((int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), conf->mainGrid->y);
                SendMessage((HWND)lParam, TBM_SETPOS, true, conf->mainGrid->x);
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
                SetLightGridSize(conf->mainGrid->x, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                SendMessage((HWND)lParam, TBM_SETPOS, true, conf->mainGrid->y);
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
            HBRUSH Brush = NULL;// , Brush2 = NULL;
            int ind = (int)GetWindowLongPtr(ditem->hwndItem, GWLP_USERDATA);
            DWORD gridVal = conf->mainGrid->grid[ind];
            if (tabLightSel == TAB_DEVICES) {
                WORD idVal = HIWORD(gridVal) << 4;
                if (gridVal) {
                    if (HIWORD(gridVal) == eLid && LOWORD(gridVal) == devID)
                        Brush = CreateSolidBrush(RGB(conf->testColor.r, conf->testColor.g, conf->testColor.b));
                    else
                        Brush = CreateSolidBrush(RGB(0xff - (idVal << 1), 0, idVal & 0xff));
                    FillRect(ditem->hDC, &ditem->rcItem, Brush);
                    DeleteObject(Brush);
                }
                /*else
                    Brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));*/
                //FillRect(ditem->hDC, &ditem->rcItem, Brush);
                //DeleteObject(Brush);
            }
            else {
                if (gridVal) {
                    GRADIENT_RECT gRect{ 0,1 };
                    TRIVERTEX vertex[2]{ {ditem->rcItem.left, ditem->rcItem.top},
                        {ditem->rcItem.right, ditem->rcItem.bottom} };
                    pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*> lightcolors = conf->colorGrid[ind];
                    if (lightcolors.first) {
                        // active
                        vertex[0].Red = lightcolors.first->r << 8;
                        vertex[0].Green = lightcolors.first->g << 8;
                        vertex[0].Blue = lightcolors.first->b << 8;
                        vertex[1].Red = lightcolors.second->r << 8;
                        vertex[1].Green = lightcolors.second->g << 8;
                        vertex[1].Blue = lightcolors.second->b << 8;
                    }
                    GradientFill(ditem->hDC, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
                }
                if (gridVal && IsLightInGroup(gridVal, conf->afx_dev.GetGroupById(eItem)))
                    DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_SUNKEN, BF_MONO | BF_RECT);
            }
            // print name
            if (conf->showGridNames && gridVal) {
                AlienFX_SDK::afx_device* dev = conf->afx_dev.GetDeviceById(LOWORD(gridVal), 0);
                AlienFX_SDK::mapping* lgh;
                if (dev && (lgh = conf->afx_dev.GetMappingByDev(dev, HIWORD(gridVal)))) {
                    SetBkMode(ditem->hDC, TRANSPARENT);
                    DrawText(ditem->hDC, lgh->name.c_str(), -1, &ditem->rcItem, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
                }
            }
            // Highlight if in selection zone
            if (PtInRect(&dragZone, { (long)(ind % conf->mainGrid->x), (long)(ind / conf->mainGrid->x) })) {
                DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_SUNKEN, /*BF_MONO |*/ BF_RECT);
            }
            else
                if (!gridVal)
                    DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_RAISED, BF_FLAT | BF_RECT);
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

    TabCtrl_DeleteAllItems(gridTab);

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

    if (!GetWindowLongPtr(gridTab, GWLP_USERDATA)) {
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

    TabCtrl_SetCurSel(gridTab, conf->gridTabSel);
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

    RepaintGrid();
}
