#include "ProfilesDialog.h"
#include "resource.h"
#include "FXHelper.h"
#include "ConfigHandler.h"
#include "EventHandler.h"
#include "toolkit.h"
#include <windowsx.h>

bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);
void ReloadProfileList(HWND hDlg);

extern FXHelper* fxhl;
extern ConfigHandler* conf;
extern EventHandler* eve;

int pCid = -1;

void ReloadProfileView(HWND hDlg, int cID) {
	int rpos = 0;
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		profile_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);
	ListView_DeleteAllItems(profile_list);
	ListView_SetExtendedListViewStyle(profile_list, LVS_EX_FULLROWSELECT);
	LVCOLUMNA lCol;
	lCol.mask = LVCF_WIDTH;
	lCol.cx = 100;
	ListView_DeleteColumn(profile_list, 0);
	ListView_InsertColumn(profile_list, 0, &lCol);
	for (int i = 0; i < conf->profiles.size(); i++) {
		LVITEMA lItem; 
		lItem.mask = LVIF_TEXT | LVIF_PARAM;
		lItem.iItem = i;
		lItem.iImage = 0;
		lItem.iSubItem = 0;
		lItem.lParam = conf->profiles[i].id;
		lItem.pszText = (char*)conf->profiles[i].name.c_str();
		if (conf->profiles[i].id == cID) {
			lItem.mask |= LVIF_STATE;
			lItem.state = LVIS_SELECTED;
			CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, conf->profiles[i].flags & PROF_DEFAULT ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_CHECK_NOMON, conf->profiles[i].flags & PROF_NOMONITORING ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, conf->profiles[i].flags & PROF_DIMMED ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, conf->profiles[i].flags & PROF_ACTIVE ? BST_CHECKED : BST_UNCHECKED);
			SendMessage(app_list, LB_RESETCONTENT, 0, 0);
			SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM)(conf->profiles[i].triggerapp.c_str()));
			rpos = i;
		}
		ListView_InsertItem(profile_list, &lItem);
	}
	ListView_SetColumnWidth(profile_list, 0, LVSCW_AUTOSIZE);
	ListView_EnsureVisible(profile_list, rpos, false);
}

BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		p_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		ReloadProfileView(hDlg, conf->activeProfile);
		pCid = conf->activeProfile;
	} break;
	case WM_COMMAND:
	{
		profile* prof = conf->FindProfile(pCid);

		switch (LOWORD(wParam))
		{
		case IDC_ADDPROFILE: {
			char buf[128]; unsigned vacID = 0;
			for (int i = 0; i < conf->profiles.size(); i++)
				if (vacID == conf->profiles[i].id) {
					vacID++; i = -1;
				}
			sprintf_s(buf, 128, "Profile %d", vacID);
			profile prof = {vacID, 0, "", buf};
			conf->profiles.push_back(prof);
			conf->profiles.back().lightsets = *conf->active_set;
			ReloadProfileView(hDlg, vacID);
			pCid = vacID;
			ReloadProfileList(NULL);
		} break;
		case IDC_REMOVEPROFILE: {
			if (prof != NULL && !(prof->flags & PROF_DEFAULT) && conf->profiles.size() > 1) {
				if (MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning!",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					// is this active profile? Switch needed!
					if (conf->activeProfile == pCid) {
						// switch to default profile..
						eve->SwitchActiveProfile(conf->defaultProfile);
					}
					// Now remove profile....
					int nCid = conf->activeProfile;
					for (std::vector <profile>::iterator Iter = conf->profiles.begin();
						 Iter != conf->profiles.end(); Iter++)
						if (Iter->id == pCid) { //prid) {
							conf->profiles.erase(Iter);
							break;
						} else
							nCid = Iter->id;
						conf->active_set = &(conf->FindProfile(conf->activeProfile)->lightsets);
						ReloadProfileView(hDlg, nCid);
						pCid = nCid;
						ReloadProfileList(NULL);
				}
			}
			else
				MessageBox(hDlg, "Can't delete last or default profile!", "Error!",
						   MB_OK | MB_ICONERROR);
		} break;
		case IDC_BUT_PROFRESET:
			if (prof != NULL && MessageBox(hDlg, "Do you really want to reset all lights settings for this profile?", "Warning!",
										   MB_YESNO | MB_ICONWARNING) == IDYES) {
				prof->lightsets.clear();
				//if (prof->id == conf->activeProfile) {
				//	conf->active_set.clear();
				//}
			}
			break;
		case IDC_APP_RESET:
			if (prof != NULL) {
				prof->triggerapp = "";
				SendMessage(app_list, LB_RESETCONTENT, 0, 0);
			}
			break;
		case IDC_APP_BROWSE: {
			if (prof != NULL) {
				// fileopen dialogue...
				OPENFILENAMEA fstruct;
				char* appName = new char[32767];
				strcpy_s(appName, MAX_PATH, prof->triggerapp.c_str());
				ZeroMemory(&fstruct, sizeof(OPENFILENAMEA));
				fstruct.lStructSize = sizeof(OPENFILENAMEA);
				fstruct.hwndOwner = hDlg;
				fstruct.hInstance = hInst;
				fstruct.lpstrFile = appName;
				fstruct.nMaxFile = 32767;
				fstruct.lpstrFilter = "Applications (*.exe)\0*.exe\0\0";
				fstruct.lpstrCustomFilter = NULL;
				fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
				if (GetOpenFileNameA(&fstruct)) {
					prof->triggerapp = appName;
					SendMessage(app_list, LB_RESETCONTENT, 0, 0);
					SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM)(prof->triggerapp.c_str()));
				}
				delete[] appName;
			}
		} break;
		case IDC_CHECK_DEFPROFILE:
			if (prof != NULL) {
				bool nflags = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				if (nflags) {
					profile* old_def = conf->FindProfile(conf->defaultProfile);
					if (old_def != NULL)
						old_def->flags = old_def->flags & ~PROF_DEFAULT;
					prof->flags = prof->flags | PROF_DEFAULT;
					conf->defaultProfile = prof->id;
				}
				else
					CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_CHECKED);
			}
			break;
		case IDC_CHECK_NOMON:
			if (prof != NULL) {
				prof->flags = (prof->flags & ~PROF_NOMONITORING) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 1;
				if (prof->id == conf->activeProfile) {
					DWORD oldState = conf->monState;
					conf->monState = prof->flags & PROF_NOMONITORING ? 0 : conf->enableMon;
					if (oldState != conf->monState)
						eve->ToggleEvents();
				}
			}
			break;
		case IDC_CHECK_PROFDIM:
			if (prof != NULL) {
				prof->flags = (prof->flags & ~PROF_DIMMED) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 2;
				if (prof->id == conf->activeProfile) {
					conf->stateDimmed = prof->flags & PROF_DIMMED;
					fxhl->RefreshState();
				}
			}
			break;
		case IDC_CHECK_FOREGROUND:
			if (prof != NULL) {
				prof->flags = (prof->flags & ~PROF_ACTIVE) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 3;
			}
			break;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_PROFILES:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE: {
				NMITEMACTIVATE* sItem = (NMITEMACTIVATE*) lParam;
				ListView_EditLabel(p_list, sItem->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & LVIS_FOCUSED) {
					// Select other item...
					if (lPoint->iItem != -1) {
						pCid = (int) lPoint->lParam;
						profile* prof = conf->FindProfile(pCid);
						if (prof) {
							CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof->flags & PROF_DEFAULT ? BST_CHECKED : BST_UNCHECKED);
							CheckDlgButton(hDlg, IDC_CHECK_NOMON, prof->flags & PROF_NOMONITORING ? BST_CHECKED : BST_UNCHECKED);
							CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof->flags & PROF_DIMMED ? BST_CHECKED : BST_UNCHECKED);
							CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, prof->flags & PROF_ACTIVE ? BST_CHECKED : BST_UNCHECKED);
							SendMessage(app_list, LB_RESETCONTENT, 0, 0);
							SendMessage(app_list, LB_ADDSTRING, 0, (LPARAM) (prof->triggerapp.c_str()));
						}
					} else {
						pCid = -1;
						CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_NOMON, BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, BST_UNCHECKED);
						CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, BST_UNCHECKED);
						SendMessage(app_list, LB_RESETCONTENT, 0, 0);
					}
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				profile* prof = conf->FindProfile((int)sItem->item.lParam);
				if (prof && sItem->item.pszText) {
					prof->name = sItem->item.pszText;
					ListView_SetItem(p_list, &sItem->item);
					ListView_SetColumnWidth(p_list, 0, LVSCW_AUTOSIZE);
					ReloadProfileList(NULL);
					return true;
				} else 
					return false;
			} break;

			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}