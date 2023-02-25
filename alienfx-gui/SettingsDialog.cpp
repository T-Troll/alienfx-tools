#include "alienfx-gui.h"
#include "EventHandler.h"
#include "MonHelper.h"
#include "common.h"

extern void ReloadProfileList();
extern bool DetectFans();
extern void SetHotkeys();
extern void SetMainTabs();

extern EventHandler* eve;
extern FXHelper* fxhl;
extern MonHelper* mon;

BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND dim_slider = GetDlgItem(hDlg, IDC_SLIDER_DIMMING);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// system settings...
		CheckDlgButton(hDlg, IDC_STARTW, conf->startWindows);
		CheckDlgButton(hDlg, IDC_STARTM, conf->startMinimized);
		CheckDlgButton(hDlg, IDC_BATTDIM, conf->dimmedBatt);
		CheckDlgButton(hDlg, IDC_SCREENOFF, conf->offWithScreen);
		CheckDlgButton(hDlg, IDC_CHECK_EFFECTS, conf->enableEffects);
		CheckDlgButton(hDlg, IDC_CHECK_EFFBAT, conf->effectsOnBattery);
		CheckDlgButton(hDlg, IDC_CHECK_LON, conf->lightsOn);
		CheckDlgButton(hDlg, IDC_POWER_DIM, conf->dimPowerButton);
		CheckDlgButton(hDlg, IDC_CHECK_GAMMA, conf->gammaCorrection);
		CheckDlgButton(hDlg, IDC_OFFPOWERBUTTON, !conf->offPowerButton);
		CheckDlgButton(hDlg, IDC_BUT_PROFILESWITCH, conf->enableProfSwitch);
		CheckDlgButton(hDlg, IDC_AWCC, conf->awcc_disable);
		CheckDlgButton(hDlg, IDC_ESIFTEMP, conf->esif_temp);
		CheckDlgButton(hDlg, IDC_FANCONTROL, conf->fanControl);
		CheckDlgButton(hDlg, IDC_CHECK_EXCEPTION, conf->noDesktop);
		CheckDlgButton(hDlg, IDC_CHECK_DIM, conf->dimmed);
		CheckDlgButton(hDlg, IDC_CHECK_UPDATE, conf->updateCheck);
		CheckDlgButton(hDlg, IDC_OFFONBATTERY, conf->offOnBattery);
		CheckDlgButton(hDlg, IDC_CHECK_LIGHTNAMES, conf->showGridNames);
		CheckDlgButton(hDlg, IDC_HOTKEYS, conf->keyShortcuts);
		SendMessage(dim_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(dim_slider, TBM_SETTICFREQ, 16, 0);
		SendMessage(dim_slider, TBM_SETPOS, true, conf->dimmingPower);
		sTip1 = CreateToolTip(dim_slider, sTip1);
		SetSlider(sTip1, conf->dimmingPower);
	} break;
	case WM_COMMAND: {
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
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
			conf->SetStates();
			fxhl->Refresh();
			break;
		case IDC_SCREENOFF:
			conf->offWithScreen = state;
			break;
		case IDC_BUT_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProfSwitch = state;
			eve->StartProfiles();
			ReloadProfileList();
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
			eve->ChangeEffectMode();
			break;
		case IDC_CHECK_EFFBAT:
			conf->effectsOnBattery = state;
			eve->ChangeEffectMode();
			break;
		case IDC_OFFPOWERBUTTON:
			conf->offPowerButton = !state;
			if (!conf->stateOn) {
				conf->stateOn = true;
				if (state) {
					fxhl->SetState(true);
				}
				fxhl->SetState();
			}
			break;
		case IDC_POWER_DIM:
			conf->dimPowerButton = state;
			if (conf->stateDimmed) {
				conf->stateDimmed = false;
				if (!state) {
					fxhl->SetState(true);
				}
				fxhl->SetState();
			}
			break;
		case IDC_OFFONBATTERY:
			conf->offOnBattery = state;
			fxhl->SetState();
			break;
		case IDC_AWCC:
			conf->awcc_disable = state;
			conf->Save();
			// Was, disabled = start
			// Was, enabled = nothing
			// Not, disabled = nothing
			// Not, enabled = stop
			// 1st - nothing, 2nd - function
			conf->wasAWCC = DoStopService((bool)conf->awcc_disable != conf->wasAWCC, conf->wasAWCC);
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
			fxhl->SetState();
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
			} else {
				eve->StopEffects();
				delete mon;
				mon = NULL;
				eve->StartEffects();
			}
			fxhl->FillAllDevs();
			SetHotkeys();
			SetMainTabs();
			break;
		case IDC_CHECK_LIGHTNAMES:
			conf->showGridNames = !conf->showGridNames;
			break;
		case IDC_HOTKEYS:
			conf->keyShortcuts = !conf->keyShortcuts;
			SetHotkeys();
			break;
		default: return false;
		}
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBPOSITION: case TB_ENDTRACK: {
			if ((HWND)lParam == dim_slider) {
				conf->dimmingPower = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				SetSlider(sTip1, conf->dimmingPower);
				//if (conf->stateDimmed) {
					conf->stateDimmed = false;
					fxhl->SetState();
				//}
			}
		} break;
		} break;
	default: return false;
	}
	return true;
}