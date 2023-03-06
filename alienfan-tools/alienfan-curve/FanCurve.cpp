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
            byte lastFan = fan_conf->lastSelectedFan;
            if (lastFan < mon->boostRaw.size()) {
                for (auto fan = fan_conf->lastProf->fanControls.begin(); fan != fan_conf->lastProf->fanControls.end(); fan++)
                    if (fan->first == lastFan) {
                        HPEN linePen;
                        for (auto senI = fan->second.begin(); senI != fan->second.end(); senI++) {
                            sen_block* sen = &senI->second;
                            if (sen->active) {
                                // Select line style
                                if (senI->first == fan_conf->lastSelectedSensor)
                                    linePen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
                                else
                                    linePen = CreatePen(PS_DOT, 1, RGB(255, 255, 0));
                                SelectObject(hdc, linePen);
                                // Draw curve
                                MoveToEx(hdc, cArea.left, cArea.bottom, NULL);
                                for (auto i = sen->points.begin(); i != sen->points.end(); i++) {
                                    mark = Fan2Screen(i->temp, i->boost);
                                    LineTo(hdc, mark.x, mark.y);
                                    Ellipse(hdc, mark.x - 2, mark.y - 2, mark.x + 2, mark.y + 2);
                                }
                                // Dots
                                if (mon->lastBoost[lastFan] == senI->first) {
                                    SetDCPenColor(hdc, RGB(255, 0, 0));
                                    SetDCBrushColor(hdc, RGB(255, 0, 0));
                                } else
                                    if (fan_conf->lastSelectedSensor == senI->first) {
                                        SetDCPenColor(hdc, RGB(0, 255, 0));
                                        SetDCBrushColor(hdc, RGB(0, 255, 0));
                                    }
                                    else {
                                        SetDCPenColor(hdc, RGB(255, 255, 0));
                                        SetDCBrushColor(hdc, RGB(255, 255, 0));
                                    }
                                SelectObject(hdc, GetStockObject(DC_PEN));
                                SelectObject(hdc, GetStockObject(DC_BRUSH));
                                mark = Fan2Screen(mon->senValues[senI->first], mon->senBoosts[lastFan][senI->first]);
                                Ellipse(hdc, mark.x - 4, mark.y - 4, mark.x + 4, mark.y + 4);
                                DeleteObject(linePen);
                            }
                        }
                        break;
                    }
                SetWindowText(tipWindow, ("Fan curve (scale " + to_string(fan_conf->GetFanScale(lastFan))
                    + ", boost " + to_string(mon->boostRaw[lastFan]) + ", " + to_string(mon->GetFanPercent(lastFan)) + "%)").c_str());
            }
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
    mon->ResetBoost();
    mon->SetCurrentMode(0);
    int rpm = mon->acpi->GetMaxRPM(fan_conf->lastSelectedFan), cSteps = 8, boost = 100/*,
        oldBoost = mon->boostRaw[fan_conf->lastSelectedFan]*//*mon->acpi->GetFanBoost(fan_conf->lastSelectedFan)*/, downScale;
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
        fan_conf->UpdateBoost(fan_conf->lastSelectedFan, bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
        ShowNotification(niData, "Max. boost calculation done", "Fan #" + to_string(fan_conf->lastSelectedFan + 1)
            + ": Final boost " + to_string(bestBoostPoint.maxBoost)
            + " @ " + to_string(bestBoostPoint.maxRPM) + " RPM.");
    }
    // Restore mode
    //mon->acpi->SetFanBoost(fan_conf->lastSelectedFan, oldBoost);
    //mon->SetCurrentGmode(fan_conf->lastProf->gmode);
    mon->inControl = true;
    SendMessage((HWND)lpParam, WM_APP + 2, 0, 1);
    return 0;
}

