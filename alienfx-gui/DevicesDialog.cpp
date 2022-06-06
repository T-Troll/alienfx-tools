#include "alienfx-gui.h"
#include "alienfx-controls.h"

extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern void RemoveLightFromGroup(AlienFX_SDK::group* grp, WORD devid, WORD lightid);
extern void RemoveLightAndClean(int dPid, int eLid);

extern BOOL CALLBACK TabGrid(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void CreateGridBlock(HWND gridTab, DLGPROC, bool);
extern void OnGridSelChanged(HWND);
extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);
extern AlienFX_SDK::mapping* FindCreateMapping();

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct gearInfo {
	string name;
	vector <AlienFX_SDK::lightgrid> grids;
	vector<AlienFX_SDK::afx_device> devs;
	bool selected = false;
};

extern HWND dDlg;

int eLid = 0, dItem = -1, dIndex = -1;// , lMaxIndex = 0;
vector<gearInfo> csv_devs;
WNDPROC oldproc;

BOOL CALLBACK WhiteBalanceDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_INITDIALOG:
	{
		AlienFX_SDK::afx_device* dev = &conf->afx_dev.fxdevs[dIndex];
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
		fxhl->TestLight(dIndex, eLid, true);
		RECT pRect;
		GetWindowRect(GetDlgItem(dDlg, IDC_BUT_WHITE), &pRect);
		SetWindowPos(hDlg, NULL, pRect.right, pRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	break;
	case WM_HSCROLL:
	{
		if (dIndex >= 0) {
			switch (LOWORD(wParam)) {
			case TB_THUMBTRACK: case TB_ENDTRACK:
			{
				if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_RED)) {
					conf->afx_dev.fxdevs[dIndex].white.r = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, conf->afx_dev.fxdevs[dIndex].white.r);
				}
				if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_GREEN)) {
					conf->afx_dev.fxdevs[dIndex].white.g = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, conf->afx_dev.fxdevs[dIndex].white.g);
				}
				if ((HWND)lParam == GetDlgItem(hDlg, IDC_SLIDER_BLUE)) {
					conf->afx_dev.fxdevs[dIndex].white.b = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip3, conf->afx_dev.fxdevs[dIndex].white.b);
				}
				fxhl->TestLight(dIndex, eLid, true);
			} break;
			}
		}
	} break;
	case WM_CLOSE:
		fxhl->TestLight(dIndex, eLid);
		EndDialog(hDlg, IDCLOSE);
		break;
	default: return false;
	}
	return true;
}

void SetLightInfo() {
	AlienFX_SDK::mapping* clight = NULL;
	if (dIndex >= 0 && (clight = conf->afx_dev.GetMappingById(&conf->afx_dev.fxdevs[dIndex], eLid))) {
		SetDlgItemText(dDlg, IDC_EDIT_NAME, clight->name.c_str());
		fxhl->TestLight(dIndex, eLid);
	}
	else {
		SetDlgItemText(dDlg, IDC_EDIT_NAME, "<not used>");
	}
	if (eLid >= 0)
		SetDlgItemInt(dDlg, IDC_LIGHTID, eLid, false);
	else
		SetDlgItemText(dDlg, IDC_LIGHTID, "");
	CheckDlgButton(dDlg, IDC_ISPOWERBUTTON, clight && clight->flags & ALIENFX_FLAG_POWER);
	CheckDlgButton(dDlg, IDC_CHECK_INDICATOR, clight && clight->flags & ALIENFX_FLAG_INDICATOR);
	RedrawGridButtonZone();
}

