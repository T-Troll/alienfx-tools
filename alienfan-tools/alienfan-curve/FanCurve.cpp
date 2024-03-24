#include "ConfigFan.h"
#include "MonHelper.h"
#include <windowsx.h>
#include "common.h"

#define DRAG_ZONE 2

extern ConfigFan* fan_conf;
extern MonHelper* mon;
extern HWND fanWindow, tipWindow;

HWND toolTip = NULL;

NOTIFYICONDATA* niData;

extern HINSTANCE hInst;
bool fanUpdateBlock = false;
RECT cArea{ 0 };

vector<fan_point>::iterator lastFanPoint;
fan_overboost* lastBoostPoint = NULL, bestBoostPoint;
vector<fan_overboost> boostCheck;

int boostScale = 10, fanMinScale = 4000, fanMaxScale = 500;

HANDLE ocStopEvent = CreateEvent(NULL, false, false, NULL);

void SetFanWindow() {
    if (!cArea.right) {
        GetClientRect(fanWindow, &cArea);
        cArea.right--; cArea.bottom--;
    }
}

fan_point Screen2Fan(LPARAM lParam) {
    SetFanWindow();
    return {
        (byte)max(0, min(100, (100 * (GET_X_LPARAM(lParam))) / cArea.right)),
        (byte)max(0, min(100, (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / cArea.bottom))
    };
}

POINT Fan2Screen(short temp, short boost) {
    return {
        temp* cArea.right / 100,
        (100 - boost)* cArea.bottom / 100
    };
}

POINT Boost2Screen(fan_overboost* boost) {
    if (boost) {
        boostScale = boost->maxBoost - 100 > boostScale ? boostScale << 1 : boostScale;
        fanMaxScale = boost->maxRPM - fanMinScale > fanMaxScale ? ((boost->maxRPM - fanMinScale)/ 500 + 1) * 500 : fanMaxScale;
        return {
            boost->maxRPM < fanMinScale ? 0 : ((boost->maxRPM - fanMinScale) * cArea.right) / fanMaxScale,
            (boostScale + 100 - boost->maxBoost)* cArea.bottom / boostScale
        };
    }
    else
        return { 0 };
}

void DrawFan()
{
    if (fanWindow && mon) {
        SetFanWindow();
        POINT mark;
        HDC hdc_r = GetDC(fanWindow);
        // Double buff...
        HDC hdc = CreateCompatibleDC(hdc_r);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, cArea.right + 1, cArea.bottom + 1);
        SetBkMode(hdc, TRANSPARENT);
        HGDIOBJ hOld = SelectObject(hdc, hbmMem);

        // Grid...
        SetDCPenColor(hdc, RGB(127, 127, 127));
        SelectObject(hdc, GetStockObject(DC_PEN));
        // grid
        for (int x = 0; x < 11; x++)
            for (int y = 0; y < 11; y++) {
                int cx = (x * cArea.right) / 10 + cArea.left,
                    cy = (y * cArea.bottom) / 10 + cArea.top;
                MoveToEx(hdc, cx, cArea.top, NULL);
                LineTo(hdc, cx, cArea.bottom);
                MoveToEx(hdc, cArea.left, cy, NULL);
                LineTo(hdc, cArea.right, cy);
            }

        if (mon->inControl) {
            // curve...
            HPEN linePen;
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            for (auto senI = fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].begin();
                senI != fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].end(); senI++) {
                sen_block* sen = &senI->second;
                if (sen->active) {
                    // Select line style
                    if (senI->first == fan_conf->lastSelectedSensor) {
                        linePen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
                        SetDCBrushColor(hdc, RGB(0, 255, 0));
                    }
                    else {
                        linePen = CreatePen(PS_DOT, 1, RGB(255, 255, 0));
                        SetDCBrushColor(hdc, RGB(255, 255, 0));
                    }
                    SelectObject(hdc, linePen);
                    // Draw curve
                    MoveToEx(hdc, cArea.left, cArea.bottom, NULL);
                    for (auto i = sen->points.begin(); i != sen->points.end(); i++) {
                        mark = Fan2Screen(i->temp, i->boost);
                        LineTo(hdc, mark.x, mark.y);
                        Ellipse(hdc, mark.x - 2, mark.y - 2, mark.x + 2, mark.y + 2);
                    }
                    // Dots
                    if (mon->lastBoost[fan_conf->lastSelectedFan] == senI->first) {
                        SetDCPenColor(hdc, RGB(255, 0, 0));
                        SetDCBrushColor(hdc, RGB(255, 0, 0));
                        SelectObject(hdc, GetStockObject(DC_PEN));
                    }
                    mark = Fan2Screen(mon->senValues[senI->first], mon->senBoosts[fan_conf->lastSelectedFan][senI->first]);
                    Ellipse(hdc, mark.x - 4, mark.y - 4, mark.x + 4, mark.y + 4);
                    DeleteObject(linePen);
                }
            }
            SetWindowText(tipWindow, ("Fan curve (scale " + to_string(fan_conf->GetFanScale(fan_conf->lastSelectedFan))
                + ", boost " + to_string(mon->boostRaw[fan_conf->lastSelectedFan]) + /*" (" + to_string(mon->acpi->GetFanBoost(fan_conf->lastSelectedFan)) + ")" +*/ ", " +
                to_string(mon->GetFanPercent(fan_conf->lastSelectedFan)) + "%)").c_str());
        }
        else {
            SetDCPenColor(hdc, RGB(0, 255, 0));
            SelectObject(hdc, GetStockObject(DC_PEN));
            MoveToEx(hdc, cArea.left, cArea.bottom, NULL);
            for (auto iter = boostCheck.begin(); iter < boostCheck.end(); iter++) {
                mark = Boost2Screen(&(*iter));
                LineTo(hdc, mark.x, mark.y);
                Ellipse(hdc, mark.x - 2, mark.y - 2, mark.x + 2, mark.y + 2);
            }
            if (lastBoostPoint) {
                SetDCPenColor(hdc, RGB(255, 255, 0));
                SelectObject(hdc, GetStockObject(DC_PEN));
                mark = Boost2Screen(lastBoostPoint);
                Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
                string rpmText = "Last " + to_string(lastBoostPoint->maxBoost) + " @ " + to_string(lastBoostPoint->maxRPM)
                    + " RPM (Max. " + to_string(bestBoostPoint.maxBoost) + " @ " + to_string(bestBoostPoint.maxRPM) + " RPM)";
                SetWindowText(tipWindow, rpmText.c_str());
            }
            SetDCPenColor(hdc, RGB(255, 0, 0));
            SetDCBrushColor(hdc, RGB(255, 0, 0));
            SelectObject(hdc, GetStockObject(DC_PEN));
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            mark = Boost2Screen(&bestBoostPoint);
            Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
        }

        BitBlt(hdc_r, 0, 0, cArea.right + 1, cArea.bottom + 1, hdc, 0, 0, SRCCOPY);

        SelectObject(hdc, hOld);
        DeleteObject(hbmMem);
        DeleteDC(hdc);
        ReleaseDC(fanWindow, hdc_r);
        DeleteDC(hdc_r);
    }
}

