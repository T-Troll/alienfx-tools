#include "alienfx-gui.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "ConfigFan.h"
#include "common.h"
#include <Shlwapi.h>

extern void UpdateProfileList();
extern void UpdateState(bool checkMode);
extern bool SetColor(HWND hDlg, AlienFX_SDK::Afx_colorcode);
extern void RedrawButton(HWND hDlg, AlienFX_SDK::Afx_colorcode);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern void RemoveUnusedGroups();

extern AlienFX_SDK::Afx_light* keySetLight;
extern string GetKeyName(WORD vkcode);
extern BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern EventHandler* eve;
extern FXHelper* fxhl;
extern ConfigFan* fan_conf;
//int /*pCid = -1, */devNum = -1;
profile* prof = NULL;
AlienFX_SDK::Afx_device* activeEffectDevice = NULL;

deviceeffect* FindDevEffect(int type, bool remove = false) {
	if (activeEffectDevice)
		for (auto effect = prof->effects[activeEffectDevice->devID].begin(); activeEffectDevice && 
			effect != prof->effects[activeEffectDevice->devID].end(); effect++)
			if (effect->globalMode == type)
				if (remove) {
					prof->effects[activeEffectDevice->devID].erase(effect);
					return nullptr;
				}
				else {
					return &(*effect);
				}
	return nullptr;
}

const static vector<string> ge_names[2]{ // 0 - v8, 1 - v5
	{ "Off", "Color or Morph", "Pulse", "Back Morph", "Breath", "Spectrum",
	"One key (K)", "Circle out (K)", "Wave out (K)", "Right wave (K)", "Default", "Rain Drop (K)",
	"Wave", "Rainbow wave", "Circle wave", "Random white (K)" },
	{ "Off", "Static", "Breathing", "Side Wave", "Dual Wave", "Pulse", "Morph", "Bounce", "Laser", "Rainbow" }};
/*
	0 - off
	1 - static color
	2 - single color breath
	3 - one side wave
	4 - stop animation (?)
	8 - pulse
	9 - 2-color morph
	10 - dobule-side wave
	12 - off
	14 - rainbow
*/
const static vector<string> cModeNames{ "Single-color", "Multi-color", "Rainbow" };
const static vector<int> ge_types[2]{ { 0,1,2,3,7,8,9,10,11,12,13,14,15,16,17,18 },
	{ 0,1,2,3,4,8,9,10,11,14 } };
const static vector<int> cModeTypes{ 1, 2, 3 };