INT_PTR CALLBACK FanCurve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_PAINT: {
        if (!toolTip)
            toolTip = CreateToolTip(hDlg, NULL);
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        DrawFan();
        EndPaint(hDlg, &ps);
        return true;
    } break;
    case WM_SIZE:
        cArea.right = 0;
        //SetFanWindow();
        break;
    case WM_ERASEBKGND:
        return true;
    case WM_TIMER:
        if (IsWindowVisible(hDlg))
            DrawFan();
        return true;
    default:
        if (mon->inControl) {
            auto clk = Screen2Fan(lParam);
            for (auto sen = fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].begin();
                sen != fan_conf->lastProf->fanControls[fan_conf->lastSelectedFan].end(); sen++)
                if (sen->first == fan_conf->lastSelectedSensor) {
                    auto cFan = &sen->second;
                    switch (message) {
                    case WM_MOUSEMOVE: {
                        if (wParam & MK_LBUTTON) {
                            *lastFanPoint = clk;
                            DrawFan();
                        }
                        SetToolTip(toolTip, "Temp: " + to_string(clk.temp) + ", Boost: " + to_string(clk.boost));
                    } break;
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
                        //lastFanPoint = find_if(cFan->points.begin(), cFan->points.end(),
                        //    [clk](auto t) {
                        //        return abs(t.temp - clk.temp) <= DRAG_ZONE && abs(t.boost - clk.boost) <= DRAG_ZONE;
                        //    });
                        //if (lastFanPoint == cFan->points.end()) {
                        //    // insert element
                        //    lastFanPoint = cFan->points.insert(find_if(cFan->points.begin(), cFan->points.end(),
                        //        [clk](auto t) {
                        //            return t.temp > clk.temp;
                        //        }), clk);
                        //}
                        break;
                    case WM_LBUTTONUP:
                        ReleaseCapture();
                        // re-sort and de-duplicate array.
                        for (auto fPi = cFan->points.begin(); fPi < cFan->points.end() - 1; fPi++) {
                            auto nfPi = fPi + 1;
                            if (fPi->temp > nfPi->temp)
                                swap(*fPi, *nfPi);
                            if (fPi->temp == nfPi->temp && fPi->boost == nfPi->boost && cFan->points.size() > 2)
                                cFan->points.erase(nfPi);
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
    char ftype[4];
    switch (mon->acpi->fans[ind].type)
    {
    case 0: strcpy_s(ftype, "CPU"); break;
    case 1: strcpy_s(ftype, "GPU"); break;
    default: strcpy_s(ftype, "Fan");
    }
    string fname = (string)ftype + " " + to_string(ind + 1) + " - " + to_string(mon->fanRpm[ind]);
    if (forTray && !fan_conf->lastProf->powerStage)
        fname += " (" + to_string(mon->boostRaw[ind]) + ")";
    return fname;
}

void ReloadFanView(HWND list) {
    fanUpdateBlock = true;
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    if (!ListView_GetColumnWidth(list, 0)) {
        LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
        ListView_InsertColumn(list, 0, &lCol);
    }
    for (int i = 0; i < mon->acpi->fans.size(); i++) {
        auto fanControls = &fan_conf->lastProf->fanControls[i];
        LVITEMA lItem{ LVIF_TEXT | LVIF_STATE, i};
        string name = GetFanName(i);
        lItem.pszText = (LPSTR)name.c_str();
        if (i == fan_conf->lastSelectedFan) {
            lItem.state = LVIS_SELECTED;
        }
        else
            lItem.state = 0;
        ListView_InsertItem(list, &lItem);
        if (fanControls->find(fan_conf->lastSelectedSensor) != fanControls->end()
            && (*fanControls)[fan_conf->lastSelectedSensor].active) {
            ListView_SetCheckState(list, i, true);
        }
    }
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
    fanUpdateBlock = false;
}


void ReloadPowerList(HWND list) {
    ComboBox_ResetContent(list);
    for (auto i = mon->acpi->powers.begin(); i != mon->acpi->powers.end(); i++) {
        int pos = ComboBox_AddString(list, (LPARAM)(fan_conf->GetPowerName(*i).c_str()));
        if (pos == fan_conf->lastProf->powerStage)
            ComboBox_SetCurSel(list, pos);
    }
    if (mon->acpi->isGmode) {
        int pos = ComboBox_AddString(list, (LPARAM)("G-Mode"));
        if (mon->IsGMode())
            ComboBox_SetCurSel(list, pos);
    }
}

void ReloadTempView(HWND list) {
    int rpos = 0;
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
    if (!ListView_GetColumnWidth(list, 1)) {
        LVCOLUMNA lCol{ LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 100, (LPSTR)"Temp" };
        ListView_InsertColumn(list, 0, &lCol);
        lCol.pszText = (LPSTR)"Name";
        lCol.iSubItem = 1;
        ListView_InsertColumn(list, 1, &lCol);
    }
    for (int i = 0; i < mon->acpi->sensors.size(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
        string name = to_string(mon->senValues[mon->acpi->sensors[i].sid]) + " (" + to_string(mon->maxTemps[mon->acpi->sensors[i].sid]) + ")";
        lItem.lParam = mon->acpi->sensors[i].sid;
        lItem.pszText = (LPSTR)name.c_str();
        if (mon->acpi->sensors[i].sid == fan_conf->lastSelectedSensor) {
            lItem.state = LVIS_SELECTED;
            rpos = i;
        }
        else
            lItem.state = 0;
        ListView_InsertItem(list, &lItem);
        name = fan_conf->GetSensorName(&mon->acpi->sensors[i]);
        ListView_SetItemText(list, i, 1, (LPSTR)name.c_str());
    }
    RECT cArea;
    GetClientRect(list, &cArea);
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(list, 1, cArea.right - ListView_GetColumnWidth(list, 0));
    ListView_EnsureVisible(list, rpos, false);
}

void TempUIEvent(NMLVDISPINFO* lParam, HWND tempList, HWND fanList) {
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
        if (((LPNMLISTVIEW)lParam)->uNewState & LVIS_SELECTED && ((LPNMLISTVIEW)lParam)->iItem != -1) {
            // Select other sensor....
            fan_conf->lastSelectedSensor = (WORD)((LPNMLISTVIEW)lParam)->lParam;
            // Redraw fans
            ReloadFanView(fanList);
        }
    } break;
    }
}

void FanUIEvent(NMLISTVIEW* lParam, HWND fanList) {
    if (!fanUpdateBlock && lParam->hdr.code == LVN_ITEMCHANGED)
    {
        if (lParam->uNewState & LVIS_SELECTED && lParam->iItem != -1) {
            // Select other fan....
            fan_conf->lastSelectedFan = lParam->iItem;
            return;
        }
        if (lParam->uNewState & 0x3000) {
            auto fan = &fan_conf->lastProf->fanControls[lParam->iItem];
            switch (lParam->uNewState & 0x3000) {
            case 0x1000:
                // Deactivate sensor
                (*fan)[fan_conf->lastSelectedSensor].active = false;
                break;
            case 0x2000:
                (*fan)[fan_conf->lastSelectedSensor].active = true;
                if ((*fan)[fan_conf->lastSelectedSensor].points.empty())
                    (*fan)[fan_conf->lastSelectedSensor].points = { {0,0},{100,100} };
            }
            ListView_SetItemState(fanList, lParam->iItem, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
}

void AlterGMode(HWND power_list) {
    if (mon->acpi->isGmode) {
        fan_conf->lastProf->powerStage = mon->IsGMode() ? 0 : (WORD)mon->acpi->powers.size();
        mon->SetCurrentMode(fan_conf->lastProf->powerStage);
        ComboBox_SetCurSel(power_list, fan_conf->lastProf->powerStage);
        BlinkNumLock(2 + mon->IsGMode());
    }
}