#include <AlienFX_SDK.h>
#include "resource.h"
#include <CommCtrl.h>
#include <windowsx.h>

extern AlienFX_SDK::lightgrid* mainGrid;
extern int cLightID, dIndex, devID;
extern AlienFX_SDK::Mappings afx_dev;

extern AlienFX_SDK::mapping* FindCreateMapping();
extern void SetLightMap(HWND);

POINT clkPoint, dragStart;
RECT dragZone;
DWORD oldClkValue;

void InitButtonZone(HWND dlg) {
    // delete zone buttons...
    for (DWORD bID = 2000; GetDlgItem(dlg, bID); bID++)
        DestroyWindow(GetDlgItem(dlg, bID));
    // Create zone buttons...
    HWND bblock = GetDlgItem(dlg, IDC_BUTTON_ZONE);
    RECT bzone;
    GetClientRect(bblock, &bzone);
    MapWindowPoints(bblock, dlg, (LPPOINT)&bzone, 1);
    bzone.right /= mainGrid->x;
    bzone.bottom /= mainGrid->y;
    LONGLONG bId = 2000;
    for (int y = 0; y < mainGrid->y; y++)
        for (int x = 0; x < mainGrid->x; x++) {
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_DISABLED,
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId,
                GetModuleHandle(NULL), NULL);
            bId++;
        }
}

void RedrawButtonZone(HWND dlg) {
    //for (int i = 0; i < mainGrid->x * mainGrid->y; i++)
    //    RedrawWindow(GetDlgItem(dlg, 2000 + i), 0, 0, RDW_INVALIDATE);
    RedrawWindow(GetDlgItem(dlg, IDC_BUTTON_ZONE), 0, 0, RDW_INVALIDATE);
}

void SetGridSize(HWND dlg, int x, int y) {
    mainGrid->x = x;
    mainGrid->y = y;
    InitButtonZone(dlg);
}

void ModifyDragZone(WORD did, WORD lid, bool clear = false) {
    for (int x = dragZone.left; x <= dragZone.right; x++)
        for (int y = dragZone.top; y <= dragZone.bottom; y++) {
            if (!mainGrid->grid[x][y] && !clear)
                mainGrid->grid[x][y] = MAKELPARAM(did, lid);
            else
                if (clear && mainGrid->grid[x][y] == MAKELPARAM(did, lid))
                    mainGrid->grid[x][y] = 0;
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
        clkPoint = { clkPoint.x / (gRect.right / mainGrid->x),
                     clkPoint.y / (gRect.bottom / mainGrid->y) };
        return true;
    }
    else {
        clkPoint = { -1, -1 };
        return false;
    }
}

void RepaintGrid(HWND hDlg) {
    InitButtonZone(hDlg);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_HSCALE), TBM_SETPOS, true, mainGrid->x);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_VSCALE), TBM_SETPOS, true, mainGrid->y);
    RedrawButtonZone(hDlg);
}

BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

	switch (message) {
	case WM_INITDIALOG:
	{
        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(3, 22));
        //SendMessage(gridX, TBM_SETPOS, true, mainGrid->x);

        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(3, 10));
        //SendMessage(gridY, TBM_SETPOS, true, mainGrid->y);

        //InitButtonZone(hDlg);
        //RedrawButtonZone(hDlg);
        RepaintGrid(hDlg);
	} break;
    case WM_RBUTTONUP: {
        // remove grid mapping
        if (TranslateClick(hDlg, lParam)) {
            mainGrid->grid[clkPoint.x][clkPoint.y] = 0;
            RedrawButtonZone(hDlg);
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
                ModifyDragZone(devID, cLightID);
                FindCreateMapping();
            }
            else {
                // single click at assigned grid
                if (oldClkValue == MAKELPARAM(devID, cLightID))
                    mainGrid->grid[dragZone.left][dragZone.top] = 0;
                else {
                    // change light to old one
                    cLightID = HIWORD(oldClkValue);
                    auto pos = find_if(afx_dev.fxdevs.begin(), afx_dev.fxdevs.end(),
                        [](auto t) {
                            return t.dev->GetPID() == LOWORD(oldClkValue);
                        });
                    if (pos != afx_dev.fxdevs.end())
                        dIndex = (int)(pos - afx_dev.fxdevs.begin());
                }
            }
            SetLightMap(GetParent(GetParent(hDlg)));
            RedrawButtonZone(hDlg);
        }
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            // drag
            if (TranslateClick(hDlg, lParam) && dragZone.top >= 0) {
                ModifyDragZone(devID, cLightID, true);
                dragZone = { min(clkPoint.x, dragStart.x), min(clkPoint.y, dragStart.y),
                    max(clkPoint.x, dragStart.x), max(clkPoint.y, dragStart.y) };
                ModifyDragZone(devID, cLightID);
                RedrawButtonZone(hDlg);
            }
        }
        break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == gridX) {
                SetGridSize(hDlg, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), mainGrid->y);
                //SetSlider(sTip2, conf->amb_conf->mainGrid->x);
            }
            break;
        default:
            if ((HWND)lParam == gridX) {
                //SetSlider(sTip2, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
        }
        break;
    case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == gridY) {
                SetGridSize(hDlg, mainGrid->x, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                //SetSlider(sTip3, conf->amb_conf->mainGrid->y);
            }
            break;
        default:
            if ((HWND)lParam == gridY) {
                //SetSlider(sTip3, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
        }
        break;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
        if (ditem->CtlID >= 2000) {
            DWORD gridVal = mainGrid->grid[(ditem->CtlID - 2000) % mainGrid->x][(ditem->CtlID - 2000) / mainGrid->x];
            WORD idVal = HIWORD(gridVal) << 4;
            HBRUSH Brush = NULL;
            if (gridVal) {
                if (HIWORD(gridVal) == cLightID && LOWORD(gridVal) == devID)
                    Brush = CreateSolidBrush(RGB(0, 255, 0));
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
	default: return false;
	}
	return true;
}