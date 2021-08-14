#include "GroupsDialog.h"
#include "resource.h"
#include "FXHelper.h"
#include "ConfigHandler.h"
#include "toolkit.h"
#include <windowsx.h>

bool RemoveMapping(std::vector<lightset>* lightsets, int did, int lid);

extern FXHelper* fxhl;
extern ConfigHandler* conf;

int	gLid = -1, gItem = -1;

int UpdateLightListG(HWND light_list, AlienFX_SDK::group* grp) {
	int pos = -1;
	size_t lights = fxhl->afx_dev.GetMappings()->size();
	SendMessage(light_list, LB_RESETCONTENT, 0, 0);
	for (int i = 0; i < lights; i++) {
		AlienFX_SDK::mapping lgh = fxhl->afx_dev.GetMappings()->at(i);
		if (fxhl->LocateDev(lgh.devid)) {
			if (grp) {
				int gl = 0;
				for (gl=0; gl < grp->lights.size(); gl++) {
					if (grp->lights.at(gl)->devid == lgh.devid &&
						grp->lights.at(gl)->lightid == lgh.lightid)
						break;
				}
				if (gl == grp->lights.size()) {
					pos = ListBox_AddString(light_list, lgh.name.c_str());
					ListBox_SetItemData(light_list, pos, i);
				}
			} else {
				pos = ListBox_AddString(light_list, lgh.name.c_str());
				ListBox_SetItemData(light_list, pos, i);
			}
		}
	}
	RedrawWindow(light_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
	return pos;
}

int UpdateGroupLights(HWND light_list, int gID, int sel) {
	int pos = -1;
	SendMessage(light_list, LB_RESETCONTENT, 0, 0);
	AlienFX_SDK::group* grp = fxhl->afx_dev.GetGroupById(gID);
	if (grp) {
		for (int i = 0; i < grp->lights.size(); i++) {
			pos = (int) SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM) (grp->lights[i]->name.c_str()));
			SendMessage(light_list, LB_SETITEMDATA, pos, i);
		}
		SendMessage(light_list, LB_SETCURSEL, sel, 0);
	}
	return pos;
}

