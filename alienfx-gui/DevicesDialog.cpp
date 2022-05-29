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
extern void RedrawGridButtonZone(bool recalc=true);
extern AlienFX_SDK::mapping* FindCreateMapping();

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct devInfo {
	AlienFX_SDK::devmap dev;
	vector<AlienFX_SDK::mapping> maps;
	bool selected = false;
};

int eLid = 0, dItem = -1, dIndex = -1;// , lMaxIndex = 0;
bool whiteTest = false;
vector<devInfo> csv_devs;
WNDPROC oldproc;

void SetLightInfo(HWND hDlg) {
	AlienFX_SDK::mapping* clight = NULL;
	if (dIndex >= 0 && (clight = conf->afx_dev.GetMappingById(conf->afx_dev.fxdevs[dIndex].dev->GetPID(), eLid))) {
		SetDlgItemText(hDlg, IDC_EDIT_NAME, clight->name.c_str());
		fxhl->TestLight(dIndex, eLid, whiteTest);
	}
	else {
		SetDlgItemText(hDlg, IDC_EDIT_NAME, "<not used>");
	}
	if (eLid >= 0)
		SetDlgItemInt(hDlg, IDC_LIGHTID, eLid, false);
	else
		SetDlgItemText(hDlg, IDC_LIGHTID, "");
	CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, clight && clight->flags & ALIENFX_FLAG_POWER);
	CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, clight && clight->flags & ALIENFX_FLAG_INDICATOR);
	RedrawGridButtonZone();
}

void UpdateDeviceInfo(HWND hDlg) {
	if (dIndex >= 0 && dIndex < conf->afx_dev.fxdevs.size()) {
		AlienFX_SDK::afx_device* dev = &conf->afx_dev.fxdevs[dIndex];
		BYTE status = dev->dev->AlienfxGetDeviceStatus();
		char descript[128];
		sprintf_s(descript, 128, "VID_%04X/PID_%04X, APIv%d, %s",
			dev->desc->vid, dev->desc->devid, dev->dev->GetVersion(),
			status && status != 0xff ? "Ok" : "Error!");
		SetWindowText(GetDlgItem(hDlg, IDC_INFO_VID), descript);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETPOS, true, dev->desc->white.r);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETPOS, true, dev->desc->white.g);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETPOS, true, dev->desc->white.b);
		SetSlider(sTip1, dev->desc->white.r);
		SetSlider(sTip2, dev->desc->white.r);
		SetSlider(sTip3, dev->desc->white.r);
		EnableWindow(GetDlgItem(hDlg, IDC_ISPOWERBUTTON), dev->dev->GetVersion() < 5); // v5 and higher doesn't support power button
	}
}

void RedrawDevList(HWND hDlg) {
	int rpos = 0;
	HWND dev_list = GetDlgItem(hDlg, IDC_LIST_DEV);
	ListView_DeleteAllItems(dev_list);
	ListView_SetExtendedListViewStyle(dev_list, LVS_EX_FULLROWSELECT);
	LVCOLUMNA lCol{ LVCF_WIDTH, LVCFMT_LEFT, 100 };
	ListView_DeleteColumn(dev_list, 0);
	ListView_InsertColumn(dev_list, 0, &lCol);
	for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++) {
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
		if (!conf->afx_dev.fxdevs[i].desc) {
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
			conf->afx_dev.GetDevices()->push_back({ (WORD)conf->afx_dev.fxdevs[i].dev->GetVid(),
				(WORD)conf->afx_dev.fxdevs[i].dev->GetPID(),
				typeName + ", #" + to_string(conf->afx_dev.fxdevs[i].dev->GetPID())
				});
			conf->afx_dev.fxdevs[i].desc = &conf->afx_dev.GetDevices()->back();
		}
		lItem.lParam = i;
		lItem.pszText = (char*)conf->afx_dev.fxdevs[i].desc->name.c_str();
		if (lItem.lParam == dIndex) {
			lItem.state = LVIS_SELECTED;
			rpos = i;
		} else
			lItem.state = 0;
		ListView_InsertItem(dev_list, &lItem);
	}
	ListView_SetColumnWidth(dev_list, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_EnsureVisible(dev_list, rpos, false);
}

