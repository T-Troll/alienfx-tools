#include "alienfx-gui.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "ConfigFan.h"
#include "common.h"
#include <Shlwapi.h>

extern void ReloadProfileList();
extern void UpdateState(bool checkMode);
extern bool SetColor(HWND hDlg, AlienFX_SDK::Afx_colorcode*);
extern void RedrawButton(HWND hDlg, AlienFX_SDK::Afx_colorcode*);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern void RemoveUnused(vector<groupset>* lightsets);
extern bool IsGroupUnused(DWORD gid);

extern AlienFX_SDK::Afx_light* keySetLight;
extern string GetKeyName(WORD vkcode);
extern BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern EventHandler* eve;
extern FXHelper* fxhl;
extern ConfigFan* fan_conf;
int pCid = -1;

vector<deviceeffect>::iterator FindDevEffect(profile* prof, int devNum, int type) {
	for (auto effect = prof->effects.begin(); devNum >=0 && effect != prof->effects.end(); effect++)
		if (conf->afx_dev.fxdevs[devNum].pid == effect->pid &&
			conf->afx_dev.fxdevs[devNum].vid == effect->vid &&
			effect->globalMode == type)
			return effect;
	return prof->effects.end();
}

void RemoveUnusedGroups() {
	for (auto i = conf->afx_dev.GetGroups()->begin(); i != conf->afx_dev.GetGroups()->end();)
		if (IsGroupUnused(i->gid)) {
			i = conf->afx_dev.GetGroups()->erase(i);
		}
		else
			i++;
}

const static vector<string> ge_names[2]{ // 0 - v8, 1 - v5
	{ "Off", "Morph", "Pulse", "Back morph", "Breath", "Rainbow", "Wave", "Rainbow wave", "Circle wave", "Reset" },
	{ "Off", "Breathing", "Single-color Wave", "Dual-color Wave", "Pulse", "Mixed Pulse", "Night Rider", "Laser" } };
const static vector<int> ge_types[2]{ { 0,1,2,3,7,8,15,16,17,19 }, { 0,2,3,4,8,9,10,11 } };

void RefreshDeviceList(HWND hDlg, int devNum, profile* prof) {
	HWND dev_list = GetDlgItem(hDlg, IDC_DE_LIST);
	ListBox_ResetContent(dev_list);
	for (auto i = conf->afx_dev.fxdevs.begin(); i != conf->afx_dev.fxdevs.end(); i++) 
		if (i->dev && i->dev->IsHaveGlobal()) {
			int pos = (int)(i - conf->afx_dev.fxdevs.begin());
			int ind = ListBox_AddString(dev_list, i->name.c_str());
			ListBox_SetItemData(dev_list, ind, pos);
			if (pos == devNum) {
				ListBox_SetCurSel(dev_list, ind);
				vector<deviceeffect>::iterator b1 = FindDevEffect(prof, devNum, 1),
					b2 = FindDevEffect(prof, devNum, 2);
				UpdateCombo(GetDlgItem(hDlg, IDC_GLOBAL_EFFECT), ge_names[i->version == 5],	b1 == prof->effects.end() ? 0 : b1->globalEffect, ge_types[i->version == 5]);
				UpdateCombo(GetDlgItem(hDlg, IDC_GLOBAL_KEYEFFECT), ge_names[i->version == 5], b2 == prof->effects.end() ? 0 : b2->globalEffect, ge_types[i->version == 5]);
				EnableWindow(GetDlgItem(hDlg, IDC_GLOBAL_KEYEFFECT), i->version == 8);

				if (b1 != prof->effects.end()) {
					SendMessage(GetDlgItem(hDlg, IDC_SLIDER_TEMPO), TBM_SETPOS, true, b1->globalDelay);
					SetSlider(sTip1, b1->globalDelay);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), &b1->effColor1);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), &b1->effColor2);
				}
				else {
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), NULL);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), NULL);
				}

				if (b2 != prof->effects.end()) {
					SendMessage(GetDlgItem(hDlg, IDC_SLIDER_KEYTEMPO), TBM_SETPOS, true, b2->globalDelay);
					SetSlider(sTip2, b2->globalDelay);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR3), &b2->effColor1);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR4), &b2->effColor2);
				}
				else {
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR3), NULL);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR4), NULL);
				}
			}
		}
}

