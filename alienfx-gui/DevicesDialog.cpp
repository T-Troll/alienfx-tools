#include "alienfx-gui.h"

bool SetColor(HWND hDlg, int id, BYTE *r, BYTE *g, BYTE *b);
bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);
void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b);

BOOL CALLBACK DetectionDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern AlienFan_SDK::Control* acpi;
int eLid = -1, eDid = -1, dItem = -1;

void UpdateLightsList(HWND hDlg, int pid, int lid) {
	int pos = -1;
	size_t lights = fxhl->afx_dev.GetMappings()->size();
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
	for (int i = 0; i < lights; i++) {
		AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(i);
		if (pid == lgh->devid) { // && fxhl->LocateDev(lgh.devid)) {
			LVITEMA lItem; 
			lItem.mask = LVIF_TEXT | LVIF_PARAM;
			lItem.iItem = i;
			lItem.iImage = 0;
			lItem.iSubItem = 0;
			lItem.lParam = lgh->lightid;
			lItem.pszText = (char*)lgh->name.c_str();
			if (lid == lgh->lightid) {
				lItem.mask |= LVIF_STATE;
				lItem.state = LVIS_SELECTED;
				pos = i;
				SetDlgItemInt(hDlg, IDC_LIGHTID, lid, false);
				CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev.GetFlags(pid, lid) & ALIENFX_FLAG_POWER ? BST_CHECKED : BST_UNCHECKED);
				CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, fxhl->afx_dev.GetFlags(pid, lid) & ALIENFX_FLAG_INACTIVE ? BST_CHECKED : BST_UNCHECKED);
			}
			ListView_InsertItem(light_list, &lItem);
		}
	}
	ListView_SetColumnWidth(light_list, 0, LVSCW_AUTOSIZE);
	ListView_EnsureVisible(light_list, pos, false);
	BYTE status = fxhl->LocateDev(eDid)->AlienfxGetDeviceStatus();
	if (status && status != 0xff)
		SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Ok");
	else
		SetDlgItemText(hDlg, IDC_DEVICE_STATUS, "Status: Error");
}