void LoadCSV(string name) {
	csv_devs.clear();
	// load csv...
	// Load mappings...
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
		devInfo tDev;
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
				case 0: // device line
					tDev.dev.vid = atoi(fields[1].c_str());
					tDev.dev.devid = atoi(fields[2].c_str());
					tDev.dev.name = fields[3];
					// add to devs.
					if (conf->afx_dev.GetDeviceById(tDev.dev.devid, tDev.dev.vid))
						csv_devs.push_back(tDev);
					else
						tDev.dev.devid = 0;
					break;
				case 1: // lights line
					if (tDev.dev.devid) {
						WORD lightid = (WORD)atoi(fields[1].c_str()),
							flags = (WORD)atoi(fields[2].c_str());
						// add to maps
						csv_devs.back().maps.push_back({tDev.dev.vid, tDev.dev.devid, lightid, flags, fields[3]});
					}
					break;
					//default: // wrong line, skip
				}
			}
		}
		CloseHandle(file);
	}
}

string OpenSaveFile(HWND hDlg, bool isOpen) {
	// Load/save device and light mappings
	OPENFILENAMEA fstruct{sizeof(OPENFILENAMEA), hDlg, hInst, "Mapping files (*.csv)\0*.csv\0\0" };
	string appName = "Current.csv";
	appName.reserve(4096);
	fstruct.lpstrFile = (LPSTR) appName.c_str();
	fstruct.nMaxFile = 4096;
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
	for (UINT i = 0; i < csv_devs.size(); i++) {
		if (force || csv_devs[i].selected) {
			AlienFX_SDK::devmap *cDev = conf->afx_dev.GetDeviceById(csv_devs[i].dev.devid, csv_devs[i].dev.vid);
			if (cDev) {
				cDev->name = csv_devs[i].dev.name;
			}
			for (int j = 0; j < csv_devs[i].maps.size(); j++) {
				AlienFX_SDK::mapping *oMap = conf->afx_dev.GetMappingById(csv_devs[i].maps[j].devid, csv_devs[i].maps[j].lightid);
				if (oMap) {
					oMap->vid = csv_devs[i].maps[j].vid;
					oMap->name = csv_devs[i].maps[j].name;
					oMap->flags = csv_devs[i].maps[j].flags;
				} else {
					AlienFX_SDK::mapping *nMap = new AlienFX_SDK::mapping(csv_devs[i].maps[j]);
					conf->afx_dev.GetMappings()->push_back(nMap);
				}
			}
		}
	}
	conf->afx_dev.AlienFXAssignDevices();
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

bool clickOnTab = false;

BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND gridTab = GetDlgItem(hDlg, IDC_TAB_DEV_GRID);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		// First, reset all light devices and re-scan!
		fxhl->UnblockUpdates(false, true);
		// Do we have some lights?
		if (!conf->afx_dev.GetMappings()->size() &&
			MessageBox( hDlg, "Light names not defined. Do you want to detect it?", "Warning", MB_ICONQUESTION | MB_YESNO )
			== IDYES) {
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_AUTODETECT), hDlg, (DLGPROC) DetectionDialog);
		}
		CreateGridBlock(gridTab, (DLGPROC)TabGrid, true);
		TabCtrl_SetCurSel(gridTab, conf->gridTabSel);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETTICFREQ, 16, 0);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETTICFREQ, 16, 0);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETTICFREQ, 16, 0);
		sTip1 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_RED), sTip1);
		sTip2 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_GREEN), sTip2);
		sTip3 = CreateToolTip(GetDlgItem(hDlg, IDC_SLIDER_BLUE), sTip3);
		if (conf->afx_dev.fxdevs.size() > 0) {
			if (dIndex < 0) dIndex = 0;
			RedrawDevList(hDlg);
		}
		oldproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_EDIT_GRID), GWLP_WNDPROC, (LONG_PTR)GridNameEdit);

		RegisterHotKey(hDlg, 1, MOD_SHIFT, VK_LEFT);
		RegisterHotKey(hDlg, 2, MOD_SHIFT, VK_RIGHT);
		RegisterHotKey(hDlg, 3, MOD_SHIFT, VK_HOME);
		RegisterHotKey(hDlg, 4, MOD_SHIFT, VK_END);

	} break;
	case WM_COMMAND: {
		WORD dPid = dIndex < 0 ? 0 : conf->afx_dev.fxdevs[dIndex].desc->devid;
		switch (LOWORD(wParam))
		{
		case IDC_BUT_NEXT:
			eLid++;
			SetLightInfo(hDlg);
			break;
		case IDC_BUT_PREV:
			if (eLid) {
				eLid--;
				SetLightInfo(hDlg);
			}
			break;
		case IDC_BUT_LAST:
			eLid = 0;
			for (auto it = conf->afx_dev.fxdevs[dIndex].lights.begin(); it < conf->afx_dev.fxdevs[dIndex].lights.end(); it++)
				eLid = max(eLid, (*it)->lightid);
			SetLightInfo(hDlg);
		break;
		case IDC_BUT_FIRST:
			eLid = 999;
			for (auto it = conf->afx_dev.fxdevs[dIndex].lights.begin(); it < conf->afx_dev.fxdevs[dIndex].lights.end(); it++)
				eLid = min(eLid, (*it)->lightid);
			SetLightInfo(hDlg);
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
				DWORD gridID = MAKELPARAM(dPid, eLid);
				for (auto it = conf->afx_dev.GetGrids()->begin(); it < conf->afx_dev.GetGrids()->end(); it++)
					for (int x = 0; x < it->x; x++)
						for (int y = 0; y < it->y; y++)
							if (it->grid[ind(x, y)] == gridID)
								it->grid[ind(x, y)] = 0;
				// delete from all groups...
				RemoveLightAndClean(dPid, eLid);
				// delete from current dev block...
				conf->afx_dev.fxdevs[dIndex].lights.erase(find_if(conf->afx_dev.fxdevs[dIndex].lights.begin(), conf->afx_dev.fxdevs[dIndex].lights.end(),
					[](auto t) {
						return t->lightid == eLid;
					}));
				// delete from mappings...
				conf->afx_dev.RemoveMapping(dPid, eLid);
				conf->afx_dev.SaveMappings();
				if (IsDlgButtonChecked(hDlg, IDC_ISPOWERBUTTON) == BST_CHECKED) {
					fxhl->ResetPower(dPid);
					MessageBox(hDlg, "Hardware Power button removed, you may need to reset light system!", "Warning",
							   MB_OK);
				}
				SetLightInfo(hDlg);
			}
			break;
		case IDC_BUTTON_TESTCOLOR: {
			SetColor(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor);
			if (eLid >= 0) {
				fxhl->TestLight(dIndex, eLid, whiteTest);
				OnGridSelChanged(gridTab);
			}
		} break;
		case IDC_ISPOWERBUTTON:
			if (eLid >= 0) {
				AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingById(dPid, eLid);
				if (lgh && IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED)
					if (MessageBox(hDlg, "Setting light to Hardware Power will reset all it settings! Are you sure?", "Warning",
								   MB_YESNO | MB_ICONWARNING) == IDYES) {
						lgh->flags |= ALIENFX_FLAG_POWER;
						// Check mappings and remove all power button data
						RemoveLightAndClean(dPid, eLid);
					} else
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				else {
					// remove power button config from chip config if unchecked and confirmed
					if (lgh) {
						if (MessageBox(hDlg, "You may need to reset light system!\nDo you want to set light as common?", "Warning",
							MB_YESNO | MB_ICONWARNING) == IDYES)
							fxhl->ResetPower(dPid);
						RemoveLightAndClean(dPid, eLid);
						lgh->flags &= ~ALIENFX_FLAG_POWER;
					}
				}
			}
			break;
		case IDC_CHECK_INDICATOR:
		{
			AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingById(dPid, eLid);
			if (lgh) {
				lgh->flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED ?
					lgh->flags | ALIENFX_FLAG_INDICATOR :
					lgh->flags & ~ALIENFX_FLAG_INDICATOR;
			}
		} break;
		case IDC_BUT_DETECT:
		{
			// Try to detect lights from DB
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_AUTODETECT), hDlg, (DLGPROC) DetectionDialog) == IDOK)
				RedrawDevList(hDlg);
		} break;
		case IDC_BUT_LOADMAP:
		{
			// Load device and light mappings
			string appName;
			if ((appName = OpenSaveFile(hDlg,true)).length()) {
				// Now load mappings...
				LoadCSV(appName);
				ApplyDeviceMaps(true);
				RedrawDevList(hDlg);
			}
		} break;
		case IDC_BUT_SAVEMAP:
		{
			// Save device and light mappings
			string appName;
			if ((appName = OpenSaveFile(hDlg,false)).length()) {
				// Now save mappings...
				HANDLE file = CreateFile(appName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				if (file != INVALID_HANDLE_VALUE) {
					for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++) {
						/// Only connected devices stored!
						DWORD writeBytes;
						string line = "'0','" + to_string(conf->afx_dev.fxdevs[i].desc->vid) + "','"
							+ to_string(conf->afx_dev.fxdevs[i].desc->devid) + "','" +
							conf->afx_dev.fxdevs[i].desc->name + "'\r\n";
						WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
						for (int j = 0; j < conf->afx_dev.fxdevs[i].lights.size(); j++) {
							AlienFX_SDK::mapping* cMap = conf->afx_dev.fxdevs[i].lights[j];
							line = "'1','" + to_string(cMap->lightid) + "','"
								+ to_string(cMap->flags) + "','" + cMap->name + "'\r\n";
							WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
						}
					}
					CloseHandle(file);
				}
			}
		} break;
		case IDC_CHECK_WHITE:
		{
			whiteTest = IsDlgButtonChecked(hDlg, IDC_CHECK_WHITE) == BST_CHECKED;
			fxhl->TestLight(dIndex, eLid, whiteTest);
		} break;
		default: return false;
		}
	} break;
	case WM_HOTKEY:
		switch (wParam) {
		case 1: SendMessage(hDlg, WM_COMMAND, IDC_BUT_PREV, 0); break;
		case 2: SendMessage(hDlg, WM_COMMAND, IDC_BUT_NEXT, 0); break;
		case 3: SendMessage(hDlg, WM_COMMAND, IDC_BUT_FIRST, 0); break;
		case 4: SendMessage(hDlg, WM_COMMAND, IDC_BUT_LAST, 0); break;
		}
		break;
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
					UpdateDeviceInfo(hDlg);
					//eLid = 0;
					SetLightInfo(hDlg);
				}
				else {
					if (!ListView_GetSelectedCount(((NMHDR*)lParam)->hwndFrom))
						RedrawDevList(hDlg);
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*)lParam;
				if (sItem->item.pszText) {
					conf->afx_dev.fxdevs[dIndex].desc->name = sItem->item.pszText;
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
	case WM_HSCROLL:
	{
		if (dIndex >= 0) {
			switch (LOWORD(wParam)) {
			case TB_THUMBTRACK: case TB_ENDTRACK:
			{
				if ((HWND) lParam == GetDlgItem(hDlg, IDC_SLIDER_RED)) {
					conf->afx_dev.fxdevs[dIndex].desc->white.r = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, conf->afx_dev.fxdevs[dIndex].desc->white.r);
				}
				if ((HWND) lParam == GetDlgItem(hDlg, IDC_SLIDER_GREEN)) {
					conf->afx_dev.fxdevs[dIndex].desc->white.g = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, conf->afx_dev.fxdevs[dIndex].desc->white.g);
				}
				if ((HWND) lParam == GetDlgItem(hDlg, IDC_SLIDER_BLUE)) {
					conf->afx_dev.fxdevs[dIndex].desc->white.b = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip3, conf->afx_dev.fxdevs[dIndex].desc->white.b);
				}
				fxhl->TestLight(dIndex, eLid, whiteTest);
			} break;
			}
		}
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_TESTCOLOR:
		{
			RedrawButton(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor);
		} break;
		}
		break;
	case WM_DESTROY:
	{
		fxhl->UnblockUpdates(true, true);
		fxhl->Refresh();
		UnregisterHotKey(hDlg, 1);
		UnregisterHotKey(hDlg, 2);
		UnregisterHotKey(hDlg, 3);
		UnregisterHotKey(hDlg, 4);
	} break;
	default: return false;
	}
	return true;
}

