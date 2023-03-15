#include "alienfx-gui.h"
#include "FXHelper.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern AlienFX_SDK::Afx_colorcode Act2Code(AlienFX_SDK::Afx_action* act);
extern void UpdateZoneList();
extern bool IsLightInGroup(DWORD lgh, AlienFX_SDK::Afx_group* grp);
extern void RemoveLightFromGroup(AlienFX_SDK::Afx_group* grp, AlienFX_SDK::Afx_groupLight lgh);

extern void SetLightInfo();
extern void RedrawDevList();

extern FXHelper* fxhl;

extern int eLid;
extern int tabLightSel;

extern HWND zsDlg;

POINT clkPoint, dragStart;
RECT dragZone, buttonZone;
DWORD oldClkValue;

HWND tipH = NULL, tipV = NULL;

HWND cgDlg;

int minGridX, minGridY, maxGridX, maxGridY, bSize;

gridClr* colorGrid = NULL;

extern BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern AlienFX_SDK::Afx_light* keySetLight;
extern AlienFX_SDK::Afx_device* activeDevice;

void FindCreateMapping() {
    if (!(keySetLight = conf->afx_dev.GetMappingByDev(activeDevice, eLid))) {
        // create new mapping
        activeDevice->lights.push_back({ (byte)eLid, {0,0}, "Light " + to_string(eLid + 1) });
        keySetLight = &activeDevice->lights.back();
        if (activeDevice->dev) {
            conf->afx_dev.activeLights++;
            // for rgb keyboards, check key...
            if (activeDevice->dev->IsHaveGlobal()) {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_KEY), NULL, (DLGPROC)KeyPressDialog);
            }
        }
        else
            keySetLight->name = "Light " + to_string(eLid + 1);
    }
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

void RedrawGridButtonZone(RECT* what = NULL) {
    // Now refresh final grids...
    RECT full = what ? *what : RECT({ 0, 0, conf->mainGrid->x, conf->mainGrid->y });
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

static AlienFX_SDK::Afx_colorcode ambient_grid{ 0xff,0xff,0xff };

void RecalcGridZone(RECT* what = NULL) {
    RECT full{ 0, 0, conf->mainGrid->x, conf->mainGrid->y };
    if (colorGrid) {
        if (what)
            full = *what;
    } else
        colorGrid = new gridClr[conf->mainGrid->x * conf->mainGrid->y];

    AlienFX_SDK::Afx_group* grp;
    for (int x = full.left; x < full.right; x++)
        for (int y = full.top; y < full.bottom; y++) {
            int ind = ind(x, y);
            colorGrid[ind].first.br = colorGrid[ind].last.br = 0xff;
            for (auto cs = conf->activeProfile->lightsets.rbegin(); cs != conf->activeProfile->lightsets.rend(); cs++)
                if ((grp = conf->afx_dev.GetGroupById(cs->group)) && IsLightInGroup(conf->mainGrid->grid[ind].lgh, grp)) {
                    if (conf->stateEffects) {
                        if (cs->events.size()) {
                            if (colorGrid[ind].first.br == 0xff && !(cs->fromColor && cs->color.size()))
                                colorGrid[ind].first = Act2Code(&cs->events.front().from);
                            colorGrid[ind].last = Act2Code(&cs->events.back().to);
                        }
                        if (cs->haptics.size()) {
                            colorGrid[ind] = { cs->haptics.front().colorfrom, cs->haptics.back().colorto };
                        }
                        if (cs->ambients.size()) {
                            colorGrid[ind].first = colorGrid[ind].last = ambient_grid;
                        }
                        if (cs->effect.trigger) {
                            if (cs->effect.trigger == 4)
                                colorGrid[ind].first = colorGrid[ind].last = ambient_grid;
                            else
                                if (cs->effect.effectColors.size())
                                    colorGrid[ind] = { cs->effect.effectColors.front(), cs->effect.effectColors.back() };
                        }
                    }
                    if (cs->color.size()) {
                        if (colorGrid[ind].first.br == 0xff)
                            colorGrid[ind].first = Act2Code(&cs->color.front());
                        if (colorGrid[ind].last.br == 0xff)
                            colorGrid[ind].last = Act2Code(&cs->color.back());
                    }
                }
        }
}

//void RedrawZoneGrid(DWORD grpID, bool repaint, bool recalc = false) {
//    zonemap zone = *conf->FindZoneMap(grpID, recalc);
//    if (zone.gridID == conf->mainGrid->id) {
//        RECT zRect = { zone.gMinX, zone.gMinY, zone.xMax + 1, zone.yMax + 1 };
//        RecalcGridZone(&zRect);
//        if (repaint)
//            RedrawGridButtonZone(&zRect);
//    }
//}

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
    if (colorGrid) {
        delete[] colorGrid;
        colorGrid = NULL;
    }
    InitGridButtonZone();
}

