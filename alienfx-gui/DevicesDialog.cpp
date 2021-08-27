#include "alienfx-gui.h"
#include <windowsx.h>
#include <fstream>

bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map);
bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);

int eLid = -1, eDid = -1, dItem = -1;

void UpdateLightsList(HWND hDlg, int pid, int lid) {
	int pos = -1;
	size_t lights = fxhl->afx_dev.GetMappings()->size();
	HWND light_list = GetDlgItem(hDlg, IDC_LIST_LIGHTS); 

	ListView_DeleteAllItems(light_list);
	ListView_SetExtendedListViewStyle(light_list, LVS_EX_FULLROWSELECT);
	LVCOLUMNA lCol;
	lCol.mask = LVCF_WIDTH;
	lCol.cx = 100;
	ListView_DeleteColumn(light_list, 0);
	ListView_InsertColumn(light_list, 0, &lCol);
	for (int i = 0; i < lights; i++) {
		AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(i);
		if (pid == lgh.devid) { // && fxhl->LocateDev(lgh.devid)) {
			LVITEMA lItem; 
			lItem.mask = LVIF_TEXT | LVIF_PARAM;
			lItem.iItem = i;
			lItem.iImage = 0;
			lItem.iSubItem = 0;
			lItem.lParam = lgh.lightid;
			lItem.pszText = (char*)lgh.name.c_str();
			if (lid == lgh.lightid) {
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

void UpdateDeviceList(HWND hDlg) {
	HWND dev_list = GetDlgItem(hDlg, IDC_DEVICES);
	//size_t lights = fxhl->afx_dev.GetMappings()->size();
	// Now check current device list..
	int pos = (-1);
	ComboBox_ResetContent(dev_list);
	for (UINT i = 0; i < fxhl->devs.size(); i++) {
		//cpid = fxhl->devs[i]->GetPID();
		AlienFX_SDK::devmap* cDev = fxhl->afx_dev.GetDeviceById(fxhl->devs[i]->GetPID(), fxhl->devs[i]->GetVid());
		if (cDev) {
			if (!cDev->vid)
				cDev->vid = fxhl->devs[i]->GetVid();
			pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(cDev->name.c_str()));
			SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)cDev->devid);
		} else {
			// no name
			string devName = "Device #" + to_string(fxhl->devs[i]->GetVid()) + "_" + to_string(fxhl->devs[i]->GetPID());
			pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(devName.c_str()));
			SendMessage(dev_list, CB_SETITEMDATA, pos, (LPARAM)fxhl->devs[i]->GetPID());
			// need to add device mapping...
			AlienFX_SDK::devmap newDev;
			newDev.vid = fxhl->devs[i]->GetVid();
			newDev.devid = fxhl->devs[i]->GetPID();
			newDev.name = devName;
			fxhl->afx_dev.GetDevices()->push_back(newDev);
			fxhl->afx_dev.SaveMappings();
		}
		if (fxhl->devs[i]->GetPID() == conf->lastActive) {
			// select this device.
			SendMessage(dev_list, CB_SETCURSEL, pos, (LPARAM)0);
			eDid = fxhl->devs[i]->GetPID(); dItem = pos;
		}
	}

	eLid = -1;

	if (fxhl->devs.size() > 0) {
		if (eDid == -1) { // no selection, let's try to select dev#0
			dItem = 0;
			eDid = fxhl->devs[0]->GetPID();
			SendMessage(dev_list, CB_SETCURSEL, 0, (LPARAM)0);
			conf->lastActive = eDid;
		}
		fxhl->TestLight(eDid, -1);
		UpdateLightsList(hDlg, eDid, -1);
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
		fxhl->FillDevs(conf->stateOn, conf->offPowerButton);
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
				conf->lastActive = did;
				fxhl->TestLight(did, -1);
				UpdateLightsList(hDlg, did, -1);
				eLid = -1;
				eDid = did; dItem = dbItem;
				switch (fxhl->LocateDev(eDid)->GetVersion()) {
				case 1: case 2: case 3:
					// unblock delay
					EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_TEMPO), true);
					break;
				case 5:
					// unblock effects and delay
					EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_TEMPO), true);
					EnableWindow(GetDlgItem(hDlg, IDC_GLOBAL_EFFECT), true);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), true);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), true);
					break;
				default:
					// block effects and delay
					EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_TEMPO), false);
					EnableWindow(GetDlgItem(hDlg, IDC_GLOBAL_EFFECT), false);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), false);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), false);
				}
			} break;
			case CBN_EDITCHANGE:
				char buffer[MAX_PATH];
				GetWindowTextA(dev_list, buffer, MAX_PATH);
				UINT i;
				for (i = 0; i < fxhl->afx_dev.GetDevices()->size(); i++) {
					if (fxhl->afx_dev.GetDevices()->at(i).devid == eDid) {
						fxhl->afx_dev.GetDevices()->at(i).name = buffer;
						fxhl->afx_dev.SaveMappings();
						SendMessage(dev_list, CB_DELETESTRING, dItem, 0);
						SendMessage(dev_list, CB_INSERTSTRING, dItem, (LPARAM) (buffer));
						SendMessage(dev_list, CB_SETITEMDATA, dItem, (LPARAM) eDid);
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
				AlienFX_SDK::mapping* lgh = &(fxhl->afx_dev.GetMappings()->at(i));
				if (lgh->devid == eDid) {
					if (lgh->lightid > maxID)
						maxID = lgh->lightid;
					if (lgh->lightid == cid)
						cid = maxID + 1;
				}
			}
			AlienFX_SDK::mapping dev;
			dev.devid = eDid;
			dev.lightid = cid;
			dev.name = "Light #" + to_string(cid); 
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
				std::vector <AlienFX_SDK::mapping>* mapps = fxhl->afx_dev.GetMappings();
				int nLid = -1;
				for (std::vector <AlienFX_SDK::mapping>::iterator Iter = mapps->begin();
					 Iter != mapps->end(); Iter++)
					if (Iter->devid == eDid && Iter->lightid == eLid) {
						mapps->erase(Iter);
						break;
					} else nLid = Iter->lightid;
				fxhl->afx_dev.SaveMappings();
				conf->Save();
				if (IsDlgButtonChecked(hDlg, IDC_ISPOWERBUTTON) == BST_CHECKED) {
					fxhl->ResetPower(did);
					MessageBox(hDlg, "Hardware Power button removed, you may need to reset light system!", "Warning!",
							   MB_OK);
					CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, BST_UNCHECKED);
				}
				if (nLid < 0) {
					SetDlgItemInt(hDlg, IDC_LIGHTID, 0, false);
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
			AlienFX_SDK::afx_act c;
			c.r = conf->testColor.cs.red;
			c.g = conf->testColor.cs.green;
			c.b = conf->testColor.cs.blue;
			SetColor(hDlg, IDC_BUTTON_TESTCOLOR, NULL, &c);
			conf->testColor.cs.red = c.r;
			conf->testColor.cs.green = c.g;
			conf->testColor.cs.blue = c.b;
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
				//fxhl->Refresh(true);
			}
			break;
		case IDC_CHECK_INDICATOR:
		{
			int flags = (fxhl->afx_dev.GetFlags(did, eLid) & 0x1) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 1;
			fxhl->afx_dev.SetFlags(did, eLid, flags);
		} break;
		case IDC_BUTTON_DEVRESET:
		{
			//fxhl->UnblockUpdates(false);
			fxhl->FillDevs(conf->stateOn, conf->offPowerButton);
			//fxhl->UnblockUpdates(true);
			AlienFX_SDK::Functions* dev = fxhl->LocateDev(eDid);
			if (dev)
				UpdateLightsList(hDlg, eDid, eLid);
		} break;
		case IDC_BUT_LOADMAP:
		{
			// Load device and light mappings
			OPENFILENAMEA fstruct = {0};
			string appName = ".\\Mappings\\Default.csv";
			appName.reserve(4096);
			fstruct.lStructSize = sizeof(OPENFILENAMEA);
			fstruct.hwndOwner = hDlg;
			fstruct.hInstance = hInst;
			fstruct.lpstrFile = (LPSTR) appName.c_str();
			fstruct.nMaxFile = 32767;
			fstruct.lpstrFilter = "Mapping files (*.csv)\0*.csv\0\0";
			fstruct.lpstrCustomFilter = NULL;
			fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
			if (GetOpenFileNameA(&fstruct)) {
				// Now load mappings...
				ifstream file(appName);
				if (file.good()) {
					// read and parse...
					string line;
					AlienFX_SDK::devmap tDev = {(DWORD) 0, (DWORD) 0, string("")};
					AlienFX_SDK::mapping tMap;
					while (!file.eof()) {
						vector<string> fields;
						size_t pos = 0, posOld = 1;
						getline(file, line);
						if (line != "") {
							while ((pos = line.find("','", pos)) != string::npos) {
								fields.push_back(line.substr(posOld, pos - posOld));
								pos+=3;
								posOld = pos;
							}
							fields.push_back(line.substr(posOld, line.size() - posOld - 1));
							switch (atoi(fields[0].c_str())) {
							case 0: // device line
								tDev.vid = atoi(fields[1].c_str());
								tDev.devid = atoi(fields[2].c_str());
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
									tMap.vid = tDev.vid;
									tMap.devid = tDev.devid;
									tMap.lightid = atoi(fields[1].c_str());
									tMap.flags = atoi(fields[2].c_str());
									tMap.name = fields[3];
									// add to maps
									AlienFX_SDK::mapping* oMap = fxhl->afx_dev.GetMappingById(tMap.devid, tMap.lightid);
									if (oMap) {
										oMap->vid = tMap.vid;
										oMap->name = tMap.name;
										oMap->flags = tMap.flags;
									} else {
										fxhl->afx_dev.GetMappings()->push_back(tMap);
									}
								}
								break;
								//default: // wrong line, skip
							}
						}
					}
					// reload lists...
					UpdateDeviceList(hDlg);
				}
				file.close();
			}
		} break;
		case IDC_BUT_SAVEMAP:
		{
			// Save device and ligh mappings
			OPENFILENAMEA fstruct = {0};
			string appName = ".\\Mappings\\Current.csv";
			appName.reserve(4096);
			fstruct.lStructSize = sizeof(OPENFILENAMEA);
			fstruct.hwndOwner = hDlg;
			fstruct.hInstance = hInst;
			fstruct.lpstrFile = (LPSTR) appName.c_str();
			fstruct.nMaxFile = 32767;
			fstruct.lpstrFilter = "Mapping files (*.csv)\0*.csv\0\0";
			fstruct.lpstrCustomFilter = NULL;
			fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
			if (GetSaveFileNameA(&fstruct)) {
				// Now save mappings...
				// appName
				ofstream file(appName);
				if (file.good()) {
					for (int i = 0; i < fxhl->afx_dev.GetDevices()->size(); i++) {
						AlienFX_SDK::devmap* cDev = &fxhl->afx_dev.GetDevices()->at(i);
						/// Only connected devices stored!
						AlienFX_SDK::Functions* dev = NULL;
						if (dev = fxhl->LocateDev(cDev->devid)) {
							file << "'0','" << dev->GetVid() << "','" << dev->GetPID() << "','" << cDev->name << "'" << endl;
							for (int j = 0; j < fxhl->afx_dev.GetMappings()->size(); j++) {
								AlienFX_SDK::mapping* cMap = &fxhl->afx_dev.GetMappings()->at(j);
								if (cMap->devid == dev->GetPID()) {
									file << "'1','" << cMap->lightid << "','" << cMap->flags << "','" << cMap->name << "'" << endl;
								}
							}
						}
					}
				}
				file.close();
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
				//inLightEdit = true;
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
						//fxhl->UnblockUpdates(false);
						// highlight to check....
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
				//inLightEdit = false;
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				if (sItem->item.pszText) {
					for (UINT i = 0; i < fxhl->afx_dev.GetMappings()->size(); i++) {
						AlienFX_SDK::mapping* lgh = &(fxhl->afx_dev.GetMappings()->at(i));
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
				SetDlgItemInt(hDlg, IDC_LIGHTID, (UINT) sItem->item.lParam, false);
				CheckDlgButton(hDlg, IDC_ISPOWERBUTTON, fxhl->afx_dev.GetFlags(eDid, (int) sItem->item.lParam) ? BST_CHECKED : BST_UNCHECKED);
				return false;
			} break;
			}
			break;
		}
		break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*)lParam)->CtlID) {
		case IDC_BUTTON_TESTCOLOR:
			RedrawButton(hDlg, IDC_BUTTON_TESTCOLOR, conf->testColor.cs.red, conf->testColor.cs.green, conf->testColor.cs.blue);
			break;
		}
		break;
	case WM_CLOSE: case WM_DESTROY:
	{
		//if (!fxhl->unblockUpdates) {
			fxhl->UnblockUpdates(true, true);
			fxhl->RefreshState();
		//}
	} break;
	case WM_SIZE:
		//if (fxhl->unblockUpdates)
			fxhl->UnblockUpdates(false, true);
		break;
	default: return false;
	}
	return true;
}