void UpdateSavedDevices(HWND hDlg) {
	HWND dev_list = GetDlgItem(hDlg, IDC_LIST_SUG);
	// Now check current device list..
	int pos = (-1);
	ListBox_ResetContent(dev_list);
	pos = ListBox_AddString(dev_list, "Manual");
	ListBox_SetCurSel(dev_list, pos);
	ListBox_SetItemData(dev_list, pos, -1);
	for (UINT i = 0; i < csv_devs.size(); i++) {
		if (LOWORD(csv_devs[i].dev.devid) == conf->afx_dev.fxdevs[dIndex].dev->GetPID()) {
				pos = ListBox_AddString(dev_list, csv_devs[i].dev.name.c_str());
				ListBox_SetItemData(dev_list, pos, i);
				if (csv_devs[i].selected)
					ListBox_SetCurSel(dev_list, pos);
		}
	}
}

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND //dev_list = GetDlgItem(hDlg, IDC_DEVICES),
		sug_list = GetDlgItem(hDlg, IDC_LIST_SUG);
	switch (message) {
	case WM_INITDIALOG:
	{
		LoadCSV(string(".\\Mappings\\devices.csv"));
		// init values according to device list
		RedrawDevList(hDlg);
		// Now init mappings...
		UpdateSavedDevices(hDlg);

	} break;
	case WM_COMMAND:
	{
		int //cDevID = (int)ListBox_GetItemData(dev_list, ListBox_GetCurSel(dev_list)),
			cSugID = (int)ListBox_GetItemData(sug_list, ListBox_GetCurSel(sug_list));
		switch (LOWORD(wParam)) {
		case IDOK:
		{
			// save mappings of selected.
			ApplyDeviceMaps();
			EndDialog(hDlg, IDOK);
		} break;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		//case IDC_DEVICES:
		//{
		//	switch (HIWORD(wParam)) {
		//	case CBN_SELCHANGE:
		//	{
		//		dIndex = cDevID;
		//		UpdateSavedDevices(hDlg);
		//	} break;
		//	}
		//} break;
		case IDC_LIST_SUG:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				// clear checkmarks...
				for (UINT i = 0; i < csv_devs.size(); i++) {
					if (csv_devs[i].dev.devid == conf->afx_dev.fxdevs[dIndex].desc->devid) {
						csv_devs[i].selected = false;
					}
				}
				if (cSugID >= 0)
					csv_devs[cSugID].selected = true;
			} break;
			}
		} break;
		}
	} break;
	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_DEV:
			switch (((NMHDR*)lParam)->code) {
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (NMLISTVIEW*)lParam;
				if (lPoint->uNewState && LVIS_FOCUSED && lPoint->iItem != -1) {
					// Select other item...
					dIndex = (int)lPoint->lParam;
					UpdateSavedDevices(hDlg);
				}
				else {
					if (!ListView_GetSelectedCount(((NMHDR*)lParam)->hwndFrom))
						RedrawDevList(hDlg);
				}
			} break;
			}
		}
		break;
    default: return false;
	}
	return true;
}