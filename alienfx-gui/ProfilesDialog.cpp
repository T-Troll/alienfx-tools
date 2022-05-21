#include "alienfx-gui.h"
#include "EventHandler.h"
#include <Shlwapi.h>

extern void ReloadProfileList();
extern void ReloadModeList(HWND, int);
extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);

extern EventHandler* eve;
int pCid = -1;

void ReloadProfSettings(HWND hDlg, profile *prof) {
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		eff_list = GetDlgItem(hDlg, IDC_GLOBAL_EFFECT),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO),
		mode_list = GetDlgItem(hDlg, IDC_COMBO_EFFMODE);
	CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof->flags & PROF_DEFAULT);
	CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, prof->flags & PROF_PRIORITY);
	CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof->flags & PROF_DIMMED);
	CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, prof->flags & PROF_ACTIVE);
	CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, prof->flags & PROF_FANS);
	CheckDlgButton(hDlg, IDC_CHECK_GLOBAL, prof->flags & PROF_GLOBAL_EFFECTS);
	ComboBox_SetCurSel(mode_list, prof->effmode);
	ListBox_ResetContent(app_list);
	for (int j = 0; j < prof->triggerapp.size(); j++)
		ListBox_AddString(app_list, prof->triggerapp[j].c_str());
	// set global effect, colors and delay
	bool flag = prof->flags & PROF_GLOBAL_EFFECTS;
	EnableWindow(eff_list, flag);
	EnableWindow(eff_tempo, flag);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), flag);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), flag);
	// now sliders...
	SendMessage(eff_tempo, TBM_SETPOS, true, prof->globalDelay);
	SetSlider(sTip2, prof->globalDelay);
	// now colors...
	RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, flag ? &prof->effColor1 : 0);
	RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, flag ? &prof->effColor2 : 0);
	ComboBox_SetCurSel(eff_list, prof->globalEffect);
}

void ReloadProfileView(HWND hDlg) {
	int rpos = 0;
	HWND profile_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);
	ListView_DeleteAllItems(profile_list);
	ListView_SetExtendedListViewStyle(profile_list, LVS_EX_FULLROWSELECT);
	LVCOLUMNA lCol{ LVCF_WIDTH, LVCFMT_LEFT, 100 };
	ListView_DeleteColumn(profile_list, 0);
	ListView_InsertColumn(profile_list, 0, &lCol);
	for (int i = 0; i < conf->profiles.size(); i++) {
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i};
		lItem.lParam = conf->profiles[i]->id;
		lItem.pszText = (char*)conf->profiles[i]->name.c_str();
		if (conf->profiles[i]->id == pCid) {
			lItem.state = LVIS_SELECTED;
			ReloadProfSettings(hDlg, conf->profiles[i]);
			rpos = i;
		}
		ListView_InsertItem(profile_list, &lItem);
	}
	ListView_SetColumnWidth(profile_list, 0, LVSCW_AUTOSIZE);
	ListView_EnsureVisible(profile_list, rpos, false);
}

void RemoveProfile(int id) {
	conf->profiles.erase(find_if(conf->profiles.begin(), conf->profiles.end(),
		[id](profile* pr) {
			return pr->id == id;
		}));
}

BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS),
		mode_list = GetDlgItem(hDlg, IDC_COMBO_EFFMODE),
		eff_list = GetDlgItem(hDlg, IDC_GLOBAL_EFFECT),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO);// ,
		//p_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);

	profile *prof = conf->FindProfile(pCid);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		pCid = conf->activeProfile ? conf->activeProfile->id : conf->FindDefaultProfile()->id;
		ReloadModeList(mode_list, conf->activeProfile? conf->activeProfile->effmode : 3);
		if (conf->haveV5) {
			//ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "None"), 1);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Color"), 0);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Breathing"), 2);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Single-color Wave"), 3);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Dual-color Wave "), 4);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Pulse"), 8);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Mixed Pulse"), 9);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Night Rider"), 10);
			ComboBox_SetItemData(eff_list, ComboBox_AddString(eff_list, "Laser"), 11);
			SendMessage(eff_tempo, TBM_SETRANGE, true, MAKELPARAM(0, 0xa));
			SendMessage(eff_tempo, TBM_SETTICFREQ, 1, 0);
			sTip2 = CreateToolTip(eff_tempo, sTip2);
		} else
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_GLOBAL), false);
		ReloadProfileView(hDlg);
	} break;
	case WM_COMMAND:
	{
		if (!prof && LOWORD(wParam) != IDC_ADDPROFILE)
			return false;

		switch (LOWORD(wParam))
		{
		case IDC_GLOBAL_EFFECT: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				prof->globalEffect = (byte) ComboBox_GetItemData(eff_list, ComboBox_GetCurSel(eff_list));
				if (prof->id == conf->activeProfile->id)
					fxhl->UpdateGlobalEffect();
			} break;
			}
		} break;
		case IDC_BUTTON_EFFCLR1:
		{
			SetColor(hDlg, IDC_BUTTON_EFFCLR1, &prof->effColor1);
			if (prof->id == conf->activeProfile->id)
				fxhl->UpdateGlobalEffect();
		} break;
		case IDC_BUTTON_EFFCLR2:
		{
			SetColor(hDlg, IDC_BUTTON_EFFCLR2, &prof->effColor2);
			if (prof->id == conf->activeProfile->id)
				fxhl->UpdateGlobalEffect();
		} break;
		case IDC_ADDPROFILE: {
			unsigned vacID = 0;
			for (int i = 0; i < conf->profiles.size(); i++)
				if (vacID == conf->profiles[i]->id) {
					vacID++; i = -1;
				}
			if (!prof)
				prof = conf->activeProfile;
			profile* new_prof = new profile({ vacID, (WORD)(prof->flags & ~PROF_DEFAULT), prof->effmode, {}, "Profile " + to_string(vacID), prof->lightsets, prof->fansets });
			conf->profiles.push_back(new_prof);
			pCid = vacID;
			ReloadProfileView(hDlg);
			ReloadProfileList();
		} break;
		case IDC_REMOVEPROFILE: {
			if (!(prof->flags & PROF_DEFAULT) && conf->profiles.size() > 1) {
				if (MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					// is this active profile? Switch needed!
					if (conf->activeProfile->id == pCid) {
						// switch to default profile..
						eve->SwitchActiveProfile(conf->FindDefaultProfile());
						pCid = conf->FindDefaultProfile()->id;
						ReloadProfileList();
					}
					RemoveProfile(pCid);
					ReloadProfileView(hDlg);
				}
			}
			else
				MessageBox(hDlg, "Can't delete last or default profile!", "Error",
						   MB_OK | MB_ICONERROR);
		} break;
		case IDC_BUT_PROFRESET:
			if (MessageBox(hDlg, "Do you really want to reset all lights settings for this profile?", "Warning",
										   MB_YESNO | MB_ICONWARNING) == IDYES) {
				prof->lightsets.colors.clear();
				prof->lightsets.ambients.clear();
				prof->lightsets.haptics.clear();
			}
			break;
		case IDC_BUT_COPYACTIVE:
			if (conf->activeProfile->id != prof->id) {
				profile* new_prof = new profile(*conf->activeProfile);
				new_prof->id = prof->id;
				new_prof->name = prof->name;
				new_prof->flags &= ~PROF_DEFAULT;
				new_prof->flags |= prof->flags & PROF_DEFAULT;
				RemoveProfile(prof->id);
				conf->profiles.push_back(new_prof);
				ReloadProfileView(hDlg);
			}
			break;
		case IDC_APP_RESET:
		{
			int ind = ListBox_GetCurSel(app_list);
			if (ind >= 0) {
				ListBox_DeleteString(app_list, ind);
				prof->triggerapp.erase(prof->triggerapp.begin() + ind);
			}
		} break;
		case IDC_APP_BROWSE: {
			// fileopen dialogue...
			OPENFILENAMEA fstruct{ sizeof(OPENFILENAMEA), hDlg, hInst, "Applications (*.exe)\0*.exe\0\0" };
			char appName[4096]; appName[0] = 0;
			fstruct.lpstrFile = (LPSTR) appName;
			fstruct.nMaxFile = 4095;
			fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
			if (GetOpenFileNameA(&fstruct)) {
				PathStripPath(fstruct.lpstrFile);
				prof->triggerapp.push_back(appName);
				ListBox_AddString(app_list, prof->triggerapp.back().c_str());
			}
		} break;
		case IDC_COMBO_EFFMODE:
		{
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				prof->effmode = ComboBox_GetCurSel(mode_list);
				if (prof->effmode != 3) {
					prof->flags &= ~PROF_GLOBAL_EFFECTS;
					ReloadProfSettings(hDlg, prof);
				}
				if (prof->id == conf->activeProfile->id)
					eve->ChangeEffectMode();
			} break;
			}
		} break;
		case IDC_CHECK_DEFPROFILE:
		{
			if (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) {
				profile *old_def = conf->FindDefaultProfile();
				old_def->flags = old_def->flags & ~PROF_DEFAULT;
				prof->flags |= PROF_DEFAULT;
				if (conf->enableProf && old_def->id == conf->activeProfile->id)
					// need to switch to this
					eve->SwitchActiveProfile(prof);
			} else
				CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_CHECKED);
		} break;
		case IDC_CHECK_PRIORITY:
			prof->flags = (prof->flags & ~PROF_PRIORITY) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 1;
			// Update active profile, if needed
			if (conf->enableProf)
				eve->SwitchActiveProfile(eve->ScanTaskList());
			break;
		case IDC_CHECK_PROFDIM:
			prof->flags = (prof->flags & ~PROF_DIMMED) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 2;
			prof->ignoreDimming = false;
			if (prof->id == conf->activeProfile->id) {
				//conf->SetStates();
				fxhl->ChangeState();
			}
			break;
		case IDC_CHECK_FOREGROUND:
			prof->flags = (prof->flags & ~PROF_ACTIVE) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 3;
			break;
		case IDC_CHECK_GLOBAL:
			prof->flags = (prof->flags & ~PROF_GLOBAL_EFFECTS) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 5;
			if (prof->flags & PROF_GLOBAL_EFFECTS)
				prof->effmode = 3;
			ReloadProfSettings(hDlg, prof);
			if (prof->id == conf->activeProfile->id) {
				fxhl->UpdateGlobalEffect();
				fxhl->Refresh(true);
			}
			break;
		case IDC_CHECK_FANPROFILE:
			prof->flags = (prof->flags & ~PROF_FANS) | (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED) << 4;
			if (prof->flags & PROF_FANS) {
				// add current fan profile...
				prof->fansets = conf->fan_conf->prof;
			} else {
				// remove fansets
				prof->fansets.fanControls.clear();
				if (prof->id == conf->activeProfile->id)
					conf->fan_conf->lastProf = &conf->fan_conf->prof;
			}
			break;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_PROFILES:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE: {
				//NMITEMACTIVATE* sItem = (NMITEMACTIVATE*) lParam;
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, ((NMITEMACTIVATE*)lParam)->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState && LVIS_FOCUSED && lPoint->iItem != -1) {
					// Select other item...
					pCid = (int) lPoint->lParam;
					//prof = conf->FindProfile(pCid);
					//if (prof)
						ReloadProfSettings(hDlg, conf->FindProfile(pCid));// prof);
				} else {
					pCid = -1;
					CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, BST_UNCHECKED);
					CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, BST_UNCHECKED);
					ListBox_ResetContent(app_list);
					ComboBox_SetCurSel(mode_list, 3);
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				profile* prof = conf->FindProfile((int)sItem->item.lParam);
				if (prof && sItem->item.pszText) {
					prof->name = sItem->item.pszText;
					ListView_SetItem(((NMHDR*)lParam)->hwndFrom, &sItem->item);
					//ListView_SetColumnWidth(((NMHDR*)lParam)->hwndFrom, 0, LVSCW_AUTOSIZE);
					ReloadProfileList();
					return true;
				} else
					return false;
			} break;
			}
			break;
		}
		break;
	case WM_HSCROLL:
	{
		if (prof)
			switch (LOWORD(wParam)) {
			case TB_THUMBPOSITION: case TB_ENDTRACK:
			{
				if ((HWND) lParam == eff_tempo) {
					prof->globalDelay = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, prof->globalDelay);
					if (prof->id == conf->activeProfile->id)
						fxhl->UpdateGlobalEffect();
				}
			} break;
			}
	} break;
	case WM_DRAWITEM:
	{
		if (prof && (prof->flags & PROF_GLOBAL_EFFECTS))
			switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
			case IDC_BUTTON_EFFCLR2:
			{
				RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, &prof->effColor1);
				RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, &prof->effColor2);
				break;
			}
			}
	} break;
	default: return false;
	}
	return true;
}