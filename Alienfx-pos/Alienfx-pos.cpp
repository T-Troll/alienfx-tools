// Alienfx-pos.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Alienfx-pos.h"
#include "Alienfx_SDK.h"
#include <CommCtrl.h>
#include <windef.h>
#include <windowsx.h>

#define MAX_LOADSTRING 100

// debug print
#ifdef _DEBUG
#define DebugPrint(_x_) OutputDebugString(_x_);
#else
#define DebugPrint(_x_)
#endif

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND mDlg;

//POINT grid{ 20,6 };

AlienFX_SDK::Mappings afx_dev;

int cLightID = 0, dIndex = 0;

AlienFX_SDK::lightgrid* mainGrid;

RECT dragZone;
POINT clkPoint, dragStart;
DWORD oldClkValue;

size_t FillAllDevs() {
    //config->haveV5 = false;
    afx_dev.AlienFXAssignDevices(NULL, 255, 0/*config->finalBrightness, config->finalPBState*/);
    // global effects check
    for (int i = 0; i < afx_dev.fxdevs.size(); i++)
        if (afx_dev.fxdevs[i].dev->GetVersion() == 5) {
            /*config->haveV5 = true;*/ break;
        }
    return afx_dev.fxdevs.size();
}

void TestLight(int did, int id, bool wp)
{
    vector<byte> opLights;

    for (auto lIter = afx_dev.fxdevs[did].lights.begin(); lIter != afx_dev.fxdevs[did].lights.end(); lIter++)
        if ((*lIter)->lightid != id && !((*lIter)->flags & ALIENFX_FLAG_POWER))
            opLights.push_back((byte)(*lIter)->lightid);

    bool dev_ready = false;
    for (int c_count = 0; c_count < 20 && !(dev_ready = afx_dev.fxdevs[did].dev->IsDeviceReady()); c_count++)
        Sleep(20);
    if (!dev_ready) return;

    AlienFX_SDK::Colorcode c = wp ? afx_dev.fxdevs[did].desc->white : AlienFX_SDK::Colorcode({ 0 });
    afx_dev.fxdevs[did].dev->SetMultiLights(&opLights, c);

    afx_dev.fxdevs[did].dev->UpdateColors();
    if (id != -1) {
        afx_dev.fxdevs[did].dev->SetColor(id, { 0, 255, 0, 255 });
        afx_dev.fxdevs[did].dev->UpdateColors();
    }

}

// Forward declarations of functions included in this code module:
HWND                InitInstance(HINSTANCE, int);
BOOL CALLBACK       DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    afx_dev.LoadMappings();
    FillAllDevs();

    if (!afx_dev.GetGrids()->size())
        afx_dev.GetGrids()->push_back({ 0, 20, 8, "Main" });
    mainGrid = &afx_dev.GetGrids()->front();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALIENFXPOS, szWindowClass, MAX_LOADSTRING);

    // Perform application initialization:
    if (!InitInstance (hInstance, SW_NORMAL))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALIENFXPOS));

    MSG msg;
    // Main message loop:
    while ((GetMessage(&msg, 0, 0, 0)) != 0) {
        if (!TranslateAccelerator(mDlg, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    afx_dev.SaveMappings();

    return (int) msg.wParam;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG_POSITION), NULL, (DLGPROC)DialogMain);

    if (mDlg) {

        SendMessage(mDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALIENFXPOS)));
        SendMessage(mDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIENFXPOS), IMAGE_ICON, 16, 16, 0));

        ShowWindow(mDlg, nCmdShow);
    }
    return mDlg;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

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
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId, hInst, NULL);
            bId++;
        }
}

void RedrawButtonZone(HWND dlg) {
    for (int i = 0; i < mainGrid->x * mainGrid->y; i++)
        RedrawWindow(GetDlgItem(dlg, 2000 + i), 0, 0, RDW_INVALIDATE);
}