int SetFanSteady(byte fanID, byte boost, bool downtrend = false) {
    mon->acpi->SetFanBoost(fanID, boost);
    // Check the trend...
    int pRpm, bRpm = mon->acpi->GetFanRPM(fanID), maxRPM;
    boostCheck.push_back({ boost, (USHORT)bRpm });
    lastBoostPoint = &boostCheck.back();
    if (WaitForSingleObject(ocStopEvent, 3000) != WAIT_TIMEOUT)
        return -1;
    lastBoostPoint->maxRPM = mon->acpi->GetFanRPM(fanID);
    do {
        pRpm = bRpm;
        bRpm = lastBoostPoint->maxRPM;
        if (WaitForSingleObject(ocStopEvent, 3000) != WAIT_TIMEOUT)
            return -1;
        lastBoostPoint->maxRPM = mon->acpi->GetFanRPM(fanID);
        maxRPM = max(lastBoostPoint->maxRPM, bRpm);
        bestBoostPoint.maxRPM = max(bestBoostPoint.maxRPM, maxRPM);
    } while ((lastBoostPoint->maxRPM > bRpm || bRpm < pRpm || lastBoostPoint->maxRPM != pRpm)
        && (!downtrend || !(lastBoostPoint->maxRPM < bRpm && bRpm < pRpm)));
    return lastBoostPoint->maxRPM = maxRPM;
}

