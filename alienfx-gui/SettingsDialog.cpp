#include "alienfx-gui.h"

//bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map);
bool SetColor(HWND hDlg, int id, BYTE *r, BYTE *g, BYTE *b);
void ReloadProfileList(HWND hDlg);
DWORD EvaluteToAdmin();
bool DoStopService(bool kind);
void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b);
HWND CreateToolTip(HWND hwndParent, HWND oldTip);
void SetSlider(HWND tt, int value);

BOOL CALLBACK TabSettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND eff_list = GetDlgItem(hDlg, IDC_GLOBAL_EFFECT),
		eff_tempo = GetDlgItem(hDlg, IDC_SLIDER_TEMPO),
		dim_slider = GetDlgItem(hDlg, IDC_SLIDER_DIMMING);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// system settings...
		CheckDlgButton(hDlg, IDC_STARTW, conf->startWindows);
		CheckDlgButton(hDlg, IDC_STARTM, conf->startMinimized);
		CheckDlgButton(hDlg, IDC_AUTOREFRESH, conf->autoRefresh);
		CheckDlgButton(hDlg, IDC_BATTDIM, conf->dimmedBatt);
		CheckDlgButton(hDlg, IDC_SCREENOFF, conf->offWithScreen);
		CheckDlgButton(hDlg, IDC_CHECK_EFFECTS, conf->enableMon);
		CheckDlgButton(hDlg, IDC_CHECK_LON, conf->lightsOn);
		CheckDlgButton(hDlg, IDC_POWER_DIM, conf->dimPowerButton);
		CheckDlgButton(hDlg, IDC_CHECK_GAMMA, conf->gammaCorrection);
		CheckDlgButton(hDlg, IDC_OFFPOWERBUTTON, !conf->offPowerButton);
		CheckDlgButton(hDlg, IDC_BUT_PROFILESWITCH, conf->enableProf);
		CheckDlgButton(hDlg, IDC_AWCC, conf->awcc_disable);
		CheckDlgButton(hDlg, IDC_ESIFTEMP, conf->esif_temp);
		CheckDlgButton(hDlg, IDC_FANCONTROL, conf->fanControl);
		CheckDlgButton(hDlg, IDC_CHECK_EXCEPTION, conf->noDesktop);
		CheckDlgButton(hDlg, IDC_CHECK_DIM, conf->dimmed);
		SendMessage(dim_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(dim_slider, TBM_SETTICFREQ, 16, 0);
		SendMessage(dim_slider, TBM_SETPOS, true, conf->dimmingPower);
		sTip = CreateToolTip(dim_slider, sTip);
		SetSlider(sTip, conf->dimmingPower);
		// set global effect, colors and delay
		if (conf->haveV5) {
			ComboBox_AddString(eff_list, "None");
			ComboBox_SetItemData(eff_list, 0, 0);
			ComboBox_AddString(eff_list, "Color");
			ComboBox_SetItemData(eff_list, 1, 1);
			ComboBox_AddString(eff_list, "Breathing");
			ComboBox_SetItemData(eff_list, 2, 2);
			ComboBox_AddString(eff_list, "Single-color Wave");
			ComboBox_SetItemData(eff_list, 3, 3);
			ComboBox_AddString(eff_list, "Dual-color Wave ");
			ComboBox_SetItemData(eff_list, 4, 4);
			ComboBox_AddString(eff_list, "Pulse");
			ComboBox_SetItemData(eff_list, 5, 8);
			ComboBox_AddString(eff_list, "Mixed Pulse");
			ComboBox_SetItemData(eff_list, 6, 9);
			ComboBox_AddString(eff_list, "Night Rider");
			ComboBox_SetItemData(eff_list, 7, 10);
			ComboBox_AddString(eff_list, "Lazer");
			ComboBox_SetItemData(eff_list, 8, 11);
			ComboBox_SetCurSel(eff_list, conf->globalEffect);
			// now sliders...
			SendMessage(eff_tempo, TBM_SETRANGE, true, MAKELPARAM(0, 0xa));
			SendMessage(eff_tempo, TBM_SETTICFREQ, 1, 0);
			SendMessage(eff_tempo, TBM_SETPOS, true, conf->globalDelay);
			lTip = CreateToolTip(eff_tempo, lTip);
			SetSlider(lTip, conf->globalDelay);
		} else {
			EnableWindow(eff_list, false);
			EnableWindow(eff_tempo, false);
			EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_EFFCLR1), false);
			EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_EFFCLR2), false);
		}
	} break;
	case WM_COMMAND: {
		int eItem = ComboBox_GetCurSel(eff_list);
		bool state = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		switch (LOWORD(wParam))
		{
		case IDC_GLOBAL_EFFECT: {
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				conf->globalEffect = (DWORD) ComboBox_GetItemData(eff_list, eItem);
				fxhl->UpdateGlobalEffect();
			} break;
			}
		} break;
		case IDC_BUTTON_EFFCLR1:
		{
			SetColor(hDlg, IDC_BUTTON_EFFCLR1, &conf->effColor1.r, &conf->effColor1.g, &conf->effColor1.b);
			fxhl->UpdateGlobalEffect();
		} break;
		case IDC_BUTTON_EFFCLR2:
		{
			SetColor(hDlg, IDC_BUTTON_EFFCLR2, &conf->effColor2.r, &conf->effColor2.g, &conf->effColor2.b);
			fxhl->UpdateGlobalEffect();
		} break;
		case IDC_STARTM:
			conf->startMinimized = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_AUTOREFRESH:
			conf->autoRefresh = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_STARTW:
		{
			conf->startWindows = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			char pathBuffer[2048];
			string shellcomm;
			if (conf->startWindows) {
				GetModuleFileNameA(NULL, pathBuffer, 2047);
				shellcomm = "Register-ScheduledTask -TaskName \"AlienFX-GUI\" -trigger $(New-ScheduledTaskTrigger -Atlogon) -settings $(New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -ExecutionTimeLimit 0) -action $(New-ScheduledTaskAction -Execute '"
					+ string(pathBuffer) + "') -force -RunLevel Highest";
				ShellExecute(NULL, "runas", "powershell.exe", shellcomm.c_str(), NULL, SW_HIDE);
			} else {
				shellcomm = "/delete /F /TN \"Alienfx-GUI\"";
				ShellExecute(NULL, "runas", "schtasks.exe", shellcomm.c_str(), NULL, SW_HIDE);
			}
		} break;
		case IDC_BATTDIM:
			conf->dimmedBatt = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->RefreshState();
			break;
		case IDC_SCREENOFF:
			conf->offWithScreen = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_BUT_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProf = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			ReloadProfileList(NULL);
			eve->StartProfiles();
			break;
		case IDC_CHECK_LON:
			conf->lightsOn = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->ChangeState();
			break;
		case IDC_CHECK_GAMMA:
			conf->gammaCorrection = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->RefreshState();
			break;
		case IDC_CHECK_EFFECTS:
			conf->enableMon = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			eve->ToggleEvents();
			break;
		case IDC_OFFPOWERBUTTON:
			conf->offPowerButton = !state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_UNCHECKED);
			fxhl->ChangeState();
			break;
		case IDC_POWER_DIM:
			conf->dimPowerButton = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->ChangeState();
			break;
		case IDC_AWCC:
			conf->awcc_disable = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (!conf->awcc_disable) {
				if (conf->wasAWCC) DoStopService(false);
			}
			else
				conf->wasAWCC = DoStopService(true);
			break;
		case IDC_ESIFTEMP:
			conf->esif_temp = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (conf->esif_temp)
				EvaluteToAdmin(); // Check admin rights!
			break;
		case IDC_CHECK_EXCEPTION:
			conf->noDesktop = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_CHECK_DIM:
			conf->dimmed = state;// IsDlgButtonChecked(hDlg, LOWORD(wParam));
			fxhl->ChangeState();
			break;
		case IDC_FANCONTROL:
			conf->fanControl = state;// (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (conf->fanControl) {
				EvaluteToAdmin();
				acpi = new AlienFan_SDK::Control();
				if (acpi->IsActivated() && acpi->Probe()) {
					conf->fan_conf->SetBoosts(acpi);
					/*conf->fan_conf = new ConfigHelper();
					conf->fan_conf->Load();*/
					mon = new MonHelper(NULL, NULL, conf->fan_conf, acpi);
					eve->mon = mon;
				} else {
					MessageBox(NULL, "Supported hardware not found. Fan control will be disabled!", "Error",
								MB_OK | MB_ICONHAND);
					delete acpi;
					acpi = NULL;
					conf->fanControl = false;
					CheckDlgButton(hDlg, IDC_FANCONTROL, BST_UNCHECKED);
				}
			} else {
				// Stop all services
				if (acpi && acpi->IsActivated()) {
					eve->mon = NULL;
					delete mon;
				}
				delete acpi;
				acpi = NULL;
			}
			break;
		default: return false;
		}
	} break;
	case WM_HSCROLL:
		switch (LOWORD(wParam)) {
		case TB_THUMBPOSITION: case TB_ENDTRACK: {
			if ((HWND)lParam == dim_slider) {
				conf->dimmingPower = (DWORD)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				SetSlider(sTip, conf->dimmingPower);
				fxhl->ChangeState();
			}
			if ((HWND)lParam == eff_tempo) {
				conf->globalDelay = (BYTE)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				SetSlider(lTip, conf->globalDelay);
				fxhl->UpdateGlobalEffect();
			}
		} break;
		} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT*) lParam)->CtlID) {
		case IDC_BUTTON_EFFCLR1:
			RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, conf->effColor1.r, conf->effColor1.g, conf->effColor1.b);
			break;
		case IDC_BUTTON_EFFCLR2:
			RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, conf->effColor2.r, conf->effColor2.g, conf->effColor2.b);
			break;
		}
		break;
	default: return false;
	}
	return true;
}