void RedrawGridList(HWND hDlg) {
    int rpos = 0;
    HWND grid_list = GetDlgItem(hDlg, IDC_LIST_GRID);
    ListView_DeleteAllItems(grid_list);
    ListView_SetExtendedListViewStyle(grid_list, LVS_EX_FULLROWSELECT);
    LVCOLUMNA lCol{ LVCF_WIDTH, LVCFMT_LEFT, 100 };
    ListView_DeleteColumn(grid_list, 0);
    ListView_InsertColumn(grid_list, 0, &lCol);
    for (int i = 0; i < afx_dev.GetGrids()->size(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
        lItem.lParam = afx_dev.GetGrids()->at(i).id;
        lItem.pszText = (char*)afx_dev.GetGrids()->at(i).name.c_str();
        if (lItem.lParam == mainGrid->id) {
            lItem.state = LVIS_SELECTED;
            RedrawButtonZone(hDlg);
            rpos = i;
        }
        ListView_InsertItem(grid_list, &lItem);
    }
    ListView_SetColumnWidth(grid_list, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_EnsureVisible(grid_list, rpos, false);
}

void SetLightMap(HWND hDlg) {
    AlienFX_SDK::mapping* lgh = afx_dev.GetMappingById(afx_dev.fxdevs[0].dev->GetPID(), cLightID);
    if (lgh) {
        // Info...
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_NAME), lgh->name.c_str());
        //SetDlgItemText(hDlg, IDC_LIGHTID, ("ID: " + to_string(lgh->lightid)).c_str());
    }
    else {
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_NAME), "<not used>");
        //SetDlgItemText(hDlg, IDC_LIGHTID, "");
    }
    SetDlgItemText(hDlg, IDC_LIGHTID, ("ID: " + to_string(cLightID)).c_str());
    // mainGrid->..
    RedrawButtonZone(hDlg);
    // Test...
    TestLight(dIndex, cLightID, false);
}

