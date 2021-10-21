#include "alienfx-gui.h"

//void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b);
HWND CreateToolTip(HWND hwndParent, HWND oldTip);
void SetSlider(HWND tt, int value);
int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);

VOID OnSelChanged(HWND hwndDlg);

extern int eItem;

mapping *FindMapping(int lid) {
    if (lid != -1) {
        if (lid > 0xffff) {
            // group
            for (int i = 0; i < conf->amb_conf->mappings.size(); i++)
                if (conf->amb_conf->mappings[i].devid == 0 && conf->amb_conf->mappings[i].lightid == lid) {
                    return &conf->amb_conf->mappings[i];
                }
        } else {
            // mapping
            AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(lid);
            for (int i = 0; i < conf->amb_conf->mappings.size(); i++)
                if (conf->amb_conf->mappings[i].devid == lgh->devid && conf->amb_conf->mappings[i].lightid == lgh->lightid)
                    return &conf->amb_conf->mappings[i];
        }
    }
    return NULL;
}

BOOL CALLBACK TabAmbientDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
    HWND brSlider = GetDlgItem(hDlg, IDC_SLIDER_BR);
    
    mapping *map = FindMapping(eItem);

    switch (message) {
    case WM_INITDIALOG:
    {

        //UpdateLightList<FXHelper>(light_list, fxhl, 3);
        if (UpdateLightList(light_list, fxhl, 3) < 0) {
            // no lights, switch to setup
            HWND tab_list = GetParent(hDlg);
            TabCtrl_SetCurSel(tab_list, 6);
            OnSelChanged(tab_list);
            return false;
        }

        // Mode...
        CheckDlgButton(hDlg, IDC_RADIO_PRIMARY, conf->amb_conf->mode ? BST_UNCHECKED : BST_CHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_SECONDARY, conf->amb_conf->mode ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hDlg, IDC_CHECK_GAMMA, conf->gammaCorrection ? BST_CHECKED : BST_UNCHECKED);

        SendMessage(brSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
        SendMessage(brSlider, TBM_SETPOS, true, conf->amb_conf->shift);
        SendMessage(brSlider, TBM_SETTICFREQ, 16, 0);

        sTip = CreateToolTip(brSlider, sTip);
        SetSlider(sTip, conf->amb_conf->shift);

        conf->amb_conf->hDlg = hDlg;

        if (eItem >= 0) {
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
        }

    } break;
    case WM_COMMAND:
    {
        
        switch (LOWORD(wParam)) {
        case IDC_LIGHTS: // should reload mappings
            switch (HIWORD(wParam)) {
                case LBN_SELCHANGE:
                {
                    // check in config - do we have mappings?
                    eItem = (int) ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
                    map = FindMapping(eItem);
                    //if (map == NULL) {
                    //    mapping newmap;
                    //    if (lid > 0xffff) {
                    //        // group
                    //        newmap.devid = 0;
                    //        newmap.lightid = lid;
                    //    } else {
                    //        // light
                    //        AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(lid);
                    //        newmap.devid = lgh.devid;
                    //        newmap.lightid = lgh.lightid;
                    //    }
                    //    conf->amb_conf->mappings.push_back(newmap);
                    //    //std::sort(conf->mappings.begin(), conf->mappings.end(), ConfigAmbient::sortMappings);
                    //    map = FindMapping(lid);
                    //}
                    UINT bid = IDC_CHECK1;
                    // clear checks...
                    for (int i = 0; i < 12; i++) {
                        CheckDlgButton(hDlg, bid + i, BST_UNCHECKED);
                    }
                    if (map) {
                        // load zones....
                        for (int j = 0; j < map->map.size(); j++) {
                            CheckDlgButton(hDlg, bid + map->map[j], BST_CHECKED);
                        }
                    }
                } break;
            } 
        break;
        case IDC_BUTTON1: case IDC_BUTTON2: case IDC_BUTTON3: case IDC_BUTTON4: case IDC_BUTTON5: case IDC_BUTTON6: case IDC_BUTTON7:
        case IDC_BUTTON8: case IDC_BUTTON9: case IDC_BUTTON10: case IDC_BUTTON11: case IDC_BUTTON12:
        {
            switch (HIWORD(wParam)) {
            case BN_CLICKED:
            {
                UINT id = LOWORD(wParam) - IDC_BUTTON1;
                UINT bid = IDC_CHECK1 + id;
                if (!map) {
                    mapping newmap;
                    if (eItem > 0xffff) {
                        // group
                        newmap.devid = 0;
                        newmap.lightid = eItem;
                    } else {
                        // light
                        AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(eItem);
                        newmap.devid = lgh->devid;
                        newmap.lightid = lgh->lightid;
                    }
                    conf->amb_conf->mappings.push_back(newmap);
                    //std::sort(conf->mappings.begin(), conf->mappings.end(), ConfigAmbient::sortMappings);
                    map = FindMapping(eItem);
                }
                // add mapping
                vector <unsigned char>::iterator Iter = map->map.begin();
                int i = 0;
                for (i = 0; i < map->map.size(); i++)
                    if (map->map[i] == id)
                        break;
                    else
                        Iter++;
                HWND cBid = GetDlgItem(hDlg, bid);
                if (i == map->map.size()) {
                    // new mapping, add and select
                    map->map.push_back(id);
                    CheckDlgButton(hDlg, bid, BST_CHECKED);
                } else {
                    map->map.erase(Iter);
                    if (!map->map.size()) {
                        // delete mapping!
                        vector<mapping>::iterator mIter;
                        for (mIter = conf->amb_conf->mappings.begin(); mIter != conf->amb_conf->mappings.end(); mIter++)
                            if (mIter->devid == map->devid && mIter->lightid == map->lightid) {
                                conf->amb_conf->mappings.erase(mIter);
                                break;
                            }
                    }
                    CheckDlgButton(hDlg, bid, BST_UNCHECKED);
                }

            } break;
            }
        } break;
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
        if ((HWND) lParam == brSlider) {
            conf->amb_conf->shift = (DWORD) SendMessage(brSlider, TBM_GETPOS, 0, 0);
            SetSlider(sTip, conf->amb_conf->shift);
        }
        break;
        default:
        if ((HWND) lParam == brSlider) {
            SetSlider(sTip, (int) SendMessage(brSlider, TBM_GETPOS, 0, 0));
        }
        }
    break;
    case WM_PAINT:
        if (lParam != NULL) {
            // repaint buttons from lparam
            RECT rect;
            HBRUSH Brush = NULL;
            UCHAR *imgui = (UCHAR *) lParam;
            for (int i = 0; i < 12; i++) {
                HWND tl = GetDlgItem(hDlg, IDC_BUTTON1 + i);
                HWND cBid = GetDlgItem(hDlg, IDC_CHECK1 + i);
                GetWindowRect(tl, &rect);
                HDC cnt = GetWindowDC(tl);
                rect.bottom -= rect.top;
                rect.right -= rect.left;
                rect.top = rect.left = 0;
                // BGR!
                Brush = CreateSolidBrush(RGB(imgui[i * 3 + 2], imgui[i * 3 + 1], imgui[i * 3]));
                FillRect(cnt, &rect, Brush);
                DeleteObject(Brush);
                UINT state = IsDlgButtonChecked(hDlg, IDC_CHECK1 + i);
                if ((state & BST_CHECKED))
                    DrawEdge(cnt, &rect, EDGE_SUNKEN, BF_RECT);
                else
                    DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
                RedrawWindow(cBid, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
                ReleaseDC(tl, cnt);
            }
        }
        return false;
    break;
    case WM_CLOSE: 
    case WM_DESTROY:
        conf->amb_conf->hDlg = NULL;
    break;
    //case WM_POWERBROADCAST:
    //switch (wParam) {
    //case PBT_APMRESUMEAUTOMATIC: case PBT_APMPOWERSTATUSCHANGE:
    //if (wParam == PBT_APMRESUMEAUTOMATIC) {
    //    //resumed from sleep
    //    cap->Restart();
    //}
    //break;
    //case PBT_POWERSETTINGCHANGE:
    //POWERBROADCAST_SETTING *sParams = (POWERBROADCAST_SETTING *) lParam;
    //if (sParams->PowerSetting == GUID_MONITOR_POWER_ON) {
    //    if (sParams->Data[0] == 0) {
    //        cap->Stop();
    //        fxhl->FadeToBlack();
    //    } else
    //        cap->Restart();
    //}
    //if (sParams->PowerSetting == GUID_SESSION_DISPLAY_STATUS) {
    //    cap->Restart();
    //}
    //break;
    //}
    //break;
    //case WM_DISPLAYCHANGE:
    //// Monitor configuration changed
    //cap->Restart();
    //break;
    default: return false;
    }
    return true;
}