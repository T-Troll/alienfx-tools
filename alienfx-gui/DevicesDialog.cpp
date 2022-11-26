#include "alienfx-gui.h"
#include "common.h"

extern bool SetColor(HWND hDlg, AlienFX_SDK::Colorcode*);
extern void RedrawButton(HWND hDlg, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern void RemoveLightFromGroup(AlienFX_SDK::group* grp, WORD devid, WORD lightid);
extern void RemoveLightAndClean(int dPid, int eLid);
extern void SetMainTabs();

extern BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void CreateGridBlock(HWND gridTab, DLGPROC, bool);
extern void OnGridSelChanged(HWND);
extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);

extern AlienFX_SDK::mapping* FindCreateMapping();

extern FXHelper* fxhl;
extern HWND mDlg;
extern int tabSel;

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct gearInfo {
	string name;
	vector <AlienFX_SDK::lightgrid> grids;
	vector<AlienFX_SDK::afx_device> devs;
	bool selected = false;
};

extern HWND dDlg;

int eLid = 0, dItem = -1, dIndex = 0;
vector<gearInfo> csv_devs;
WNDPROC oldproc;
HHOOK dEvent;
HWND kDlg = NULL;

AlienFX_SDK::afx_device* FindActiveDevice() {
	if (dIndex >= 0 && dIndex < conf->afx_dev.fxdevs.size())
		//if (conf->afx_dev.fxdevs[dIndex].dev)
			return &conf->afx_dev.fxdevs[dIndex];
	return nullptr;
}

BOOL CALLBACK WhiteBalanceDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	AlienFX_SDK::afx_device* dev = FindActiveDevice();

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (dev) {
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETRANGE, true, MAKELPARAM(0, 255));
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETTICFREQ, 16, 0);
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETRANGE, true, MAKELPARAM(0, 255));
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETTICFREQ, 16, 0);
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETRANGE, true, MAKELPARAM(0, 255));
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETTICFREQ, 16, 0);
			sTip1 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_RED), sTip1);
			sTip2 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_GREEN), sTip2);
			sTip3 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_BLUE), sTip3);
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETPOS, true, dev->white.r);
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETPOS, true, dev->white.g);
			SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETPOS, true, dev->white.b);
			SetSlider(sTip1, dev->white.r);
			SetSlider(sTip2, dev->white.r);
			SetSlider(sTip3, dev->white.r);
			fxhl->TestLight(dev, eLid, true, true);
		}
		RECT pRect;
		GetWindowRect(GetDlgItem(dDlg, IDC_BUT_WHITE), &pRect);
		SetWindowPos(hDlg, NULL, pRect.right, pRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	break;
	case WM_HSCROLL:
	{
		if (dev) {
			switch (LOWORD(wParam)) {
			case TB_THUMBTRACK: case TB_ENDTRACK:
			{
				if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_RED)) {
					conf->afx_dev.fxdevs[dIndex].white.r = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, conf->afx_dev.fxdevs[dIndex].white.r);
				} else
					if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_GREEN)) {
						conf->afx_dev.fxdevs[dIndex].white.g = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
						SetSlider(sTip2, conf->afx_dev.fxdevs[dIndex].white.g);
					} else
						if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_BLUE)) {
							conf->afx_dev.fxdevs[dIndex].white.b = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
							SetSlider(sTip3, conf->afx_dev.fxdevs[dIndex].white.b);
						}
				fxhl->TestLight(dev, eLid, true, true);
			} break;
			}
		}
	} break;
	case WM_CLOSE:
		if (dev)
			fxhl->TestLight(dev, eLid, true);
		EndDialog(hDlg, IDCLOSE);
		break;
	default: return false;
	}
	return true;
}

void SetLightInfo() {
	AlienFX_SDK::afx_device* dev = FindActiveDevice();
	if (dev) {
		fxhl->TestLight(dev, eLid);
		AlienFX_SDK::mapping* clight;
		if (clight = conf->afx_dev.GetMappingByDev(dev, eLid)) {
			SetDlgItemText(dDlg, IDC_EDIT_NAME, clight->name.c_str());
		}
		else {
			SetDlgItemText(dDlg, IDC_EDIT_NAME, "<not used>");
		}
		SetDlgItemInt(dDlg, IDC_LIGHTID, eLid, false);

		CheckDlgButton(dDlg, IDC_ISPOWERBUTTON, clight && clight->flags & ALIENFX_FLAG_POWER);
		CheckDlgButton(dDlg, IDC_CHECK_INDICATOR, clight && clight->flags & ALIENFX_FLAG_INDICATOR);
		RedrawGridButtonZone();
	}
}