void RedrawDevList(HWND hDlg) {
    int rpos = 0;
    HWND dev_list = GetDlgItem(hDlg, IDC_LIST_DEV);
    ListView_DeleteAllItems(dev_list);
    ListView_SetExtendedListViewStyle(dev_list, LVS_EX_FULLROWSELECT);
    LVCOLUMNA lCol{ LVCF_WIDTH, LVCFMT_LEFT, 100 };
    ListView_DeleteColumn(dev_list, 0);
    ListView_InsertColumn(dev_list, 0, &lCol);
    for (int i = 0; i < afx_dev.fxdevs.size(); i++) {
        LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
        if (!afx_dev.fxdevs[i].desc) {
            // no name
            string typeName = "Unknown";
            switch (afx_dev.fxdevs[i].dev->GetVersion()) {
            case 0: typeName = "Desktop"; break;
            case 1: case 2: case 3: case 4: typeName = "Notebook"; break;
            case 5: typeName = "Keyboard"; break;
            case 6: typeName = "Display"; break;
            case 7: typeName = "Mouse"; break;
            }
            afx_dev.GetDevices()->push_back({ (WORD)afx_dev.fxdevs[i].dev->GetVid(),
                (WORD)afx_dev.fxdevs[i].dev->GetPID(),
                typeName + ", #" + to_string(afx_dev.fxdevs[i].dev->GetPID())
                });
            afx_dev.fxdevs[i].desc = &afx_dev.GetDevices()->back();
        }
        lItem.lParam = i;
        lItem.pszText = (char*)afx_dev.fxdevs[i].desc->name.c_str();
        if (lItem.lParam == dIndex) {
            lItem.state = LVIS_SELECTED;
            rpos = i;
        }
        ListView_InsertItem(dev_list, &lItem);
    }
    ListView_SetColumnWidth(dev_list, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_EnsureVisible(dev_list, rpos, false);
}

void SetGridSize(HWND dlg, int x, int y) {
    mainGrid->x = x;
    mainGrid->y = y;
    InitButtonZone(dlg);
}

#define MAKEDWORD(_a, _b)   (DWORD)(((WORD)(((DWORD_PTR)(_a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(_b)) & 0xffff))) << 16)

void ModifyDragZone(WORD did, WORD lid, bool clear = false) {
    for (int x = dragZone.left; x <= dragZone.right; x++)
        for (int y = dragZone.top; y <= dragZone.bottom; y++) {
            if (!mainGrid->grid[x][y] && !clear)
                mainGrid->grid[x][y] = MAKEDWORD(did, lid);
            else
                if (clear && mainGrid->grid[x][y] == MAKEDWORD(did, lid))
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

AlienFX_SDK::mapping* FindCreateMapping() {
    AlienFX_SDK::mapping* lgh = afx_dev.GetMappingById(afx_dev.fxdevs[dIndex].dev->GetPID(), cLightID);
    if (!lgh) {
        // create new mapping
        lgh = new AlienFX_SDK::mapping({ 0, (WORD)afx_dev.fxdevs[dIndex].dev->GetPID(), (WORD)cLightID, 0,
            "Light " + to_string(cLightID + 1)});
        afx_dev.GetMappings()->push_back(lgh);
        afx_dev.fxdevs[dIndex].lights.push_back(afx_dev.GetMappings()->back());
    }
    return lgh;
}

BOOL CALLBACK DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    int devID = afx_dev.fxdevs[dIndex].dev->GetPID();

    switch (message) {
    case WM_INITDIALOG:
    {
        mDlg = hDlg;

        RedrawDevList(hDlg);

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(1, 22));
        SendMessage(gridX, TBM_SETPOS, true, mainGrid->x);
        //SendMessage(gridX, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(1, 10));
        SendMessage(gridY, TBM_SETPOS, true, mainGrid->y);
        //SendMessage(gridY, TBM_SETTICFREQ, 16, 0);

        InitButtonZone(hDlg);
        RedrawGridList(hDlg);
        SetLightMap(hDlg);
    } break;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDCLOSE: case IDM_EXIT:
            SendMessage(hDlg, WM_CLOSE, 0, 0);
        break;
        case IDC_BUT_NEXT:
            cLightID++;
            SetLightMap(hDlg);
            break;
        case IDC_BUT_PREV:
            if (cLightID) cLightID--;
            SetLightMap(hDlg);
            break;
        case IDC_EDIT_NAME:
            switch (HIWORD(wParam)) {
            case EN_CHANGE:
                if (Edit_GetModify(GetDlgItem(hDlg, IDC_EDIT_NAME))) {
                    AlienFX_SDK::mapping* lgh = FindCreateMapping();
                    lgh->name.resize(128);
                    GetDlgItemText(hDlg, IDC_EDIT_NAME, (LPSTR)lgh->name.c_str(), 127);
                    lgh->name.shrink_to_fit();
                }
                break;
            }
            break;
        case IDC_BUT_ADDGRID: {
            byte newGridIndex = 0;
            for (auto it = afx_dev.GetGrids()->begin(); it < afx_dev.GetGrids()->end(); ) {
                if (newGridIndex == it->id) {
                    newGridIndex++;
                    it = afx_dev.GetGrids()->begin();
                }
                else
                    it++;
            }
            afx_dev.GetGrids()->push_back({ newGridIndex, mainGrid->x, mainGrid->y, "Grid #" + to_string(newGridIndex) });
            mainGrid = &afx_dev.GetGrids()->back();
            RedrawGridList(hDlg);
        } break;
        case IDC_BUT_REMGRID:
            if (afx_dev.GetGrids()->size() > 1) {
                for (auto it = afx_dev.GetGrids()->begin(); it < afx_dev.GetGrids()->end(); it++ ) {
                    if (it->id == mainGrid->id) {
                        // remove
                        if ((it+1) != afx_dev.GetGrids()->end())
                            mainGrid = &(*(it+1));
                        else
                            mainGrid = &(*(it - 1));
                        afx_dev.GetGrids()->erase(it);
                        RedrawGridList(hDlg);
                        break;
                    }
                }
            }
        break;
        case IDC_BUT_CLEAR: {
            auto pos = find_if(afx_dev.fxdevs[dIndex].lights.begin(), afx_dev.fxdevs[dIndex].lights.end(),
                [devID](AlienFX_SDK::mapping* t) {
                    return t->devid == devID && t->lightid == cLightID;
                });
            if (pos != afx_dev.fxdevs[dIndex].lights.end())
                afx_dev.fxdevs[dIndex].lights.erase(pos);
            // Remove from mapping list...
            pos = find_if(afx_dev.GetMappings()->begin(), afx_dev.GetMappings()->end(),
                [devID](AlienFX_SDK::mapping* t) {
                    return t->devid == devID && t->lightid == cLightID;
                });
            if (pos != afx_dev.GetMappings()->end())
                afx_dev.GetMappings()->erase(pos);
            // Clear grid
            for (auto it = afx_dev.GetGrids()->begin(); it < afx_dev.GetGrids()->end(); it++)
                for (int x = 0; x < it->x; x++)
                    for (int y = 0; y < it->y; y++)
                        if (it->grid[x][y] == MAKEDWORD(devID, cLightID))
                            it->grid[x][y] = 0;
            SetLightMap(hDlg);
        } break;
        }
    } break;
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
            HBRUSH Brush = NULL;
            if (gridVal) {
                if (HIWORD(gridVal) == cLightID && LOWORD(gridVal) == devID)
                    Brush = CreateSolidBrush(RGB(0, 255, 0));
                else
                    Brush = CreateSolidBrush(RGB(255, 0, 0));
            }
            else
                Brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(ditem->hDC, &ditem->rcItem, Brush);
            DeleteObject(Brush);
            if (!gridVal)
                DrawEdge(ditem->hDC, &ditem->rcItem, /*selected ? EDGE_SUNKEN : */EDGE_RAISED, BF_RECT);
        }
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
                if (oldClkValue == MAKEDWORD(devID, cLightID))
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
            SetLightMap(hDlg);
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
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_LIST_GRID:
            switch (((NMHDR*)lParam)->code) {
            case LVN_ITEMACTIVATE: {
                NMITEMACTIVATE* sItem = (NMITEMACTIVATE*)lParam;
                ListView_EditLabel(GetDlgItem(hDlg, ((NMHDR*)lParam)->idFrom), sItem->iItem);
            } break;

            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (NMLISTVIEW*)lParam;
                if (lPoint->uNewState && LVIS_FOCUSED && lPoint->iItem != -1) {
                    // Select other item...
                    for (auto it = afx_dev.GetGrids()->begin(); it < afx_dev.GetGrids()->end(); it++)
                        if (it->id == lPoint->lParam) {
                            mainGrid = &(*it);
                            RedrawButtonZone(hDlg);
                            break;
                        }
                }
                else {
                    if (!ListView_GetSelectedCount(GetDlgItem(hDlg, ((NMHDR*)lParam)->idFrom)))
                        RedrawGridList(hDlg);
                }
            } break;
            case LVN_ENDLABELEDIT:
            {
                NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
                if (sItem->item.pszText) {
                    mainGrid->name = sItem->item.pszText;
                    ListView_SetItem(GetDlgItem(hDlg, IDC_LIST_GRID), &sItem->item);
                    ListView_SetColumnWidth(GetDlgItem(hDlg, IDC_LIST_GRID), 0, LVSCW_AUTOSIZE);
                    return true;
                }
                else
                    return false;
            } break;
            }
            break;
        case IDC_LIST_DEV:
            switch (((NMHDR*)lParam)->code) {
            case LVN_ITEMACTIVATE: {
                NMITEMACTIVATE* sItem = (NMITEMACTIVATE*)lParam;
                ListView_EditLabel(GetDlgItem(hDlg, ((NMHDR*)lParam)->idFrom), sItem->iItem);
            } break;

            case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* lPoint = (NMLISTVIEW*)lParam;
                if (lPoint->uNewState && LVIS_FOCUSED && lPoint->iItem != -1) {
                    // Select other item...
                    dIndex = (int)lPoint->lParam;
                    cLightID = 0;
                    SetLightMap(hDlg);
                }
                else {
                    if (!ListView_GetSelectedCount(GetDlgItem(hDlg, ((NMHDR*)lParam)->idFrom)))
                        RedrawDevList(hDlg);
                }
            } break;
            case LVN_ENDLABELEDIT:
            {
                NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
                if (sItem->item.pszText) {
                    afx_dev.fxdevs[dIndex].desc->name = sItem->item.pszText;
                    ListView_SetItem(GetDlgItem(hDlg, IDC_LIST_DEV), &sItem->item);
                    ListView_SetColumnWidth(GetDlgItem(hDlg, IDC_LIST_DEV), 0, LVSCW_AUTOSIZE);
                    return true;
                }
                else
                    return false;
            } break;
            }
            break;
        } break;
    case WM_CLOSE:
        EndDialog(hDlg, IDOK);
        DestroyWindow(hDlg);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default: return false;
    }
    return true;
}