DWORD WINAPI CheckFanOverboost(LPVOID lpParam) {
    mon->inControl = false;
    SendMessage((HWND)lpParam, WM_APP + 2, 0, 0);
    mon->SetCurrentMode(0);
    int rpm = mon->acpi->GetMaxRPM(fan_conf->lastSelectedFan), cSteps = 8, boost = 100, downScale;
    boostCheck.clear();
    bestBoostPoint = { (byte)boost, (unsigned short)rpm };
    boostScale = 10;
    fanMaxScale = 500;
    fanMinScale = (rpm / 100) * 100;
    // 100 rpm step patch
    downScale = fanMinScale == rpm ? 101 : 56;
    int crpm = SetFanSteady(fan_conf->lastSelectedFan, 100);
    for (int steps = cSteps; crpm >= 0 && steps; steps = steps >> 1) {
        // Check for uptrend
        boost += steps;
        while ((crpm = SetFanSteady(fan_conf->lastSelectedFan, boost, true)) > rpm)
        {
            rpm = lastBoostPoint->maxRPM;
            cSteps = steps;
            bestBoostPoint.maxBoost = boost;
            boost += steps;
        }
        boost = bestBoostPoint.maxBoost;
    }
    for (int steps = cSteps; crpm >= 0 && steps; steps = steps >> 1) {
        // Check for uptrend
        boost -= steps;
        while (boost > 100 && (crpm = SetFanSteady(fan_conf->lastSelectedFan, boost)) > bestBoostPoint.maxRPM - downScale) {
            bestBoostPoint.maxBoost = boost;
            boost -= steps;
        }
        boost = bestBoostPoint.maxBoost;
    }
    if (crpm >= 0) {
        fan_conf->UpdateBoost(fan_conf->lastSelectedFan, max(bestBoostPoint.maxBoost, 100), bestBoostPoint.maxRPM);
        ShowNotification(niData, "Max. boost calculation done", "Fan #" + to_string(fan_conf->lastSelectedFan + 1)
            + ": Final boost " + to_string(bestBoostPoint.maxBoost)
            + " @ " + to_string(bestBoostPoint.maxRPM) + " RPM.");
    }
    mon->ResetBoost();
    mon->SetCurrentMode();
    mon->inControl = true;
    SendMessage((HWND)lpParam, WM_APP + 2, 0, 1);
    return 0;
}

