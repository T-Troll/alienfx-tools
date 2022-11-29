#include "alienfx-gui.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern AlienFX_SDK::Afx_action* Code2Act(AlienFX_SDK::Afx_colorcode* c);
extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void UpdateZoneList();
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::Afx_group* grp);

extern void SetLightInfo();
extern void RedrawDevList();

extern AlienFX_SDK::Afx_device* FindActiveDevice();
extern FXHelper* fxhl;

extern int eLid, dIndex;
extern bool whiteTest;
extern int tabLightSel;

extern HWND zsDlg;

POINT clkPoint, dragStart;
RECT dragZone, buttonZone;
DWORD oldClkValue;

HWND tipH = NULL, tipV = NULL;

HWND cgDlg;

int minGridX, minGridY, maxGridX, maxGridY, bSize;

extern BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

AlienFX_SDK::Afx_light* FindCreateMapping(bool allowKey) {
    AlienFX_SDK::Afx_device* dev = FindActiveDevice();
    AlienFX_SDK::Afx_light* lgh = conf->afx_dev.GetMappingByDev(dev, eLid);
    if (dev && !lgh) {
        // create new mapping
        dev->lights.push_back({ (WORD)eLid, 0, "Light " + to_string(eLid + 1) });
        if (dev->dev) {
            conf->afx_dev.activeLights++;
            // for rgb keyboards, check key...
            if (allowKey && dev->dev->IsHaveGlobal())
                DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_KEY), NULL, (DLGPROC)KeyPressDialog);
        }
        lgh = &dev->lights.back();
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
    MapWindowPoints(bblock, cgDlg, (LPPOINT)&buttonZone, 1);
    buttonZone.right /= conf->mainGrid->x;
    buttonZone.bottom /= conf->mainGrid->y;
    LONGLONG bId = 2000;
    for (int y = 0; y < conf->mainGrid->y; y++)
        for (int x = 0; x < conf->mainGrid->x; x++) {
            int size = 1;
            while (x + size < conf->mainGrid->x &&
                conf->mainGrid->grid[ind(x, y)].lgh == conf->mainGrid->grid[ind(x + size, y)].lgh) size++;
            if (conf->mainGrid->grid[ind(x, y)].lgh) {
                // now ajust sizes
                minGridX = min(minGridX, x);
                minGridY = min(minGridY, y);
                maxGridX = max(maxGridX, x+size-1);
                maxGridY = max(maxGridY, y);
            }
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
                buttonZone.left + x * buttonZone.right, buttonZone.top + y * buttonZone.bottom, size * buttonZone.right, buttonZone.bottom, cgDlg, (HMENU)bId++,
                GetModuleHandle(NULL), NULL);
            SetWindowLongPtr(btn, GWLP_USERDATA, (LONG_PTR)(ind(x,y)));
            x+=size-1;
        }
}