void RefreshDeviceList(HWND hDlg) {
	HWND dev_list = GetDlgItem(hDlg, IDC_DE_LIST);
	ListBox_ResetContent(dev_list);
	for (auto i = conf->afx_dev.fxdevs.begin(); i != conf->afx_dev.fxdevs.end(); i++) 
		if (i->dev && i->dev->IsHaveGlobal()) {
			int pos = (int)(i - conf->afx_dev.fxdevs.begin());
			int ind = ListBox_AddString(dev_list, i->name.c_str());
			if (!activeEffectDevice)
				activeEffectDevice = &(*i);
			ListBox_SetItemData(dev_list, ind, pos);
			if (i->devID == activeEffectDevice->devID) {
				ListBox_SetCurSel(dev_list, ind);
				deviceeffect* b1 = FindDevEffect(1), *b2 = FindDevEffect(2);
				UpdateCombo(GetDlgItem(hDlg, IDC_GLOBAL_EFFECT), ge_names[i->version == 5],	b1 ? b1->globalEffect : 0, ge_types[i->version == 5]);
				UpdateCombo(GetDlgItem(hDlg, IDC_GLOBAL_KEYEFFECT), ge_names[i->version == 5], b2 ? b2->globalEffect : 0, ge_types[i->version == 5]);

				UpdateCombo(GetDlgItem(hDlg, IDC_CMODE), cModeNames, b1 ? b1->colorMode : 1, cModeTypes);
				UpdateCombo(GetDlgItem(hDlg, IDC_CMODE_KEY), cModeNames, b2 ? b2->colorMode : 1, cModeTypes);

				EnableWindow(GetDlgItem(hDlg, IDC_GLOBAL_KEYEFFECT), i->version == 8);
				//EnableWindow(GetDlgItem(hDlg, IDC_CMODE), i->version == 8);
				EnableWindow(GetDlgItem(hDlg, IDC_CMODE_KEY), i->version == 8);

				if (b1) {
					SendMessage(GetDlgItem(hDlg, IDC_SLIDER_TEMPO), TBM_SETPOS, true, b1->globalDelay);
					SetSlider(sTip1, b1->globalDelay);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), b1->effColor1);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), b1->effColor2);
				}
				else {
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR1), AlienFX_SDK::Afx_colorcode({ 0,0,0,0xff }));
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR2), AlienFX_SDK::Afx_colorcode({ 0,0,0,0xff }));
				}

				if (b2) {
					SendMessage(GetDlgItem(hDlg, IDC_SLIDER_KEYTEMPO), TBM_SETPOS, true, b2->globalDelay);
					SetSlider(sTip2, b2->globalDelay);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR3), b2->effColor1);
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR4), b2->effColor2);
				}
				else {
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR3), AlienFX_SDK::Afx_colorcode({ 0,0,0,0xff }));
					RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_EFFCLR4), AlienFX_SDK::Afx_colorcode({ 0,0,0,0xff }));
				}
			}
		}
}

BOOL CALLBACK DeviceEffectDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND dev_list = GetDlgItem(hDlg, IDC_DE_LIST),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO),
		eff_keytempo = GetDlgItem(hDlg, IDC_SLIDER_KEYTEMPO);

	deviceeffect* b;

	switch (LOWORD(wParam)) {
	case IDC_CMODE:
	case IDC_GLOBAL_EFFECT:
	case IDC_BUTTON_EFFCLR1:
	case IDC_BUTTON_EFFCLR2:
	case IDC_SLIDER_TEMPO:
		// b1
		b = FindDevEffect(1);
		break;
	default: // b2
		b = FindDevEffect(2);
	}

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
		RefreshDeviceList(hDlg);
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case IDCLOSE: case IDCANCEL: EndDialog(hDlg, IDCLOSE); break;
		case IDC_DE_LIST:
			activeEffectDevice = &conf->afx_dev.fxdevs[ListBox_GetItemData(dev_list, ListBox_GetCurSel(dev_list))];
			RefreshDeviceList(hDlg);
			break;
		case IDC_CMODE: case IDC_CMODE_KEY:
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				if (b) {
					b->colorMode = (byte)ComboBox_GetItemData(GetDlgItem(hDlg, LOWORD(wParam)), ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam))));
				}
			}
			}
			break;
		case IDC_GLOBAL_EFFECT: case IDC_GLOBAL_KEYEFFECT: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				byte newEffect = (byte)ComboBox_GetItemData(GetDlgItem(hDlg, LOWORD(wParam)), ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam))));
				if (b)
					b->globalEffect = newEffect;
				else {
					prof->effects[activeEffectDevice->devID].push_back({ {0}, {0}, newEffect, 5, (byte)(LOWORD(wParam) == IDC_GLOBAL_EFFECT ? 1 : 2) });
					b = &prof->effects[activeEffectDevice->devID].back();
				}
				if (!newEffect) {
					b = FindDevEffect(b->globalMode, true);
					if (prof->id == conf->activeProfile->id)
						fxhl->UpdateGlobalEffect(activeEffectDevice, true);
				}
				RefreshDeviceList(hDlg);
			} break;
			}
		} break;
		case IDC_BUTTON_EFFCLR1: case IDC_BUTTON_EFFCLR3: case IDC_BUTTON_EFFCLR2: case IDC_BUTTON_EFFCLR4:
		{
			if (b) {
				SetColor(GetDlgItem(hDlg, LOWORD(wParam)), LOWORD(wParam) == IDC_BUTTON_EFFCLR1 || LOWORD(wParam) == IDC_BUTTON_EFFCLR3 ?
					b->effColor1 : b->effColor2);
			}
		} break;
		default: return false;
		}
		if (activeEffectDevice && prof->id == conf->activeProfile->id)
			fxhl->UpdateGlobalEffect(activeEffectDevice);
	} break;
	case WM_DRAWITEM: {
		UINT CtlID = ((DRAWITEMSTRUCT*)lParam)->CtlID;
		switch (CtlID) {
		case IDC_BUTTON_EFFCLR1: case IDC_BUTTON_EFFCLR2: case IDC_BUTTON_EFFCLR3: case IDC_BUTTON_EFFCLR4:
			if (b) {
				RedrawButton(((DRAWITEMSTRUCT*)lParam)->hwndItem, CtlID == IDC_BUTTON_EFFCLR1 || CtlID == IDC_BUTTON_EFFCLR3 ?
					b->effColor1 : b->effColor2);
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
				if (b) {
					b->globalDelay = (BYTE) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
					SetSlider((HWND)lParam == eff_tempo ? sTip1 : sTip2, b->globalDelay);
					if (prof->id == conf->activeProfile->id)
						fxhl->UpdateGlobalEffect(activeEffectDevice);
				}
			} break;
			}
	} break;
	case WM_DESTROY:
		if (prof->id == conf->activeProfile->id)
			fxhl->Refresh();
		break;
	default: return false;
	}
	return true;
}

