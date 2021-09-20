#include "alienfx-gui.h"
#include <windowsx.h>

bool SetColor(HWND hDlg, int id, lightset* mmap, AlienFX_SDK::afx_act* map);
void ReloadProfileList(HWND hDlg);
DWORD EvaluteToAdmin();
bool DoStopService(bool kind);
AlienFan_SDK::Control *InitAcpi();

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
		if (conf->startWindows) CheckDlgButton(hDlg, IDC_STARTW, BST_CHECKED);
		if (conf->startMinimized) CheckDlgButton(hDlg, IDC_STARTM, BST_CHECKED);
		if (conf->autoRefresh) CheckDlgButton(hDlg, IDC_AUTOREFRESH, BST_CHECKED);
		if (conf->dimmedBatt) CheckDlgButton(hDlg, IDC_BATTDIM, BST_CHECKED);
		if (conf->offWithScreen) CheckDlgButton(hDlg, IDC_SCREENOFF, BST_CHECKED);
		if (conf->enableMon) CheckDlgButton(hDlg, IDC_BUT_MONITOR, BST_CHECKED);
		if (conf->lightsOn) CheckDlgButton(hDlg, IDC_CHECK_LON, BST_CHECKED);
		if (conf->dimmed) CheckDlgButton(hDlg, IDC_CHECK_DIM, BST_CHECKED);
		if (conf->dimPowerButton) CheckDlgButton(hDlg, IDC_POWER_DIM, BST_CHECKED);
		if (conf->gammaCorrection) CheckDlgButton(hDlg, IDC_CHECK_GAMMA, BST_CHECKED);
		if (conf->offPowerButton) CheckDlgButton(hDlg, IDC_OFFPOWERBUTTON, BST_CHECKED);
		if (conf->enableProf) CheckDlgButton(hDlg, IDC_BUT_PROFILESWITCH, BST_CHECKED);
		if (conf->awcc_disable) CheckDlgButton(hDlg, IDC_AWCC, BST_CHECKED);
		if (conf->esif_temp) CheckDlgButton(hDlg, IDC_ESIFTEMP, BST_CHECKED);
		if (conf->fanControl) CheckDlgButton(hDlg, IDC_FANCONTROL, BST_CHECKED);
		SendMessage(dim_slider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(dim_slider, TBM_SETTICFREQ, 16, 0);
		SendMessage(dim_slider, TBM_SETPOS, true, conf->dimmingPower);
		sTip = CreateToolTip(dim_slider, sTip);
		SetSlider(sTip, conf->dimmingPower);
		// set effect, colors and delay
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
	} break;
	case WM_COMMAND: {
		int eItem = ComboBox_GetCurSel(eff_list);
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
			AlienFX_SDK::afx_act c;
			c.r = conf->effColor1.cs.red;
			c.g = conf->effColor1.cs.green;
			c.b = conf->effColor1.cs.blue;
			SetColor(hDlg, IDC_BUTTON_EFFCLR1, NULL, &c);
			conf->effColor1.cs.red = c.r;
			conf->effColor1.cs.green = c.g;
			conf->effColor1.cs.blue = c.b;
			fxhl->UpdateGlobalEffect();
		} break;
		case IDC_BUTTON_EFFCLR2:
		{
			AlienFX_SDK::afx_act c;
			c.r = conf->effColor2.cs.red;
			c.g = conf->effColor2.cs.green;
			c.b = conf->effColor2.cs.blue;
			SetColor(hDlg, IDC_BUTTON_EFFCLR2, NULL, &c);
			conf->effColor2.cs.red = c.r;
			conf->effColor2.cs.green = c.g;
			conf->effColor2.cs.blue = c.b;
			fxhl->UpdateGlobalEffect();
		} break;
		case IDC_STARTM:
			conf->startMinimized = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_AUTOREFRESH:
			conf->autoRefresh = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_STARTW:
		{
			conf->startWindows = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
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
			conf->dimmedBatt = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->RefreshState();
			break;
		case IDC_SCREENOFF:
			conf->offWithScreen = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			break;
		case IDC_BUT_MONITOR:
			conf->enableMon = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			eve->ToggleEvents();
			break;
		case IDC_BUT_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProf = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			ReloadProfileList(NULL);
			eve->StartProfiles();
			break;
		case IDC_CHECK_LON:
			conf->lightsOn = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->ChangeState();
			break;
		case IDC_CHECK_DIM:
			conf->dimmed = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			//fxhl->RefreshState();
			fxhl->ChangeState();
			break;
		case IDC_CHECK_GAMMA:
			conf->gammaCorrection = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			fxhl->RefreshState();
			break;
		case IDC_OFFPOWERBUTTON:
			conf->offPowerButton = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			//fxhl->RefreshState();
			fxhl->ChangeState();
			break;
		case IDC_POWER_DIM:
			conf->dimPowerButton = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			//fxhl->RefreshState();
			fxhl->ChangeState();
			break;
		case IDC_AWCC:
			conf->awcc_disable = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (!conf->awcc_disable) {
				if (conf->wasAWCC) DoStopService(false);
			}
			else
				conf->wasAWCC = DoStopService(true);
			break;
		case IDC_ESIFTEMP:
			conf->esif_temp = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (conf->esif_temp)
				EvaluteToAdmin(); // Check admin rights!
			break;
		case IDC_FANCONTROL:
			conf->fanControl = (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
			if (conf->fanControl) {
				EvaluteToAdmin();
				acpi = InitAcpi();
				if (acpi && acpi->Probe()) {
					mon = new MonHelper(NULL, NULL, fan_conf, acpi);
					mon->Start();
					eve->SetFanMon(mon);
				} else {
					MessageBox(NULL, "Supported hardware not found. Fan control will be disabled!", "Error",
								MB_OK | MB_ICONHAND);
					if (acpi) delete acpi;
					acpi = NULL;
					conf->fanControl = false;
					CheckDlgButton(hDlg, IDC_FANCONTROL, BST_UNCHECKED);
				}
			} else {
				// Stop all services
				if (acpi && acpi->IsActivated()) {
					mon->Stop();
					eve->SetFanMon(NULL);
					delete mon;
				}
				if (acpi)
					delete acpi;
				acpi = NULL;
				//if (MessageBox(NULL, "Do you want to restore system settings?", "Warning!",
				//			   MB_YESNO | MB_ICONWARNING) == IDYES) {
				//	string shellcom = "/set testsigning off";
				//	ShellExecute(NULL, "runas", "bcdedit", shellcom.c_str(), NULL, SW_HIDE);
				//	MessageBox(NULL, "System settings restored. Please restart you system.", "Information",
				//			   MB_OK | MB_ICONHAND);
				//}
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
			RedrawButton(hDlg, IDC_BUTTON_EFFCLR1, conf->effColor1.cs.red, conf->effColor1.cs.green, conf->effColor1.cs.blue);
			break;
		case IDC_BUTTON_EFFCLR2:
			RedrawButton(hDlg, IDC_BUTTON_EFFCLR2, conf->effColor2.cs.red, conf->effColor2.cs.green, conf->effColor2.cs.blue);
			break;
		}
		break;
	default: return false;
	}
	return true;
}