BOOL TabGroupsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS),
		groups_list = GetDlgItem(hDlg, IDC_GROUPS),
		glights_list = GetDlgItem(hDlg, IDC_LIST_INGROUP);

	switch (message) {
	case WM_INITDIALOG:
	{
		int pos = -1;
		AlienFX_SDK::group* grp = NULL;
		size_t numgroups = fxhl->afx_dev.GetGroups()->size();
		if (numgroups > 0) {
			if (gLid < 0)
				gLid = fxhl->afx_dev.GetGroups()->at(0).gid;
			for (UINT i = 0; i < numgroups; i++) {
				pos = (int) SendMessage(groups_list, CB_ADDSTRING, 0, (LPARAM) (fxhl->afx_dev.GetGroups()->at(i).name.c_str()));
				SendMessage(groups_list, CB_SETITEMDATA, pos, (LPARAM) fxhl->afx_dev.GetGroups()->at(i).gid);
				if (fxhl->afx_dev.GetGroups()->at(i).gid == gLid) {
					SendMessage(groups_list, CB_SETCURSEL, pos, (LPARAM) 0);
					gItem = pos;
					grp = &fxhl->afx_dev.GetGroups()->at(i);
				}
			}
			UpdateGroupLights(glights_list, gLid, 0);
		} else {
			EnableWindow(groups_list, false);
			EnableWindow(glights_list, false);
		}
		UpdateLightListG(light_list, grp);
	} break;
	case WM_COMMAND:
	{
		int gbItem = (int)SendMessage(groups_list, CB_GETCURSEL, 0, 0);
		int gid = (int)SendMessage(groups_list, CB_GETITEMDATA, gbItem, 0);
		int glItem = (int)SendMessage(glights_list, LB_GETCURSEL, 0, 0);
		AlienFX_SDK::group* grp = fxhl->afx_dev.GetGroupById(gLid);
		switch (LOWORD(wParam)) {
		case IDC_GROUPS: {
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE: {
				gLid = gid; gItem = gbItem;
				UpdateGroupLights(glights_list, gid,0);
				UpdateLightListG(light_list, grp);
			} break;
			case CBN_EDITCHANGE:
				char buffer[MAX_PATH];
				GetWindowTextA(groups_list, buffer, MAX_PATH);
				AlienFX_SDK::group* cgrp = fxhl->afx_dev.GetGroupById(gLid);
				if (cgrp) {
					cgrp->name = buffer;
					fxhl->afx_dev.SaveMappings();
					SendMessage(groups_list, CB_DELETESTRING, gItem, 0);
					SendMessage(groups_list, CB_INSERTSTRING, gItem, (LPARAM)(buffer));
					SendMessage(groups_list, CB_SETITEMDATA, gItem, (LPARAM)gLid);
				}
				break;
			}
		} break;
		case IDC_BUT_GRP_UP:
			if (grp && glItem > 0) {
				auto gIter = grp->lights.begin();
				iter_swap(gIter + glItem, gIter + glItem -1);
				UpdateGroupLights(glights_list,gLid, glItem-1);
			}
			break;
		case IDC_BUT_GRP_DOWN:
			if (grp && glItem < grp->lights.size() - 1 ) {
				auto gIter = grp->lights.begin();
				iter_swap(gIter + glItem, gIter + glItem +1);
				UpdateGroupLights(glights_list,gLid, glItem+1);
			}
			break;
		case IDC_BUTTON_ADDG: {
			char buffer[MAX_PATH];
			unsigned maxID = 0x10000;
			size_t lights = fxhl->afx_dev.GetGroups()->size();
			for (int i = 0; i < lights; i++) {
				AlienFX_SDK::group* lgh = &(fxhl->afx_dev.GetGroups()->at(i));
				if (lgh->gid >= maxID)
					maxID = lgh->gid + 1;
			}
			AlienFX_SDK::group dev;
			dev.gid = maxID;
			sprintf_s(buffer, MAX_PATH, "Group #%d", maxID & 0xffff);
			dev.name = buffer;
			fxhl->afx_dev.GetGroups()->push_back(dev);
			fxhl->afx_dev.SaveMappings();
			gLid = maxID;
			int pos = (int) SendMessage(groups_list, CB_ADDSTRING, 0, (LPARAM)(buffer));
			SendMessage(groups_list, CB_SETITEMDATA, pos, (LPARAM)gLid);
			SendMessage(groups_list, CB_SETCURSEL, pos, (LPARAM) 0);
			gItem = pos;
			EnableWindow(groups_list, true);
			EnableWindow(glights_list, true);
			UpdateGroupLights(glights_list,gLid,0);
			grp = &fxhl->afx_dev.GetGroups()->back();
			UpdateLightListG(light_list, grp);
		} break;
		case IDC_BUTTON_REMG: {
			if (gLid > 0) {
				for (std::vector <AlienFX_SDK::group>::iterator Iter = fxhl->afx_dev.GetGroups()->begin();
					 Iter != fxhl->afx_dev.GetGroups()->end(); Iter++)
					if (Iter->gid == gLid) {
						fxhl->afx_dev.GetGroups()->erase(Iter);
						break;
					}
				// delete from all profiles...
				for (std::vector <profile>::iterator Iter = conf->profiles.begin();
					 Iter != conf->profiles.end(); Iter++) {
					// erase mappings
					RemoveMapping(&Iter->lightsets, 0, gLid);
				}
				fxhl->afx_dev.SaveMappings();
				conf->Save();
				SendMessage(groups_list, CB_DELETESTRING, gItem, 0);
				if (fxhl->afx_dev.GetGroups()->size() > 0) {
					gLid = fxhl->afx_dev.GetGroups()->at(0).gid;
					gItem = 0;
					grp = &fxhl->afx_dev.GetGroups()->front();
					UpdateLightListG(light_list, grp);
				} else {
					gLid = -1;
					gItem = -1;
					EnableWindow(groups_list, false);
					EnableWindow(glights_list, false);
				}
				SendMessage(groups_list, CB_SETCURSEL, 0, 0);
				UpdateGroupLights(glights_list, gLid,0);
			}
		} break;
		case IDC_BUT_ADDTOG:
		{
			int numSelLights = ListBox_GetSelCount(light_list);
			if (grp && numSelLights > 0) {
				int* selLights = new int[numSelLights];
				ListBox_GetSelItems(light_list, numSelLights, selLights);
				for (int i = 0; i < numSelLights; i++) {
					AlienFX_SDK::mapping* clight = &fxhl->afx_dev.GetMappings()->at(ListBox_GetItemData(light_list, selLights[i]));
					if (clight) {
						/*bool nothislight = true;
						for (int i = 0; i < grp->lights.size(); i++)
						if (grp->lights[i] == clight) {
						nothislight = false;
						break;
						}
						if (nothislight)*/
						grp->lights.push_back(clight);
					}
				}
				UpdateGroupLights(glights_list, gLid, (int) grp->lights.size() - 1);
				UpdateLightListG(light_list, grp);
			}
		} break;
		case IDC_BUT_DELFROMG:
			if (grp && glItem >= 0) {
				if (grp && glItem < grp->lights.size()) {
					std::vector <AlienFX_SDK::mapping*>::iterator Iter = grp->lights.begin() + glItem;
					grp->lights.erase(Iter);
				}
				UpdateGroupLights(glights_list, gLid, --glItem);
				UpdateLightListG(light_list, grp);
			}
			break;
		}
	} break;
	default: return false;
	}
	return true;
}
