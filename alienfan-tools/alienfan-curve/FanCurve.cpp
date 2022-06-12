#include "ConfigFan.h"
#include "MonHelper.h"
#include "alienfan-SDK.h"
#include <windowsx.h>
#include <common.h>

#define DRAG_ZONE 2

extern ConfigFan* fan_conf;
extern MonHelper* mon;
extern HWND fanWindow, tipWindow;
extern AlienFan_SDK::Control* acpi;

extern void SetToolTip(HWND, string);

extern NOTIFYICONDATA niData;

HWND toolTip = NULL;
extern HINSTANCE hInst;
bool fanMode = true;
RECT cArea{ 0 };

fan_point* lastFanPoint = NULL;
fan_overboost* lastBoostPoint = NULL, bestBoostPoint{ 0 };
vector<fan_overboost> boostCheck;

int boostScale = 10, fanMinScale = 4000, fanMaxScale = 500;

HANDLE ocStopEvent = CreateEvent(NULL, false, false, NULL);

fan_point Screen2Fan(LPARAM lParam) {
    return {
        (short)max(0, min(100, (100 * (GET_X_LPARAM(lParam))) / cArea.right)),
        (short)max(0, min(100, (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / cArea.bottom))
        //(short)max(0, min(100, (100 * (GET_X_LPARAM(lParam) - cArea.left)) / (cArea.right - cArea.left))),
        //(short)max(0, min(100, (100 * (cArea.bottom - GET_Y_LPARAM(lParam))) / (cArea.bottom - cArea.top)))
    };
}

POINT Fan2Screen(short temp, short boost) {
    return {
        temp* cArea.right / 100,
        (100 - boost)* cArea.bottom / 100
        //temp * cArea.right / 100 + cArea.left,
        //(100 - boost)* cArea.bottom / 100 + cArea.top
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
        POINT mark;
        HDC hdc_r = GetDC(fanWindow);

        // Double buff...
        HDC hdc = CreateCompatibleDC(hdc_r);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, cArea.right - cArea.left + 1, cArea.bottom - cArea.top + 1);
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

        if (fanMode /*&& fan_conf->lastSelectedFan != -1*/) {
            // curve...
            temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
            fan_block* fan = NULL;
            int fanBoost = acpi->GetFanBoost(fan_conf->lastSelectedFan);
            for (auto senI = fan_conf->lastProf->fanControls.begin();
                senI < fan_conf->lastProf->fanControls.end(); senI++)
                if (fan = fan_conf->FindFanBlock(&(*senI), fan_conf->lastSelectedFan)) {
                    // draw fan curve
                    if (sen && sen->sensorIndex == senI->sensorIndex)
                        SetDCPenColor(hdc, RGB(0, 255, 0));
                    else
                        SetDCPenColor(hdc, RGB(255, 255, 0));
                    SelectObject(hdc, GetStockObject(DC_PEN));
                    // First point
                    MoveToEx(hdc, cArea.left, cArea.bottom, NULL);
                    for (int i = 0; i < fan->points.size(); i++) {
                        mark = Fan2Screen(fan->points[i].temp, fan->points[i].boost);
                        LineTo(hdc, mark.x, mark.y);
                        Ellipse(hdc, mark.x - 2, mark.y - 2, mark.x + 2, mark.y + 2);
                    }
                    // Yellow dots
                    if (sen && senI->sensorIndex != sen->sensorIndex) {
                        SetDCPenColor(hdc, RGB(255, 255, 0));
                        SetDCBrushColor(hdc, RGB(255, 255, 0));
                        SelectObject(hdc, GetStockObject(DC_PEN));
                        SelectObject(hdc, GetStockObject(DC_BRUSH));
                        mark = Fan2Screen(mon->senValues[senI->sensorIndex], fanBoost);
                        Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
                    }
                }
            // Red dot
            SetDCPenColor(hdc, RGB(255, 0, 0));
            SetDCBrushColor(hdc, RGB(255, 0, 0));
            SelectObject(hdc, GetStockObject(DC_PEN));
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            mark = Fan2Screen(mon->senValues[fan_conf->lastSelectedSensor], fanBoost);
            Ellipse(hdc, mark.x - 3, mark.y - 3, mark.x + 3, mark.y + 3);
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
                string rpmText = to_string(lastBoostPoint->maxBoost) + " @ " + to_string(lastBoostPoint->maxRPM)
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

        BitBlt(hdc_r, 0, 0, cArea.right - cArea.left + 1, cArea.bottom - cArea.top + 1, hdc, 0, 0, SRCCOPY);

        SelectObject(hdc, hOld);
        DeleteObject(hbmMem);
        DeleteDC(hdc);
        ReleaseDC(fanWindow, hdc_r);
        DeleteDC(hdc_r);
    }
}

int SetFanSteady(byte boost, bool downtrend = false) {
    acpi->SetFanBoost(bestBoostPoint.fanID, boost, true);
    // Check the trend...
    int fRpm, fDelta = -1, oDelta, bRpm = acpi->GetFanRPM(bestBoostPoint.fanID);
    boostCheck.push_back({ bestBoostPoint.fanID, boost, (USHORT)bRpm });
    lastBoostPoint = &boostCheck.back();
    DrawFan();
    //if (WaitForSingleObject(ocStopEvent, 5000) != WAIT_TIMEOUT)
    //    return -1;
    do {
        oDelta = fDelta;
        if (WaitForSingleObject(ocStopEvent, 6000) != WAIT_TIMEOUT)
            return -1;
        fRpm = acpi->GetFanRPM(bestBoostPoint.fanID);
        fDelta = fRpm - bRpm;
        lastBoostPoint->maxRPM = max(bRpm, fRpm);
        bestBoostPoint.maxRPM = max(bestBoostPoint.maxRPM, lastBoostPoint->maxRPM);
        bRpm = fRpm;
        DrawFan();
    } while ((fDelta > 0 || oDelta < 0) && (!downtrend || !(fDelta < -40 && oDelta < -40)));
    return lastBoostPoint->maxRPM;
}

void UpdateBoost() {
    auto pos = find_if(fan_conf->boosts.begin(), fan_conf->boosts.end(),
        [](auto t) {
            return t.fanID == bestBoostPoint.fanID;
        });
    if (pos != fan_conf->boosts.end()) {
        pos->maxBoost = bestBoostPoint.maxBoost;
        pos->maxRPM = max(bestBoostPoint.maxRPM, pos->maxRPM);
    }
    else {
        fan_conf->boosts.push_back(bestBoostPoint);
    }
    acpi->boosts[bestBoostPoint.fanID] = bestBoostPoint.maxBoost;
    acpi->maxrpm[bestBoostPoint.fanID] = max(bestBoostPoint.maxRPM, acpi->maxrpm[bestBoostPoint.fanID]);
    fan_conf->Save();
}

DWORD WINAPI CheckFanOverboost(LPVOID lpParam) {
    int num = (int)lpParam, steps = 8, cSteps, boost = 100, cBoost = 100, crpm, rpm, oldBoost = acpi->GetFanBoost(num, true);
    mon->Stop();
    fanMode = false;
    acpi->Unlock();
    for (int i = 0; i < acpi->HowManyFans(); i++)
        if (num < 0 || num == i) {
            fan_conf->lastSelectedFan = i;
            bestBoostPoint = { (byte)i, 100, 0 };
            boostScale = 10; fanMinScale = 4000; fanMaxScale = 500;
            if ((rpm = SetFanSteady(boost)) < 0)
                goto finish;
            fanMinScale = (rpm / 100) * 100;
            for (int steps = 8; steps; steps = steps >> 1) {
                // Check for uptrend
                while ((boost += steps) != cBoost)
                {
                    if ((crpm = SetFanSteady(boost, true)) > rpm) {
                        rpm = lastBoostPoint->maxRPM;
                        cSteps = steps;
                        bestBoostPoint.maxBoost = boost;
                        DrawFan();
                        //steps = steps << 1;
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
            for (int steps = cSteps > 1 ? cSteps >> 1 : 1; steps; steps = steps >> 1) {
                // Check for uptrend
                boost -= steps;
                while ((crpm = SetFanSteady(boost, true)) >= bestBoostPoint.maxRPM - 60) {
                    bestBoostPoint.maxBoost = boost;
                    DrawFan();
                    boost -= steps;
                }
                if (crpm < 0)
                    goto finish;
                boost = bestBoostPoint.maxBoost;
            }
            acpi->SetFanBoost(num, oldBoost, true);
            UpdateBoost();
            DrawFan();
        }
    ShowNotification(&niData, "Overboost calculation done", " Final boost " + to_string(bestBoostPoint.maxBoost)
        + " @ " + to_string(bestBoostPoint.maxRPM) + " RPM.", false);
finish:
    lastBoostPoint = NULL;
    boostCheck.clear();
    acpi->SetPower(fan_conf->lastProf->powerStage);
    fanMode = true;
    mon->Start();
    return 0;
}

INT_PTR CALLBACK FanCurve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    fan_block* cFan = fan_conf->FindFanBlock(fan_conf->FindSensor(fan_conf->lastSelectedSensor), fan_conf->lastSelectedFan);
    GetClientRect(hDlg, &cArea);
    cArea.right--; cArea.bottom--;

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
            if (lastFanPoint && wParam & MK_LBUTTON) {
                *lastFanPoint = clk;
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
        if (fanMode && cFan) {
            SetCapture(hDlg);
            // check and add point
            fan_point clk = Screen2Fan(lParam);
            for (auto fPi = cFan->points.begin(); fPi < cFan->points.end(); fPi++) {
                if (abs(fPi->temp - clk.temp) <= DRAG_ZONE && abs(fPi->boost - clk.boost) <= DRAG_ZONE) {
                    // Starting drag'n'drop...
                    lastFanPoint = &(*fPi);
                    break;
                }
                if (fPi->temp > clk.temp) {
                    // Insert point here...
                    lastFanPoint = &(*cFan->points.insert(fPi, clk));
                    break;
                }
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
            fan_conf->Save();
        }
        lastFanPoint = NULL;
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
            fan_conf->Save();
        }
        SetFocus(GetParent(hDlg));
    } break;
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_ERASEBKGND:
        return true;
        break;
    }
    return DefWindowProc(hDlg, message, wParam, lParam);
}

void ReloadFanView(HWND list, int cID) {
    temp_block* sen = fan_conf->FindSensor(fan_conf->lastSelectedSensor);
    //HWND list = GetDlgItem(hDlg, IDC_FAN_LIST);
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES /*| LVS_EX_AUTOSIZECOLUMNS*/ | LVS_EX_FULLROWSELECT);
    if (!ListView_GetColumnWidth(list, 0)) {
        LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
        ListView_InsertColumn(list, 0, &lCol);
    }
    for (int i = 0; i < acpi->HowManyFans(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
        string name = "Fan " + to_string(i + 1) + " (" + to_string(acpi->GetFanRPM(i)) + ")";
        lItem.lParam = i;
        lItem.pszText = (LPSTR)name.c_str();
        if (i == cID) {
            lItem.state = LVIS_SELECTED;
            SendMessage(fanWindow, WM_PAINT, 0, 0);
        }
        else
            lItem.state = 0;
        ListView_InsertItem(list, &lItem);
        if (sen && fan_conf->FindFanBlock(sen, i)) {
            fan_conf->lastSelectedSensor = -1;
            ListView_SetCheckState(list, i, true);
            fan_conf->lastSelectedSensor = sen->sensorIndex;
        }
    }

    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
}


void ReloadPowerList(HWND list, int id) {
    //HWND list = GetDlgItem(hDlg, IDC_COMBO_POWER);
    ComboBox_ResetContent(list);
    for (int i = 0; i < acpi->HowManyPower(); i++) {
        string name;
        if (i) {
            auto pwr = fan_conf->powers.find(acpi->powers[i]);
            name = pwr != fan_conf->powers.end() ? pwr->second : "Level " + to_string(i);
        }
        else
            name = "Manual";
        int pos = ComboBox_AddString(list, (LPARAM)(name.c_str()));
        ComboBox_SetItemData(list, pos, i);
        if (i == id)
            ComboBox_SetCurSel(list, pos);
    }
}

void ReloadTempView(HWND list, int cID) {
    int rpos = 0;
    //HWND list = GetDlgItem(hDlg, IDC_TEMP_LIST);
    ListView_DeleteAllItems(list);
    ListView_SetExtendedListViewStyle(list, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    if (!ListView_GetColumnWidth(list, 1)) {
        LVCOLUMNA lCol{ LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 100, (LPSTR)"Temp" };
        ListView_InsertColumn(list, 0, &lCol);
        lCol.pszText = (LPSTR)"Name";
        lCol.iSubItem = 1;
        ListView_InsertColumn(list, 1, &lCol);
    }
    for (int i = 0; i < acpi->HowManySensors(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
        string name = to_string(acpi->GetTempValue(i)) + " (" + to_string(mon->maxTemps[i]) + ")";
        lItem.lParam = i;
        lItem.pszText = (LPSTR)name.c_str();
        if (i == cID) {
            lItem.state = LVIS_SELECTED;
            rpos = i;
        }
        else
            lItem.state = 0;
        ListView_InsertItem(list, &lItem);
        ListView_SetItemText(list, i, 1, (LPSTR)acpi->sensors[i].name.c_str());
    }
    RECT cArea;
    GetClientRect(list, &cArea);
    ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(list, 1, cArea.right - ListView_GetColumnWidth(list, 0));
    ListView_EnsureVisible(list, rpos, false);
}
