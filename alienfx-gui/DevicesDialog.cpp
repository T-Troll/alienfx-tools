#include "alienfx-gui.h"
#include "alienfx-controls.h"

extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);
extern void RemoveMapping(groupset* lightsets);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern void RemoveHapMapping(int devid, int lightid);
extern void RemoveAmbMapping(int devid, int lightid);
extern void RemoveLightFromGroup(AlienFX_SDK::group* grp, WORD devid, WORD lightid);
extern void RemoveLightAndClean(int dPid, int eLid);

extern void CreateGridBlock(HWND gridTab);
extern void OnGridSelChanged(HWND);
extern AlienFX_SDK::mapping* FindCreateMapping();

extern int gridTabSel;
extern AlienFX_SDK::lightgrid* mainGrid;

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct devInfo {
	AlienFX_SDK::devmap dev;
	vector<AlienFX_SDK::mapping> maps;
	bool selected = false;
};

//extern AlienFan_SDK::Control* acpi;
int eLid = 0, dItem = -1, dIndex = 0;// , lMaxIndex = 0;
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
		}
		ListView_InsertItem(dev_list, &lItem);
	}
	ListView_SetColumnWidth(dev_list, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_EnsureVisible(dev_list, rpos, false);
}

//void UpdateLightsList(HWND hDlg, int lid) {
//	int pos = -1;
//	HWND light_list = GetDlgItem(hDlg, IDC_LIST_LIGHTS);
//
//	lMaxIndex = 0;
//
//	ListView_DeleteAllItems(light_list);
//	ListView_SetExtendedListViewStyle(light_list, LVS_EX_FULLROWSELECT);
//	LVCOLUMNA lCol;
//	lCol.mask = LVCF_WIDTH;
//	lCol.cx = 100;
//	ListView_DeleteColumn(light_list, 0);
//	ListView_InsertColumn(light_list, 0, &lCol);
//	for (int i = 0; i < conf->afx_dev.fxdevs[dIndex].lights.size(); i++) {
//		AlienFX_SDK::mapping *clight = conf->afx_dev.fxdevs[dIndex].lights[i];
//		if (lMaxIndex < clight->lightid) lMaxIndex = clight->lightid;
//		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i };
//		lItem.lParam = clight->lightid;
//		lItem.pszText = (char*)clight->name.c_str();
//		if (lid == clight->lightid) {
//			lItem.state = LVIS_SELECTED;
//			pos = i;
//		}
//		ListView_InsertItem(light_list, &lItem);
//	}
//	ListView_SetColumnWidth(light_list, 0, LVSCW_AUTOSIZE);
//	ListView_EnsureVisible(light_list, pos, false);
//	SetLightInfo(hDlg);
//}

void UpdateDeviceInfo(HWND hDlg) {
	if (dIndex >= 0 && dIndex < conf->afx_dev.fxdevs.size()) {
		AlienFX_SDK::afx_device *dev = &conf->afx_dev.fxdevs[dIndex];
		char descript[128];
		sprintf_s(descript,128, "VID_%04X, PID_%04X, APIv%d",
			dev->desc->vid, dev->desc->devid, dev->dev->GetVersion());
		SetWindowText(GetDlgItem(hDlg, IDC_INFO_VID), descript);
		//Static_SetText(GetDlgItem(hDlg, IDC_INFO_VID), ("VID: " + to_string(dev->desc->vid)).c_str());
		//Static_SetText(GetDlgItem(hDlg, IDC_INFO_PID), ("PID: " + to_string(dev->desc->devid)).c_str());
		//Static_SetText(GetDlgItem(hDlg, IDC_INFO_VER), ("API: v" + to_string(dev->dev->GetVersion())).c_str());
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETPOS, true, dev->desc->white.r);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETPOS, true, dev->desc->white.g);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETPOS, true, dev->desc->white.b);
		SetSlider(sTip1, dev->desc->white.r);
		SetSlider(sTip2, dev->desc->white.r);
		SetSlider(sTip3, dev->desc->white.r);
		BYTE status = dev->dev->AlienfxGetDeviceStatus();
		//SetDlgItemText(hDlg, IDC_DEVICE_STATUS, status && status != 0xff ? "" : "Error!");
		EnableWindow(GetDlgItem(hDlg, IDC_ISPOWERBUTTON), dev->dev->GetVersion() < 5); // v5 and higher doesn't support power button
	}
}

