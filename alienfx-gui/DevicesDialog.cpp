#include "alienfx-gui.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "common.h"

extern bool SetColor(HWND hDlg, AlienFX_SDK::Afx_colorcode*);
extern void RedrawButton(HWND hDlg, AlienFX_SDK::Afx_colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern void RemoveLightAndClean(int dPid, int eLid);
extern void SetMainTabs();

extern BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void CreateGridBlock(HWND gridTab, DLGPROC, bool);
extern void OnGridSelChanged(HWND);
extern void RedrawGridButtonZone(RECT* what = NULL);

extern void FindCreateMapping();

BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern AlienFX_SDK::Afx_light* keySetLight;
extern string GetKeyName(WORD vkcode);

extern FXHelper* fxhl;
extern EventHandler* eve;
extern HWND mDlg;
extern int tabSel;

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct gearInfo {
	string name;
	vector <AlienFX_SDK::Afx_grid> grids;
	vector<AlienFX_SDK::Afx_device> devs;
	bool selected = false, found = true;
};

extern HWND dDlg;

int eLid = 0;
vector<gearInfo> csv_devs;
WNDPROC oldproc;

extern AlienFX_SDK::Afx_device* activeDevice;

BOOL CALLBACK WhiteBalanceDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_INITDIALOG:
	{
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETTICFREQ, 16, 0);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETTICFREQ, 16, 0);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETTICFREQ, 16, 0);
		sTip1 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_RED), sTip1);
		sTip2 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_GREEN), sTip2);
		sTip3 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_BLUE), sTip3);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETPOS, true, activeDevice->white.r);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETPOS, true, activeDevice->white.g);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETPOS, true, activeDevice->white.b);
		SetSlider(sTip1, activeDevice->white.r);
		SetSlider(sTip2, activeDevice->white.r);
		SetSlider(sTip3, activeDevice->white.r);
		fxhl->TestLight(activeDevice, eLid, true, true);
		RECT pRect;
		GetWindowRect(GetDlgItem(dDlg, IDC_BUT_WHITE), &pRect);
		SetWindowPos(hDlg, NULL, pRect.right, pRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
		{
			if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_RED)) {
				activeDevice->white.r = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				SetSlider(sTip1, activeDevice->white.r);
			} else
				if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_GREEN)) {
					activeDevice->white.g = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, activeDevice->white.g);
				} else
					if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_BLUE)) {
						activeDevice->white.b = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
						SetSlider(sTip3, activeDevice->white.b);
					}
			fxhl->TestLight(activeDevice, eLid, true, true);
		} break;
		}
		break;
	case WM_CLOSE:
		fxhl->TestLight(activeDevice, eLid, true);
		EndDialog(hDlg, IDCLOSE);
		break;
	default: return false;
	}
	return true;
}

void SetLightInfo() {
	fxhl->TestLight(activeDevice, eLid);
	keySetLight = conf->afx_dev.GetMappingByDev(activeDevice, eLid);
	SetDlgItemText(dDlg, IDC_EDIT_NAME, keySetLight ? keySetLight->name.c_str() : "<unused>");
	SetDlgItemText(dDlg, IDC_STATIC_SCANCODE, keySetLight && keySetLight->scancode ? GetKeyName((WORD)keySetLight->scancode).c_str() : "Off");
	SetDlgItemInt(dDlg, IDC_LIGHTID, eLid, false);
	CheckDlgButton(dDlg, IDC_ISPOWERBUTTON, keySetLight && keySetLight->flags & ALIENFX_FLAG_POWER);
	CheckDlgButton(dDlg, IDC_CHECK_INDICATOR, keySetLight && keySetLight->flags & ALIENFX_FLAG_INDICATOR);
	EnableWindow(GetDlgItem(dDlg, IDC_BUT_CLEAR), (bool)keySetLight);
	RedrawGridButtonZone();
}

