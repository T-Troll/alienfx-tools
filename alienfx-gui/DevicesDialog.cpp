#include "alienfx-gui.h"
#include "alienfx-controls.h"

extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);
extern bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


struct devInfo {
	AlienFX_SDK::devmap dev;
	vector<AlienFX_SDK::mapping> maps;
	bool selected = false;
};

extern AlienFan_SDK::Control* acpi;
int eLid = -1, dItem = -1, dIndex = -1;
vector<devInfo> csv_devs;

void UpdateLightsList(HWND hDlg, int pid, int lid) {
	int pos = -1;
	HWND light_list = GetDlgItem(hDlg, IDC_LIST_LIGHTS); 

	// clear light options...
	SetDlgItemInt(hDlg, IDC_LIGHTID, 0, false);
	CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, BST_UNCHECKED);

	ListView_DeleteAllItems(light_list);
	ListView_SetExtendedListViewStyle(light_list, LVS_EX_FULLROWSELECT);
	LVCOLUMNA lCol;
	lCol.mask = LVCF_WIDTH;
	lCol.cx = 100;
	ListView_DeleteColumn(light_list, 0);
	ListView_InsertColumn(light_list, 0, &lCol);
	for (int i = 0; i < fxhl->afx_dev.fxdevs[pid].lights.size(); i++) {
		LVITEMA lItem; 
		lItem.mask = LVIF_TEXT | LVIF_PARAM;
		lItem.iItem = i;
		lItem.iImage = 0;
		lItem.iSubItem = 0;
		lItem.lParam = fxhl->afx_dev.fxdevs[pid].lights[i]->lightid;
		lItem.pszText = (char*)fxhl->afx_dev.fxdevs[pid].lights[i]->name.c_str();
		if (lid == fxhl->afx_dev.fxdevs[pid].lights[i]->lightid) {
			lItem.mask |= LVIF_STATE;
			lItem.state = LVIS_SELECTED;
			pos = i;
			SetDlgItemInt(hDlg, IDC_LIGHTID, lid, false);
			CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, 
							fxhl->afx_dev.fxdevs[pid].lights[i]->flags & ALIENFX_FLAG_POWER ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, 
							fxhl->afx_dev.fxdevs[pid].lights[i]->flags & ALIENFX_FLAG_INDICATOR ? BST_CHECKED : BST_UNCHECKED);
		}
		ListView_InsertItem(light_list, &lItem);
	}
	ListView_SetColumnWidth(light_list, 0, LVSCW_AUTOSIZE);
	ListView_EnsureVisible(light_list, pos, false);
	BYTE status = fxhl->afx_dev.fxdevs[pid].dev->AlienfxGetDeviceStatus();
	if (status && status != 0xff)
		SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Ok");
	else
		SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Error");
}

void UpdateDeviceInfo(HWND hDlg) {
	if (dIndex >= 0 && dIndex < fxhl->afx_dev.fxdevs.size()) {
		AlienFX_SDK::afx_device *dev = &fxhl->afx_dev.fxdevs[dIndex];
		Static_SetText(GetDlgItem(hDlg, IDC_INFO_VID), ("VID: " + to_string(dev->desc->vid)).c_str());
		Static_SetText(GetDlgItem(hDlg, IDC_INFO_PID), ("PID: " + to_string(dev->desc->devid)).c_str());
		Static_SetText(GetDlgItem(hDlg, IDC_INFO_VER), ("API: v" + to_string(dev->dev->GetVersion())).c_str());
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_RED), TBM_SETPOS, true, dev->desc->white.r);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_GREEN), TBM_SETPOS, true, dev->desc->white.g);
		SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BLUE), TBM_SETPOS, true, dev->desc->white.b);
		SetSlider(sTip1, dev->desc->white.r);
		SetSlider(sTip2, dev->desc->white.r);
		SetSlider(sTip3, dev->desc->white.r);
	}
}