INT_PTR CALLBACK FanCurve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        DrawFan();
        EndPaint(hDlg, &ps);
        return true;
    } break;
    case WM_SIZE:
        cArea.right = 0;
        break;
    case WM_ERASEBKGND:
        toolTip = CreateToolTip(hDlg, toolTip);
        return true;
    default:
        if (mon->inControl) {
            auto clk = Screen2Fan(lParam);
            if (message == WM_MOUSEMOVE)
                SetToolTip(toolTip, "Temp: " + to_string(clk.temp) + ", Boost: " + to_string(clk.boost));
            for (auto sen = fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].begin();
                sen != fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].end(); sen++)
                if (sen->first == fan_conf->lastSelectedSensor) {
                    auto cFan = &sen->second;
                    switch (message) {
                    case WM_MOUSEMOVE:
                        if (wParam & MK_LBUTTON) {
                            *lastFanPoint = clk;
                            DrawFan();
                        }
                        break;
                    case WM_LBUTTONDOWN:
                        SetCapture(hDlg);
                        // check and add point
                        for (auto fp = cFan->points.begin(); fp != cFan->points.end(); fp++) {
                            if (abs(fp->temp - clk.temp) <= DRAG_ZONE && abs(fp->boost - clk.boost) <= DRAG_ZONE) {
                                lastFanPoint = fp;
                                break;
                            }
                            if (fp->temp > clk.temp) {
                                lastFanPoint = cFan->points.insert(fp, clk);
                                break;
                            }
                        }
                        break;
                    case WM_LBUTTONUP:
                        ReleaseCapture();
                        // re-sort and de-duplicate array.
                        for (auto fPi = cFan->points.begin(); fPi < cFan->points.end() - 1; ) {
                            auto nfPi = fPi + 1;
                            if (fPi->temp > nfPi->temp)
                                swap(*fPi, *nfPi);
                            if (fPi->temp == nfPi->temp && fPi->boost == nfPi->boost && cFan->points.size() > 2)
                                fPi = cFan->points.erase(nfPi);
                            else
                                fPi++;
                        }
                        cFan->points.front().temp = 0;
                        cFan->points.back().temp = 100;
                        SetFocus(GetParent(hDlg));
                        break;
                    case WM_RBUTTONUP:
                        // remove point from curve...
                        if (cFan->points.size() > 2) {
                            // check and remove point
                            for (auto fPi = cFan->points.begin() + 1; fPi < cFan->points.end() - 1; fPi++)
                                if (abs(fPi->temp - clk.temp) <= DRAG_ZONE && abs(fPi->boost - clk.boost) <= DRAG_ZONE) {
                                    // Remove this element...
                                    cFan->points.erase(fPi);
                                    DrawFan();
                                    break;
                                }
                        }
                        SetFocus(GetParent(hDlg));
                        break;
                    }
                    break;
                }
        }
        else
            if (message == WM_MOUSEMOVE)
                SetToolTip(toolTip, "Boost " + to_string((cArea.bottom - GET_Y_LPARAM(lParam)) * boostScale / cArea.bottom + 100) + " @ " +
                    to_string(GET_X_LPARAM(lParam) * fanMaxScale / cArea.right + fanMinScale) + " RPM");
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

string GetFanName(int ind, bool forTray = false) {
    string fname = "";
    if (ind < mon->acpi->fans.size()) {
        switch (mon->acpi->fans[ind].type)
        {
        case 1: fname = "CPU"; break;
        case 6: fname = "GPU"; break;
        default: fname = "Fan";
        }
        fname += " " + to_string(ind + 1) + " - " + to_string(mon->fanRpm[ind]);
        if (forTray && !fan_conf->lastProf->powerStage)
            fname += " (" + to_string(mon->senValues[mon->lastBoost[ind]]/*mon->boostRaw[ind]*/) + ")";
    }
    return fname;
}

void ReloadFanView(HWND list) {
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    if (!ListView_GetColumnWidth(list, 0)) {
        LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
        ListView_InsertColumn(list, 0, &lCol);
    }
    for (int i = 0; i < mon->fansize; i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_STATE, i};
        string name = GetFanName(i);
        lItem.pszText = (LPSTR)name.c_str();
        if (i == fan_conf->lastSelectedFan) {
            lItem.state = LVIS_SELECTED | LVIS_FOCUSED;
        }
        ListView_InsertItem(list, &lItem);
    }
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
}


void ReloadPowerList(HWND list) {
    ComboBox_ResetContent(list);
    for (auto i = mon->acpi->powers.begin(); i != mon->acpi->powers.end(); i++)
        ComboBox_AddString(list, (LPARAM)(fan_conf->GetPowerName(*i)->c_str()));
    if (mon->acpi->isGmode)
        ComboBox_AddString(list, (LPARAM)("G-Mode"));
    ComboBox_SetCurSel(list, mon->powerMode);
}