//void UpdateDeviceList(HWND hDlg, bool isList = false) {
//	HWND dev_list = GetDlgItem(hDlg, IDC_DEVICES);
//	// Now check current device list..
//	int pos = (-1);
//	if (isList)
//		ListBox_ResetContent(dev_list);
//	else
//		ComboBox_ResetContent(dev_list);
//	if (dIndex == -1 && conf->afx_dev.fxdevs.size()) {
//		dIndex = 0;
//	}
//	for (int i = 0; i < conf->afx_dev.fxdevs.size(); i++) {
//		if (!conf->afx_dev.fxdevs[i].desc) {
//			// no name
//			string typeName = "Unknown";
//			switch (conf->afx_dev.fxdevs[i].dev->GetVersion()) {
//			case 0: typeName = "Desktop"; break;
//			case 1: case 2: case 3: case 4: typeName = "Notebook"; break;
//			case 5: typeName = "Keyboard"; break;
//			case 6: typeName = "Display"; break;
//			case 7: typeName = "Mouse"; break;
//			}
//			AlienFX_SDK::devmap newDev{(WORD)conf->afx_dev.fxdevs[i].dev->GetVid(),
//				(WORD)conf->afx_dev.fxdevs[i].dev->GetPID(),
//				typeName + ", #" + to_string(conf->afx_dev.fxdevs[i].dev->GetPID())
//			};
//			conf->afx_dev.GetDevices()->push_back(newDev);
//			conf->afx_dev.fxdevs[i].desc = &conf->afx_dev.GetDevices()->back();
//			conf->afx_dev.SaveMappings();
//		}
//
//		if (isList) {
//			pos = ListBox_AddString(dev_list, conf->afx_dev.fxdevs[i].desc->name.c_str());
//			ListBox_SetItemData(dev_list, pos, i);
//		} else {
//			pos = ComboBox_AddString(dev_list, conf->afx_dev.fxdevs[i].desc->name.c_str());
//			ComboBox_SetItemData(dev_list, pos, i);
//		}
//
//		if (dIndex == i) {
//			// select this device.
//			if (isList)
//				ListBox_SetCurSel(dev_list, pos);
//			else {
//				ComboBox_SetCurSel(dev_list, pos);
//				UpdateDeviceInfo(hDlg);
//				fxhl->TestLight(dIndex, -1, whiteTest);
//				//UpdateLightsList(hDlg, -1);
//			}
//			dItem = pos;
//		}
//	}
//
//	eLid = -1;
//}

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
	mainGrid->name.resize(GetWindowTextLength(hDlg) + 1);
	Edit_GetText(hDlg, (LPSTR)mainGrid->name.c_str(), 255);
	ShowWindow(hDlg, SW_HIDE);
	// change tab text
	TCITEM tie{ TCIF_TEXT };
	tie.pszText = (LPSTR)mainGrid->name.c_str();
	TabCtrl_SetItem(GetDlgItem(GetParent(hDlg), IDC_TAB_DEV_GRID), gridTabSel, &tie);
	RedrawWindow(GetDlgItem(GetParent(hDlg), IDC_TAB_DEV_GRID), NULL, NULL, RDW_INVALIDATE);
}