void UpdateDeviceList(HWND hDlg, bool isList = false) {
	HWND dev_list = GetDlgItem(hDlg, IDC_DEVICES);
	// Now check current device list..
	int pos = (-1);
	if (isList)
		ListBox_ResetContent(dev_list);
	else
		ComboBox_ResetContent(dev_list);
	if (dIndex == -1 && fxhl->afx_dev.fxdevs.size()) {
		dIndex = 0;
	}
	for (int i = 0; i < fxhl->afx_dev.fxdevs.size(); i++) {
		if (!fxhl->afx_dev.fxdevs[i].desc) {
			// no name
			unsigned devtype = fxhl->afx_dev.fxdevs[i].dev->GetType();
			string typeName = "Unknown";
			switch (devtype) {
			case 0: typeName = "Notebook"; break;
			case 1: typeName = "Keyboard"; break;
			case 2: typeName = "Monitor"; break;
			case 3: typeName = "Mouse"; break;
			}
			AlienFX_SDK::devmap newDev{(WORD)fxhl->afx_dev.fxdevs[i].dev->GetVid(),
				(WORD)fxhl->afx_dev.fxdevs[i].dev->GetPID(),
				typeName + ", #" + to_string(fxhl->afx_dev.fxdevs[i].dev->GetPID())
			};
			fxhl->afx_dev.GetDevices()->push_back(newDev);
			fxhl->afx_dev.fxdevs[i].desc = &fxhl->afx_dev.GetDevices()->back();
			fxhl->afx_dev.SaveMappings();
		}

		if (isList) {
			pos = ListBox_AddString(dev_list, fxhl->afx_dev.fxdevs[i].desc->name.c_str());
			ListBox_SetItemData(dev_list, pos, i);
		} else {
			pos = ComboBox_AddString(dev_list, fxhl->afx_dev.fxdevs[i].desc->name.c_str());
			ComboBox_SetItemData(dev_list, pos, i);
		}

		if (dIndex == i) {
			// select this device.
			if (isList)
				ListBox_SetCurSel(dev_list, pos);
			else {
				ComboBox_SetCurSel(dev_list, pos);
				UpdateDeviceInfo(hDlg);
				fxhl->TestLight(i, -1);
				UpdateLightsList(hDlg, i, -1);
			}
			dItem = pos;
		}
	}

	eLid = -1;
}

void LoadCSV(string name) {
	csv_devs.clear();
	// load csv...
	// Load mappings...
	HANDLE file = CreateFile(
		name.c_str(), 
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
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
					if (fxhl->afx_dev.GetDeviceById(tDev.dev.devid, tDev.dev.vid))
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
	OPENFILENAMEA fstruct{sizeof(OPENFILENAMEA)};
	string appName = isOpen ? "Default.csv" : "Current.csv";
	appName.reserve(4096);
	fstruct.hwndOwner = hDlg;
	fstruct.hInstance = hInst;
	fstruct.lpstrFile = (LPSTR) appName.c_str();
	fstruct.nMaxFile = 4096;
	fstruct.lpstrInitialDir = ".\\Mappings";
	fstruct.lpstrFilter = "Mapping files (*.csv)\0*.csv\0\0";
	fstruct.lpstrCustomFilter = NULL;
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
			AlienFX_SDK::devmap *cDev = fxhl->afx_dev.GetDeviceById(csv_devs[i].dev.devid, csv_devs[i].dev.vid);
			if (cDev) {
				cDev->name = csv_devs[i].dev.name;
			}
			for (int j = 0; j < csv_devs[i].maps.size(); j++) {
				AlienFX_SDK::mapping *oMap = fxhl->afx_dev.GetMappingById(csv_devs[i].maps[j].devid, csv_devs[i].maps[j].lightid);
				if (oMap) {
					oMap->vid = csv_devs[i].maps[j].vid;
					oMap->name = csv_devs[i].maps[j].name;
					oMap->flags = csv_devs[i].maps[j].flags;
				} else {
					AlienFX_SDK::mapping *nMap = new AlienFX_SDK::mapping(csv_devs[i].maps[j]);
					fxhl->afx_dev.GetMappings()->push_back(nMap);
				}
			}
		}
	}
	fxhl->afx_dev.AlienFXAssignDevices();
	csv_devs.clear();
}