void UpdateDeviceInfo() {
	AlienFX_SDK::afx_device* dev = FindActiveDevice();
	if (dev) {
		char descript[128];
		sprintf_s(descript, 128, "VID_%04X/PID_%04X, %d lights, %s",
			dev->vid, dev->pid, (int)dev->lights.size(), (dev->dev ? "APIv" + to_string(dev->dev->GetVersion()) : "Inactive").c_str());
		SetWindowText(GetDlgItem(dDlg, IDC_INFO_VID), descript);
		EnableWindow(GetDlgItem(dDlg, IDC_ISPOWERBUTTON), dev->dev && dev->dev->GetVersion() && dev->dev->GetVersion() < 5); // v5 and higher doesn't support power button
		SetLightInfo();
	}
}

LRESULT CALLBACK DetectKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	if (wParam == WM_KEYUP) {
		char keyname[32];
		GetKeyNameText(MAKELPARAM(0, ((LPKBDLLHOOKSTRUCT)lParam)->scanCode), keyname, 31);
		// set text...
		auto lgh = conf->afx_dev.GetMappingByDev(FindActiveDevice(), eLid);
		if (lgh) lgh->name = keyname;
		SendMessage(kDlg, WM_CLOSE, 0, 0);
	}

	return true;
}

BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_INITDIALOG:
		kDlg = hDlg;
		dEvent = SetWindowsHookExW(WH_KEYBOARD_LL, DetectKeyProc, NULL, 0);
		break;
	case WM_CLOSE:
		UnhookWindowsHookEx(dEvent);
		EndDialog(hDlg, IDCLOSE);
		kDlg = NULL;
		break;
	default: return false;
	}
	return true;
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
	for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++) {
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
		if (!conf->afx_dev.fxdevs[i].name.length() && conf->afx_dev.fxdevs[i].dev) {
			// no name
			string typeName = "Unknown";
			switch (conf->afx_dev.fxdevs[i].dev->GetVersion()) {
			case 0: typeName = "Desktop"; break;
			case 1: case 2: case 3: typeName = "Notebook"; break;
			case 4: typeName = "Notebook/Tron"; break;
			case 5: case 8: typeName = "Keyboard"; break;
			case 6: typeName = "Display"; break;
			case 7: typeName = "Mouse"; break;
			}
			conf->afx_dev.fxdevs[i].name = typeName + ", #" + to_string(conf->afx_dev.fxdevs[i].pid);
		}
		//lItem.lParam = i;
		lItem.pszText = (char*)conf->afx_dev.fxdevs[i].name.c_str();
		if (i == dIndex) {
			lItem.state = LVIS_SELECTED;
			rpos = i;
		}
		else
			lItem.state = 0;
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
		gearInfo tGear{ "" };
		AlienFX_SDK::afx_device tDev;
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
				fields.push_back(line.substr(posOld, line.size() - posOld - 1));
				switch (atoi(fields[0].c_str())) {
				case 0: { // device line
					if (!tGear.selected) {
						WORD vid = (WORD)atoi(fields[1].c_str()),
							pid = (WORD)atoi(fields[2].c_str());
						AlienFX_SDK::afx_device* dev = conf->afx_dev.GetDeviceById(pid, vid);
						DebugPrint("Device " + tGear.name + " - " + to_string(vid) + "/" + to_string(pid) + ": ");
						if (dev && dev->dev) {
							tGear.devs.push_back({vid, pid, NULL, fields[3] });
							DebugPrint("Matched.\n")
						}
						else {
							// Skip to next "3" block!
							tGear.selected = true;
							DebugPrint("Skipped.\n");
						}
					}
				} break;
				case 1: // lights line
					if (!tGear.selected) {
						WORD ltId = (WORD)atoi(fields[1].c_str());
						DWORD gridval = MAKELPARAM(tGear.devs.back().pid, ltId);
						tGear.devs.back().lights.push_back({ ltId, (WORD)atoi(fields[2].c_str()), fields[3] });
						// grid maps
						for (int i = 4; i < fields.size(); i += 2) {
							int gid = atoi(fields[i].c_str());
							for (auto pos = tGear.grids.begin(); pos != tGear.grids.end(); pos++)
								if (pos->id == gid) {
									pos->grid[atoi(fields[i + 1].c_str())].lgh = gridval;
									break;
								}
						}
					}
					break;
				case 2: // Grid info
					tGear.grids.push_back({ (byte)atoi(fields[1].c_str()), (byte)atoi(fields[2].c_str()), (byte)atoi(fields[3].c_str()), fields[4] });
					tGear.grids.back().grid = new AlienFX_SDK::grpLight[tGear.grids.back().x * tGear.grids.back().y]{ 0 };
					break;
				case 3: // Device info
					if (tGear.name != "" && !tGear.selected) {
						tGear.name = tGear.devs.front().name;
						csv_devs.push_back(tGear);
					}
					tGear = { "" };
					tGear.name = fields[1];
					DebugPrint("Device " + tGear.name + ":\n");
					break;
				//default: // wrong line, skip
				//	DebugPrint("Line skipped\n");
				}
			}
		}
		if (!tGear.selected) {
			if (tGear.name == "")
				tGear.name = tGear.devs.front().name;
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
	if (isOpen ? GetOpenFileName(&fstruct) : GetSaveFileName(&fstruct)) {
		return appName;
	} else
		return "";
}