void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false) {
    // Now refresh final grids...
    RECT full = what ? *what : RECT({ 0, 0, conf->mainGrid->x, conf->mainGrid->y });

    if (recalc) {
        if (conf->colorGrid)
            delete[] conf->colorGrid;
        conf->colorGrid = new gridClr[conf->mainGrid->x * conf->mainGrid->y]{};
        for (auto cs = conf->activeProfile->lightsets.rbegin(); cs != conf->activeProfile->lightsets.rend(); cs++) {
            AlienFX_SDK::Afx_group* grp = conf->afx_dev.GetGroupById(cs->group);
            if (grp)
                //for (auto clgh = grp->lights.begin(); clgh < grp->lights.end(); clgh++) {
                    for (int x = full.left; x < full.right; x++)
                        for (int y = full.top; y < full.bottom; y++) {
                            int ind = ind(x, y);
                            if (IsLightInGroup(conf->mainGrid->grid[ind].lgh, grp)) {
                                if (conf->enableMon)
                                    switch (conf->GetEffect()) {
                                    case 1: { // monitoring
                                        for (auto i = cs->events.begin(); i != cs->events.end(); i++) {
                                                if (!conf->colorGrid[ind].first && !cs->fromColor) {
                                                    conf->colorGrid[ind].first = &i->from;
                                                }
                                                conf->colorGrid[ind].last = &i->to;
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
                                    if (!conf->colorGrid[ind].last)
                                        conf->colorGrid[ind].last = &cs->color.back();
                                }
                            }
                        }
                //}
        }
    }
    RECT pRect;
    GetWindowRect(GetDlgItem(cgDlg, IDC_BUTTON_ZONE), &pRect);
    MapWindowPoints(HWND_DESKTOP, cgDlg, (LPPOINT)&pRect, 2);
    int sizeX = (pRect.right - pRect.left) / conf->mainGrid->x,
        sizeY = (pRect.bottom - pRect.top) / conf->mainGrid->y;
    pRect.right = pRect.left + full.right * sizeX;
    pRect.bottom = pRect.top + full.bottom * sizeY;
    pRect.left += sizeX * full.left;
    pRect.top += sizeY * full.top;
    RedrawWindow(cgDlg, &pRect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN);
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
    AlienFX_SDK::Afx_groupLight* newgrid = new AlienFX_SDK::Afx_groupLight[x * y]{ 0 };
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
    DWORD newVal = MAKELPARAM(did, lid);
    for (int x = dragZone.left; x < dragZone.right; x++)
        for (int y = dragZone.top; y < dragZone.bottom; y++) {
            if (!newVal || conf->mainGrid->grid[ind(x, y)].lgh == newVal)
                conf->mainGrid->grid[ind(x, y)].lgh = 0;
            else
                if (!conf->mainGrid->grid[ind(x,y)].lgh)
                    conf->mainGrid->grid[ind(x,y)].lgh = newVal;
        }
    conf->zoneMaps.clear();
    InitGridButtonZone();
}

void ModifyColorDragZone(bool clear = false) {
    AlienFX_SDK::Afx_group* grp = conf->afx_dev.GetGroupById(eItem);
    vector<AlienFX_SDK::Afx_groupLight> markRemove, markAdd;
    if (grp && dragZone.top >= 0) {
        for (int x = dragZone.left; x < dragZone.right; x++)
            for (int y = dragZone.top; y < dragZone.bottom; y++) {
                int ind = ind(x, y);
                if (conf->mainGrid->grid[ind].lgh) {
                    if (IsLightInGroup(conf->mainGrid->grid[ind].lgh, grp))
                        markRemove.push_back(conf->mainGrid->grid[ind]);
                    else
                        if (!clear)
                            markAdd.push_back(conf->mainGrid->grid[ind]);
                }
            }
        // now clear by remove list and add new...
        for (auto tr = markRemove.begin(); tr < markRemove.end(); tr++) {
            for (auto pos = grp->lights.begin(); pos < grp->lights.end(); pos++)
                if (pos->lgh == tr->lgh) {
                    grp->lights.erase(pos);
                    break;
                }
        }
        for (auto tr = markAdd.begin(); tr < markAdd.end(); tr++) {
            if (!IsLightInGroup(tr->lgh, grp))
                grp->lights.push_back(*tr);
        }
        // now check for power...
        for (auto gpos = grp->lights.begin(); gpos != grp->lights.end(); gpos++) {
            if (grp->have_power = conf->afx_dev.GetFlags(gpos->did, gpos->lid) & ALIENFX_FLAG_POWER)
                break;
        }

        for (auto gpos = conf->zoneMaps.begin(); gpos != conf->zoneMaps.end(); gpos++)
            if (gpos->gID == grp->gid) {
                conf->zoneMaps.erase(gpos);
                break;
            }

        conf->SortGroupGauge(grp->gid);
        UpdateZoneList();
        RedrawGridButtonZone(NULL, true);
    }
}

void TranslateClick(LPARAM lParam) {
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
    }
    else {
        clkPoint = { -1, -1 };
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

	switch (message) {
	case WM_INITDIALOG:
	{
        cgDlg = hDlg;

        if (tabLightSel == TAB_DEVICES) {
            tipH = CreateToolTip(gridX, tipH);
            tipV = CreateToolTip(gridY, tipV);

            SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(3, 80));
            SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(3, 20));
        } else {
            EnableWindow(gridX, false);
            EnableWindow(gridY, false);
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
                    AlienFX_SDK::Afx_groupLight cur = { FindActiveDevice()->pid, (WORD)eLid };
                    for (int ind = 0; ind < conf->mainGrid->x * conf->mainGrid->y; ind++)
                        if (conf->mainGrid->grid[ind].lgh == cur.lgh)
                            conf->mainGrid->grid[ind].lgh = 0;
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
            ModifyDragZone(0, 0);
            //InitGridButtonZone();
        }
        else
            ModifyColorDragZone(true);
        dragZone = { -1, -1, -1, -1 };
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
                DWORD oldLightValue = conf->mainGrid->grid[ind(dragZone.left, dragZone.top)].lgh;
                auto dev = FindActiveDevice();
                if (dragZone.bottom - dragZone.top == 1 && dragZone.right - dragZone.left == 1 &&
                   oldLightValue && oldLightValue != MAKELPARAM(dev->pid, eLid)) {
                    // switch light
                    if (LOWORD(oldLightValue) != dev->pid) {
                        // Switch device, if possible
                        for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++)
                            if (conf->afx_dev.fxdevs[i].pid == LOWORD(oldLightValue)) {
                                dIndex = i;
                                RedrawDevList();
                            }
                    }
                    eLid = HIWORD(oldLightValue);
                } else {
                    ModifyDragZone(dev->pid, eLid);
                    FindCreateMapping(true);
                }
                SetLightInfo();
            }
            else {
                ModifyColorDragZone();
                fxhl->Refresh();
            }
            dragZone = { -1 , -1, -1, -1 };
        }
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON || wParam & MK_RBUTTON) {
            // drag
            TranslateClick(lParam);
            if (clkPoint.x >=0) {
                RECT oldDragZone = dragZone;
                if (dragZone.top < 0)
                    dragStart = clkPoint;
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
        if ((HWND)lParam == gridX) {
            if (LOWORD(wParam) == TB_ENDTRACK) {
                SetLightGridSize((int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), conf->mainGrid->y);
                SendMessage((HWND)lParam, TBM_SETPOS, true, conf->mainGrid->x);
            }
            SetSlider(tipH, conf->mainGrid->x);
        }
        break;
    case WM_VSCROLL:
        if ((HWND)lParam == gridY) {
            if (LOWORD(wParam) == TB_ENDTRACK) {
                SetLightGridSize(conf->mainGrid->x, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                SendMessage((HWND)lParam, TBM_SETPOS, true, conf->mainGrid->y);
            }
            SetSlider(tipV, conf->mainGrid->y);
        }
        break;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
        if (ditem->CtlID >= 2000) {
            int ind = (int)GetWindowLongPtr(ditem->hwndItem, GWLP_USERDATA);
            DWORD gridVal = conf->mainGrid->grid[ind].lgh;
            int x = ind % conf->mainGrid->x, y = ind / conf->mainGrid->x,
                size = ditem->rcItem.right / buttonZone.right;
            HDC wDC = ditem->hDC;
            if (tabLightSel == TAB_DEVICES) {
                if (gridVal) {
                    HBRUSH Brush = CreateSolidBrush(gridVal == MAKELPARAM(FindActiveDevice()->pid, eLid) ?
                        RGB(conf->testColor.r, conf->testColor.g, conf->testColor.b) :
                        RGB(0xff - (HIWORD(gridVal) << 5), LOWORD(gridVal), HIWORD(gridVal) << 1));
                    FillRect(wDC, &ditem->rcItem, Brush);
                    DeleteObject(Brush);
                }
            }
            else {
                if (gridVal) {
                    RECT rectClip = ditem->rcItem;
                    //GRADIENT_RECT gRect{ 0,1 };
                    //TRIVERTEX vertex[2];
                    gridClr lightcolors = conf->colorGrid[ind];
                    if (lightcolors.first) {
                        // active
                        GRADIENT_RECT gRect{ 0,1 };
                        TRIVERTEX vertex[2]{{ rectClip.left, rectClip.top, (COLOR16)(lightcolors.first->r << 8),
                            (COLOR16)(lightcolors.first->g << 8),
                            (COLOR16)(lightcolors.first->b << 8) },
                        { rectClip.right, rectClip.bottom, (COLOR16)(lightcolors.last->r << 8),
                            (COLOR16)(lightcolors.last->g << 8),
                            (COLOR16)(lightcolors.last->b << 8) } };
                        GradientFill(wDC, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
                    }
                    if (IsLightInGroup(gridVal, conf->afx_dev.GetGroupById(eItem))) {
                        for (int cx = 0; cx < size; cx++) {
                            rectClip.right = rectClip.left + buttonZone.right;
                            int border = (cx == 0 * BF_LEFT) |
                                (cx == size - 1) * BF_RIGHT |
                                (!y || gridVal != conf->mainGrid->grid[ind + cx - conf->mainGrid->x].lgh) * BF_TOP |
                                (y == conf->mainGrid->y - 1 || gridVal != conf->mainGrid->grid[ind + cx + conf->mainGrid->x].lgh) * BF_BOTTOM;
                            DrawEdge(wDC, &rectClip, EDGE_SUNKEN, BF_MONO | border);
                            rectClip.left += buttonZone.right;
                        }
                    }
                }
            }
            // print name
            if (conf->showGridNames && gridVal) {
                AlienFX_SDK::Afx_device* dev = conf->afx_dev.GetDeviceById(LOWORD(gridVal), 0);
                AlienFX_SDK::Afx_light* lgh;
                if (dev && (lgh = conf->afx_dev.GetMappingByDev(dev, HIWORD(gridVal)))) {
                    SetBkMode(ditem->hDC, TRANSPARENT);
                    DrawText(ditem->hDC, lgh->name.c_str(), -1, &ditem->rcItem, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
                }
            }
            // Highlight if in selection zone
            RECT rectClip = ditem->rcItem;
            for (int cx = 0; cx < size; cx++) {
                rectClip.right = rectClip.left + buttonZone.right;
                if (PtInRect(&dragZone, { x + cx, y }))
                    DrawEdge(wDC, &rectClip, EDGE_SUNKEN, BF_RECT);
                else
                    if (!gridVal)
                        DrawEdge(wDC, &rectClip, EDGE_RAISED, BF_FLAT | BF_RECT);
                rectClip.left += buttonZone.right;
            }
        }
    } break;
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
