// Alienfx-pos.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Alienfx-pos.h"
#include "Alienfx_SDK.h"
#include <CommCtrl.h>
//#include <windef.h>
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

int cLightID = 0, dIndex = 0, devID = 0;

AlienFX_SDK::lightgrid* mainGrid;

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
    SetDlgItemInt(hDlg, IDC_LIGHTID, cLightID, false);
    CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, lgh && lgh->flags & ALIENFX_FLAG_POWER);
    CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, lgh && lgh->flags & ALIENFX_FLAG_INDICATOR);
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

typedef struct tag_dlghdr {
    HWND hwndTab;       // tab control
    HWND hwndDisplay;   // current child dialog box
    RECT rcDisplay;     // display rectangle for the tab control
    DLGTEMPLATE* apRes;
} DLGHDR;

int tabSel = 0;

BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void RepaintGrid(HWND);

void OnSelChanged(HWND hwndDlg)
{
    // Get the dialog header data.
    DLGHDR* pHdr = (DLGHDR*)GetWindowLongPtr(
        hwndDlg, GWLP_USERDATA);

    // Get the index of the selected tab.
    tabSel = TabCtrl_GetCurSel(hwndDlg);
    mainGrid = &afx_dev.GetGrids()->at(tabSel);

    // Repaint.
    RepaintGrid(pHdr->hwndDisplay);
}

WNDPROC oldproc;

void SetNewGridName(HWND hDlg) {
    // set text and close
    mainGrid->name.resize(GetWindowTextLength(hDlg) + 1);
    GetWindowText(hDlg, (LPSTR)mainGrid->name.c_str(), 255);
    // change tab text, close window
    TCITEM tie{ TCIF_TEXT };
    tie.pszText = (LPSTR)mainGrid->name.c_str();
    TabCtrl_SetItem(GetParent(hDlg), tabSel, &tie);
    OnSelChanged(GetParent(hDlg));
    DestroyWindow(hDlg);
}

LRESULT CALLBACK GridNameEdit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_KEYUP: {
        switch (wParam) {
        case VK_RETURN: case VK_TAB: {
            SetNewGridName(hDlg);
        } break;
        case VK_ESCAPE: {
            // just close edit
            DestroyWindow(hDlg);
        } break;
        default:
            CallWindowProcW(oldproc, hDlg, message, wParam, lParam);
        }
    } break;
    case WM_KILLFOCUS:
        SetNewGridName(hDlg);
        break;
    default: return CallWindowProcW(oldproc, hDlg, message, wParam, lParam);
    }
    return 0;
}

