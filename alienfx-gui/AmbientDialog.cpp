#include "alienfx-gui.h"
#include "EventHandler.h"

extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);
extern zone *FindAmbMapping(int lid);
extern void RemoveAmbMapping(int devid, int lightid);

extern void SwitchTab(int);

extern EventHandler* eve;
extern int eItem;

ThreadHelper* ambUIupdate = NULL;

void AmbUpdate(LPVOID);

void InitButtonZone(HWND dlg) {
    // delete zone buttons...
    for (DWORD bID = 2000; GetDlgItem(dlg, bID); bID++)
        DestroyWindow(GetDlgItem(dlg, bID));
    // Create zone buttons...
    HWND bblock = GetDlgItem(dlg, IDC_BUTTON_ZONE);
    RECT bzone;
    GetClientRect(bblock, &bzone);
    MapWindowPoints(bblock, dlg, (LPPOINT)&bzone, 1);
    bzone.right /= conf->amb_conf->grid.x;
    bzone.bottom /= conf->amb_conf->grid.y;
    LONGLONG bId = 2000;
    for (int y = 0; y < conf->amb_conf->grid.y; y++)
        for (int x = 0; x < conf->amb_conf->grid.x; x++) {
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId, hInst, NULL);
            bId++;
        }
}

void RedrawButtonZone(HWND dlg) {
    for (int i = 0; i < conf->amb_conf->grid.x * conf->amb_conf->grid.y; i++)
        RedrawWindow(GetDlgItem(dlg, 2000 + i), 0, 0, RDW_INVALIDATE);
}

void SetGridSize(HWND dlg, int x, int y) {
    delete ambUIupdate;
    if (eve->capt) {
        eve->capt->SetGridSize(x, y);
    }
    else {
        conf->amb_conf->grid.x = x;
        conf->amb_conf->grid.y = y;
    }
    InitButtonZone(dlg);
    ambUIupdate = new ThreadHelper(AmbUpdate, dlg);
}

BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
    HWND brSlider = GetDlgItem(hDlg, IDC_SLIDER_BR),
        gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    zone *map = FindAmbMapping(eItem);

    switch (message) {
    case WM_INITDIALOG:
    {

        if (UpdateLightList(light_list, fxhl, 3) < 0) {
            // no lights, switch to setup
            SwitchTab(TAB_DEVICES);
            return false;
        }

        // Mode...
        CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, conf->amb_conf->mode ? BST_UNCHECKED : BST_CHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, conf->amb_conf->mode ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hDlg, IDC_CHECK_GAMMA, conf->gammaCorrection ? BST_CHECKED : BST_UNCHECKED);

        SendMessage(brSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(brSlider, TBM_SETPOS, true, conf->amb_conf->shift);
        SendMessage(brSlider, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(1, 20));
        SendMessage(gridX, TBM_SETPOS, true, conf->amb_conf->grid.x);
        //SendMessage(gridX, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(1, 12));
        SendMessage(gridY, TBM_SETPOS, true, conf->amb_conf->grid.y);
        //SendMessage(gridY, TBM_SETTICFREQ, 16, 0);

        sTip1 = CreateToolTip(brSlider, sTip1);
        SetSlider(sTip1, conf->amb_conf->shift);

        sTip2 = CreateToolTip(gridX, sTip2);
        SetSlider(sTip2, conf->amb_conf->grid.x);

        sTip3 = CreateToolTip(gridY, sTip3);
        SetSlider(sTip3, conf->amb_conf->grid.y);

        // Start UI update thread...
        ambUIupdate = new ThreadHelper(AmbUpdate, hDlg, 200);

        if (eItem >= 0) {
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
        }

        InitButtonZone(hDlg);

    } break;
    case WM_COMMAND:
    {
        if (LOWORD(wParam) >= 2000) { // grid button
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                if (eItem == (-1))
                    break;
                UINT id = LOWORD(wParam) - 2000;
                if (!map) {
                    map = new zone({ 0 });
                    if (eItem > 0xffff) {
                        // group
                        map->lightid = eItem;
                    }
                    else {
                        // light
                        AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(eItem);
                        map->devid = lgh->devid;
                        map->lightid = lgh->lightid;
                    }
                    conf->amb_conf->zones.push_back(*map);
                    map = &conf->amb_conf->zones.back();
                }
                // add mapping
                auto Iter = map->map.begin();
                for (; Iter != map->map.end(); Iter++)
                    if (*Iter == id)
                        break;
                if (Iter == map->map.end()) {
                    // new mapping, add and select
                    map->map.push_back(id);
                }
                else {
                    map->map.erase(Iter);
                    if (!map->map.size()) {
                        // delete mapping!
                        RemoveAmbMapping(map->devid, map->lightid);
                    }
                }
                RedrawWindow(GetDlgItem(hDlg, LOWORD(wParam)), 0, 0, RDW_INVALIDATE);
                break;
            }
        }
        switch (LOWORD(wParam)) {
        case IDC_LIGHTS: // should reload mappings
            switch (HIWORD(wParam)) {
            case LBN_SELCHANGE:
            {
                eItem = (int)ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
                RedrawButtonZone(hDlg);
            } break;
            }
            break;
        case IDC_RADIO_PRIMARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_CHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_UNCHECKED);
                conf->amb_conf->mode = 0;
                if (eve->capt)
                    eve->capt->Restart();
                break;
            } break;
        case IDC_RADIO_SECONDARY:
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
                CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, BST_CHECKED);
                conf->amb_conf->mode = 1;
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
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == brSlider) {
                conf->amb_conf->shift = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
                SetSlider(sTip1, conf->amb_conf->shift);
                break;
            }
            if ((HWND)lParam == gridX) {
                SetGridSize(hDlg, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), conf->amb_conf->grid.y);
                SetSlider(sTip2, conf->amb_conf->grid.x);
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
                SetGridSize(hDlg, conf->amb_conf->grid.x, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                SetSlider(sTip3, conf->amb_conf->grid.y);
            }
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
            if (map)
                for (auto Iter = map->map.begin(); Iter != map->map.end(); Iter++)
                    if (*Iter == idx)
                        selected = true;
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
        eve->capt->needUpdate = false;
    }
}