void ReloadProfSettings(HWND hDlg) {
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS);

	CheckDlgButton(hDlg, IDC_CHECK_DEFPROFILE, prof && prof->flags & PROF_DEFAULT);
	CheckDlgButton(hDlg, IDC_CHECK_PRIORITY, prof && prof->flags & PROF_PRIORITY);
	CheckDlgButton(hDlg, IDC_CHECK_PROFDIM, prof && prof->flags & PROF_DIMMED);
	CheckDlgButton(hDlg, IDC_CHECK_FOREGROUND, prof && prof->flags & PROF_ACTIVE);
	CheckDlgButton(hDlg, IDC_CHECK_FANPROFILE, prof && prof->flags & PROF_FANS);
	CheckDlgButton(hDlg, IDC_CHECK_SCRIPT, prof && prof->flags & PROF_RUN_SCRIPT);
	CheckDlgButton(hDlg, IDC_CHECK_EFFECTS, prof && prof->effmode);

	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_AC, prof && prof->triggerFlags & PROF_TRIGGER_AC);
	CheckDlgButton(hDlg, IDC_TRIGGER_POWER_BATTERY, prof && prof->triggerFlags & PROF_TRIGGER_BATTERY);
	CheckDlgButton(hDlg, IDC_TRIGGER_KEYS, prof && prof->triggerkey);

	SetDlgItemText(hDlg, IDC_TRIGGER_KEYS, ("Keyboard (" + (prof && prof->triggerkey ? GetKeyName(prof->triggerkey) : "Off") + ")").c_str());
	SetDlgItemText(hDlg, IDC_SCRIPT_NAME, prof ? prof->script.c_str() : NULL);
	ListBox_ResetContent(app_list);
	if (prof)
		for (auto j = prof->triggerapp.begin(); j != prof->triggerapp.end(); j++)
			ListBox_AddString(app_list, j->c_str());
}