BOOL CALLBACK DeviceEffectDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND dev_list = GetDlgItem(hDlg, IDC_DE_LIST),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO),
		eff_keytempo = GetDlgItem(hDlg, IDC_SLIDER_KEYTEMPO);
	profile* prof = conf->FindProfile(pCid);
	static int devNum = -1;

	vector<deviceeffect>::iterator b1 = FindDevEffect(prof, devNum, 1),
		b2 = FindDevEffect(prof, devNum, 2);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		SendMessage(eff_tempo, TBM_SETRANGE, true, MAKELPARAM(0, 0xff));
		SendMessage(eff_tempo, TBM_SETTICFREQ, 16, 0);
		sTip1 = CreateToolTip(eff_tempo, sTip1);
		SendMessage(eff_keytempo, TBM_SETRANGE, true, MAKELPARAM(0, 0xff));
		SendMessage(eff_keytempo, TBM_SETTICFREQ, 16, 0);
		sTip2 = CreateToolTip(eff_keytempo, sTip2);
		RefreshDeviceList(hDlg, devNum, prof);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCLOSE: case IDCANCEL: EndDialog(hDlg, IDCLOSE); break;
		case IDC_DE_LIST: {
			devNum = (int) ListBox_GetItemData(dev_list, ListBox_GetCurSel(dev_list));
			RefreshDeviceList(hDlg, devNum, prof);
		} break;
		case IDC_GLOBAL_EFFECT: case IDC_GLOBAL_KEYEFFECT: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				vector<deviceeffect>::iterator b = LOWORD(wParam) == IDC_GLOBAL_EFFECT ? b1 : b2;
				if (pCid == conf->activeProfile->id)
					fxhl->UpdateGlobalEffect(conf->afx_dev.fxdevs[devNum].dev, true);
				byte newEffect = (byte)ComboBox_GetItemData(GetDlgItem(hDlg, LOWORD(wParam)), ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam))));
				if (b != prof->effects.end())
					b->globalEffect = newEffect;
				else {
					prof->effects.push_back({ conf->afx_dev.fxdevs[devNum].vid,
						conf->afx_dev.fxdevs[devNum].pid, {0}, {0},
						newEffect, 5, (byte)(LOWORD(wParam) == IDC_GLOBAL_EFFECT ? 1 : 2) });
					b = prof->effects.end() - 1;
				}
				if (!newEffect)
					prof->effects.erase(b);
				if (pCid == conf->activeProfile->id)
					fxhl->Refresh();
				RefreshDeviceList(hDlg, devNum, prof);
			} break;
			}
		} break;
		case IDC_BUTTON_EFFCLR1: case IDC_BUTTON_EFFCLR3: case IDC_BUTTON_EFFCLR2: case IDC_BUTTON_EFFCLR4:
		{
			vector<deviceeffect>::iterator b = LOWORD(wParam) == IDC_BUTTON_EFFCLR1 || LOWORD(wParam) == IDC_BUTTON_EFFCLR2 ? b1 : b2;
			if (b != prof->effects.end()) {
				SetColor(GetDlgItem(hDlg, LOWORD(wParam)), LOWORD(wParam) == IDC_BUTTON_EFFCLR1 || LOWORD(wParam) == IDC_BUTTON_EFFCLR3 ?
					&b->effColor1 : &b->effColor2);
				if (pCid == conf->activeProfile->id)
					fxhl->Refresh();
			}
		} break;
		} break;
	case WM_DRAWITEM: {
		UINT CtlID = ((DRAWITEMSTRUCT*)lParam)->CtlID;
		switch (CtlID) {
		case IDC_BUTTON_EFFCLR1: case IDC_BUTTON_EFFCLR2: case IDC_BUTTON_EFFCLR3: case IDC_BUTTON_EFFCLR4:
			vector<deviceeffect>::iterator b = CtlID == IDC_BUTTON_EFFCLR1 || CtlID == IDC_BUTTON_EFFCLR2 ? b1 : b2;
			if (b != prof->effects.end()) {
				RedrawButton(((DRAWITEMSTRUCT*)lParam)->hwndItem, CtlID == IDC_BUTTON_EFFCLR1 || CtlID == IDC_BUTTON_EFFCLR3 ?
					&b->effColor1 : &b->effColor2);
			}
			break;
		}
	} break;
	case WM_HSCROLL:
	{
		if (prof)
			switch (LOWORD(wParam)) {
			case TB_THUMBPOSITION: case TB_ENDTRACK:
			{
				vector<deviceeffect>::iterator b = (HWND)lParam == eff_tempo ? b1 : b2;
				if (b != prof->effects.end()) {
					b->globalDelay = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider((HWND)lParam == eff_tempo ? sTip1 : sTip2, b->globalDelay);
					if (prof->id == conf->activeProfile->id)
						fxhl->Refresh();
				}
			} break;
			}
	} break;
	default: return false;
	}
	return true;
}