void UpdateLightsList() {
	HWND llist = GetDlgItem(dDlg, IDC_LIGHTS_LIST);
	ListBox_ResetContent(llist);
	int spos = -1;
	for (auto i = activeDevice->lights.begin(); i != activeDevice->lights.end(); i++) {
		int pos = ListBox_AddString(llist, i->name.c_str());
		ListBox_SetItemData(llist, pos, (LPARAM)i->lightid);
		if (i->lightid == eLid) {
			spos = pos;
			keySetLight = &(*i);
		}
	}
	ListBox_SetCurSel(llist, spos);
	SetLightInfo();
}

void UpdateDeviceInfo() {
	char descript[128];
	sprintf_s(descript, 128, "VID_%04X/PID_%04X, %d lights, APIv%d %s",
		activeDevice->vid, activeDevice->pid, (int)activeDevice->lights.size(), activeDevice->version, activeDevice->dev ? "" : "(inactive)");
	SetWindowText(GetDlgItem(dDlg, IDC_INFO_VID), descript);
	EnableWindow(GetDlgItem(dDlg, IDC_ISPOWERBUTTON), activeDevice->version && activeDevice->version < 5); // v5 and higher doesn't support power button
	UpdateLightsList();
}

void RedrawDevList() {
	int rpos = 0;
	HWND dev_list = GetDlgItem(dDlg, IDC_LIST_DEV);
	ListView_DeleteAllItems(dev_list);
	ListView_SetExtendedListViewStyle(dev_list, LVS_EX_FULLROWSELECT);
	if (!ListView_GetColumnWidth(dev_list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(dev_list, 0, &lCol);
	}
	for (auto i = conf->afx_dev.fxdevs.begin(); i != conf->afx_dev.fxdevs.end(); i++) {
		int pos = (int)(i - conf->afx_dev.fxdevs.begin());
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, pos};
		if (!i->name.length()) {
			//string typeName;
			switch (i->version) {
			case 0: i->name = "Desktop"; break;
			case 1: case 2: case 3: i->name = "Notebook"; break;
			case 4: i->name = "Notebook/Chassis"; break;
			case 5: case 8: i->name = "Keyboard"; break;
			case 6: i->name = "Display"; break;
			case 7: i->name = "Mouse"; break;
			default: i->name = "Unknown";
			}
			i->name += ", #" + to_string(i->pid);
		}
		lItem.pszText = (LPSTR)i->name.c_str();
		if (activeDevice->pid == i->pid) {
			lItem.state = LVIS_SELECTED | LVIS_FOCUSED;
			rpos = pos;
		}
		ListView_InsertItem(dev_list, &lItem);
	}
	ListView_SetColumnWidth(dev_list, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_EnsureVisible(dev_list, rpos, false);
}

void LoadCSV(string name) {
	csv_devs.clear();
	// Load mappings...
	//DebugPrint(("Opening file " + name).c_str());
	HANDLE file = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE) {
		// read and parse...
		size_t linePos = 0, oldLinePos = 0;
		DWORD filesize = GetFileSize(file, NULL);
		byte* filebuf = new byte[filesize+1];
		ReadFile(file, (LPVOID) filebuf, filesize, NULL, NULL);
		filebuf[filesize] = 0;
		string content = (char *) filebuf;
		delete[] filebuf;
		string line;
		gearInfo tGear;
		while ((linePos = content.find("\r\n", oldLinePos)) != string::npos) {
			vector<string> fields;
			size_t pos = 0, posOld = 1;
			line = content.substr(oldLinePos, linePos - oldLinePos);
			oldLinePos = linePos + 2;
			if (!line.empty()) {
				while ((pos = line.find("','", posOld)) != string::npos) {
					fields.push_back(line.substr(posOld, pos - posOld));
					posOld = pos + 3;
				}
				fields.push_back(line.substr(posOld, line.length() - posOld - 1));
				if (tGear.found || fields.front()[0] == '3') {
					switch (fields.front()[0]) {
					case '0': { // device line
						WORD vid = (WORD)atoi(fields[1].c_str()),
							pid = (WORD)atoi(fields[2].c_str());
						DebugPrint("Device " + tGear.name + " - " + to_string(vid) + "/" + to_string(pid) + ": ");
						if (tGear.found = conf->afx_dev.GetDeviceById(pid, vid)) {
							tGear.devs.push_back({ vid, pid, NULL, fields[3] });
							DebugPrint("Matched.\n")
						}
#ifdef _DEBUG
						else {
							DebugPrint("Skipped.\n");
						}
#endif // _DEBUG
					} break;
					case '1': { // lights line
						byte ltId = (byte)atoi(fields[1].c_str());
						DWORD gridval = MAKELPARAM(tGear.devs.back().pid, ltId);
						DWORD data = atoi(fields[2].c_str());
						tGear.devs.back().lights.push_back({ ltId, { LOWORD(data), HIWORD(data) }, fields[3]});
						// grid maps
						for (int i = 4; i < fields.size(); i += 2) {
							int gid = atoi(fields[i].c_str());
							for (auto pos = tGear.grids.begin(); pos != tGear.grids.end(); pos++)
								if (pos->id == gid) {
									pos->grid[atoi(fields[i + 1].c_str())].lgh = gridval;
									break;
								}
						}
					} break;
					case '2': // Grid info
						tGear.grids.push_back({ (byte)atoi(fields[1].c_str()), (byte)atoi(fields[2].c_str()), (byte)atoi(fields[3].c_str()), fields[4] });
						tGear.grids.back().grid = new AlienFX_SDK::Afx_groupLight[tGear.grids.back().x * tGear.grids.back().y]{ 0 };
						break;
					case '3': // Device info
						if (tGear.name.length() && tGear.found) {
							csv_devs.push_back(tGear);
						}
						tGear = { fields[1] };
						DebugPrint("Gear " + tGear.name + ":\n");
						break;
					//default: // wrong line, skip
					//	DebugPrint("Line skipped\n");
					}
				}
			}
		}
		if (tGear.found) {
			csv_devs.push_back(tGear);
		}
		CloseHandle(file);
	}
}