void ReloadProfileView(HWND hDlg) {
	int rpos = 0;
	HWND profile_list = GetDlgItem(hDlg, IDC_LIST_PROFILES);
	ListView_DeleteAllItems(profile_list);
	ListView_SetExtendedListViewStyle(profile_list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	LVCOLUMNA lCol{ LVCF_WIDTH, LVCFMT_LEFT, 100 };
	ListView_DeleteColumn(profile_list, 0);
	ListView_InsertColumn(profile_list, 0, &lCol);
	for (int i = 0; i < conf->profiles.size(); i++) {
		auto cprof = conf->profiles[i];
		LVITEMA lItem{ LVIF_TEXT | LVIF_PARAM | LVIF_STATE, i};
		lItem.lParam = cprof->id;
		lItem.pszText = (char*)cprof->name.c_str();
		if (cprof->id == prof->id) {
			lItem.state = LVIS_SELECTED | LVIS_FOCUSED;
			rpos = i;
		}
		ListView_InsertItem(profile_list, &lItem);
	}
	ListView_SetColumnWidth(profile_list, 0, LVSCW_AUTOSIZE);
	ListView_EnsureVisible(profile_list, rpos, false);
}

BOOL CALLBACK TabProfilesDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND app_list = GetDlgItem(hDlg, IDC_LIST_APPLICATIONS);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (!prof)
			prof = conf->activeProfile;
		for (auto i = conf->afx_dev.fxdevs.begin(); i != conf->afx_dev.fxdevs.end(); i++)
			if (i->dev && i->dev->IsHaveGlobal()) {
				EnableWindow(GetDlgItem(hDlg, IDC_DEV_EFFECT), true);
				break;
			}
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
			UpdateProfileList();
			ReloadProfileView(hDlg);
		} break;
		case IDC_REMOVEPROFILE:
			if (!(prof->flags & PROF_DEFAULT) && conf->profiles.size() > 1) {
				if (GetKeyState(VK_SHIFT) & 0xf0 || MessageBox(hDlg, "Do you really want to remove selected profile and all settings for it?", "Warning",
							   MB_YESNO | MB_ICONWARNING) == IDYES) {
					for (auto pf = conf->profiles.begin(); pf != conf->profiles.end(); pf++)
						if ((*pf)->id == prof->id) {
							profile* newpCid = (pf + 1) == conf->profiles.end() ? *(pf - 1) : *(pf + 1);
							conf->profiles.erase(pf);
							if (conf->activeProfile->id == prof->id) {
								// switch to default profile..
								eve->SwitchActiveProfile(conf->FindDefaultProfile());
							}
							delete prof;
							prof = newpCid;
							RemoveUnusedGroups();
							UpdateProfileList();
							ReloadProfileView(hDlg);
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
					if (IsDlgButtonChecked(hDlg, IDC_CP_GRID) == BST_CHECKED)
						it->effect.type = 0;
					// remove if unused
					if (it->color.size() + it->events.size() + it->ambients.size() + it->haptics.size() + it->effect.type)
						it++;
					else
						it = prof->lightsets.erase(it);
				}
				if (IsDlgButtonChecked(hDlg, IDC_CP_SETTINGS) == BST_CHECKED)
					prof->flags &= PROF_DEFAULT;
				if (IsDlgButtonChecked(hDlg, IDC_CP_TRIGGERS) == BST_CHECKED) {
					prof->triggerFlags = 0;
					prof->triggerkey = 0;
				}
				if (IsDlgButtonChecked(hDlg, IDC_CP_APPS) == BST_CHECKED)
					prof->triggerapp.clear();
				RemoveUnusedGroups();
				if (IsDlgButtonChecked(hDlg, IDC_CP_FANS) == BST_CHECKED && prof->fansets) {
					if (conf->activeProfile->id == prof->id)
						fan_conf->lastProf = &fan_conf->prof;
					delete (fan_profile*)prof->fansets;
					prof->fansets = NULL;
					prof->flags &= ~PROF_FANS;
					//ReloadProfileView(hDlg);
				}
				if (conf->activeProfile->id == prof->id)
					UpdateState(true);
				ReloadProfSettings(hDlg);
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
				if (IsDlgButtonChecked(hDlg, IDC_CP_SETTINGS) == BST_CHECKED)
					prof->flags = conf->activeProfile->flags & ~PROF_DEFAULT;
				if (IsDlgButtonChecked(hDlg, IDC_CP_TRIGGERS) == BST_CHECKED) {
					prof->triggerFlags = conf->activeProfile->triggerFlags;
					prof->triggerkey = conf->activeProfile->triggerkey;
				}
				if (IsDlgButtonChecked(hDlg, IDC_CP_APPS) == BST_CHECKED)
					prof->triggerapp = conf->activeProfile->triggerapp;
				if (IsDlgButtonChecked(hDlg, IDC_CP_FANS) == BST_CHECKED) {
					if (prof->fansets)
						delete (fan_profile*)prof->fansets;
					prof->fansets = conf->activeProfile->fansets ? new fan_profile(*(fan_profile*)conf->activeProfile->fansets) : NULL;
					SetBitMask(prof->flags, PROF_FANS, (conf->activeProfile->flags & PROF_FANS) > 0);
					//ReloadProfileView(hDlg);
				}
				ReloadProfSettings(hDlg);
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
		case IDC_SCRIPT_BROWSE: {
			OPENFILENAMEA fstruct{ sizeof(OPENFILENAMEA), hDlg, hInst, "All files (*.*)\0*.*\0\0" };
			static char appName[4096]; appName[0] = 0;
			fstruct.lpstrFile = (LPSTR)appName;
			fstruct.nMaxFile = 4095;
			fstruct.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT;
			if (GetOpenFileNameA(&fstruct)) {
				prof->script = fstruct.lpstrFile;
				SetDlgItemText(hDlg, IDC_SCRIPT_NAME, fstruct.lpstrFile);
			}
		} break;
		case IDC_SCRIPT_NAME:
			switch (HIWORD(wParam)) {
			case EN_CHANGE:
				if (Edit_GetModify(GetDlgItem(hDlg, IDC_SCRIPT_NAME))) {
					int textsize = GetWindowTextLength(GetDlgItem(hDlg, IDC_SCRIPT_NAME));
					prof->script.resize(textsize);
					GetDlgItemText(hDlg, IDC_SCRIPT_NAME, (LPSTR)prof->script.data(), textsize + 1);
				}
				else
					return false;
				break;
			}
			break;
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
			if (prof->id == conf->activeProfile->id)
				fxhl->SetState();
			break;
		case IDC_CHECK_FOREGROUND:
			SetBitMask(prof->flags, PROF_ACTIVE, state);
			break;
		case IDC_CHECK_SCRIPT:
			SetBitMask(prof->flags, PROF_RUN_SCRIPT, state);
			break;
		case IDC_CHECK_FANPROFILE:
			SetBitMask(prof->flags, PROF_FANS, state);
			if (state && !prof->fansets) {
				// add current fan profile...
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
			break;
		default: return false;
		}
	} break;
	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->idFrom) {
		case IDC_LIST_PROFILES:
			switch (((NMHDR*) lParam)->code) {
			case LVN_ITEMACTIVATE: case NM_RETURN: {
				ListView_EditLabel(((NMHDR*)lParam)->hwndFrom, ((NMITEMACTIVATE*)lParam)->iItem);
			} break;
			case LVN_ITEMCHANGED:
			{
				NMLISTVIEW* lPoint = (LPNMLISTVIEW) lParam;
				if (lPoint->uChanged & LVIF_STATE) {
					if (lPoint->uNewState & LVIS_SELECTED)
						prof = conf->FindProfile((int)lPoint->lParam);
					else
						prof = NULL;
					ReloadProfSettings(hDlg);
				}
			} break;
			case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO* sItem = (NMLVDISPINFO*) lParam;
				if (prof && sItem->item.pszText) {
					prof->name = sItem->item.pszText;
					UpdateProfileList();
					ReloadProfileView(hDlg);
				}
				else
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