BOOL CALLBACK TabDevicesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND light_view = GetDlgItem(hDlg, IDC_LIST_LIGHTS),
		dev_list = GetDlgItem(hDlg, IDC_DEVICES),
		light_id = GetDlgItem(hDlg, IDC_LIGHTID);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// First, reset all light devices and re-scan!
		fxhl->UnblockUpdates(false, true);
		// Do we have some lights?
		if (!fxhl->afx_dev.GetMappings()->size() &&
			MessageBox( hDlg, "There is no light names defined. Do you want to detect it?", "Warning", MB_ICONQUESTION | MB_YESNO )
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
		if (fxhl->afx_dev.fxdevs.size() > 0) {
			if (dIndex < 0) dIndex = 0;
			UpdateDeviceList(hDlg);
		}
	} break;
	case WM_COMMAND: {
		int dbItem = ComboBox_GetCurSel(dev_list),
			did = (int)ComboBox_GetItemData(dev_list, dbItem);
		WORD dPid = fxhl->afx_dev.fxdevs[dIndex].desc->devid;
		switch (LOWORD(wParam))
		{
		case IDC_DEVICES:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				eLid = -1;
				dIndex = did; dItem = dbItem;
				fxhl->TestLight(dIndex, -1);
				UpdateDeviceInfo(hDlg);
				UpdateLightsList(hDlg, dIndex, -1);
			} break;
			case CBN_EDITCHANGE:
				char buffer[MAX_PATH];
				GetWindowTextA(dev_list, buffer, MAX_PATH);
				if (dIndex >= 0) {
					fxhl->afx_dev.fxdevs[dIndex].desc->name = buffer;
					fxhl->afx_dev.SaveMappings();
					ComboBox_DeleteString(dev_list, dItem);
					ComboBox_InsertString(dev_list, dItem, buffer);
					ComboBox_SetItemData(dev_list, dItem, dIndex);
				}
				break;
			}
		} break;
		case IDC_BUTTON_ADDL: {
			int cid = GetDlgItemInt(hDlg, IDC_LIGHTID, NULL, false);
			// let's check if we have the same ID, need to use max+1 in this case
			unsigned maxID = 0;
			for (int i = 0; i < fxhl->afx_dev.fxdevs[dIndex].lights.size(); i++) {
				WORD aPid = fxhl->afx_dev.fxdevs[dIndex].lights[i]->lightid;
				if (aPid > maxID)
					maxID = aPid;
				if (aPid == cid)
					cid = -1;
			}
			if (cid < 0) cid = maxID + 1;
			fxhl->afx_dev.AddMapping(MAKELONG(fxhl->afx_dev.fxdevs[dIndex].desc->devid,
											  fxhl->afx_dev.fxdevs[dIndex].desc->vid),
									 cid,
									 ("Light #" + to_string(cid)).c_str(), 0);
			fxhl->afx_dev.fxdevs[dIndex].lights.push_back(fxhl->afx_dev.GetMappings()->back());
			fxhl->afx_dev.SaveMappings();
			eLid = cid;
			UpdateLightsList(hDlg, dIndex, eLid);
			fxhl->TestLight(dIndex, eLid);
		} break;
		case IDC_BUTTON_REML:
			if (MessageBox(hDlg, "Do you really want to remove current light name and all it's settings from all groups and profiles?", "Warning!",
						   MB_YESNO | MB_ICONWARNING) == IDYES) {
				// delete from all groups...
				for (int i = 0; i < fxhl->afx_dev.GetGroups()->size(); i++) {
					AlienFX_SDK::group* grp = &fxhl->afx_dev.GetGroups()->at(i);
					for (vector<AlienFX_SDK::mapping*>::iterator gIter = grp->lights.begin();
						 gIter < grp->lights.end(); gIter++)
						if ((*gIter)->devid == dPid && (*gIter)->lightid == eLid) {
							grp->lights.erase(gIter);
							break;
						}
				}
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					 Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, dPid, eLid);
				}

				int nLid = -1;
				// delete from current dev block...
				for (vector <AlienFX_SDK::mapping*>::iterator Iter = fxhl->afx_dev.fxdevs[dIndex].lights.begin();
					 Iter != fxhl->afx_dev.fxdevs[dIndex].lights.end(); Iter++)
					if ((*Iter)->lightid == eLid) {
						fxhl->afx_dev.fxdevs[dIndex].lights.erase(Iter);
						break;
					} else nLid = (*Iter)->lightid;
				// delete from mappings...
				for (vector <AlienFX_SDK::mapping*>::iterator Iter = fxhl->afx_dev.GetMappings()->begin();
						Iter != fxhl->afx_dev.GetMappings()->end(); Iter++)
					if ((*Iter)->devid == dPid && (*Iter)->lightid == eLid) {
						delete *Iter;
						fxhl->afx_dev.GetMappings()->erase(Iter);
						break;
					}

				fxhl->afx_dev.SaveMappings();
				conf->Save();
				if (IsDlgButtonChecked(hDlg, IDC_ISPOWERBUTTON) == BST_CHECKED) {
					fxhl->ResetPower(did);
					MessageBox(hDlg, "Hardware Power button removed, you may need to reset light system!", "Warning!",
							   MB_OK);
				}
				eLid = nLid;
				UpdateLightsList(hDlg, dIndex, eLid);
				fxhl->TestLight(dIndex, eLid);
			}
			break;
		case IDC_BUTTON_RESETCOLOR:
			if (MessageBox(hDlg, "Do you really want to remove current light control settings from all profiles?", "Warning!",
						   MB_YESNO | MB_ICONWARNING) == IDYES) {
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					 Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, fxhl->afx_dev.fxdevs[dIndex].desc->devid, eLid);
				}
			}
			break;
		case IDC_BUTTON_TESTCOLOR: {
			SetColor(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor);
			if (eLid != -1) {
				fxhl->TestLight(dIndex, eLid);
			}
		} break;
		case IDC_ISPOWERBUTTON:
			if (eLid != -1) {
				unsigned flags = fxhl->afx_dev.GetFlags(dPid, eLid) & ~ALIENFX_FLAG_POWER;
				flags &= IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED ? ALIENFX_FLAG_POWER : 0;
				if (flags)
					if (MessageBox(hDlg, "Setting light to Hardware Power button slow down updates and can hang you light system! Are you sure?", "Warning!",
								   MB_YESNO | MB_ICONWARNING) == IDYES) {
						fxhl->afx_dev.SetFlagsById(fxhl->afx_dev.fxdevs[dIndex].desc->devid, eLid, flags);
					} else
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				else {
					// remove power button config from chip config if unchecked and confirmed
					if (MessageBox(hDlg, "Hardware Power button disabled, you may need to reset light system! Do you want to reset Power button light as well?", "Warning!",
								   MB_YESNO | MB_ICONWARNING) == IDYES)
						fxhl->ResetPower(did);
					fxhl->afx_dev.SetFlagsById(dPid, eLid, flags);
				}
			}
			break;
		case IDC_CHECK_INDICATOR:
		{
			unsigned flags = fxhl->afx_dev.GetFlags(dPid, eLid) & ~ALIENFX_FLAG_INDICATOR;
			flags &= IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED ? ALIENFX_FLAG_INDICATOR : 0;
			fxhl->afx_dev.SetFlagsById(dPid, eLid, flags);
		} break;
		case IDC_BUTTON_DEVRESET:
		{
			fxhl->FillAllDevs(acpi);
			UpdateDeviceList(hDlg);
		} break;
		case IDC_BUT_DETECT:
		{
			// Try to detect lights from DB
			if (DialogBox(hInst,
						  MAKEINTRESOURCE(IDD_DIALOG_AUTODETECT),
						  hDlg,
						  (DLGPROC) DetectionDialog) == IDOK)
				UpdateDeviceList(hDlg);
		} break;
		case IDC_BUT_LOADMAP:
		{	
			// Load device and light mappings
			string appName;
			if ((appName = OpenSaveFile(hDlg,true)).length()) {
				// Now load mappings...
				LoadCSV(appName);
				ApplyDeviceMaps(true);
				UpdateDeviceList(hDlg);
			}
		} break;
		case IDC_BUT_SAVEMAP:
		{
			// Save device and ligh mappings
			string appName;
			if ((appName = OpenSaveFile(hDlg,false)).length()) {
				// Now save mappings...
				HANDLE file = CreateFile(appName.c_str(),
										 GENERIC_WRITE,
										 0,
										 NULL,
										 CREATE_ALWAYS,
										 0,
										 NULL );
				if (file != INVALID_HANDLE_VALUE) {
					for (int i = 0; i < fxhl->afx_dev.fxdevs.size(); i++) {
						/// Only connected devices stored!
						DWORD writeBytes;
						string line = "'0','" + to_string(fxhl->afx_dev.fxdevs[i].desc->vid) + "','" 
							+ to_string(fxhl->afx_dev.fxdevs[i].desc->devid) + "','" + 
							fxhl->afx_dev.fxdevs[i].desc->name + "'\r\n";
						WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
						for (int j = 0; j < fxhl->afx_dev.fxdevs[i].lights.size(); j++) {
							AlienFX_SDK::mapping* cMap = fxhl->afx_dev.fxdevs[i].lights[j];
							line = "'1','" + to_string(cMap->lightid) + "','"
								+ to_string(cMap->flags) + "','" + cMap->name + "'\r\n";
							WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
						}
					}
					CloseHandle(file);
				}
			}
		} break;
		default: return false;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*) lParam)->idFrom) {
		case IDC_LIST_LIGHTS:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE:
			{
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*) lParam;
				ListView_EditLabel(light_view, sItem->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & LVIS_FOCUSED) {
					// Select other item...
					if (lPoint->iItem != -1) {
						eLid = (int)lPoint->lParam;
						SetDlgItemInt(hDlg, IDC_LIGHTID, eLid, false);
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev.GetFlags(fxhl->afx_dev.fxdevs[dIndex].desc->devid, eLid) & ALIENFX_FLAG_POWER ? BST_CHECKED : BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, fxhl->afx_dev.GetFlags(fxhl->afx_dev.fxdevs[dIndex].desc->devid, eLid) & ALIENFX_FLAG_INDICATOR ? BST_CHECKED : BST_UNCHECKED);
					} else {
						SetDlgItemInt(hDlg, IDC_LIGHTID, 0, false);
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, BST_UNCHECKED);
						eLid = -1;
					}
					fxhl->TestLight(dIndex, eLid);
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				if (sItem->item.pszText) {
					for (UINT i = 0; i < fxhl->afx_dev.fxdevs[dIndex].lights.size(); i++) {
						if (fxhl->afx_dev.fxdevs[dIndex].lights[i]->lightid == sItem->item.lParam) {
							fxhl->afx_dev.fxdevs[dIndex].lights[i]->name = sItem->item.pszText;
							ListView_SetItem(light_view, &sItem->item);
							ListView_SetColumnWidth(light_view, 0, LVSCW_AUTOSIZE);
							fxhl->afx_dev.SaveMappings();
							break;
						}
					}
				}
				return false;
			} break;
			}
			break;
		}
		break;
	case WM_HSCROLL:
	{
		if (dIndex >= 0) {
			switch (LOWORD(wParam)) {
			case TB_THUMBTRACK: case TB_ENDTRACK:
			{
				if ((HWND) lParam == GetDlgItem(hDlg, IDC_SLIDER_RED)) {
					fxhl->afx_dev.fxdevs[dIndex].desc->white.r = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, fxhl->afx_dev.fxdevs[dIndex].desc->white.r);
				}
				if ((HWND) lParam == GetDlgItem(hDlg, IDC_SLIDER_GREEN)) {
					fxhl->afx_dev.fxdevs[dIndex].desc->white.g = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, fxhl->afx_dev.fxdevs[dIndex].desc->white.g);
				}
				if ((HWND) lParam == GetDlgItem(hDlg, IDC_SLIDER_BLUE)) {
					fxhl->afx_dev.fxdevs[dIndex].desc->white.b = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip3, fxhl->afx_dev.fxdevs[dIndex].desc->white.b);
				}
				fxhl->TestLight(dIndex, eLid, true);
			} break;
			}
		}
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_TESTCOLOR:
		{
			RedrawButton(hDlg, IDC_BUTTON_TESTCOLOR, conf->testColor);
		} break;
		}
		break;
	case WM_DESTROY:
	{
		fxhl->UnblockUpdates(true, true);
		fxhl->RefreshState();
	} break;
	//case WM_PAINT:
	//	fxhl->UnblockUpdates(false, true);
	//	if (dIndex >= 0) 
	//		fxhl->TestLight(dIndex, eLid);
	//	return false;
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
		if (LOWORD(csv_devs[i].dev.devid) == fxhl->afx_dev.fxdevs[dIndex].dev->GetPID()) {
				pos = ListBox_AddString(dev_list, csv_devs[i].dev.name.c_str());
				ListBox_SetItemData(dev_list, pos, i);
				if (csv_devs[i].selected)
					ListBox_SetCurSel(dev_list, pos);
		}
	}
}

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND dev_list = GetDlgItem(hDlg, IDC_DEVICES),
		sug_list = GetDlgItem(hDlg, IDC_LIST_SUG);
	switch (message) {
	case WM_INITDIALOG:
	{
		LoadCSV(string(".\\Mappings\\devices.csv"));
		// init values according to device list
		UpdateDeviceList(hDlg, true);
		// Now init mappings...
		UpdateSavedDevices(hDlg);

	} break;
	case WM_COMMAND:
	{
		int cDevID = (int) ListBox_GetItemData(dev_list, ListBox_GetCurSel(dev_list)),
			cSugID = (int) ListBox_GetItemData(sug_list, ListBox_GetCurSel(sug_list));
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
		case IDC_DEVICES:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				dIndex = cDevID;
				UpdateSavedDevices(hDlg);
			} break;
			}
		} break;
		case IDC_LIST_SUG:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				// clear checkmarks...
				for (UINT i = 0; i < csv_devs.size(); i++) {
					if (csv_devs[i].dev.devid == fxhl->afx_dev.fxdevs[dIndex].desc->devid) {
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
    default: return false;
	}
	return true;
}