void ReloadTempView(HWND list) {
    int rpos = 0;
    fanUpdateBlock = true;
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
    if (!ListView_GetColumnWidth(list, 1)) {
        LVCOLUMNA lCol{ LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 100, (LPSTR)"Temp" };
        ListView_InsertColumn(list, 0, &lCol);
        lCol.pszText = (LPSTR)"Name";
        lCol.iSubItem = 1;
        ListView_InsertColumn(list, 1, &lCol);
    }
    auto fanControls = &fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan];
    for (int i = 0; i < mon->sensorSize; i++) {
        WORD sid = mon->acpi->sensors[i].sid;
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
        string name = to_string(mon->senValues[sid]) + " (" + to_string(mon->maxTemps[sid]) + ")";
        lItem.lParam = sid;
        lItem.pszText = (LPSTR)name.c_str();
        if (sid == fan_conf->lastSelectedSensor) {
            lItem.state = LVIS_SELECTED;
            rpos = i;
        }
        ListView_InsertItem(list, &lItem);
        name = fan_conf->GetSensorName(&mon->acpi->sensors[i]);
        ListView_SetItemText(list, i, 1, (LPSTR)name.c_str());
        for (auto sc = fanControls->begin(); sc != fanControls->end(); sc++)
            if (sc->first == sid && sc->second.active) {
                ListView_SetCheckState(list, i, true);
                break;
            }
    }
    RECT cArea;
    GetClientRect(list, &cArea);
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(list, 1, cArea.right - ListView_GetColumnWidth(list, 0));
    ListView_EnsureVisible(list, rpos, false);
    fanUpdateBlock = false;
}

void TempUIEvent(NMLVDISPINFO* lParam, HWND tempList) {
    switch (lParam->hdr.code) {
    case LVN_BEGINLABELEDIT: {
        KillTimer(GetParent(tempList), 0);
        Edit_SetText(ListView_GetEditControl(tempList), fan_conf->GetSensorName(&mon->acpi->sensors[lParam->item.iItem]).c_str());
    } break;
    case LVN_ITEMACTIVATE: case NM_RETURN:
    {
        int pos = ListView_GetSelectionMark(tempList);
        RECT rect;
        ListView_GetSubItemRect(tempList, pos, 1, LVIR_BOUNDS, &rect);
        SetWindowPos(ListView_EditLabel(tempList, pos), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
    } break;
    case LVN_ENDLABELEDIT:
    {
        if (lParam->item.pszText) {
            fan_conf->sensors[(WORD)lParam->item.lParam] = lParam->item.pszText;
            ReloadTempView(tempList);
        }
        SetTimer(GetParent(tempList), 0, 500, NULL);
    } break;
    case LVN_ITEMCHANGED:
    {
        LPNMLISTVIEW item = (LPNMLISTVIEW)lParam;
        if (item->uChanged & LVIF_STATE) {
            if (item->uNewState & LVIS_SELECTED) {
                // Select other sensor....
                fan_conf->lastSelectedSensor = (WORD)item->lParam;
                DrawFan();
            }
            if (!fanUpdateBlock && item->uNewState & 0x3000) {
                auto fan = &fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan];
                switch (item->uNewState & 0x3000) {
                case 0x1000:
                    // Deactivate sensor
                    (*fan)[(WORD)item->lParam].active = false;
                    break;
                case 0x2000:
                    // Activate sensor
                    (*fan)[(WORD)item->lParam].active = true;
                    if ((*fan)[(WORD)item->lParam].points.empty())
                        (*fan)[(WORD)item->lParam].points = { {0,0},{100,100} };
                }
                ListView_SetItemState(tempList, item->iItem, LVIS_SELECTED, LVIS_SELECTED);
            }
        }
    } break;
    }
}

void FanUIEvent(NMLISTVIEW* lParam, HWND fanList, HWND tempList) {
    if (lParam->hdr.code == LVN_ITEMCHANGED && lParam->uChanged & LVIF_STATE)
    {
        if (lParam->uNewState & LVIS_SELECTED) {
            // Select other fan....
            if (fan_conf->lastSelectedFan != lParam->iItem)
                ListView_SetItemState(fanList, fan_conf->lastSelectedFan, 0, LVIS_SELECTED);
            fan_conf->lastSelectedFan = lParam->iItem;
            ReloadTempView(tempList);
        } else
            if (ListView_GetItemState(fanList, lParam->iItem, LVIS_FOCUSED)) {
                ListView_SetItemState(fanList, lParam->iItem, LVIS_SELECTED, LVIS_SELECTED);
            }
    }
}

void AlterGMode(HWND power_list) {
    if (mon->acpi->isGmode) {
        fan_conf->lastProf->gmodeStage = !fan_conf->lastProf->gmodeStage;
        mon->SetCurrentMode();
        ComboBox_SetCurSel(power_list, mon->powerMode);
        BlinkNumLock(2 + fan_conf->lastProf->gmodeStage);
    }
}