#include "alienfx-gui.h"
#include "EventHandler.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);

extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);
extern void RemoveUnused(vector<groupset>*);

extern EventHandler* eve;
extern int eItem;

ThreadHelper* ambUIupdate = NULL;

void AmbUpdate(LPVOID);

void InitButtonZone(HWND dlg) {
    // delete zone buttons...
    for (DWORD bID = 2000; GetDlgItem(dlg, bID); bID++)
        DestroyWindow(GetDlgItem(dlg, bID));
    // Create zone buttons...
    HWND bblock = GetDlgItem(dlg, IDC_AMB_BUTTON_ZONE);
    RECT bzone;
    GetClientRect(bblock, &bzone);
    MapWindowPoints(bblock, dlg, (LPPOINT)&bzone, 1);
    bzone.right /= LOWORD(conf->amb_grid);
    bzone.bottom /= HIWORD(conf->amb_grid);
    LONGLONG bId = 2000;
    for (int y = 0; y < HIWORD(conf->amb_grid); y++)
        for (int x = 0; x < LOWORD(conf->amb_grid); x++) {
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId, hInst, NULL);
            bId++;
        }
}

void RedrawButtonZone(HWND dlg) {
    RECT pRect;
    GetWindowRect(GetDlgItem(dlg, IDC_AMB_BUTTON_ZONE), &pRect);
    MapWindowPoints(HWND_DESKTOP, dlg, (LPPOINT)&pRect, 2);
    RedrawWindow(dlg, &pRect, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    //for (int i = 0; i < LOWORD(conf->grid) * HIWORD(conf->grid); i++)
    //    RedrawWindow(GetDlgItem(dlg, 2000 + i), 0, 0, RDW_INVALIDATE);
}

void SetGridSize(HWND dlg, int x, int y) {
    delete ambUIupdate;
    if (eve->capt) {
        eve->capt->SetLightGridSize(x, y);
    }
    else {
        conf->amb_grid = MAKELPARAM(x, y);
    }
    InitButtonZone(dlg);
    ambUIupdate = new ThreadHelper(AmbUpdate, dlg);
}

BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    //HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
    HWND brSlider = GetDlgItem(hDlg, IDC_SLIDER_BR),
        gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID),
        gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    groupset* map = FindMapping(eItem);// FindAmbMapping(eItem);

    switch (message) {
    case WM_INITDIALOG:
    {
        //zsDlg = CreateDialog(hInst, (LPSTR)IDD_ZONESELECTION, hDlg, (DLGPROC)ZoneSelectionDialog);
        //RECT mRect;
        //GetWindowRect(GetDlgItem(hDlg, IDC_STATIC_ZONES), &mRect);
        //ScreenToClient(hDlg, (LPPOINT)&mRect);
        //SetWindowPos(zsDlg, NULL, mRect.left, mRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);

        //if (!conf->afx_dev.GetMappings()->size())
        //    OnGridSelChanged(gridTab);

        // Mode...
        CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, conf->amb_mode ? BST_UNCHECKED : BST_CHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, conf->amb_mode ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hDlg, IDC_CHECK_GAMMA, conf->gammaCorrection ? BST_CHECKED : BST_UNCHECKED);

        SendMessage(brSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(brSlider, TBM_SETPOS, true, conf->amb_shift);
        SendMessage(brSlider, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(1, 20));
        SendMessage(gridX, TBM_SETPOS, true, LOWORD(conf->amb_grid));
        //SendMessage(gridX, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(1, 12));
        SendMessage(gridY, TBM_SETPOS, true, HIWORD(conf->amb_grid));
        //SendMessage(gridY, TBM_SETTICFREQ, 16, 0);

        sTip1 = CreateToolTip(brSlider, sTip1);
        SetSlider(sTip1, conf->amb_shift);

        sTip2 = CreateToolTip(gridX, sTip2);
        SetSlider(sTip2, LOWORD(conf->amb_grid));

        sTip3 = CreateToolTip(gridY, sTip3);
        SetSlider(sTip3, HIWORD(conf->amb_grid));

        // init grids...
        //CreateGridBlock(gridTab, (DLGPROC)TabColorGrid);
        //TabCtrl_SetCurSel(gridTab, conf->gridTabSel);
        //OnGridSelChanged(gridTab);

        InitButtonZone(hDlg);

        // Start UI update thread...
        ambUIupdate = new ThreadHelper(AmbUpdate, hDlg, 200);

    } break;
    case WM_COMMAND:
    {
        if (LOWORD(wParam) >= 2000) { // grid button
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                if (map) {
                    UINT id = LOWORD(wParam) - 2000;
                    // add/remove mapping
                    auto pos = map->ambients.begin();
                    if ((pos = find_if(map->ambients.begin(), map->ambients.end(),
                        [id](auto t) {
                            return t == id;
                        })) == map->ambients.end())
                        map->ambients.push_back(id);
                    else {
                        map->ambients.erase(pos);
                        if (map->ambients.empty())
                            RemoveUnused(conf->active_set);
                    }
                    RedrawButtonZone(hDlg);
                }
                break;
            }
        }
        switch (LOWORD(wParam)) {
        case IDC_RADIO_PRIMARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_CHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_UNCHECKED);
                conf->amb_mode = 0;
                if (eve->capt)
                    eve->capt->Restart();
                break;
            } break;
        case IDC_RADIO_SECONDARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_CHECKED);
                conf->amb_mode = 1;
                if (eve->capt)
                    eve->capt->Restart();
                break;
            }
            break;
        case IDC_BUTTON_RESET:
            if (eve->capt) {
                eve->capt->Restart();
            }
            break;
        default: return false;
        }
    } break;
    case WM_APP + 2: {
        RedrawButtonZone(hDlg);
    } break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == brSlider) {
                conf->amb_shift = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                SetSlider(sTip1, conf->amb_shift);
                break;
            }
            if ((HWND)lParam == gridX) {
                SetGridSize(hDlg, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), HIWORD(conf->amb_grid));
                SetSlider(sTip2, LOWORD(conf->amb_grid));
                break;
            }
            break;
        default:
            if ((HWND)lParam == brSlider) {
                SetSlider(sTip1, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                break;
            }
            if ((HWND)lParam == gridX) {
                SetSlider(sTip2, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                break;
            }
        }
        break;
    case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == gridY) {
                SetGridSize(hDlg, LOWORD(conf->amb_grid), (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                SetSlider(sTip3, HIWORD(conf->amb_grid));
            }
            break;
        default:
            if ((HWND)lParam == gridY) {
                SetSlider(sTip3, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
            }
        }
        break;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* ditem = (DRAWITEMSTRUCT*)lParam;
        if (ditem->CtlID >= 2000) {
            int idx = ditem->CtlID - 2000;
            HBRUSH Brush;
            if (eve->capt)
                Brush = CreateSolidBrush(RGB(eve->capt->imgz[idx * 3 + 2], eve->capt->imgz[idx * 3 + 1], eve->capt->imgz[idx * 3]));
            else
                Brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(ditem->hDC, &ditem->rcItem, Brush);
            DeleteObject(Brush);
            bool selected = false;
            if (map && map->ambients.size()) {
                auto pos = find_if(map->ambients.begin(), map->ambients.end(),
                    [idx](auto t) {
                        return t == idx;
                    });
                selected = pos != map->ambients.end();
            }
            DrawEdge(ditem->hDC, &ditem->rcItem, selected ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
        }
    } break;
    case WM_CLOSE: case WM_DESTROY:
        delete ambUIupdate;
    break;
    default: return false;
    }
    return true;
}

void AmbUpdate(LPVOID param) {
    if (eve->capt && eve->capt->needUpdate && IsWindowVisible((HWND)param)) {
        //DebugPrint("Ambient UI update...\n");
        RedrawButtonZone((HWND)param);
        if (eve->capt) eve->capt->needUpdate = false;
    }
}
