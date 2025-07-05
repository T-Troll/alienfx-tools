#include "alienfx-gui.h"
#include "EventHandler.h"
#include "CaptureHelper.h"
#include "Common.h"

extern void UpdateZoneList();
extern void UpdateZoneAndGrid();
extern void dxgi_Restart();

extern EventHandler* eve;

void RedrawButtonZone(HWND dlg) {
    RECT pRect;
    GetWindowRect(GetDlgItem(dlg, IDC_AMB_BUTTON_ZONE), &pRect);
    MapWindowPoints(HWND_DESKTOP, dlg, (LPPOINT)&pRect, 2);
    RedrawWindow(dlg, &pRect, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void InitButtonZone(HWND dlg) {
    // delete zone buttons...
    for (DWORD bID = 2000; GetDlgItem(dlg, bID); bID++)
        DestroyWindow(GetDlgItem(dlg, bID));
    // Create zone buttons...
    HWND bblock = GetDlgItem(dlg, IDC_AMB_BUTTON_ZONE);
    RECT bzone;
    GetClientRect(bblock, &bzone);
    MapWindowPoints(bblock, dlg, (LPPOINT)&bzone, 1);
    bzone.right /= conf->amb_grid.x;
    bzone.bottom /= conf->amb_grid.y;
    LONGLONG bId = 2000;
    for (int y = 0; y < conf->amb_grid.y; y++)
        for (int x = 0; x < conf->amb_grid.x; x++) {
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId, hInst, NULL);
            bId++;
        }
    //RedrawButtonZone(dlg);
}

void SetGridSize(HWND dlg, WORD x, WORD y) {
    //KillTimer(dlg, 0);
    if (eve->capt) {
        ((CaptureHelper*)eve->capt)->SetLightGridSize(x, y);
    }
    conf->amb_grid = { x, y };
    InitButtonZone(dlg);
    //SetTimer(dlg, 0, 300, NULL);
}

BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND brSlider = GetDlgItem(hDlg, IDC_SLIDER_BR),
        gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID),
        gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    switch (message) {
    case WM_INITDIALOG:
    {
        // Mode...
        CheckDlgButton(hDlg, conf->amb_mode ? IDC_RADIO_SECONDARY : IDC_RADIO_PRIMARY, BST_CHECKED);
        CheckDlgButton(hDlg, conf->amb_calc ? IDC_RADIO_PREVEALING : IDC_RADIO_MEDIUM, BST_CHECKED);

        SendMessage(brSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        //SendMessage(brSlider, TBM_SETPOS, true, conf->amb_shift);
        SendMessage(brSlider, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(1, 20));
        //SendMessage(gridX, TBM_SETPOS, true, conf->amb_grid.x);
        //SendMessage(gridX, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(1, 12));
        //SendMessage(gridY, TBM_SETPOS, true, conf->amb_grid.y);
        //SendMessage(gridY, TBM_SETTICFREQ, 16, 0);

        CreateToolTip(brSlider, sTip1, conf->amb_shift);
        //SetSlider(sTip1, conf->amb_shift);

        CreateToolTip(gridX, sTip2, conf->amb_grid.x);
        //SetSlider(sTip2, conf->amb_grid.x);

        CreateToolTip(gridY, sTip3, conf->amb_grid.y);
        //SetSlider(sTip3, conf->amb_grid.y);

        // init grids...
        InitButtonZone(hDlg);

        // Start UI update thread...
        SetTimer(hDlg, 0, 300, NULL);

    } break;
    case WM_COMMAND:
        if (LOWORD(wParam) >= 2000 && mmap && HIWORD(wParam) == BN_CLICKED) { // grid button
            UINT id = LOWORD(wParam) - 2000;
            // add/remove mapping
            conf->modifyProfile.lockWrite();
            auto pos = mmap->ambients.begin();
            for (; pos != mmap->ambients.end(); pos++)
                if (*pos == id)
                    break;
            if (pos == mmap->ambients.end())
                mmap->ambients.push_back(id);
            else
                mmap->ambients.erase(pos);
            conf->modifyProfile.unlockWrite();
            eve->ChangeEffects();
            UpdateZoneAndGrid();
            break;
        }
        switch (LOWORD(wParam)) {
        case IDC_RADIO_MEDIUM: case IDC_RADIO_PREVEALING:
            conf->amb_calc = LOWORD(wParam) == IDC_RADIO_PREVEALING;
            break;
        case IDC_RADIO_PRIMARY: case IDC_RADIO_SECONDARY:
            conf->amb_mode = LOWORD(wParam) == IDC_RADIO_SECONDARY;
            //dxgi_Restart();
            //break;
        case IDC_BUTTON_RESET:
            dxgi_Restart();
            break;
        }
        break;
    case WM_APP + 2: {
        RedrawButtonZone(hDlg);
    } break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == brSlider) {
                conf->amb_shift = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                if ((CaptureHelper*)eve->capt)
                    ((CaptureHelper*)eve->capt)->needUpdate = true;
                break;
            }
            if ((HWND)lParam == gridX) {
                SetGridSize(hDlg, (WORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), conf->amb_grid.y);
            }
        }
        if ((HWND)lParam == brSlider) {
            SetSlider(sTip1, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            break;
        }
        if ((HWND)lParam == gridX) {
            SetSlider(sTip2, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
        }
        break;
    case WM_VSCROLL:
        if ((HWND)lParam == gridY) {
            switch (LOWORD(wParam)) {
            case TB_THUMBPOSITION: case TB_ENDTRACK:
                SetGridSize(hDlg, conf->amb_grid.x, (WORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
            SetSlider(sTip3, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
        }
        break;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
        if (ditem->CtlID >= 2000) {
            int idx = ditem->CtlID - 2000;
            CaptureHelper* capt = (CaptureHelper*)eve->capt;
            HBRUSH Brush = CreateSolidBrush(capt ?
                RGB(capt->imgz[idx * 3 + 2], capt->imgz[idx * 3 + 1], capt->imgz[idx * 3]) :
                GetSysColor(COLOR_BTNFACE));
            FillRect(ditem->hDC, &ditem->rcItem, Brush);
            if (mmap && mmap->ambients.size() && find(mmap->ambients.begin(), mmap->ambients.end(), idx) != mmap->ambients.end())
                DrawEdge(ditem->hDC, &ditem->rcItem, EDGE_SUNKEN, BF_RECT);
            else
                FrameRect(ditem->hDC, &ditem->rcItem, GetSysColorBrush(COLOR_GRAYTEXT));
            DeleteObject(Brush);
        }
    } break;
    case WM_TIMER:
        if (eve->capt) {
            //DebugPrint("Ambient UI update...\n");
            RedrawButtonZone(hDlg);
        }
        break;
    default: return false;
    }
    return true;
}
