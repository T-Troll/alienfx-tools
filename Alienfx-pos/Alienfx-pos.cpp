// Alienfx-pos.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Alienfx-pos.h"
#include "Alienfx_SDK.h"
#include <CommCtrl.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND mDlg;

POINT grid{ 20,6 };

AlienFX_SDK::Mappings afx_dev;

int cLightID = 0;

vector<lightpos> cLightsPos;

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

    return (int) msg.wParam;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
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
    bzone.right /= grid.x;
    bzone.bottom /= grid.y;
    LONGLONG bId = 2000;
    for (int y = 0; y < grid.y; y++)
        for (int x = 0; x < grid.x; x++) {
            HWND btn = CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                bzone.left + x * bzone.right, bzone.top + y * bzone.bottom, bzone.right, bzone.bottom, dlg, (HMENU)bId, hInst, NULL);
            bId++;
        }
}

void RedrawButtonZone(HWND dlg) {
    for (int i = 0; i < grid.x * grid.y; i++)
        RedrawWindow(GetDlgItem(dlg, 2000 + i), 0, 0, RDW_INVALIDATE);
    RedrawWindow(GetDlgItem(dlg, IDC_BUT_PWR), 0, 0, RDW_INVALIDATE);
    RedrawWindow(GetDlgItem(dlg, IDC_BUT_TOUCH), 0, 0, RDW_INVALIDATE);
}

void SetGridSize(HWND dlg, int x, int y) {
    //delete ambUIupdate;
    //if (eve->capt) {
    //    eve->capt->SetGridSize(x, y);
    //}
    //else {
        grid.x = x;
        grid.y = y;
    //}
    InitButtonZone(dlg);
    //ambUIupdate = new ThreadHelper(AmbUpdate, dlg);
}

vector<lightpos>::iterator FindLightPos(int did, int lid) {
    auto res = find_if(cLightsPos.begin(), cLightsPos.end(), [did, lid](lightpos ls) {
        return ls.devID == did && ls.lightID == lid;
        });
    return res;
    //for (auto it = cLightsPos.begin(); it < cLightsPos.end(); it++)
    //    if (it->devID == did && it->lightID == lid)
    //        return &(*it);
    //return nullptr;
}

void SetLightMap(HWND hDlg) {
    int cDevID = afx_dev.fxdevs[0].dev->GetPID();
    AlienFX_SDK::mapping* lgh = afx_dev.GetMappingById(cDevID, cLightID);
    if (lgh) {
        // Grid...
        RedrawButtonZone(hDlg);
        // Info...
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_NAME), lgh->name.c_str());
        // Test...
        TestLight(0, lgh->lightid, false);
    }
}

void ModifyGrid(DWORD did, WORD lid, byte zone, byte x, byte y) {
    gridpos t{ zone, x, y };
    vector<lightpos>::iterator cLp = FindLightPos(did, lid);
    if (cLp == cLightsPos.end()) {
        cLightsPos.push_back({ did, lid });
        cLp = cLightsPos.end() - 1;
    }
    auto res = find_if(cLp->pos.begin(), cLp->pos.end(), [t](gridpos ls) {
        return ls.zone == t.zone && ls.x == t.x && ls.y == t.y;
        });
    if (res == cLp->pos.end())
        cLp->pos.push_back(t);
    else
        cLp->pos.erase(res);
    if (cLp->pos.empty())
        cLightsPos.erase(cLp);
}

BOOL CALLBACK DialogMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND gridX = GetDlgItem(hDlg, IDC_SLIDER_HSCALE),
        gridY = GetDlgItem(hDlg, IDC_SLIDER_VSCALE);

    int devID = afx_dev.fxdevs[0].dev->GetPID();

    switch (message) {
    case WM_INITDIALOG:
    {
        mDlg = hDlg;

        SendMessage(gridX, TBM_SETRANGE, true, MAKELPARAM(1, 20));
        SendMessage(gridX, TBM_SETPOS, true, grid.x);
        //SendMessage(gridX, TBM_SETTICFREQ, 16, 0);

        SendMessage(gridY, TBM_SETRANGE, true, MAKELPARAM(1, 6));
        SendMessage(gridY, TBM_SETPOS, true, grid.y);
        //SendMessage(gridY, TBM_SETTICFREQ, 16, 0);

        InitButtonZone(hDlg);
        SetLightMap(hDlg);
        //RedrawButtonZone(hDlg);
    } break;
    case WM_COMMAND:
    {
        if (LOWORD(wParam) >= 2000) { // grid button
            switch (HIWORD(wParam)) {
            case BN_CLICKED: {
                ModifyGrid(devID, cLightID, 1, (wParam - 2000) % grid.x, (wParam - 2000) / grid.x);
                RedrawButtonZone(hDlg);
            } break;
            }
        }
        switch (LOWORD(wParam))
        {
        case IDCLOSE: case IDC_CLOSE: case IDM_EXIT:
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
        case IDC_BUT_PWR:
            ModifyGrid(devID, cLightID, 0, 0, 0);
            RedrawButtonZone(hDlg);
            break;
        case IDC_BUT_TOUCH:
            ModifyGrid(devID, cLightID, 2, 0, 0);
            RedrawButtonZone(hDlg);
            break;
        }
    } break;
    case WM_HSCROLL:
        switch (LOWORD(wParam)) {
        case TB_THUMBPOSITION: case TB_ENDTRACK:
            if ((HWND)lParam == gridX) {
                SetGridSize(hDlg, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0), grid.y);
                //SetSlider(sTip2, conf->amb_conf->grid.x);
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
                SetGridSize(hDlg, grid.x, (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
                //SetSlider(sTip3, conf->amb_conf->grid.y);
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
        if (ditem->CtlID >= 2000 || ditem->CtlID == IDC_BUT_PWR || ditem->CtlID == IDC_BUT_TOUCH) {
            gridpos t;
            HBRUSH Brush = NULL;
            bool selected = false;
            vector<lightpos>::iterator cLp = FindLightPos(devID, cLightID);
            switch (ditem->CtlID) {
            case IDC_BUT_PWR:
                t = { 0, 0, 0 };
                break;
            case IDC_BUT_TOUCH:
                t = { 2, 0, 0 };
                break;
            default:
                t = { 1, (byte)((ditem->CtlID - 2000) % grid.x), (byte)((ditem->CtlID - 2000) / grid.x) };
            }
            if (cLp != cLightsPos.end()) {
                auto res = find_if(cLp->pos.begin(), cLp->pos.end(), [t](gridpos ls) {
                    return ls.zone == t.zone && ls.x == t.x && ls.y == t.y;
                    });
                if (res != cLp->pos.end()) {
                    Brush = CreateSolidBrush(RGB(0, 255, 0));
                    selected = true;
                }
            }
            if (!Brush)
                Brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(ditem->hDC, &ditem->rcItem, Brush);
            DeleteObject(Brush);
            DrawEdge(ditem->hDC, &ditem->rcItem, selected ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
        }
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