void UpdateDeviceInfo() {
	if (dIndex >= 0 && dIndex < conf->afx_dev.fxdevs.size()) {
		AlienFX_SDK::afx_device* dev = &conf->afx_dev.fxdevs[dIndex];
		BYTE status = dev->dev->AlienfxGetDeviceStatus();
		char descript[128];
		sprintf_s(descript, 128, "VID_%04X/PID_%04X, APIv%d, %s",
			dev->vid, dev->pid, dev->dev->GetVersion(),
			status && status != 0xff ? "Ok" : "Error!");
		SetWindowText(GetDlgItem(dDlg, IDC_INFO_VID), descript);
		EnableWindow(GetDlgItem(dDlg, IDC_ISPOWERBUTTON), dev->dev->GetVersion() < 5); // v5 and higher doesn't support power button
	}
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
		if (conf->afx_dev.fxdevs[i].dev) {
			LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
			if (!conf->afx_dev.fxdevs[i].name.length()) {
				// no name
				string typeName = "Unknown";
				switch (conf->afx_dev.fxdevs[i].dev->GetVersion()) {
				case 0: typeName = "Desktop"; break;
				case 1: case 2: case 3: typeName = "Notebook"; break;
				case 4: typeName = "Notebook/Tron"; break;
				case 5: typeName = "Keyboard"; break;
				case 6: typeName = "Display"; break;
				case 7: typeName = "Mouse"; break;
				}
				conf->afx_dev.fxdevs[i].name = typeName + ", #" + to_string(conf->afx_dev.fxdevs[i].pid);
			}
			lItem.lParam = i;
			lItem.pszText = (char*)conf->afx_dev.fxdevs[i].name.c_str();
			if (lItem.lParam == dIndex) {
				lItem.state = LVIS_SELECTED;
				rpos = i;
			}
			else
				lItem.state = 0;
			ListView_InsertItem(dev_list, &lItem);
		}
	}
	ListView_SetColumnWidth(dev_list, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_EnsureVisible(dev_list, rpos, false);
	UpdateDeviceInfo();
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
						AlienFX_SDK::afx_device* dev = conf->afx_dev.GetDeviceById(MAKELPARAM(pid, vid));
						if (dev && dev->dev) {
							tGear.devs.push_back({vid, pid, NULL, fields[3] });
						}
						else {
							// Skip to next "3" block!
							tGear.selected = true;
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
							auto pos = find_if(tGear.grids.begin(), tGear.grids.end(),
								[gid](auto t) {
									return t.id == gid;
								});
							if (pos != tGear.grids.end())
								pos->grid[atoi(fields[i + 1].c_str())] = gridval;
						}
					}
					break;
				case 2: // Grid info
					tGear.grids.push_back({ (byte)atoi(fields[1].c_str()), (byte)atoi(fields[2].c_str()), (byte)atoi(fields[3].c_str()), fields[4] });
					tGear.grids.back().grid = new DWORD[tGear.grids.back().x * tGear.grids.back().y]{ 0 };
					break;
				case 3: // Device info
					if (tGear.name != "" && !tGear.selected) {
						tGear.name = tGear.devs.front().name;
						csv_devs.push_back(tGear);
					}
					tGear = { "" };
					tGear.name = fields[1];
					break;
					//default: // wrong line, skip
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