BOOL CALLBACK DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE),
        gridTab = GetDlgItem(hDlg, IDC_TAB_GRIDS);

    devID = afx_dev.fxdevs[dIndex].dev->GetPID();

    switch (message) {
    case WM_INITDIALOG:
    {
        mDlg = hDlg;

        DLGHDR* pHdr = (DLGHDR*)LocalAlloc(LPTR, sizeof(DLGHDR));
        SetWindowLongPtr(gridTab, GWLP_USERDATA, (LONG_PTR)pHdr);

        pHdr->hwndTab = gridTab;

        TCITEM tie{ TCIF_TEXT };

        GetClientRect(pHdr->hwndTab, &pHdr->rcDisplay);

        pHdr->rcDisplay.left = GetSystemMetrics(SM_CXBORDER);
        pHdr->rcDisplay.top = GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER);
        pHdr->rcDisplay.right -= 2 * GetSystemMetrics(SM_CXBORDER) + 1;
        pHdr->rcDisplay.bottom -= GetSystemMetrics(SM_CYBORDER) + 1;

        pHdr->apRes = (DLGTEMPLATE*)LockResource(LoadResource(hInst, FindResource(NULL, MAKEINTRESOURCE(IDD_GRIDBLOCK), RT_DIALOG)));// DoLockDlgRes(MAKEINTRESOURCE((IDD_DIALOG_COLORS + i)));

        for (int i = 0; i < afx_dev.GetGrids()->size(); i++) {
            tie.pszText = (char*)afx_dev.GetGrids()->at(i).name.c_str();
            TabCtrl_InsertItem(gridTab, i, (LPARAM)&tie);
        }

        // Special tabs for add/remove
        tie.pszText = LPSTR("+");
        TabCtrl_InsertItem(gridTab, afx_dev.GetGrids()->size(), (LPARAM)&tie);
        tie.pszText = LPSTR("-");
        TabCtrl_InsertItem(gridTab, afx_dev.GetGrids()->size()+1, (LPARAM)&tie);

        TabCtrl_SetMinTabWidth(gridTab, 10);

        pHdr->hwndDisplay = CreateDialogIndirect(hInst, (DLGTEMPLATE*)pHdr->apRes, pHdr->hwndTab, (DLGPROC)TabGrid);

        SetWindowPos(pHdr->hwndDisplay, NULL,
            pHdr->rcDisplay.left, pHdr->rcDisplay.top,
            pHdr->rcDisplay.right - pHdr->rcDisplay.left,
            pHdr->rcDisplay.bottom - pHdr->rcDisplay.top,
            SWP_SHOWWINDOW);

        RedrawDevList(hDlg);
        //OnSelChanged(gridTab);
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
            OnSelChanged(gridTab);
            break;
        case IDC_BUT_PREV:
            if (cLightID) cLightID--;
            SetLightMap(hDlg);
            OnSelChanged(gridTab);
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
        case IDC_BUT_CLEAR: {
            auto pos = find_if(afx_dev.fxdevs[dIndex].lights.begin(), afx_dev.fxdevs[dIndex].lights.end(),
                [](AlienFX_SDK::mapping* t) {
                    return t->devid == devID && t->lightid == cLightID;
                });
            if (pos != afx_dev.fxdevs[dIndex].lights.end())
                afx_dev.fxdevs[dIndex].lights.erase(pos);
            // Remove from mapping list...
            pos = find_if(afx_dev.GetMappings()->begin(), afx_dev.GetMappings()->end(),
                [](AlienFX_SDK::mapping* t) {
                    return t->devid == devID && t->lightid == cLightID;
                });
            if (pos != afx_dev.GetMappings()->end())
                afx_dev.GetMappings()->erase(pos);
            // Clear grid
            for (auto it = afx_dev.GetGrids()->begin(); it < afx_dev.GetGrids()->end(); it++)
                for (int x = 0; x < it->x; x++)
                    for (int y = 0; y < it->y; y++)
                        if (it->grid[x][y] == MAKELPARAM(devID, cLightID))
                            it->grid[x][y] = 0;
            SetLightMap(hDlg);
        } break;
        case IDC_BUT_LAST: {
            cLightID = 0;
            for (auto it = afx_dev.fxdevs[dIndex].lights.begin(); it < afx_dev.fxdevs[dIndex].lights.end(); it++)
                cLightID = max(cLightID, (*it)->lightid);
            SetLightMap(hDlg);
        } break;
        case IDC_CHECK_INDICATOR:
        {
            AlienFX_SDK::mapping* lgh = afx_dev.GetMappingById(devID, cLightID);
            if (lgh) {
                lgh->flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED ?
                    lgh->flags | ALIENFX_FLAG_INDICATOR :
                    lgh->flags & ~ALIENFX_FLAG_INDICATOR;
            }
        } break;
        }
    } break;
    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->idFrom) {
        case IDC_TAB_GRIDS: {
            switch (((NMHDR*)lParam)->code) {
            case NM_RCLICK: {
                RECT tRect;
                TabCtrl_GetItemRect(gridTab, tabSel, &tRect);
                //GetClientRect(gridTab, &tRect);
                HWND tabEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                    tRect.left - 1, tRect.top -1, tRect.right - tRect.left + 2,
                    tRect.bottom - tRect.top + 2/*GetSystemMetrics(SM_CYSMSIZE) - GetSystemMetrics(SM_CYBORDER)*/, gridTab, NULL,
                    GetModuleHandle(NULL), NULL);
                oldproc = (WNDPROC)SetWindowLongPtr(tabEdit, GWLP_WNDPROC, (LONG_PTR)GridNameEdit);
                Edit_SetText(tabEdit, mainGrid->name.c_str());
                SetFocus(tabEdit);
            } break;
            case TCN_SELCHANGE: {
                int newSel = TabCtrl_GetCurSel(gridTab);
                if ( newSel != tabSel) { // selection changed!
                    if (newSel < afx_dev.GetGrids()->size())
                        OnSelChanged(gridTab);
                    else
                        if (newSel == afx_dev.GetGrids()->size()) {
                            // add grid
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
                            //RedrawGridList(hDlg);
                            TCITEM tie{ TCIF_TEXT };
                            tie.pszText = (LPSTR)mainGrid->name.c_str();
                            TabCtrl_InsertItem(gridTab, afx_dev.GetGrids()->size() - 1, (LPARAM)&tie);
                            TabCtrl_SetCurSel(gridTab, afx_dev.GetGrids()->size() - 1);
                            OnSelChanged(gridTab);
                        }
                        else
                        {
                            if (afx_dev.GetGrids()->size() > 1) {
                                int newTab = tabSel;
                                for (auto it = afx_dev.GetGrids()->begin(); it < afx_dev.GetGrids()->end(); it++) {
                                    if (it->id == mainGrid->id) {
                                        // remove
                                        if ((it + 1) == afx_dev.GetGrids()->end())
                                            newTab--;
                                        afx_dev.GetGrids()->erase(it);
                                        TabCtrl_DeleteItem(gridTab, tabSel);
                                        TabCtrl_SetCurSel(gridTab, newTab);
                                        OnSelChanged(gridTab);
                                        break;
                                    }
                                }
                            }
                        }
                }
            } break;
            }
        } break;
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