void ReloadProfSettings(HWND hDlg, profile *prof) {
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS);

	CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof && prof->flags & PROF_DEFAULT);
	CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, prof && prof->flags & PROF_PRIORITY);
	CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof && prof->flags & PROF_DIMMED);
	CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, prof && prof->flags & PROF_ACTIVE);
	CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, prof && prof->flags & PROF_FANS);
	CheckDlgButton(hDlg, IDC_CHECK_EFFECTS, prof && prof->effmode);

	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_AC, prof && prof->triggerFlags & PROF_TRIGGER_AC);
	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_BATTERY, prof && prof->triggerFlags & PROF_TRIGGER_BATTERY);
	CheckDlgButton(hDlg, IDC_TRIGGER_KEYS, prof && prof->triggerkey);

	SetDlgItemText(hDlg, IDC_TRIGGER_KEYS, ("Keyboard (" + (prof && prof->triggerkey ? GetKeyName(prof->triggerkey) : "Off") + ")").c_str());
	ListBox_ResetContent(app_list);
	if (prof)
		for (auto j = prof->triggerapp.begin(); j != prof->triggerapp.end(); j++)
			ListBox_AddString(app_list, j->c_str());
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
		auto prof = conf->profiles[i];
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i};
		lItem.lParam = prof->id;
		lItem.pszText = (char*)prof->name.c_str();
		if (prof->id == pCid) {
			lItem.state = LVIS_SELECTED;
			rpos = i;
		}
		else
			lItem.state = 0;
		ListView_InsertItem(profile_list, &lItem);
	}
	ListView_SetColumnWidth(profile_list, 0, LVSCW_AUTOSIZE);
	ListView_EnsureVisible(profile_list, rpos, false);
}

BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS);

	profile *prof = conf->FindProfile(pCid);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (pCid < 0)
			pCid = conf->activeProfile->id;
		ReloadProfileView(hDlg);
	} break;
	case WM_COMMAND:
	{
		if (!prof && LOWORD(wParam) != IDC_ADDPROFILE)
			return false;

		WORD state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;

		switch (LOWORD(wParam))
		{
		case IDC_DEV_EFFECT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_DEVICEEFFECTS), hDlg, (DLGPROC)DeviceEffectDialog);
			break;
		case IDC_ADDPROFILE: {
			unsigned vacID = 0;
			for (auto prof = conf->profiles.begin(); prof != conf->profiles.end();)
				if (vacID == (*prof)->id) {
					vacID++; prof = conf->profiles.begin();
				}
				else
					prof++;
			prof = new profile(prof ? *prof : *conf->activeProfile);
			conf->profiles.push_back(prof);
			prof->id = vacID;
			prof->flags &= ~PROF_DEFAULT;
			prof->name = "Profile " + to_string(vacID);
			pCid = vacID;
			ReloadProfileView(hDlg);
			ReloadProfileList();
		} break;
		case IDC_REMOVEPROFILE:
			if (!(prof->flags & PROF_DEFAULT) && conf->profiles.size() > 1) {
				if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					for (auto pf = conf->profiles.begin(); pf != conf->profiles.end(); pf++)
						if ((*pf)->id == pCid) {
							int newpCid = pf + 1 == conf->profiles.end() ? (*(pf - 1))->id : (*(pf + 1))->id;
							conf->profiles.erase(pf);
							if (conf->activeProfile->id == pCid) {
								// switch to default profile..
								eve->SwitchActiveProfile(conf->FindDefaultProfile());
							}
							delete prof;
							pCid = newpCid;
							RemoveUnusedGroups();
							ReloadProfileView(hDlg);
							ReloadProfileList();
							break;
						}
				}
			}
			else
				ShowNotification(&conf->niData, "Error", "Can't delete last or default profile!");
			break;
		case IDC_BUT_PROFRESET:
			if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you really want to remove selected settings from this profile?", "Warning",
										   MB_YESNO | MB_ICONWARNING) == IDYES) {
				for (auto it = prof->lightsets.begin(); it != prof->lightsets.end();) {
					if (IsDlgButtonChecked(hDlg, IDC_CP_COLORS) == BST_CHECKED)
						it->color.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_EVENTS) == BST_CHECKED)
						it->events.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_AMBIENT) == BST_CHECKED)
						it->ambients.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_HAPTICS) == BST_CHECKED)
						it->haptics.clear();
					if (IsDlgButtonChecked(hDlg, IDC_CP_GRID) == BST_CHECKED)
						it->effect.type = 0;
					// remove if unused now
					if (!(it->color.size() + it->events.size() + it->ambients.size() + it->haptics.size() + it->effect.type)) {
						it = prof->lightsets.erase(it);
					}
					else
						it++;
				}
				RemoveUnusedGroups();
				if (IsDlgButtonChecked(hDlg, IDC_CP_FANS) == BST_CHECKED) {
					delete prof->fansets;// = { };
					prof->fansets = NULL;
					prof->flags &= ~PROF_FANS;
					ReloadProfileView(hDlg);
				}
				if (conf->activeProfile->id == prof->id)
					UpdateState(true);
			}
			break;
		case IDC_BUT_COPYACTIVE:
			if (conf->activeProfile->id != prof->id) {
				for (auto t = conf->activeProfile->lightsets.begin(); t < conf->activeProfile->lightsets.end(); t++) {
					groupset* lset = conf->FindMapping(t->group, &prof->lightsets);
					if (!lset) {
						prof->lightsets.push_back({ t->group });
						lset = &prof->lightsets.back();
					}
					lset->gauge = t->gauge;
					lset->gaugeflags = t->gaugeflags;
					if (IsDlgButtonChecked(hDlg, IDC_CP_COLORS) == BST_CHECKED)
						lset->color = t->color;
					if (IsDlgButtonChecked(hDlg, IDC_CP_EVENTS) == BST_CHECKED) {
						lset->events = t->events;
						lset->fromColor = t->fromColor;
					}
					if (IsDlgButtonChecked(hDlg, IDC_CP_AMBIENT) == BST_CHECKED)
						lset->ambients = t->ambients;
					if (IsDlgButtonChecked(hDlg, IDC_CP_HAPTICS) == BST_CHECKED)
						lset->haptics = t->haptics;
					if (IsDlgButtonChecked(hDlg, IDC_CP_GRID) == BST_CHECKED)
						lset->effect = t->effect;
				}
				if (IsDlgButtonChecked(hDlg, IDC_CP_FANS) == BST_CHECKED) {
					if (prof->fansets) 
						delete prof->fansets;
					prof->fansets = conf->activeProfile->fansets ? new fan_profile(*(fan_profile*)conf->activeProfile->fansets) : NULL;
					SetBitMask(prof->flags, PROF_FANS, (conf->activeProfile->flags & PROF_FANS) > 0);
					ReloadProfileView(hDlg);
				}
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
			OPENFILENAMEA fstruct{ sizeof(OPENFILENAMEA), hDlg, hInst, "Applications (*.exe)\0*.exe\0\0" };
			static char appName[4096]; appName[0] = 0;
			fstruct.lpstrFile = (LPSTR) appName;
			fstruct.nMaxFile = 4095;
			fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
			if (GetOpenFileNameA(&fstruct)) {
				PathStripPath(fstruct.lpstrFile);
				prof->triggerapp.push_back(appName);
				ListBox_AddString(app_list, prof->triggerapp.back().c_str());
			}
		} break;
		case IDC_CHECK_EFFECTS:
			prof->effmode = state;
			if (prof->id == conf->activeProfile->id)
				UpdateState(true);
			break;
		case IDC_CHECK_DEFPROFILE:
		{
			if (state) {
				for (auto op = conf->profiles.begin(); op != conf->profiles.end(); op++)
					if ((*op)->flags & PROF_DEFAULT && conf->SamePower(*op, prof))
						(*op)->flags &= ~PROF_DEFAULT;
				prof->flags |= PROF_DEFAULT;
			} else
				CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_CHECKED);
		} break;
		case IDC_CHECK_PRIORITY:
			SetBitMask(prof->flags, PROF_PRIORITY, state);
			break;
		case IDC_CHECK_PROFDIM:
			SetBitMask(prof->flags, PROF_DIMMED, state);
			if (prof->id == conf->activeProfile->id) {
				fxhl->SetState();
			}
			break;
		case IDC_CHECK_FOREGROUND:
			SetBitMask(prof->flags, PROF_ACTIVE, state);
			break;
		case IDC_CHECK_FANPROFILE:
			SetBitMask(prof->flags, PROF_FANS, state);
			if (state) {
				// add current fan profile...
				if (!prof->fansets)
					prof->fansets = new fan_profile(*fan_conf->lastProf);
			}
			// Switch fan control if needed
			if (prof->id == conf->activeProfile->id)
				fan_conf->lastProf = state ? (fan_profile*)prof->fansets : &fan_conf->prof;
			break;
		case IDC_TRIGGER_POWER_AC:
			SetBitMask(prof->triggerFlags, PROF_TRIGGER_AC, state);
			break;
		case IDC_TRIGGER_POWER_BATTERY:
			SetBitMask(prof->triggerFlags, PROF_TRIGGER_BATTERY, state);
			break;
		case IDC_TRIGGER_KEYS:
			if (state) {
				AlienFX_SDK::Afx_light lgh;
				keySetLight = &lgh;
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_KEY), hDlg, (DLGPROC)KeyPressDialog);
				prof->triggerkey = (WORD)lgh.scancode;
			}
			else
				prof->triggerkey = 0;
			ReloadProfSettings(hDlg, prof);
			break;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_PROFILES:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE: {
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, ((NMITEMACTIVATE*)lParam)->iItem);
			} break;

			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uNewState & (LVIS_FOCUSED | LVIS_SELECTED) && lPoint->iItem != -1) {
					// Select other item...
					pCid = (int) lPoint->lParam;
					//ReloadProfSettings(hDlg, conf->FindProfile(pCid));
				} else {
					pCid = -1;
					//CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, BST_UNCHECKED);
					//CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, BST_UNCHECKED);
					//CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, BST_UNCHECKED);
					//CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, BST_UNCHECKED);
					//CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, BST_UNCHECKED);
					//CheckDlgButton(hDlg, IDC_CHECK_EFFECTS, BST_UNCHECKED);
					//ListBox_ResetContent(app_list);
					//ComboBox_SetCurSel(mode_list, 0);
				}
				ReloadProfSettings(hDlg, conf->FindProfile(pCid));
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				if (prof && sItem->item.pszText) {
					prof->name = sItem->item.pszText;
					//ListView_SetItem(((NMHDR*)lParam)->hwndFrom, &sItem->item);
					ReloadProfileList();
					ReloadProfileView(hDlg);
					//ReloadProfSettings(hDlg, prof);
				}
			} break;
			}
			break;
		}
		break;
	default: return false;
	}
	return true;
}