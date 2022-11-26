#include "ConfigFan.h"
#include "MonHelper.h"
#include "alienfan-SDK.h"
#include <windowsx.h>
#include "common.h"

#define DRAG_ZONE 2

extern ConfigFan* fan_conf;
extern MonHelper* mon;
extern HWND fanWindow, tipWindow;
extern AlienFan_SDK::Control* acpi;

extern void SetToolTip(HWND, string);

NOTIFYICONDATA* niData;

HWND toolTip = NULL;
extern HINSTANCE hInst;
bool fanMode = true;
bool fanUpdateBlock = false;
RECT cArea{ 0 };

vector<fan_point>::iterator lastFanPoint;
fan_overboost* lastBoostPoint = NULL, bestBoostPoint{ 0 };
vector<fan_overboost> boostCheck;

int boostScale = 10, fanMinScale = 4000, fanMaxScale = 500;

HANDLE ocStopEvent = CreateEvent(NULL, false, false, NULL);

fan_point Screen2Fan(LPARAM lParam) {
    if (!cArea.right) {
        GetClientRect(fanWindow, &cArea);
        cArea.right--; cArea.bottom--;
    }
    return {
        (short)max(0, min(100, (100 * (GET_X_LPARAM(lParam))) / cArea.right)),
        (short)max(0, min(100, (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / cArea.bottom))
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
    if (fanWindow && mon && cArea.right) {
        POINT mark;
        HDC hdc_r = GetDC(fanWindow);
        // Double buff...
        HDC hdc = CreateCompatibleDC(hdc_r);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, cArea.right /*- cArea.left*/ + 1, cArea.bottom /*- cArea.top*/ + 1);
        SetBkMode(hdc, TRANSPARENT);
        HGDIOBJ hOld = SelectObject(hdc, hbmMem);

        // Grid...
        SetDCPenColor(hdc, RGB(127, 127, 127));
        SelectObject(hdc, GetStockObject(DC_PEN));
        for (int x = 0; x < 11; x++)
            for (int y = 0; y < 11; y++) {
                int cx = (x * cArea.right) / 10 + cArea.left,
                    cy = (y * cArea.bottom) / 10 + cArea.top;
                MoveToEx(hdc, cx, cArea.top, NULL);
                LineTo(hdc, cx, cArea.bottom);
                MoveToEx(hdc, cArea.left, cy, NULL);
                LineTo(hdc, cArea.right, cy);
            }

        if (fanMode) {
            // curve...
            auto fan = fan_conf->FindFanBlock(fan_conf->lastSelectedFan);
            if (fan) {
                HPEN linePen;
                for (auto senI = fan->begin(); senI != fan->end(); senI++) {
                    sen_block* sen = &senI->second;
                    if (sen->active) {
                        // draw fan curve
                        if (senI->first == fan_conf->lastSelectedSensor)
                            linePen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
                        else
                            linePen = CreatePen(PS_DOT, 1, RGB(255, 255, 0));
                        SelectObject(hdc, linePen);
                        // First point
                        MoveToEx(hdc, cArea.left, cArea.bottom, NULL);
                        for (auto i = sen->points.begin(); i != sen->points.end(); i++) {
                            mark = Fan2Screen(i->temp, i->boost);
                            LineTo(hdc, mark.x, mark.y);
                            Ellipse(hdc, mark.x - 2, mark.y - 2, mark.x + 2, mark.y + 2);
                        }
                        // Yellow dots
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
                        mark = Fan2Screen(mon->senValues[senI->first], mon->senBoosts[fan_conf->lastSelectedFan][senI->first]);
                        Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
                        DeleteObject(linePen);
                    }
                }
            }
            // Red dot
            SetDCPenColor(hdc, RGB(255, 0, 0));
            SetDCBrushColor(hdc, RGB(255, 0, 0));
            SelectObject(hdc, GetStockObject(DC_PEN));
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            int fanBoost = (int)round(mon->boostRaw[fan_conf->lastSelectedFan] * 100.0 / acpi->boosts[fan_conf->lastSelectedFan]);
            mark = Fan2Screen(mon->senValues[fan_conf->lastSelectedSensor], fanBoost);
            Ellipse(hdc, mark.x - 4, mark.y - 4, mark.x + 4, mark.y + 4);
            string rpmText = "Fan curve (scale: " + to_string(acpi->boosts[fan_conf->lastSelectedFan])
                + ", boost: " + to_string(fanBoost) + ", " + to_string(acpi->GetFanPercent(fan_conf->lastSelectedFan)) + "%)";
            SetWindowText(tipWindow, rpmText.c_str());
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
    acpi->SetFanBoost(fanID, boost, true);
    // Check the trend...
    int pRpm, bRpm = acpi->GetFanRPM(fanID), maxRPM;
    boostCheck.push_back({ boost, (USHORT)bRpm });
    lastBoostPoint = &boostCheck.back();
    //DrawFan();
    if (WaitForSingleObject(ocStopEvent, 3000) != WAIT_TIMEOUT)
        return -1;
    lastBoostPoint->maxRPM = acpi->GetFanRPM(fanID);
    //DrawFan();
    do {
        pRpm = bRpm;
        bRpm = lastBoostPoint->maxRPM;
        if (WaitForSingleObject(ocStopEvent, 3000) != WAIT_TIMEOUT)
            return -1;
        lastBoostPoint->maxRPM = acpi->GetFanRPM(fanID);
        maxRPM = max(lastBoostPoint->maxRPM, bRpm);
        bestBoostPoint.maxRPM = max(bestBoostPoint.maxRPM, maxRPM);
        //DrawFan();
    } while ((lastBoostPoint->maxRPM > bRpm || bRpm < pRpm || lastBoostPoint->maxRPM != pRpm)
        && (!downtrend || !(lastBoostPoint->maxRPM < bRpm && bRpm < pRpm)));
    return lastBoostPoint->maxRPM = maxRPM;
}

void UpdateBoost(byte fanID) {
    fan_overboost* fo = &fan_conf->boosts[fanID];
    fo->maxBoost = max(bestBoostPoint.maxBoost, 100);
    fo->maxRPM = max(bestBoostPoint.maxRPM, fo->maxRPM);

    acpi->boosts[fanID] = max(bestBoostPoint.maxBoost, 100);
    acpi->maxrpm[fanID] = max(fo->maxRPM, acpi->maxrpm[fanID]);
}

DWORD WINAPI CheckFanRPM(LPVOID lpParam) {
    byte fanID = (byte)(ULONG64)lpParam;
    mon->Stop();
    fanMode = false;
    acpi->Unlock();
    boostCheck.clear();
    bestBoostPoint = { 100, 3000 };
    SetFanSteady(fanID, 100);
    fan_conf->boosts[fanID].maxRPM = max(bestBoostPoint.maxRPM, acpi->maxrpm[fanID]);
    acpi->maxrpm[fanID] = fan_conf->boosts[fanID].maxRPM;
    ShowNotification(niData, "Max. RPM calculation done", "Fan #" + to_string(fanID + 1) + ": " + to_string(bestBoostPoint.maxRPM) + " RPM.", false);
    acpi->SetPower(acpi->powers[fan_conf->lastProf->powerStage]);
    fanMode = true;
    mon->Start();
    return 0;
}

DWORD WINAPI CheckFanOverboost(LPVOID lpParam) {
    int num = (int)(ULONG64)lpParam, rpm, crpm;
    mon->Stop();
    fanMode = false;
    acpi->Unlock();
    for (byte i = 0; i < acpi->fans.size(); i++)
        if (num < 0 || num == i) {
            int cSteps = 8, boost = 100, cBoost = 100, oldBoost = acpi->GetFanBoost(num, true);
            fan_conf->lastSelectedFan = i;
            boostCheck.clear();
            bestBoostPoint = { 100, 3000 };
            boostScale = 10; fanMinScale = 3000; fanMaxScale = 500;
            if ((rpm = SetFanSteady(i, boost)) < 0)
                break;
            fanMinScale = (rpm / 100) * 100;
            for (int steps = cSteps; steps; steps = steps >> 1) {
                // Check for uptrend
                while ((boost += steps) != cBoost)
                {
                    if ((crpm = SetFanSteady(i, boost, true)) > rpm) {
                        rpm = lastBoostPoint->maxRPM;
                        cSteps = steps;
                        bestBoostPoint.maxBoost = boost;
                        //DrawFan();
                    }
                    else {
                        if (crpm > 0)
                            break;
                        else
                            goto finish;
                    }
                }
                boost = bestBoostPoint.maxBoost;
                cBoost = boost + steps;
            }
            for (int steps = cSteps; steps; steps = steps >> 1) {
                // Check for uptrend
                boost -= steps;
                while (boost > 100 && (crpm = SetFanSteady(i, boost)) >= bestBoostPoint.maxRPM - 55) {
                    bestBoostPoint.maxBoost = boost;
                    //DrawFan();
                    boost -= steps;
                }
                if (crpm < 0)
                    goto finish;
                boost = bestBoostPoint.maxBoost;
            }
        finish:
            acpi->SetFanBoost(num, oldBoost, true);
            UpdateBoost(i);
            //DrawFan();
            ShowNotification(niData, "Max. boost calculation done", "Fan #" + to_string(i+1) + ": Final boost " + to_string(bestBoostPoint.maxBoost)
                + " @ " + to_string(bestBoostPoint.maxRPM) + " RPM.", false);
        }
    acpi->SetPower(acpi->powers[fan_conf->lastProf->powerStage]);
    fanMode = true;
    mon->Start();
    return 0;
}

INT_PTR CALLBACK FanCurve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    sen_block* cFan = fan_conf->FindSensor();
    if (!cArea.right) {
        GetClientRect(fanWindow, &cArea);
        cArea.right--; cArea.bottom--;
    }

    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        DrawFan();
        EndPaint(hDlg, &ps);
        return true;
    } break;
    case WM_MOUSEMOVE: {
        if (fanMode) {
            fan_point clk = Screen2Fan(lParam);
            if (cFan && wParam & MK_LBUTTON) {
                *lastFanPoint = clk;
                //lastFanPoint->temp = clk.temp;
                DrawFan();
            }
            SetToolTip(toolTip, "Temp: " + to_string(clk.temp) + ", Boost: " + to_string(clk.boost));
        }
        else {
            SetToolTip(toolTip, "Boost " + to_string((cArea.bottom - GET_Y_LPARAM(lParam)) * boostScale / cArea.bottom + 100) + " @ " +
                to_string(GET_X_LPARAM(lParam) * fanMaxScale / cArea.right + fanMinScale) + " RPM");
        }
    } break;
    case WM_LBUTTONDOWN:
    {
        SetCapture(hDlg);
        if (fanMode && cFan) {
            // check and add point
            fan_point clk = Screen2Fan(lParam);
            lastFanPoint = find_if(cFan->points.begin(), cFan->points.end(),
                [clk](auto t) {
                    return abs(t.temp - clk.temp) <= DRAG_ZONE && abs(t.boost - clk.boost) <= DRAG_ZONE;
                });
            if (lastFanPoint == cFan->points.end()) {
                // insert element
                lastFanPoint = cFan->points.insert(find_if(cFan->points.begin(), cFan->points.end(),
                    [clk](auto t) {
                        return t.temp > clk.temp;
                    }), clk);
            }
            DrawFan();
        }
    } break;
    case WM_LBUTTONUP: {
        ReleaseCapture();
        // re-sort and de-duplicate array.
        if (fanMode && cFan) {
            for (auto fPi = cFan->points.begin(); fPi < cFan->points.end() - 1; fPi++) {
                if (fPi->temp > (fPi + 1)->temp) {
                    fan_point t = *fPi;
                    *fPi = *(fPi + 1);
                    *(fPi + 1) = t;
                }
                if (fPi->temp == (fPi + 1)->temp && fPi->boost == (fPi + 1)->boost)
                    cFan->points.erase(fPi + 1);
            }
            cFan->points.front().temp = 0;
            cFan->points.back().temp = 100;
            DrawFan();
        }
        SetFocus(GetParent(hDlg));
    } break;
    case WM_RBUTTONUP: {
        // remove point from curve...
        if (fanMode && cFan && cFan->points.size() > 2) {
            // check and remove point
            fan_point clk = Screen2Fan(lParam);
            for (auto fPi = cFan->points.begin() + 1; fPi < cFan->points.end() - 1; fPi++)
                if (abs(fPi->temp - clk.temp) <= DRAG_ZONE && abs(fPi->boost - clk.boost) <= DRAG_ZONE) {
                    // Remove this element...
                    cFan->points.erase(fPi);
                    break;
                }
            DrawFan();
        }
        SetFocus(GetParent(hDlg));
    } break;
    case WM_SIZE:
        GetClientRect(fanWindow, &cArea);
        cArea.right--; cArea.bottom--;
        //DrawFan();
        break;
    case WM_ERASEBKGND:
        return true;
    case WM_TIMER:
        if (IsWindowVisible(hDlg))
            DrawFan();
        break;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

string GetFanName(int ind) {
    string name;
    switch (acpi->fans[ind].type)
    {
    case 0: name = "CPU"; break;
    case 1: name = "GPU"; break;
    default: name = "Fan";
    }
    return name + " " + to_string(ind + 1) + " (" + to_string(mon->fanRpm[ind]) + ")";
}

void ReloadFanView(HWND list) {
    fanUpdateBlock = true;
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    if (!ListView_GetColumnWidth(list, 0)) {
        LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
        ListView_InsertColumn(list, 0, &lCol);
    }
    for (int i = 0; i < acpi->fans.size(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_STATE, i};
        string name = GetFanName(i);
        lItem.pszText = (LPSTR)name.c_str();
        if (i == fan_conf->lastSelectedFan) {
            lItem.state = LVIS_SELECTED;
        }
        else
            lItem.state = 0;
        ListView_InsertItem(list, &lItem);
        if (fan_conf->lastProf->fanControls[i].find(fan_conf->lastSelectedSensor) != fan_conf->lastProf->fanControls[i].end() 
            && fan_conf->lastProf->fanControls[i][fan_conf->lastSelectedSensor].active) {
            ListView_SetCheckState(list, i, true);
        }
    }
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
    fanUpdateBlock = false;
}


void ReloadPowerList(HWND list) {
    ComboBox_ResetContent(list);
    for (auto i = fan_conf->powers.begin(); i != fan_conf->powers.end(); i++) {
        int pos = ComboBox_AddString(list, (LPARAM)(i->second.c_str()));
        if (pos == fan_conf->lastProf->powerStage)
            ComboBox_SetCurSel(list, pos);
    }
    if (acpi->GetDeviceFlags() & DEV_FLAG_GMODE) {
        int pos = ComboBox_AddString(list, (LPARAM)("G-Mode"));
        if (fan_conf->lastProf->gmode)
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
    for (int i = 0; i < acpi->sensors.size(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
        string name = to_string(acpi->GetTempValue(i)) + " (" + to_string(mon->maxTemps[acpi->sensors[i].sid]) + ")";
        lItem.lParam = acpi->sensors[i].sid;
        lItem.pszText = (LPSTR)name.c_str();
        if (acpi->sensors[i].sid == fan_conf->lastSelectedSensor) {
            lItem.state = LVIS_SELECTED;
            rpos = i;
        }
        else
            lItem.state = 0;
        ListView_InsertItem(list, &lItem);
        auto pwr = fan_conf->sensors.find(acpi->sensors[i].sid);
        name = pwr != fan_conf->sensors.end() ? pwr->second : acpi->sensors[i].name;
        ListView_SetItemText(list, i, 1, (LPSTR)name.c_str());
    }
    RECT cArea;
    GetClientRect(list, &cArea);
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(list, 1, cArea.right - ListView_GetColumnWidth(list, 0));
    ListView_EnsureVisible(list, rpos, false);
}

void TempUIEvent(NMLVDISPINFO* lParam, HWND tempList, HWND fanList) {
    auto pwr = fan_conf->sensors.find((WORD)lParam->item.lParam);
    switch (lParam->hdr.code) {
    case LVN_BEGINLABELEDIT: {
        KillTimer(GetParent(tempList), 0);
        Edit_SetText(ListView_GetEditControl(tempList), (pwr != fan_conf->sensors.end() ? pwr->second : acpi->sensors[lParam->item.iItem].name).c_str());
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
            if (strlen(lParam->item.pszText))
                fan_conf->sensors[(WORD)lParam->item.lParam] = lParam->item.pszText;
            else {
                if (pwr != fan_conf->sensors.end())
                    fan_conf->sensors.erase(pwr);
            }
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
            DrawFan();
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
            DrawFan();
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
            DrawFan();
            ListView_SetItemState(fanList, lParam->iItem, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
}