LRESULT GridNameEdit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_KEYUP: {
		switch (wParam) {
		case VK_RETURN: case VK_TAB: {
			SetNewGridName(hDlg);
		} break;
		case VK_ESCAPE: {
			// just close edit
			ShowWindow(hDlg, SW_HIDE);
		} break;
		}
	} break;
	}
	return CallWindowProc(oldproc, hDlg, message, wParam, lParam);
}

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
			//UpdateDeviceList(hDlg);
			RedrawDevList(hDlg);
		}
		CreateGridBlock(gridTab);
		TabCtrl_SetCurSel(gridTab, gridTabSel);
		OnGridSelChanged(gridTab);
		oldproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_EDIT_GRID), GWLP_WNDPROC, (LONG_PTR)GridNameEdit);
	} break;
	case WM_COMMAND: {
		WORD dPid = dIndex < 0 ? 0 : conf->afx_dev.fxdevs[dIndex].desc->devid;
		switch (LOWORD(wParam))
		{
		//case IDC_DEVICES:
		//{
		//	switch (HIWORD(wParam)) {
		//	case CBN_SELCHANGE:
		//	{
		//		eLid = -1;
		//		dItem = ComboBox_GetCurSel(dev_list);
		//		dIndex = (int)ComboBox_GetItemData(dev_list, dItem);
		//		fxhl->TestLight(dIndex, -1, whiteTest);
		//		UpdateDeviceInfo(hDlg);
		//		UpdateLightsList(hDlg, -1);
		//	} break;
		//	case CBN_EDITCHANGE:
		//		char buffer[MAX_PATH];
		//		GetWindowTextA(dev_list, buffer, MAX_PATH);
		//		if (dIndex >= 0) {
		//			conf->afx_dev.fxdevs[dIndex].desc->name = buffer;
		//			conf->afx_dev.SaveMappings();
		//			ComboBox_DeleteString(dev_list, dItem);
		//			ComboBox_InsertString(dev_list, dItem, buffer);
		//			ComboBox_SetItemData(dev_list, dItem, dIndex);
		//		}
		//		break;
		//	}
		//} break;
		//case IDC_BUTTON_ADDL: {
		//	int cid = GetDlgItemInt(hDlg, IDC_LIGHTID, NULL, false);
		//	// let's check if we have the same ID, need to use max+1 in this case
		//	if (conf->afx_dev.GetMappingById(dPid, cid)) {
		//		// have light, need to use max ID
		//		cid = lMaxIndex + 1;
		//	}
		//	conf->afx_dev.AddMapping(MAKELONG(conf->afx_dev.fxdevs[dIndex].desc->devid,
		//									  conf->afx_dev.fxdevs[dIndex].desc->vid),
		//							 cid,
		//							 ("Light #" + to_string(cid)).c_str(), 0);
		//	conf->afx_dev.fxdevs[dIndex].lights.push_back(conf->afx_dev.GetMappings()->back());
		//	conf->afx_dev.SaveMappings();
		//	eLid = cid;
		//	UpdateLightsList(hDlg, eLid);
		//	fxhl->TestLight(dIndex, eLid, whiteTest);
		//} break;
		case IDC_BUT_NEXT:
			eLid++;
			SetLightInfo(hDlg);
			OnGridSelChanged(gridTab);
			break;
		case IDC_BUT_PREV:
			if (eLid) eLid--;
			SetLightInfo(hDlg);
			OnGridSelChanged(gridTab);
			break;
		case IDC_BUT_LAST:
			eLid = 0;
			for (auto it = conf->afx_dev.fxdevs[dIndex].lights.begin(); it < conf->afx_dev.fxdevs[dIndex].lights.end(); it++)
				eLid = max(eLid, (*it)->lightid);
			SetLightInfo(hDlg);
			OnGridSelChanged(gridTab);
		break;
		case IDC_BUT_FIRST:
			eLid = 0;
			SetLightInfo(hDlg);
			OnGridSelChanged(gridTab);
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
				for (auto it = conf->afx_dev.GetGrids()->begin(); it < conf->afx_dev.GetGrids()->end(); it++)
					for (int x = 0; x < it->x; x++)
						for (int y = 0; y < it->y; y++)
							if (it->grid[x][y] == MAKELPARAM(dPid, eLid))
								it->grid[x][y] = 0;

				// delete from all groups...
				RemoveLightAndClean(dPid, eLid);

				//int nLid = -1;
				// delete from current dev block...
				auto pos = find_if(conf->afx_dev.fxdevs[dIndex].lights.begin(), conf->afx_dev.fxdevs[dIndex].lights.end(),
					[](auto t) {
						return t->lightid == eLid;
					});
				//if ((pos + 1) != conf->afx_dev.fxdevs[dIndex].lights.end())
				//	nLid = (*(pos + 1))->lightid;
				//else
				//	if (pos != conf->afx_dev.fxdevs[dIndex].lights.begin())
				//		nLid = (*(pos - 1))->lightid;
				conf->afx_dev.fxdevs[dIndex].lights.erase(pos);
				// delete from mappings...
				conf->afx_dev.RemoveMapping(dPid, eLid);
				// delete from haptics and ambient
				RemoveHapMapping(dPid, eLid);
				RemoveAmbMapping(dPid, eLid);

				conf->afx_dev.SaveMappings();
				conf->Save();
				if (IsDlgButtonChecked(hDlg, IDC_ISPOWERBUTTON) == BST_CHECKED) {
					fxhl->ResetPower(dPid);
					MessageBox(hDlg, "Hardware Power button removed, you may need to reset light system!", "Warning",
							   MB_OK);
				}
				//eLid = nLid;
				SetLightInfo(hDlg);
				//int nMaxIndex = lMaxIndex;
				//UpdateLightsList(hDlg, eLid);
				//lMaxIndex = nMaxIndex;
				//fxhl->TestLight(dIndex, eLid, whiteTest);
			}
			break;
		//case IDC_BUTTON_RESETCOLOR:
		//	if (MessageBox(hDlg, "Do you really want to remove current light control settings from all profiles and groups?", "Warning",
		//				   MB_YESNO | MB_ICONWARNING) == IDYES) {
		//		// delete from all groups...
		//		RemoveLightAndClean(dPid, eLid);

		//		RemoveHapMapping(dPid, eLid);
		//		RemoveAmbMapping(dPid, eLid);
		//	}
		//	break;
		case IDC_BUTTON_TESTCOLOR: {
			SetColor(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor);
			if (eLid != -1) {
				fxhl->TestLight(dIndex, eLid, whiteTest);
			}
		} break;
		case IDC_ISPOWERBUTTON:
			if (eLid != -1) {
				AlienFX_SDK::mapping* lgh = conf->afx_dev.GetMappingById(dPid, eLid);
				if (lgh && IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED)
					if (MessageBox(hDlg, "Setting light to Hardware Power button slow down updates and can hang you light system! Are you sure?", "Warning",
								   MB_YESNO | MB_ICONWARNING) == IDYES) {
						lgh->flags |= ALIENFX_FLAG_POWER;
						// Check mappings and remove all power button data
						RemoveLightAndClean(dPid, eLid);

					} else
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				else {
					// remove power button config from chip config if unchecked and confirmed
					if (lgh) {
						if (MessageBox(hDlg, "Hardware Power button disabled, you may need to reset light system! Do you want to reset Power button light as well?", "Warning",
							MB_YESNO | MB_ICONWARNING) == IDYES)
							fxhl->ResetPower(dPid);
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
		//case IDC_BUTTON_DEVRESET:
		//{
		//	fxhl->FillAllDevs(acpi);
		//	UpdateDeviceList(hDlg);
		//} break;
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
		//case IDC_EDIT_GRID:
		//	switch (HIWORD(wParam)) {
		//	case EN_CHANGE: {
		//		// check condition and modify name
		//		if (Edit_GetModify(GetDlgItem(hDlg, IDC_EDIT_GRID))) {
		//			// set
		//			char buf[256];
		//			Edit_GetText(GetDlgItem(hDlg, IDC_EDIT_GRID), buf, 255);
		//			int i = 0;
		//		}
		//	} break;
		//	}
		//	break;
		default: return false;
		}
	} break;
	case WM_NOTIFY: {
		HWND gridTab = GetDlgItem(hDlg, IDC_TAB_DEV_GRID);
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_TAB_DEV_GRID: {
			switch (((NMHDR*)lParam)->code) {
			case NM_RCLICK: {
				RECT tRect;
				TabCtrl_GetItemRect(gridTab, gridTabSel, &tRect);
				//GetClientRect(gridTab, &tRect);
				MapWindowPoints(gridTab, hDlg, (LPPOINT)&tRect, 2);
				SetDlgItemText(hDlg, IDC_EDIT_GRID, mainGrid->name.c_str());
				ShowWindow(GetDlgItem(hDlg, IDC_EDIT_GRID), SW_RESTORE);
				SetWindowPos(GetDlgItem(hDlg, IDC_EDIT_GRID), NULL, tRect.left - 1, tRect.top - 1,
					tRect.right - tRect.left > 60 ? tRect.right - tRect.left : 60, tRect.bottom - tRect.top + 2, SWP_SHOWWINDOW);
				SetFocus(GetDlgItem(hDlg, IDC_EDIT_GRID));
			} break;
			case TCN_SELCHANGE: {
				int newSel = TabCtrl_GetCurSel(gridTab);
				if (newSel != gridTabSel) { // selection changed!
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
							conf->afx_dev.GetGrids()->push_back({ newGridIndex, mainGrid->x, mainGrid->y, "Grid #" + to_string(newGridIndex) });
							mainGrid = &conf->afx_dev.GetGrids()->back();
							//RedrawGridList(hDlg);
							TCITEM tie{ TCIF_TEXT };
							tie.pszText = (LPSTR)mainGrid->name.c_str();
							TabCtrl_InsertItem(gridTab, conf->afx_dev.GetGrids()->size() - 1, (LPARAM)&tie);
							TabCtrl_SetCurSel(gridTab, conf->afx_dev.GetGrids()->size() - 1);
							OnGridSelChanged(gridTab);
						}
						else
						{
							if (conf->afx_dev.GetGrids()->size() > 1) {
								int newTab = gridTabSel;
								for (auto it = conf->afx_dev.GetGrids()->begin(); it < conf->afx_dev.GetGrids()->end(); it++) {
									if (it->id == mainGrid->id) {
										// remove
										if ((it + 1) == conf->afx_dev.GetGrids()->end())
											newTab--;
										conf->afx_dev.GetGrids()->erase(it);
										TabCtrl_DeleteItem(gridTab, gridTabSel);
										TabCtrl_SetCurSel(gridTab, newTab);
										OnGridSelChanged(gridTab);
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
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, sItem->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (NMLISTVIEW*)lParam;
				if (lPoint->uNewState && LVIS_FOCUSED && lPoint->iItem != -1) {
					// Select other item...
					dIndex = (int)lPoint->lParam;
					UpdateDeviceInfo(hDlg);
					eLid = 0;
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
		fxhl->RefreshState();
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