void UpdateDeviceList(HWND hDlg, bool isList = false) {
	HWND dev_list = GetDlgItem(hDlg, IDC_DEVICES);
	// Now check current device list..
	int pos = (-1);
	if (isList)
		ListBox_ResetContent(dev_list);
	else
		ComboBox_ResetContent(dev_list);
	for (UINT i = 0; i < fxhl->devs.size(); i++) {
		AlienFX_SDK::devmap* cDev = fxhl->afx_dev.GetDeviceById(fxhl->devs[i]->GetPID(), fxhl->devs[i]->GetVid());
		if (cDev) {
			if (!cDev->vid)
				cDev->vid = fxhl->devs[i]->GetVid();
			if (isList) {
				pos = ListBox_AddString(dev_list, cDev->name.c_str());
				ListBox_SetItemData(dev_list, pos, cDev->devid);
			} else {
				pos = ComboBox_AddString(dev_list, cDev->name.c_str());
				ComboBox_SetItemData(dev_list, pos, cDev->devid);
			}
		} else {
			// no name
			string devName = "Device #" + to_string(fxhl->devs[i]->GetVid()) + "_" + to_string(fxhl->devs[i]->GetPID());
			if (isList) {
				pos = ListBox_AddString(dev_list, devName.c_str());
				ListBox_SetItemData(dev_list, pos, fxhl->devs[i]->GetPID());
			} else {
				pos = ComboBox_AddString(dev_list, devName.c_str());
				ComboBox_SetItemData(dev_list, pos, fxhl->devs[i]->GetPID());
			}
			// need to add device mapping...
			AlienFX_SDK::devmap newDev;
			newDev.vid = fxhl->devs[i]->GetVid();
			newDev.devid = fxhl->devs[i]->GetPID();
			newDev.name = devName;
			fxhl->afx_dev.GetDevices()->push_back(newDev);
			fxhl->afx_dev.SaveMappings();
		}
		if (fxhl->devs[i]->GetPID() == eDid) {
			// select this device.
			if (isList)
				ListBox_SetCurSel(dev_list, pos);
			else
				ComboBox_SetCurSel(dev_list, pos);
			eDid = fxhl->devs[i]->GetPID(); 
			dItem = pos;
		}
	}

	eLid = -1;

	if (fxhl->devs.size()) {
		if (!isList) {
			if (eDid == -1) { // no selection, let's try to select dev#0
				dItem = 0;
				eDid = fxhl->devs[0]->GetPID();
				ComboBox_SetCurSel(dev_list, 0);
			}
			fxhl->TestLight(eDid, -1);
			UpdateLightsList(hDlg, eDid, -1);
		}
	}
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
			MessageBox(
				NULL,
				"There is no light names defined. Do you want to detect it?",
				"Warning",
				MB_ICONQUESTION | MB_YESNO
			) == IDYES) {
			DialogBox(hInst,
					  MAKEINTRESOURCE(IDD_DIALOG_AUTODETECT),
					  hDlg,
					  (DLGPROC) DetectionDialog);
		}
		UpdateDeviceList(hDlg);
	} break;
	case WM_COMMAND: {
		int dbItem = ComboBox_GetCurSel(dev_list);
		int did = (int)ComboBox_GetItemData(dev_list, dbItem);
		switch (LOWORD(wParam))
		{
		case IDC_DEVICES:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				eLid = -1;
				eDid = did; dItem = dbItem;
				fxhl->TestLight(eDid, -1);
				UpdateLightsList(hDlg, eDid, -1);
			} break;
			case CBN_EDITCHANGE:
				char buffer[MAX_PATH];
				GetWindowTextA(dev_list, buffer, MAX_PATH);
				UINT i;
				for (i = 0; i < fxhl->afx_dev.GetDevices()->size(); i++) {
					if (fxhl->afx_dev.GetDevices()->at(i).devid == eDid) {
						fxhl->afx_dev.GetDevices()->at(i).name = buffer;
						fxhl->afx_dev.SaveMappings();
						ComboBox_DeleteString(dev_list, dItem);
						ComboBox_InsertString(dev_list, dItem, buffer);
						ComboBox_SetItemData(dev_list, dItem, eDid);
						break;
					}
				}
				break;
			}
		} break;
		case IDC_BUTTON_ADDL: {
			int cid = GetDlgItemInt(hDlg, IDC_LIGHTID, NULL, false);
			// let's check if we have the same ID, need to use max+1 in this case
			unsigned maxID = 0;
			size_t lights = fxhl->afx_dev.GetMappings()->size();
			for (int i = 0; i < lights; i++) {
				AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(i);
				if (lgh->devid == eDid) {
					if (lgh->lightid > maxID)
						maxID = lgh->lightid;
					if (lgh->lightid == cid)
						cid = maxID + 1;
				}
			}
			AlienFX_SDK::mapping *dev = new AlienFX_SDK::mapping({0,(DWORD)eDid,(DWORD)cid,0,"Light #" + to_string(cid)});
			fxhl->afx_dev.GetMappings()->push_back(dev);
			fxhl->afx_dev.SaveMappings();
			eLid = cid;
			UpdateLightsList(hDlg, eDid, cid);
			fxhl->TestLight(eDid, cid);
		} break;
		case IDC_BUTTON_REML:
			if (MessageBox(hDlg, "Do you really want to remove current light name and all it's settings from all groups and profiles?", "Warning!",
						   MB_YESNO | MB_ICONWARNING) == IDYES) {
				// delete from all groups...
				for (int i = 0; i < fxhl->afx_dev.GetGroups()->size(); i++) {
					AlienFX_SDK::group* grp = &fxhl->afx_dev.GetGroups()->at(i);
					for (vector<AlienFX_SDK::mapping*>::iterator gIter = grp->lights.begin();
						 gIter < grp->lights.end(); gIter++)
						if ((*gIter)->devid == eDid && (*gIter)->lightid == eLid) {
							grp->lights.erase(gIter);
							break;
						}
				}
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					 Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, eDid, eLid);
				}
				std::vector <AlienFX_SDK::mapping*>* mapps = fxhl->afx_dev.GetMappings();
				int nLid = -1;
				for (std::vector <AlienFX_SDK::mapping*>::iterator Iter = mapps->begin();
					 Iter != mapps->end(); Iter++)
					if ((*Iter)->devid == eDid && (*Iter)->lightid == eLid) {
						delete *Iter;
						mapps->erase(Iter);
						break;
					} else nLid = (*Iter)->lightid;
				fxhl->afx_dev.SaveMappings();
				conf->Save();
				if (IsDlgButtonChecked(hDlg, IDC_ISPOWERBUTTON) == BST_CHECKED) {
					fxhl->ResetPower(did);
					MessageBox(hDlg, "Hardware Power button removed, you may need to reset light system!", "Warning!",
							   MB_OK);
				}
				eLid = nLid;
				UpdateLightsList(hDlg, eDid, eLid);
				fxhl->TestLight(eDid, eLid);
			}
			break;
		case IDC_BUTTON_RESETCOLOR:
			if (MessageBox(hDlg, "Do you really want to remove current light control settings from all profiles?", "Warning!",
						   MB_YESNO | MB_ICONWARNING) == IDYES) {
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					 Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, eDid, eLid);
				}
			}
			break;
		case IDC_BUTTON_TESTCOLOR: {
			SetColor(hDlg, IDC_BUTTON_TESTCOLOR, &conf->testColor.r, &conf->testColor.g, &conf->testColor.b);
			if (eLid != -1) {
				fxhl->TestLight(did, eLid);
				SetFocus(light_view);
			}
		} break;
		case IDC_ISPOWERBUTTON:
			if (eLid != -1) {
				int flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED ;
				if (flags)
					if (MessageBox(hDlg, "Setting light to Hardware Power button slow down updates and can hang you light system! Are you sure?", "Warning!",
								   MB_YESNO | MB_ICONWARNING) == IDYES) {
						flags = fxhl->afx_dev.GetFlags(did, eLid) & 0x2 | flags;
						fxhl->afx_dev.SetFlags(did, eLid, flags);
					} else
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				else {
					// remove power button config from chip config if unchecked and confirmed
					if (MessageBox(hDlg, "Hardware Power button disabled, you may need to reset light system! Do you want to reset Power button light as well?", "Warning!",
								   MB_YESNO | MB_ICONWARNING) == IDYES)
						fxhl->ResetPower(did);
					flags = fxhl->afx_dev.GetFlags(did, eLid) & 0x2 | flags;
					fxhl->afx_dev.SetFlags(did, eLid, flags);
				}
			}
			break;
		case IDC_CHECK_INDICATOR:
		{
			int flags = (fxhl->afx_dev.GetFlags(did, eLid) & 0x1) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 1;
			fxhl->afx_dev.SetFlags(did, eLid, flags);
		} break;
		case IDC_BUTTON_DEVRESET:
		{
			fxhl->FillAllDevs(acpi);
			AlienFX_SDK::Functions* dev = fxhl->LocateDev(eDid);
			if (dev)
				UpdateLightsList(hDlg, eDid, eLid);
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
			OPENFILENAMEA fstruct = {0};
			string appName = "Default.csv";
			appName.reserve(4096);
			fstruct.lStructSize = sizeof(OPENFILENAMEA);
			fstruct.hwndOwner = hDlg;
			fstruct.hInstance = hInst;
			fstruct.lpstrFile = (LPSTR) appName.c_str();
			fstruct.nMaxFile = 4096;
			fstruct.lpstrInitialDir = ".\\Mappings";
			fstruct.lpstrFilter = "Mapping files (*.csv)\0*.csv\0\0";
			fstruct.lpstrCustomFilter = NULL;
			fstruct.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
			if (GetOpenFileName(&fstruct)) {
				// Now load mappings...
				HANDLE file = CreateFile(
					appName.c_str(), 
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
					AlienFX_SDK::devmap tDev = {(DWORD) 0, (DWORD) 0, string("")};
					while ((linePos = content.find("\r\n", oldLinePos)) != string::npos) {
						vector<string> fields;
						size_t pos = 0, posOld = 1;
						line = content.substr(oldLinePos, linePos - oldLinePos);
						oldLinePos = linePos + 2;
						if (!line.empty()) {
							while ((pos = line.find("','", posOld)) != string::npos) {
								fields.push_back(line.substr(posOld, pos - posOld));
								//pos+=3;
								posOld = pos + 3;
							}
							fields.push_back(line.substr(posOld, line.size() - posOld - 1));
							switch (atoi(fields[0].c_str())) {
							case 0: // device line
								tDev.vid = atol(fields[1].c_str());
								tDev.devid = atol(fields[2].c_str());
								tDev.name = fields[3];
								// add to devs.
								if (!fxhl->afx_dev.GetDeviceById(tDev.devid, tDev.vid))
									fxhl->afx_dev.GetDevices()->push_back(tDev);
								else {
									fxhl->afx_dev.GetDeviceById(tDev.devid, tDev.vid)->vid = tDev.vid;
									fxhl->afx_dev.GetDeviceById(tDev.devid, tDev.vid)->name = tDev.name;
								}
								break;
							case 1: // lights line
								if (tDev.devid) {
									DWORD lightid = atol(fields[1].c_str()),
										flags = atol(fields[2].c_str());
									// add to maps
									AlienFX_SDK::mapping* oMap = fxhl->afx_dev.GetMappingById(tDev.devid, lightid);
									if (oMap) {
										oMap->vid = tDev.vid;
										oMap->name = fields[3];
										oMap->flags = flags;
									} else {
										AlienFX_SDK::mapping *tMap = new AlienFX_SDK::mapping({
											tDev.vid, tDev.devid, lightid, flags, fields[3]
										});
										fxhl->afx_dev.GetMappings()->push_back(tMap);
									}
								}
								break;
								//default: // wrong line, skip
							}
						}
					}
					// reload lists...
					CloseHandle(file);
					UpdateDeviceList(hDlg);
					UpdateLightsList(hDlg, eDid, -1);
				}
			}
		} break;
		case IDC_BUT_SAVEMAP:
		{
			// Save device and ligh mappings
			OPENFILENAMEA fstruct = {0};
			string appName = "Current.csv";
			appName.reserve(4096);
			fstruct.lStructSize = sizeof(OPENFILENAMEA);
			fstruct.hwndOwner = hDlg;
			fstruct.hInstance = hInst;
			fstruct.lpstrFile = (LPSTR) appName.c_str();
			fstruct.lpstrInitialDir = ".\\Mappings";
			fstruct.nMaxFile = 4096;
			fstruct.nFilterIndex= 1;
			fstruct.lpstrFilter = "Mapping files (*.csv)\0*.csv\0\0";
			fstruct.lpstrCustomFilter = NULL;
			fstruct.Flags = OFN_ENABLESIZING | OFN_LONGNAMES | OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST;
			if (GetSaveFileName(&fstruct)) {
				// Now save mappings...
				HANDLE file = CreateFile(appName.c_str(),
										 GENERIC_WRITE,
										 0,
										 NULL,
										 CREATE_ALWAYS,
										 0,
										 NULL );
				if (file != INVALID_HANDLE_VALUE) {
					for (int i = 0; i < fxhl->afx_dev.GetDevices()->size(); i++) {
						AlienFX_SDK::devmap* cDev = &fxhl->afx_dev.GetDevices()->at(i);
						/// Only connected devices stored!
						AlienFX_SDK::Functions* dev = NULL;
						if (dev = fxhl->LocateDev(cDev->devid)) {
							DWORD writeBytes;
							string line = "'0','" + to_string(dev->GetVid()) + "','" 
								+ to_string(dev->GetPID()) + "','" + cDev->name + "'\r\n";
							WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
							for (int j = 0; j < fxhl->afx_dev.GetMappings()->size(); j++) {
								AlienFX_SDK::mapping* cMap = fxhl->afx_dev.GetMappings()->at(j);
								if (cMap->devid == dev->GetPID()) {
									line = "'1','" + to_string(cMap->lightid) + "','"
										+ to_string(cMap->flags) + "','" + cMap->name + "'\r\n";
									WriteFile(file, line.c_str(), (DWORD)line.size(), &writeBytes, NULL);
								}
							}
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
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev.GetFlags(eDid, eLid) & ALIENFX_FLAG_POWER ? BST_CHECKED : BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, fxhl->afx_dev.GetFlags(eDid, eLid) & ALIENFX_FLAG_INACTIVE ? BST_CHECKED : BST_UNCHECKED);
					} else {
						SetDlgItemInt(hDlg, IDC_LIGHTID, 0, false);
						CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_INDICATOR, BST_UNCHECKED);
						eLid = -1;
					}
					fxhl->TestLight(eDid, eLid);
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				if (sItem->item.pszText) {
					for (UINT i = 0; i < fxhl->afx_dev.GetMappings()->size(); i++) {
						AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(i);
						if (lgh->devid == eDid &&
							lgh->lightid == sItem->item.lParam) {
							lgh->name = sItem->item.pszText;
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
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_TESTCOLOR:
			RedrawButton(hDlg, IDC_BUTTON_TESTCOLOR, conf->testColor.r, conf->testColor.g, conf->testColor.b);
			break;
		}
		break;
	case WM_DESTROY:
	{
		fxhl->UnblockUpdates(true, true);
		fxhl->RefreshState();
	} break;
	case WM_SIZE:
		fxhl->UnblockUpdates(false, true);
	default: return false;
	}
	return true;
}

struct devInfo {
	AlienFX_SDK::devmap dev;
	vector<AlienFX_SDK::mapping> maps;
	bool selected = false;
};

vector<devInfo> csv_devs;

void UpdateSavedDevices(HWND hDlg) {
	HWND dev_list = GetDlgItem(hDlg, IDC_LIST_SUG);
	// Now check current device list..
	int pos = (-1);
	ListBox_ResetContent(dev_list);
	pos = ListBox_AddString(dev_list, "Manual");
	ListBox_SetCurSel(dev_list, pos);
	ListBox_SetItemData(dev_list, pos, -1);
	for (UINT i = 0; i < csv_devs.size(); i++) {
		if (csv_devs[i].dev.devid == eDid) {
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
		csv_devs.clear();
		// load csv...
		// Load mappings...
		HANDLE file = CreateFile(
			".\\Mappings\\devices.csv", 
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
						//pos+=3;
						posOld = pos + 3;
					}
					fields.push_back(line.substr(posOld, line.size() - posOld - 1));
					switch (atoi(fields[0].c_str())) {
					case 0: // device line
						tDev.dev.vid = atol(fields[1].c_str());
						tDev.dev.devid = atol(fields[2].c_str());
						tDev.dev.name = fields[3];
						// add to devs.
						if (fxhl->afx_dev.GetDeviceById(tDev.dev.devid, tDev.dev.vid))
							csv_devs.push_back(tDev);
						else
							tDev.dev.devid = 0;
						break;
					case 1: // lights line
						if (tDev.dev.devid) {
							DWORD lightid = atol(fields[1].c_str()),
								flags = atol(fields[2].c_str());
							// add to maps
							csv_devs.back().maps.push_back({tDev.dev.vid, tDev.dev.devid, lightid, flags, fields[3]});
						}
						break;
						//default: // wrong line, skip
					}
				}
			}
			// reload lists...
			CloseHandle(file);
		}
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
			for (UINT i = 0; i < csv_devs.size(); i++) {
				if (csv_devs[i].selected) {
					AlienFX_SDK::devmap *cDev = fxhl->afx_dev.GetDeviceById(csv_devs[i].dev.devid, csv_devs[i].dev.vid);
					if (cDev)
						cDev->name = csv_devs[i].dev.name;
					for (int j = 0; j < csv_devs[i].maps.size(); j++) {
						AlienFX_SDK::mapping *tDev = &csv_devs[i].maps[j];
						AlienFX_SDK::mapping *oMap = fxhl->afx_dev.GetMappingById(tDev->devid, tDev->lightid);
						if (oMap) {
							oMap->vid = tDev->vid;
							oMap->name = tDev->name;
							oMap->flags = tDev->flags;
						} else {
							fxhl->afx_dev.GetMappings()->push_back(tDev);
						}
					}
				}
			}
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
				eDid = cDevID;
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
					if (csv_devs[i].dev.devid == eDid) {
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