void ApplyDeviceMaps(bool force = false) {
	// save mappings of selected.
	int oldGridID = conf->mainGrid->id;
	for (auto i = csv_devs.begin(); i < csv_devs.end(); i++) {
		if (force || i->selected) {
			for (auto td = i->devs.begin(); td < i->devs.end(); td++) {
				AlienFX_SDK::afx_device* cDev = conf->afx_dev.AddDeviceById(MAKELPARAM(td->pid, td->vid));
				cDev->name = td->name;
				for (auto j = td->lights.begin(); j < td->lights.end(); j++) {
					AlienFX_SDK::mapping* oMap = conf->afx_dev.GetMappingById(cDev, j->lightid);
					if (oMap) {
						oMap->flags = j->flags;
						oMap->name = j->name;
					}
					else {
						cDev->lights.push_back(*j);
					}
				}
			}
			for (auto gg = i->grids.begin(); gg < i->grids.end(); gg++) {
				auto pos = find_if(conf->afx_dev.GetGrids()->begin(), conf->afx_dev.GetGrids()->end(),
					[gg](auto tg) {
						return tg.id == gg->id;
					});
				if (pos != conf->afx_dev.GetGrids()->end())
					conf->afx_dev.GetGrids()->erase(pos);
				conf->afx_dev.GetGrids()->push_back(*gg);
			}
		}
	}
	auto pos = find_if(conf->afx_dev.GetGrids()->begin(), conf->afx_dev.GetGrids()->end(),
		[oldGridID](auto tg) {
			return tg.id == oldGridID;
		});
	if (pos != conf->afx_dev.GetGrids()->end()) {
		conf->mainGrid = &(*pos);
	}
	//conf->afx_dev.AlienFXAssignDevices();
	csv_devs.clear();
	RedrawDevList();
	OnGridSelChanged(GetDlgItem(dDlg, IDC_TAB_DEV_GRID));
	SetLightInfo();
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

	switch (message)
	{
	case WM_INITDIALOG:
	{
		dDlg = hDlg;
		// First, reset all light devices and re-scan!
		fxhl->UnblockUpdates(false, true);
		// Do we have some lights?
		//conf->afx_dev.AlienFXAssignDevices();
		CreateGridBlock(gridTab, (DLGPROC)TabGrid, true);
		TabCtrl_SetCurSel(gridTab, conf->gridTabSel);
		if (conf->afx_dev.fxdevs.size() > 0) {
			if (dIndex < 0) dIndex = 0;
			RedrawDevList();
		}

		if (!conf->afx_dev.haveLights &&
			MessageBox(hDlg, "Lights and grids not defined. Do you want to detect it?", "Warning", MB_ICONQUESTION | MB_YESNO)
			== IDYES) {
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_AUTODETECT), hDlg, (DLGPROC)DetectionDialog);
		}

		oldproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_EDIT_GRID), GWLP_WNDPROC, (LONG_PTR)GridNameEdit);

		RegisterHotKey(hDlg, 1, MOD_SHIFT, VK_LEFT);
		RegisterHotKey(hDlg, 2, MOD_SHIFT, VK_RIGHT);
		RegisterHotKey(hDlg, 3, MOD_SHIFT, VK_HOME);
		RegisterHotKey(hDlg, 4, MOD_SHIFT, VK_END);

	} break;
	case WM_COMMAND: {
		//WORD dPid = dIndex < 0 ? 0 : conf->afx_dev.fxdevs[dIndex].desc->devid;
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
			eLid = 0;
			for (auto it = conf->afx_dev.fxdevs[dIndex].lights.begin(); it < conf->afx_dev.fxdevs[dIndex].lights.end(); it++)
				eLid = max(eLid, it->lightid);
			SetLightInfo();
		break;
		case IDC_BUT_FIRST:
			eLid = 999;
			for (auto it = conf->afx_dev.fxdevs[dIndex].lights.begin(); it < conf->afx_dev.fxdevs[dIndex].lights.end(); it++)
				eLid = min(eLid, it->lightid);
			SetLightInfo();
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
		case IDC_BUT_CLEAR:// IDC_BUTTON_REML:
			if (eLid >= 0 && MessageBox(hDlg, "Do you really want to remove current light name and all it's settings?", "Warning",
						   MB_YESNO | MB_ICONWARNING) == IDYES) {
				// Clear grid
				DWORD gridID = MAKELPARAM(conf->afx_dev.fxdevs[dIndex].pid, eLid);
				for (auto it = conf->afx_dev.GetGrids()->begin(); it < conf->afx_dev.GetGrids()->end(); it++)
					for (int ind = 0; ind < it->x*it->y; ind++)
						if (it->grid[ind] == gridID)
							it->grid[ind] = 0;
				// delete from all groups...
				RemoveLightAndClean(conf->afx_dev.fxdevs[dIndex].pid, eLid);
				// delete from current dev block...
				conf->afx_dev.fxdevs[dIndex].lights.erase(find_if(conf->afx_dev.fxdevs[dIndex].lights.begin(), conf->afx_dev.fxdevs[dIndex].lights.end(),
					[](auto t) {
						return t.lightid == eLid;
					}));
				// delete from mappings...
				conf->afx_dev.RemoveMapping(&conf->afx_dev.fxdevs[dIndex], eLid);
				conf->afx_dev.SaveMappings();
				if (IsDlgButtonChecked(hDlg, IDC_ISPOWERBUTTON) == BST_CHECKED) {
					fxhl->ResetPower(&conf->afx_dev.fxdevs[dIndex]);
					MessageBox(hDlg, "Hardware Power button removed, you may need to reset light system!", "Warning",
							   MB_OK);
				}
				SetLightInfo();
			}
			break;
		case IDC_BUTTON_TESTCOLOR: {
			SetColor(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor);
			if (eLid >= 0 && dIndex >= 0 && dIndex < conf->afx_dev.fxdevs.size()) {
				fxhl->TestLight(dIndex, eLid);
			}
			RedrawGridButtonZone();
		} break;
		case IDC_ISPOWERBUTTON:
			if (eLid >= 0) {
				AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingById(&conf->afx_dev.fxdevs[dIndex], eLid);
				if (lgh && IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED)
					if (MessageBox(hDlg, "Setting light to Hardware Power will reset all it settings! Are you sure?", "Warning",
								   MB_YESNO | MB_ICONWARNING) == IDYES) {
						lgh->flags |= ALIENFX_FLAG_POWER;
						// Check mappings and remove all power button data
						RemoveLightAndClean(conf->afx_dev.fxdevs[dIndex].pid, eLid);
					} else
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				else {
					// remove power button config from chip config if unchecked and confirmed
					if (lgh) {
						if (MessageBox(hDlg, "You may need to reset light system!\nDo you want to set light as common?", "Warning",
							MB_YESNO | MB_ICONWARNING) == IDYES)
							fxhl->ResetPower(&conf->afx_dev.fxdevs[dIndex]);
						RemoveLightAndClean(conf->afx_dev.fxdevs[dIndex].pid, eLid);
						lgh->flags &= ~ALIENFX_FLAG_POWER;
					}
				}
			}
			break;
		case IDC_CHECK_INDICATOR:
		{
			AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingById(&conf->afx_dev.fxdevs[dIndex], eLid);
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
				ApplyDeviceMaps();
			}
		} break;
		case IDC_BUT_LOADMAP:
		{
			// Load device and light mappings
			string appName;
			if ((appName = OpenSaveFile(true)).length()) {
				// Now load mappings...
				LoadCSV(appName);
				ApplyDeviceMaps(true);
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
					conf->afx_dev.AlienFXAssignDevices();
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
								DWORD gridid = MAKELPARAM(i->pid, j->lightid);
								for (auto i = conf->afx_dev.GetGrids()->begin(); i < conf->afx_dev.GetGrids()->end(); i++) {
									for (int pos = 0; pos < i->x * i->y; pos++) {
										if (gridid == i->grid[pos]) {
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
		default: return false;
		}
	} break;
	case WM_HOTKEY: {
		DWORD message = 0;
		switch (wParam) {
		case 1: message = IDC_BUT_PREV; break;
		case 2: message = IDC_BUT_NEXT; break;
		case 3: message = IDC_BUT_FIRST; break;
		case 4: message = IDC_BUT_LAST; break;
		}
		SendMessage(hDlg, WM_COMMAND, message, 0);
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
						DWORD* newGrid = new DWORD[conf->mainGrid->x * conf->mainGrid->y]{ 0 };
						conf->afx_dev.GetGrids()->push_back({ newGridIndex, conf->mainGrid->x, conf->mainGrid->y, "Grid #" + to_string(newGridIndex), newGrid });
						conf->mainGrid = &conf->afx_dev.GetGrids()->back();
						//RedrawGridList(hDlg);
						TCITEM tie{ TCIF_TEXT };
						tie.pszText = (LPSTR)conf->mainGrid->name.c_str();
						TabCtrl_InsertItem(gridTab, conf->afx_dev.GetGrids()->size() - 1, (LPARAM)&tie);
						TabCtrl_SetCurSel(gridTab, conf->afx_dev.GetGrids()->size() - 1);
						OnGridSelChanged(gridTab);
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
				if (lPoint->uNewState && LVIS_FOCUSED && lPoint->iItem != -1) {
					// Select other item...
					int oldIndex = dIndex;
					dIndex = (int)lPoint->lParam;
					UpdateDeviceInfo();
					//eLid = 0;
					SetLightInfo();
				}
				else {
					if (!ListView_GetSelectedCount(((NMHDR*)lParam)->hwndFrom))
						RedrawDevList();
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				if (sItem->item.pszText) {
					conf->afx_dev.fxdevs[dIndex].name = sItem->item.pszText;
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
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_TESTCOLOR:
		{
			RedrawButton(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor);
		} break;
		}
		break;
	case WM_APP + 3: {
		fxhl->UnblockUpdates(true, true);
		fxhl->Refresh();
		UnregisterHotKey(hDlg, 1);
		UnregisterHotKey(hDlg, 2);
		UnregisterHotKey(hDlg, 3);
		UnregisterHotKey(hDlg, 4);
		conf->SortAllGauge();
	} break;
	case WM_DESTROY:
	{
		SendMessage(hDlg, WM_APP + 3, 0, 0);
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