void ModifyDragZone(AlienFX_SDK::Afx_groupLight lgh) {
    for (int x = dragZone.left; x < dragZone.right; x++)
        for (int y = dragZone.top; y < dragZone.bottom; y++) {
            if (!lgh.lgh || conf->mainGrid->grid[ind(x, y)].lgh == lgh.lgh)
                conf->mainGrid->grid[ind(x, y)].lgh = 0;
            else
                if (!conf->mainGrid->grid[ind(x,y)].lgh)
                    conf->mainGrid->grid[ind(x,y)] = lgh;
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
            RemoveLightFromGroup(grp, *tr);
        }
        for (auto tr = markAdd.begin(); tr < markAdd.end(); tr++) {
            if (!IsLightInGroup(tr->lgh, grp))
                grp->lights.push_back(*tr);
        }
        //RedrawZoneGrid(grp->gid, false, true);
        conf->FindZoneMap(grp->gid, true);
        RecalcGridZone();
        UpdateZoneList();
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
    RecalcGridZone();
    RedrawGridButtonZone();
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
    case WM_COMMAND: {
        switch (LOWORD(wParam))
        {
        case IDC_BUT_CLEARGRID:
            if (tabLightSel == TAB_DEVICES) {
                if (eLid >= 0) {
                    AlienFX_SDK::Afx_groupLight cur = { activeDevice->pid, (WORD)eLid };
                    for (int ind = 0; ind < conf->mainGrid->x * conf->mainGrid->y; ind++)
                        if (conf->mainGrid->grid[ind].lgh == cur.lgh)
                            conf->mainGrid->grid[ind].lgh = 0;
                }
            }
            else
                if (eItem > 0) {
                    conf->afx_dev.GetGroupById(eItem)->lights.clear();
                }
            RepaintGrid();
            break;
        }
    } break;
    case WM_RBUTTONUP: {
        // remove grid mapping
        if (dragZone.top >= 0)
            if (tabLightSel == TAB_DEVICES)
                ModifyDragZone({ 0 });
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
                if (dragZone.bottom - dragZone.top == 1 && dragZone.right - dragZone.left == 1 &&
                   oldLightValue && oldLightValue != MAKELPARAM(activeDevice->pid, eLid)) {
                    // switch light
                    eLid = HIWORD(oldLightValue);
                    if (LOWORD(oldLightValue) != activeDevice->pid) {
                        // Switch device, if possible
                        activeDevice = conf->afx_dev.GetDeviceById(LOWORD(oldLightValue));
                    }
                    RedrawDevList();
                } else {
                    ModifyDragZone({ activeDevice->pid, (byte)eLid });
                    FindCreateMapping();
                    SetLightInfo();
                }
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
            if (gridVal) {
                if (tabLightSel == TAB_DEVICES) {
                    HBRUSH Brush = CreateSolidBrush(gridVal == MAKELPARAM(activeDevice->pid, eLid) ?
                        RGB(conf->testColor.r, conf->testColor.g, conf->testColor.b) :
                        RGB(0xff - (HIWORD(gridVal) << 5), LOWORD(gridVal), HIWORD(gridVal) << 1));
                    FillRect(wDC, &ditem->rcItem, Brush);
                    DeleteObject(Brush);
                }
                else {
                    RECT rectClip = ditem->rcItem;
                    gridClr lightcolors = colorGrid[ind];
                    if (lightcolors.first.br != 0xff) {
                        // active
                        GRADIENT_RECT gRect{ 0,1 };
                        TRIVERTEX vertex[2]{ { rectClip.left, rectClip.top, (COLOR16)(lightcolors.first.r << 8),
                            (COLOR16)(lightcolors.first.g << 8),
                            (COLOR16)(lightcolors.first.b << 8) },
                        { rectClip.right, rectClip.bottom, (COLOR16)(lightcolors.last.r << 8),
                            (COLOR16)(lightcolors.last.g << 8),
                            (COLOR16)(lightcolors.last.b << 8) } };
                        GradientFill(wDC, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
                    }
                    else {
                        // inactive
                        HBRUSH Brush = CreateHatchBrush(HS_DIAGCROSS, RGB(255, 255, 255));
                        FillRect(wDC, &ditem->rcItem, Brush);
                        DeleteObject(Brush);
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
                // print name
                if (conf->showGridNames) {
                    AlienFX_SDK::Afx_light* lgh = conf->afx_dev.GetMappingByID(LOWORD(gridVal), HIWORD(gridVal));
                    if (lgh) {
                        SetBkMode(ditem->hDC, TRANSPARENT);
                        DrawText(ditem->hDC, lgh->name.c_str(), -1, &ditem->rcItem, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
                    }
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
    HWND pHdr = (HWND)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    // Get the index of the selected tab.
    conf->gridTabSel = TabCtrl_GetCurSel(hwndDlg);
    if (conf->gridTabSel < 0) conf->gridTabSel = 0;
    conf->mainGrid = &conf->afx_dev.GetGrids()->at(conf->gridTabSel);
    if (colorGrid) {
        delete[] colorGrid;
        colorGrid = NULL;
    }

    RepaintGrid();
}