string OpenSaveFile(bool isOpen) {
	// Load/save device and light mappings
	OPENFILENAMEA fstruct{sizeof(OPENFILENAMEA), dDlg, hInst, "Mapping files (*.csv)\0*.csv\0\0" };
	char appName[4096]{ "Current.csv" };
	fstruct.lpstrFile = (LPSTR) appName;
	fstruct.nMaxFile = 4095;
	fstruct.lpstrInitialDir = ".\\Mappings";
	fstruct.Flags = OFN_ENABLESIZING | OFN_LONGNAMES | OFN_DONTADDTORECENT |
		(isOpen ? OFN_FILEMUSTEXIST : OFN_PATHMUSTEXIST);
	return (isOpen ? GetOpenFileName(&fstruct) : GetSaveFileName(&fstruct)) ? appName : "";
}

void ApplyDeviceMaps(HWND gridTab, bool force = false) {
	// save mappings of selected.
	int oldGridID = conf->mainGrid->id;
	for (auto i = csv_devs.begin(); i < csv_devs.end(); i++) {
		if (force || i->selected) {
			for (auto td = i->devs.begin(); td < i->devs.end(); td++) {
				AlienFX_SDK::Afx_device* cDev = conf->afx_dev.AddDeviceById(td->pid, td->vid);
				cDev->name = td->name;
				if (cDev->dev)
					conf->afx_dev.activeLights += (int)td->lights.size() - (int)cDev->lights.size();
				cDev->lights = td->lights;
			}
			for (auto gg = i->grids.begin(); gg < i->grids.end(); gg++) {
				for (auto pos = conf->afx_dev.GetGrids()->begin(); pos != conf->afx_dev.GetGrids()->end(); pos++)
					if (pos->id == gg->id) {
						conf->afx_dev.GetGrids()->erase(pos);
						break;
					}
				conf->afx_dev.GetGrids()->push_back(*gg);
			}
		}
	}
	conf->mainGrid = conf->afx_dev.GetGridByID(oldGridID);
	CreateGridBlock(gridTab, (DLGPROC)TabGrid, true);
	OnGridSelChanged(gridTab);
	RedrawDevList();
	csv_devs.clear();
}