void ApplyDeviceMaps(HWND gridTab, bool force = false) {
	// save mappings of selected.
	int oldGridID = conf->mainGrid->id;
	for (auto i = csv_devs.begin(); i < csv_devs.end(); i++) {
		if (force || i->selected) {
			for (auto td = i->devs.begin(); td < i->devs.end(); td++) {
				AlienFX_SDK::afx_device* cDev = conf->afx_dev.AddDeviceById(td->pid, td->vid);
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
	SetLightInfo();
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
	HWND gridTab = GetDlgItem(hDlg, IDC_TAB_DEV_GRID);

	static bool clickOnTab = false;

	AlienFX_SDK::afx_device* dev = FindActiveDevice();
	AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingByDev(dev, eLid);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		dDlg = hDlg;
		fxhl->Stop();

		//if (!conf->afx_dev.activeLights && DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_AUTODETECT), hDlg, (DLGPROC)DetectionDialog) == IDOK) {
		//	ApplyDeviceMaps();
		//}

		CreateGridBlock(gridTab, (DLGPROC)TabGrid, true);
		fxhl->TestLight(dev, -1);
		RedrawDevList();

		oldproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_EDIT_GRID), GWLP_WNDPROC, (LONG_PTR)GridNameEdit);

	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDC_BUT_NEXT:
			eLid++;
			SetLightInfo();
			break;
		case IDC_BUT_PREV:
			if (eLid) {
				eLid--;
				SetLightInfo();
			}
			break;
		case IDC_BUT_LAST:
			if (dev) {
				eLid = 0;
				for (auto it = dev->lights.begin(); it < dev->lights.end(); it++)
					eLid = max(eLid, it->lightid);
				SetLightInfo();
			}
		break;
		case IDC_BUT_FIRST:
			if (dev) {
				eLid = 999;
				for (auto it = dev->lights.begin(); it < dev->lights.end(); it++)
					eLid = min(eLid, it->lightid);
				SetLightInfo();
			}
			break;
		case IDC_BUT_KEY:
			FindCreateMapping();
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_KEY), hDlg, (DLGPROC)KeyPressDialog);
			UpdateDeviceInfo();
			break;
		case IDC_EDIT_NAME:
			switch (HIWORD(wParam)) {
			case EN_CHANGE:
				if (Edit_GetModify(GetDlgItem(hDlg, IDC_EDIT_NAME))) {
					lgh = FindCreateMapping();
					lgh->name.resize(128);
					GetDlgItemText(hDlg, IDC_EDIT_NAME, (LPSTR)lgh->name.data(), 127);
					lgh->name.shrink_to_fit();
					UpdateDeviceInfo();
				}
				break;
			}
			break;
		case IDC_BUT_DEVCLEAR:
			if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you want to clear all device information?", "Warning",
				MB_YESNO | MB_ICONWARNING) == IDYES) {
				// remove all lights
				for (auto lgh = dev->lights.begin(); lgh != dev->lights.end(); lgh++) {
					RemoveLightAndClean(dev->pid, lgh->lightid);
				}
				// remove device if not active
				if (!dev->dev) {
					conf->afx_dev.fxdevs.erase(conf->afx_dev.fxdevs.begin() + dIndex);
					dIndex = 0;
					if (conf->afx_dev.fxdevs.empty()) {
						// switch tab
						//dIndex = -1;
						//HWND mainTab = GetDlgItem(mDlg, IDC_TAB_MAIN);
						//TabCtrl_SetCurSel(mainTab, TAB_SETTINGS);
						//OnSelChanged(mainTab);
						tabSel = TAB_SETTINGS;
						SetMainTabs();
						break;
					}
					else
						RedrawDevList();
				}
				else {
					// decrease active lights
					conf->afx_dev.activeLights -= (int)dev->lights.size();
					dev->lights.clear();
				}
				UpdateDeviceInfo();
			}
			break;
		case IDC_BUT_CLEAR:
			if (lgh) {
				if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you want to remove light?", "Warning",
					MB_YESNO | MB_ICONWARNING) == IDYES) {
					if (lgh->flags & ALIENFX_FLAG_POWER) {
						fxhl->ResetPower(dev);
						ShowNotification(&conf->niData, "Warning", "Hardware Power button removed, you may need to reset light system!", true);
					}
					// delete from all groups and grids...
					RemoveLightAndClean(dev->pid, eLid);
					// delete from mappings...
					conf->afx_dev.RemoveMapping(dev, eLid);
					if (dev->dev)
						conf->afx_dev.activeLights--;
					UpdateDeviceInfo();
				}
			}
			break;
		case IDC_BUTTON_TESTCOLOR: {
			SetColor(GetDlgItem(hDlg, IDC_BUTTON_TESTCOLOR), &conf->testColor);
			fxhl->TestLight(dev, -1);
			fxhl->TestLight(dev, eLid);
			RedrawGridButtonZone();
		} break;
		case IDC_ISPOWERBUTTON: {
			if (lgh) {
				if (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) {
					ShowNotification(&conf->niData, "Warning", "Setting light to Hardware Power will reset all light settings!", true);
					lgh->flags |= ALIENFX_FLAG_POWER;
				}
				else {
					// remove power button config from chip config if unchecked and confirmed
					ShowNotification(&conf->niData, "Warning", "You may need to reset light system!", true);
					fxhl->ResetPower(dev);
					lgh->flags &= ~ALIENFX_FLAG_POWER;
				}
				RemoveLightAndClean(dev->pid, eLid);
			}
		} break;
		case IDC_CHECK_INDICATOR:
		{
			if (lgh) {
				lgh->flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED ?
					lgh->flags | ALIENFX_FLAG_INDICATOR :
					lgh->flags & ~ALIENFX_FLAG_INDICATOR;
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
			string appName;
			if ((appName = OpenSaveFile(false)).length()) {
				// Now save mappings...
				HANDLE file = CreateFile(appName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				if (file != INVALID_HANDLE_VALUE) {
					//conf->afx_dev.AlienFXAssignDevices();
					if (conf->afx_dev.fxdevs.size()) {
						DWORD writeBytes;
						string line = "'3','" + conf->afx_dev.fxdevs.front().name + "'\r\n";
						WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
						for (auto i = conf->afx_dev.GetGrids()->begin(); i < conf->afx_dev.GetGrids()->end(); i++) {
							line = "'2','" + to_string(i->id) + "','" + to_string(i->x) + "','" + to_string(i->y)
								+ "','" + i->name + "'\r\n";
							WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
						}
						for (auto i = conf->afx_dev.fxdevs.begin(); i < conf->afx_dev.fxdevs.end(); i++) {
							/// Only connected devices stored!
							line = "'0','" + to_string(i->vid) + "','" + to_string(i->pid) + "','" + i->name + "'\r\n";
							WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
							for (auto j = i->lights.begin(); j < i->lights.end(); j++) {
								line = "'1','" + to_string(j->lightid) + "','" + to_string(j->flags) + "','" + j->name;
								AlienFX_SDK::grpLight gridid = { i->pid, j->lightid };
								for (auto i = conf->afx_dev.GetGrids()->begin(); i < conf->afx_dev.GetGrids()->end(); i++) {
									for (int pos = 0; pos < i->x * i->y; pos++) {
										if (gridid.lgh == i->grid[pos].lgh) {
											line += "','" + to_string(i->id) + "','" + to_string(pos);
										}
									}
								}
								line += "'\r\n";
								WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
							}
						}
					}
					CloseHandle(file);
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
						AlienFX_SDK::grpLight* newGrid = new AlienFX_SDK::grpLight[conf->mainGrid->x * conf->mainGrid->y]{ 0 };
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
					dIndex = (int)lPoint->iItem;
					fxhl->TestLight(FindActiveDevice(), eLid, true);
					UpdateDeviceInfo();
				}
				else {
					if (!lPoint->uNewState && ListView_GetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, LVIS_FOCUSED)) {
						ListView_SetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, LVIS_SELECTED, LVIS_SELECTED);
					}
					else {
						fxhl->TestLight(dev, -1);
						ListView_SetItemState(((NMHDR*)lParam)->hwndFrom, lPoint->iItem, 0, LVIS_SELECTED);
					}
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				if (sItem->item.pszText) {
					dev->name = sItem->item.pszText;
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
		//switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		//case IDC_BUTTON_TESTCOLOR:
		//{
			RedrawButton(((DRAWITEMSTRUCT*)lParam)->hwndItem, &conf->testColor);
		//} break;
		//}
		break;
	case WM_DESTROY:
	{
		//conf->SortAllGauge();
		fxhl->Start();
		fxhl->Refresh();
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