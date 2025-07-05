#include "alienfx-gui.h"
#include "EventHandler.h"
#include "FXHelper.h"
#include "MonHelper.h"
#include "common.h"

//extern void UpdateProfileList();
extern bool DetectFans();
extern void SetHotkeys();
extern void SetMainTabs();
extern void UpdateProfileList();

extern MonHelper* mon;
extern EventHandler* eve;
extern FXHelper* fxhl;
extern ConfigFan* fan_conf;

extern const char* freqNames[];
extern const int* freqValues;
extern bool wasAWCC;

BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	//HWND dim_slider = GetDlgItem(hDlg, IDC_SLIDER_DIMMING);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HWND dim_slider = GetDlgItem(hDlg, IDC_SLIDER_DIMMING);
		// system settings...
		CheckDlgButton(hDlg, IDC_STARTW, conf->startWindows);
		CheckDlgButton(hDlg, IDC_STARTM, conf->startMinimized);
		CheckDlgButton(hDlg, IDC_BATTDIM, conf->dimmedBatt);
		CheckDlgButton(hDlg, IDC_SCREENOFF, conf->offWithScreen);
		CheckDlgButton(hDlg, IDC_CHECK_EFFECTS, conf->enableEffects);
		CheckDlgButton(hDlg, IDC_CHECK_EFFBAT, !conf->effectsOnBattery);
		CheckDlgButton(hDlg, IDC_CHECK_LON, conf->lightsOn);
		CheckDlgButton(hDlg, IDC_POWER_DIM, conf->dimPowerButton);
		CheckDlgButton(hDlg, IDC_CHECK_GAMMA, conf->gammaCorrection);
		CheckDlgButton(hDlg, IDC_OFFPOWERBUTTON, !conf->offPowerButton);
		CheckDlgButton(hDlg, IDC_BUT_PROFILESWITCH, conf->enableProfSwitch);
		CheckDlgButton(hDlg, IDC_AWCC, conf->awcc_disable);
		CheckDlgButton(hDlg, IDC_ESIFTEMP, conf->esif_temp);
		CheckDlgButton(hDlg, IDC_FANCONTROL, conf->fanControl);
		CheckDlgButton(hDlg, IDC_KEEPSYSTEM, fan_conf->keepSystem);
		CheckDlgButton(hDlg, IDC_OCENABLE, fan_conf->ocEnable);
		CheckDlgButton(hDlg, IDC_DISKSENSORS, fan_conf->diskSensors);
		CheckDlgButton(hDlg, IDC_BAT_FAN, !conf->fansOnBattery);
		CheckDlgButton(hDlg, IDC_CHECK_EXCEPTION, conf->noDesktop);
		CheckDlgButton(hDlg, IDC_CHECK_DIM, conf->dimmed);
		CheckDlgButton(hDlg, IDC_CHECK_UPDATE, conf->updateCheck);
		CheckDlgButton(hDlg, IDC_OFFONBATTERY, conf->offOnBattery);
		CheckDlgButton(hDlg, IDC_CHECK_LIGHTNAMES, conf->showGridNames);
		CheckDlgButton(hDlg, IDC_HOTKEYS, conf->keyShortcuts);
		CheckDlgButton(hDlg, IDC_LIGHTACTION, conf->actionLights);

		SetDlgItemInt(hDlg, IDC_EDIT_POLLING, fan_conf->pollingRate, false);
		SetDlgItemInt(hDlg, IDC_EDIT_ACTION, conf->actionTimeout, false);

		UpdateCombo(GetDlgItem(hDlg, IDC_COMBO_FREQ), freqNames, conf->dcFreq, freqValues);

		SendMessage(dim_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(dim_slider, TBM_SETTICFREQ, 16, 0);
		//SendMessage(dim_slider, TBM_SETPOS, true, conf->dimmingPower);
		CreateToolTip(dim_slider, sTip1, conf->dimmingPower);
		//SetSlider(sTip1, conf->dimmingPower);
	} break;
	case WM_COMMAND: {
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDC_EDIT_POLLING:
			if (HIWORD(wParam) == EN_KILLFOCUS) {
				if (mon) mon->Stop();
				fan_conf->pollingRate = GetDlgItemInt(hDlg, IDC_EDIT_POLLING, NULL, false);
				if (mon) mon->Start();
			}
			break;
		case IDC_EDIT_ACTION:
			if (HIWORD(wParam) == EN_KILLFOCUS) {
				conf->actionTimeout = GetDlgItemInt(hDlg, IDC_EDIT_ACTION, NULL, false);
				eve->ChangeAction();
			}
			break;
		case IDC_STARTM:
			conf->startMinimized = state;
			break;
		case IDC_STARTW:
		{
			conf->startWindows = state;
			WindowsStartSet(state, "AlienFX-GUI");
		} break;
		case IDC_CHECK_UPDATE:
			conf->updateCheck = state;
			break;
		case IDC_BATTDIM:
			conf->dimmedBatt = state;
			fxhl->SetState();
			break;
		case IDC_SCREENOFF:
			conf->offWithScreen = state;
			break;
		case IDC_BUT_PROFILESWITCH:
			conf->enableProfSwitch = state;
			break;
		case IDC_CHECK_LON:
			conf->lightsOn = state;
			eve->ChangeEffectMode();
			break;
		case IDC_CHECK_GAMMA:
			conf->gammaCorrection = state;
			fxhl->Refresh();
			break;
		case IDC_CHECK_EFFECTS:
			conf->enableEffects = state;
			eve->ChangeEffects();
			UpdateProfileList();
			break;
		case IDC_CHECK_EFFBAT:
			conf->effectsOnBattery = !state;
			eve->ChangeEffects();
			break;
		case IDC_BAT_FAN:
			conf->fansOnBattery = !state;
			eve->ToggleFans();
			break;
		case IDC_OFFPOWERBUTTON:
			conf->offPowerButton = !state;
			if (!conf->stateOn) {
				fxhl->SetState(state);
			}
			break;
		case IDC_POWER_DIM:
			conf->dimPowerButton = state;
			if (conf->stateDimmed) {
				fxhl->SetState(!state);
			}
			break;
		case IDC_OFFONBATTERY:
			conf->offOnBattery = state;
			fxhl->SetState();
			break;
		case IDC_AWCC:
			conf->awcc_disable = state;
			conf->Save();
			wasAWCC = DoStopAWCC((bool)conf->awcc_disable != wasAWCC, wasAWCC);
			break;
		case IDC_ESIFTEMP:
			if (conf->esif_temp = state) {
				conf->Save();
				EvaluteToAdmin(mDlg);
			}
			break;
		case IDC_CHECK_EXCEPTION:
			conf->noDesktop = state;
			break;
		case IDC_CHECK_DIM:
			conf->dimmed = state;
			fxhl->SetState(!(conf->lightsOn || conf->offPowerButton || !conf->dimPowerButton));
			break;
		case IDC_FANCONTROL:
			conf->fanControl = state;
			if (state) {
				conf->Save();
				if (!(conf->fanControl = DetectFans())) {
					if (mDlg) {
						CheckDlgButton(hDlg, IDC_FANCONTROL, BST_UNCHECKED);
						ShowNotification(&conf->niData, "Error", "Fan control not available.");
					}
					break;
				}
			}
			else {
				eve->ChangeEffects(true);
				delete mon;
				mon = NULL;
				eve->ChangeEffects();
			}
			fxhl->FillAllDevs();
			fxhl->Refresh();
			SetHotkeys();
			SetMainTabs();
			break;
		case IDC_DISKSENSORS:
			fan_conf->diskSensors = state;
			if (mon) {
				delete mon;
				mon = new MonHelper();
			}
			break;
		case IDC_KEEPSYSTEM:
			fan_conf->keepSystem = state;
			break;
		case IDC_OCENABLE:
			fan_conf->ocEnable = state;
			if (mon)
				mon->SetOC();
			break;
		case IDC_CHECK_LIGHTNAMES:
			conf->showGridNames = state;
			break;
		case IDC_HOTKEYS:
			conf->keyShortcuts = state;
			SetHotkeys();
			break;
		case IDC_LIGHTACTION:
			conf->actionLights = state;
			eve->ChangeAction();
			break;
		case IDC_COMBO_FREQ:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				conf->dcFreq = (DWORD)ComboBox_GetItemData(GetDlgItem(hDlg, IDC_COMBO_FREQ), ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBO_FREQ)));
				if (!conf->statePower)
					eve->SetDisplayFreq(conf->dcFreq);
			}
			break;
		}
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBPOSITION: case TB_ENDTRACK: {
			//if ((HWND)lParam == dim_slider) {
				conf->dimmingPower = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				SetSlider(sTip1, conf->dimmingPower);
				conf->stateDimmed = false;
				fxhl->SetState();
			//}
		} break;
		} break;
	default: return false;
	}
	return true;
}