void SetNewGridName(HWND hDlg) {
	// set text and close
	conf->mainGrid->name.resize(GetWindowTextLength(hDlg) + 1);
	Edit_GetText(hDlg, (LPSTR)conf->mainGrid->name.c_str(), 255);
	ShowWindow(hDlg, SW_HIDE);
	// change tab text
	TCITEM tie{ TCIF_TEXT };
	tie.pszText = (LPSTR)conf->mainGrid->name.c_str();
	TabCtrl_SetItem(GetDlgItem(GetParent(hDlg), IDC_TAB_DEV_GRID), conf->gridTabSel, &tie);
	RedrawWindow(GetDlgItem(GetParent(hDlg), IDC_TAB_DEV_GRID), NULL, NULL, RDW_INVALIDATE);
}

LRESULT GridNameEdit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_KEYUP: {
		switch (wParam) {
		case VK_RETURN: case VK_TAB: {
			ShowWindow(hDlg, SW_HIDE);
		} break;
		case VK_ESCAPE: {
			// restore text
			SetWindowText(hDlg, conf->mainGrid->name.c_str());
			ShowWindow(hDlg, SW_HIDE);
		} break;
		}
	} break;
	case WM_KILLFOCUS: {
		SetNewGridName(hDlg);
	} break;
	}
	return CallWindowProc(oldproc, hDlg, message, wParam, lParam);
}

BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND gridTab = GetDlgItem(hDlg, IDC_TAB_DEV_GRID),
		 llist = GetDlgItem(hDlg, IDC_LIGHTS_LIST);

	static bool clickOnTab = false;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		dDlg = hDlg;
		eve->StopProfiles();
		eve->ChangeEffects(true);
		fxhl->Stop();

		if (!activeDevice)
			activeDevice = &conf->afx_dev.fxdevs.front();

		CreateGridBlock(gridTab, (DLGPROC)TabGrid, true);
		fxhl->TestLight(activeDevice, -1);
		RedrawDevList();

		oldproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_EDIT_GRID), GWLP_WNDPROC, (LONG_PTR)GridNameEdit);

	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_BUT_NEXT:
			eLid++;
			UpdateLightsList();
			break;
		case IDC_BUT_PREV:
			if (eLid) {
				eLid--;
				UpdateLightsList();
			}
			break;
		case IDC_BUT_LAST:
			eLid = 0;
			for (auto it = activeDevice->lights.begin(); it < activeDevice->lights.end(); it++)
				eLid = max(eLid, it->lightid);
			UpdateLightsList();
			break;
		case IDC_BUT_FIRST:
			eLid = 999;
			for (auto it = activeDevice->lights.begin(); it < activeDevice->lights.end(); it++)
				eLid = min(eLid, it->lightid);
			UpdateLightsList();
			break;
		case IDC_BUT_KEY:
			if (keySetLight)
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_KEY), hDlg, (DLGPROC)KeyPressDialog);
			else {
				FindCreateMapping();
				UpdateDeviceInfo();
			}
			break;
		case IDC_LIGHTS_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				eLid = (int)ListBox_GetItemData(llist, ListBox_GetCurSel(llist));
				SetLightInfo();
				break;
			}
			break;
		case IDC_EDIT_NAME:
			switch (HIWORD(wParam)) {
			case EN_CHANGE:
				if (Edit_GetModify(GetDlgItem(hDlg, IDC_EDIT_NAME))) {
					if (!(keySetLight = conf->afx_dev.GetMappingByDev(activeDevice, eLid))) {
						activeDevice->lights.push_back({ (byte)eLid });
						keySetLight = &activeDevice->lights.back();
					}
					keySetLight->name.resize(128);
					keySetLight->name.resize(GetDlgItemText(hDlg, IDC_EDIT_NAME, (LPSTR)keySetLight->name.data(), 127));
					keySetLight->name.shrink_to_fit();
				}
				break;
			case EN_KILLFOCUS:
				UpdateDeviceInfo();
				break;
			}
			break;
		case IDC_BUT_DEVCLEAR:
			if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you want to clear all device information?", "Warning",
				MB_YESNO | MB_ICONWARNING) == IDYES) {
				// remove all lights
				for (auto lgh = activeDevice->lights.begin(); lgh != activeDevice->lights.end(); lgh++) {
					RemoveLightAndClean(activeDevice->pid, lgh->lightid);
				}
				// remove device if not active
				if (!activeDevice->dev) {
					for (auto i = conf->afx_dev.fxdevs.begin(); i != conf->afx_dev.fxdevs.end(); i++)
						if (i->pid == activeDevice->pid) {
							conf->afx_dev.fxdevs.erase(i);
							break;
						}
					if (conf->afx_dev.fxdevs.empty()) {
						// switch tab
						tabSel = TAB_SETTINGS;
						SetMainTabs();
						break;
					}
					else {
						activeDevice = &conf->afx_dev.fxdevs.front();
						RedrawDevList();
					}
				}
				else {
					// decrease active lights
					conf->afx_dev.activeLights -= (int)activeDevice->lights.size();
					activeDevice->lights.clear();
					UpdateDeviceInfo();
				}
			}
			break;
		case IDC_BUT_CLEAR:
			if (keySetLight) {
				if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you want to remove light?", "Warning",
					MB_YESNO | MB_ICONWARNING) == IDYES) {
					if (keySetLight->flags & ALIENFX_FLAG_POWER) {
						fxhl->ResetPower(activeDevice);
						ShowNotification(&conf->niData, "Warning", "Hardware Power button removed, you may need to reset light system!");
					}
					// delete from all groups and grids...
					RemoveLightAndClean(activeDevice->pid, eLid);
					// delete from mappings...
					conf->afx_dev.RemoveMapping(activeDevice, eLid);
					if (activeDevice->dev)
						conf->afx_dev.activeLights--;
					UpdateDeviceInfo();
				}
			}
			break;
		case IDC_BUTTON_TESTCOLOR: {
			SetColor(GetDlgItem(hDlg, IDC_BUTTON_TESTCOLOR), &conf->testColor);
			fxhl->TestLight(activeDevice, -1);
			fxhl->TestLight(activeDevice, eLid);
			RedrawGridButtonZone();
		} break;
		case IDC_ISPOWERBUTTON: {
			if (keySetLight) {
				SetBitMask(keySetLight->flags, ALIENFX_FLAG_POWER, IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				if (!(keySetLight->flags & ALIENFX_FLAG_POWER)) {
					// remove power button config from chip config
					ShowNotification(&conf->niData, "Warning", "You may need to reset light system hardware!");
					fxhl->ResetPower(activeDevice);
				}
				//RemoveLightAndClean(activeDevice->pid, eLid);
			}
		} break;
		case IDC_CHECK_INDICATOR:
		{
			if (keySetLight) {
				SetBitMask(keySetLight->flags, ALIENFX_FLAG_INDICATOR, IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			}
		} break;
		case IDC_BUT_DETECT:
		{
			// Try to detect lights from DB
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_AUTODETECT), hDlg, (DLGPROC)DetectionDialog) == IDOK) {
				ApplyDeviceMaps(gridTab);
			}
		} break;
		case IDC_BUT_LOADMAP:
		{
			// Load device and light mappings
			string appName;
			if ((appName = OpenSaveFile(true)).length()) {
				// Now load mappings...
				LoadCSV(appName);
				ApplyDeviceMaps(gridTab, true);
			}
		} break;
		case IDC_BUT_SAVEMAP:
		{
			// Save device and light mappings
			if (conf->afx_dev.fxdevs.size()) {
				string appName;
				if ((appName = OpenSaveFile(false)).length()) {
					// Now save mappings...
					HANDLE file = CreateFile(appName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
					if (file != INVALID_HANDLE_VALUE) {
						DWORD writeBytes;
						char name[] = "'3','Current gear'\r\n";
						WriteFile(file, name, (DWORD)strlen(name), &writeBytes, NULL);
						for (auto i = conf->afx_dev.GetGrids()->begin(); i != conf->afx_dev.GetGrids()->end(); i++) {
							string line = "'2','" + to_string(i->id) + "','" + to_string(i->x) + "','" + to_string(i->y)
								+ "','" + i->name + "'\r\n";
							WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
						}
						for (auto i = conf->afx_dev.fxdevs.begin(); i != conf->afx_dev.fxdevs.end(); i++) {
							string line = "'0','" + to_string(i->vid) + "','" + to_string(i->pid) + "','" + i->name + "'\r\n";
							WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
							for (auto j = i->lights.begin(); j != i->lights.end(); j++) {
								line = "'1','" + to_string(j->lightid) + "','" + to_string(j->data) + "','" + j->name;
								AlienFX_SDK::Afx_groupLight gridid = { i->pid, j->lightid };
								for (auto k = conf->afx_dev.GetGrids()->begin(); k != conf->afx_dev.GetGrids()->end(); k++)
									for (int pos = 0; pos < k->x * k->y; pos++)
										if (gridid.lgh == k->grid[pos].lgh)
											line += "','" + to_string(k->id) + "','" + to_string(pos);
								line += "'\r\n";
								WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
							}
						}
						CloseHandle(file);
					}
				}
			}
		} break;
		case IDC_BUT_WHITE:
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_WHITE), hDlg, (DLGPROC)WhiteBalanceDialog);
		} break;
		}
	} break;
	case WM_NOTIFY: {
		HWND gridTab = GetDlgItem(hDlg, IDC_TAB_DEV_GRID);
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_DEV_GRID: {
			switch (((NMHDR*)lParam)->code) {
			case NM_CLICK: {
				if (clickOnTab) {
					RECT tRect;
					TabCtrl_GetItemRect(gridTab, conf->gridTabSel, &tRect);
					MapWindowPoints(gridTab, hDlg, (LPPOINT)&tRect, 2);
					SetDlgItemText(hDlg, IDC_EDIT_GRID, conf->mainGrid->name.c_str());
					ShowWindow(GetDlgItem(hDlg, IDC_EDIT_GRID), SW_RESTORE);
					SetWindowPos(GetDlgItem(hDlg, IDC_EDIT_GRID), NULL, tRect.left - 1, tRect.top - 1,
						tRect.right - tRect.left > 60 ? tRect.right - tRect.left : 60, tRect.bottom - tRect.top + 2, SWP_SHOWWINDOW);
					SetFocus(GetDlgItem(hDlg, IDC_EDIT_GRID));
				} else
					clickOnTab = true;
			} break;
			case TCN_SELCHANGE: {
				clickOnTab = false;
				int newSel = TabCtrl_GetCurSel(gridTab);
				if (newSel < conf->afx_dev.GetGrids()->size())
					OnGridSelChanged(gridTab);
				else
					if (newSel == conf->afx_dev.GetGrids()->size()) {
						// add grid
						byte newGridIndex = 0;
						for (auto it = conf->afx_dev.GetGrids()->begin(); it < conf->afx_dev.GetGrids()->end(); ) {
							if (newGridIndex == it->id) {
								newGridIndex++;
								it = conf->afx_dev.GetGrids()->begin();
							}
							else
								it++;
						}
						AlienFX_SDK::Afx_groupLight* newGrid = new AlienFX_SDK::Afx_groupLight[conf->mainGrid->x * conf->mainGrid->y]{ 0 };
						conf->afx_dev.GetGrids()->push_back({ newGridIndex, conf->mainGrid->x, conf->mainGrid->y, "Grid #" + to_string(newGridIndex), newGrid });
						conf->mainGrid = &conf->afx_dev.GetGrids()->back();
						TCITEM tie{ TCIF_TEXT };
						tie.pszText = (LPSTR)conf->mainGrid->name.c_str();
						TabCtrl_InsertItem(gridTab, conf->afx_dev.GetGrids()->size() - 1, (LPARAM)&tie);
						TabCtrl_SetCurSel(gridTab, conf->afx_dev.GetGrids()->size() - 1);
						OnGridSelChanged(gridTab);
						RedrawWindow(gridTab, NULL, NULL, RDW_INVALIDATE);
					}
					else
					{
						if (conf->afx_dev.GetGrids()->size() > 1) {
							int newTab = conf->gridTabSel;
							for (auto it = conf->afx_dev.GetGrids()->begin(); it < conf->afx_dev.GetGrids()->end(); it++) {
								if (it->id == conf->mainGrid->id) {
									// remove
									if ((it + 1) == conf->afx_dev.GetGrids()->end())
										newTab--;
									delete[] it->grid;
									conf->afx_dev.GetGrids()->erase(it);
									TabCtrl_DeleteItem(gridTab, conf->gridTabSel);
									TabCtrl_SetCurSel(gridTab, newTab);
									OnGridSelChanged(gridTab);
									RedrawWindow(gridTab, NULL, NULL, RDW_INVALIDATE);
									break;
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
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, sItem->iItem);
			} break;
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (NMLISTVIEW*)lParam;
				if (lPoint->uNewState & LVIS_SELECTED && lPoint->iItem != -1) {
					activeDevice = &conf->afx_dev.fxdevs[lPoint->iItem];
					fxhl->TestLight(activeDevice, eLid, true);
					UpdateDeviceInfo();
				}
				else {
					if (ListView_GetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, LVIS_FOCUSED)) {
						ListView_SetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, LVIS_SELECTED, LVIS_SELECTED);
					}
					else {
						fxhl->TestLight(activeDevice, -1);
						ListView_SetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, 0, LVIS_SELECTED);
					}
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				if (sItem->item.pszText) {
					activeDevice->name = sItem->item.pszText;
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
	} break;
	case WM_DRAWITEM:
		RedrawButton(((DRAWITEMSTRUCT*)lParam)->hwndItem, &conf->testColor);
		break;
	case WM_DESTROY:
	{
		fxhl->Start();
		eve->ChangeEffectMode();
		eve->StartProfiles();
		dDlg = NULL;
	} break;
	default: return false;
	}
	return true;
}

void UpdateSavedDevices(HWND hDlg) {
	HWND list = GetDlgItem(hDlg, IDC_LIST_FOUND);
	// Now check current device list..
	ListView_DeleteAllItems(list);
	ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
	if (!ListView_GetColumnWidth(list, 0)) {
		LVCOLUMNA lCol{ LVCF_FMT, LVCFMT_LEFT };
		ListView_InsertColumn(list, 0, &lCol);
	}
	for (int i = 0; i < csv_devs.size(); i++) {
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
		lItem.lParam = i;
		lItem.pszText = (LPSTR)csv_devs[i].name.c_str();
		if (csv_devs[i].selected)
			lItem.state = 0x2000;
		else
			lItem.state = 0x1000;
		ListView_InsertItem(list, &lItem);
	}

	ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
}

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
	{
		// add fake device for testing...
		//conf->afx_dev.GetDevices()->push_back({ 0x0461 , 20160, "Unknown mouse" });

		LoadCSV(".\\Mappings\\devices.csv");
		UpdateSavedDevices(hDlg);
	} break;
	case WM_COMMAND:
	{

		switch (LOWORD(wParam)) {
		case IDOK: case IDCANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
		} break;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_FOUND:
			switch (((NMHDR*)lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW)lParam;
				if (lPoint->uNewState & 0x2000) {
					csv_devs[lPoint->lParam].selected = true;
				} else
					if (lPoint->uNewState & 0x1000 && lPoint->uOldState & 0x2000) {
						csv_devs[lPoint->lParam].selected = false;
				}
			} break;
			} break;
		} break;
	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		break;
    default: return false;
	}
	return true;
}