#include "alienfx-gui.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern groupset* FindMapping(int mid);
extern void UpdateZoneList(HWND hDlg, byte flag = 0);

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
extern HWND tipH, tipV, zsDlg;
//
extern int gridTabSel, eItem;

HWND cgDlg = NULL;

pair<AlienFX_SDK::afx_act*, AlienFX_SDK::afx_act*> colorGrid[30][15];

void ModifyColorDragZone(bool inverse = false, bool clear = false) {
    AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(eItem);
    if (grp) {
        for (int x = dragZone.left; x <= dragZone.right; x++)
            for (int y = dragZone.top; y <= dragZone.bottom; y++)
                if (mainGrid->grid[ind(x, y)]) {
                    auto pos = find_if(grp->lights.begin(), grp->lights.end(),
                        [x, y](auto t) {
                            return t.first == LOWORD(mainGrid->grid[ind(x, y)]) &&
                                t.second == HIWORD(mainGrid->grid[ind(x, y)]);
                        });
                    if (inverse && pos != grp->lights.end()) {
                        grp->lights.erase(pos);
                        continue;
                    }
                    if (!clear && pos == grp->lights.end()) {
                        grp->lights.push_back({ (DWORD)LOWORD(mainGrid->grid[ind(x, y)]), (DWORD)HIWORD(mainGrid->grid[ind(x, y)]) });
                        // Sort!
                        sort(grp->lights.begin(), grp->lights.end(),
                            [](auto t, auto t2) {
                                return t.first == t2.first ? t2.second > t.second : t2.first > t.first;
                            });
                    }
                }
    }
}

BOOL CALLBACK TabColorGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    //WORD devID = dIndex < 0 ? 0 : conf->afx_dev.fxdevs[dIndex].desc->devid;

	switch (message) {
	case WM_INITDIALOG:
	{
        cgDlg = hDlg;
        if (!conf->afx_dev.GetGrids()->size()) {
            DWORD* grid = new DWORD[20 * 8]{ 0 };
            conf->afx_dev.GetGrids()->push_back({ 0, 20, 8, "Main", grid });
        }
        if (!mainGrid)
            mainGrid = &conf->afx_dev.GetGrids()->front();

        //tipH = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), tipH);
        //tipV = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), tipV);

        //SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(3, 22));
        ////SendMessage(gridX, TBM_SETPOS, true, mainGrid->x);
        EnableWindow(gridX, false);
        EnableWindow(gridY, false);

        //SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(3, 10));
        //SendMessage(gridY, TBM_SETPOS, true, mainGrid->y);

        RepaintGrid(hDlg);
	} break;
    case WM_COMMAND: {
        switch (LOWORD(wParam))
        {
        case IDC_BUT_CLEARGRID:
            if (eItem > 0) {
                AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(eItem);
                dragZone = { 0, 0, mainGrid->x - 1, mainGrid->y - 1 };
                ModifyColorDragZone(true, true);
                RedrawGridButtonZone();
                UpdateZoneList(zsDlg);
            }
            break;
        }
    } break;
    //case WM_RBUTTONUP: {
    //    // remove grid mapping
    //    //if (TranslateClick(hDlg, lParam)) {
    //    //    mainGrid->grid[clkPoint.x][clkPoint.y] = 0;
    //    //    RedrawGridButtonZone();
    //    //}
    //} break;
    case WM_LBUTTONDOWN: {
        // selection mark
        TranslateClick(hDlg, lParam);
        dragZone = { clkPoint.x, clkPoint.y, clkPoint.x, clkPoint.y };
        dragStart = clkPoint;
    } break;
    case WM_LBUTTONUP:
        // end selection
        if (dragZone.top >= 0) {
            ModifyColorDragZone(!(dragZone.bottom - dragZone.top || dragZone.right - dragZone.left));
            UpdateZoneList(zsDlg);
            RedrawGridButtonZone();
        }
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            // drag
            if (TranslateClick(hDlg, lParam) && dragZone.top >= 0) {
                ModifyColorDragZone(true);
                dragZone = { min(clkPoint.x, dragStart.x), min(clkPoint.y, dragStart.y),
                    max(clkPoint.x, dragStart.x), max(clkPoint.y, dragStart.y) };
                ModifyColorDragZone();
                RedrawGridButtonZone();
            }
        }
        break;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
        if (ditem->CtlID >= 2000) {
            DWORD gridVal = mainGrid->grid[(ditem->CtlID - 2000)];//% mainGrid->x][(ditem->CtlID - 2000) / mainGrid->x];
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
                AlienFX_SDK::group* grp = conf->afx_dev.GetGroupById(eItem);
                if (grp && find_if(grp->lights.begin(), grp->lights.end(),
                                [gridVal](auto lgh) {
                                    return lgh.first == LOWORD(gridVal) && lgh.second == HIWORD(gridVal);
                                }) != grp->lights.end())
                        DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_SUNKEN, BF_MONO | BF_RECT);
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
