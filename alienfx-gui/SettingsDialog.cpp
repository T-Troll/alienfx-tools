#include "alienfx-gui.h"
#include "EventHandler.h"

extern void ReloadProfileList();
extern DWORD EvaluteToAdmin();
extern bool DoStopService(bool kind);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);

extern EventHandler* eve;
extern AlienFan_SDK::Control* acpi;

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
		//CheckDlgButton(hDlg, IDC_AUTOREFRESH, conf->autoRefresh);
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
			char pathBuffer[2048];
			string shellcomm;
			if (conf->startWindows) {
				GetModuleFileNameA(NULL, pathBuffer, 2047);
				shellcomm = "Register-ScheduledTask -TaskName \"AlienFX-GUI\" -trigger $(New-ScheduledTaskTrigger -Atlogon) -settings $(New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0) -action $(New-ScheduledTaskAction -Execute '"
					+ string(pathBuffer) + "') -force -RunLevel Highest";
				ShellExecute(NULL, "runas", "powershell.exe", shellcomm.c_str(), NULL, SW_HIDE);
			} else {
				shellcomm = "/delete /F /TN \"Alienfx-GUI\"";
				ShellExecute(NULL, "runas", "schtasks.exe", shellcomm.c_str(), NULL, SW_HIDE);
			}
		} break;
		case IDC_BATTDIM:
			conf->dimmedBatt = state;
			fxhl->RefreshState();
			break;
		case IDC_SCREENOFF:
			conf->offWithScreen = state;
			break;
		case IDC_BUT_PROFILESWITCH:
			eve->StopProfiles();
			conf->enableProf = state;
			ReloadProfileList();
			eve->StartProfiles();
			break;
		case IDC_CHECK_LON:
			conf->lightsOn = state;
			fxhl->ChangeState();
			eve->ToggleEvents();
			break;
		case IDC_CHECK_GAMMA:
			conf->gammaCorrection = state;
			fxhl->RefreshState();
			break;
		case IDC_CHECK_EFFECTS:
			conf->enableMon = state;
			eve->ToggleEvents();
			break;
		case IDC_OFFPOWERBUTTON:
			conf->offPowerButton = !state;
			fxhl->ChangeState();
			break;
		case IDC_POWER_DIM:
			conf->dimPowerButton = state;
			fxhl->ChangeState();
			break;
		case IDC_AWCC:
			conf->awcc_disable = state;
			if (!conf->awcc_disable) {
				if (conf->wasAWCC) DoStopService(false);
			}
			else
				conf->wasAWCC = DoStopService(true);
			break;
		case IDC_ESIFTEMP:
			conf->esif_temp = state;
			if (conf->esif_temp)
				EvaluteToAdmin(); // Check admin rights!
			break;
		case IDC_CHECK_EXCEPTION:
			conf->noDesktop = state;
			break;
		case IDC_CHECK_DIM:
			conf->dimmed = state;
			fxhl->ChangeState();
			break;
		case IDC_FANCONTROL:
			conf->fanControl = state;
			if (conf->fanControl) {
				EvaluteToAdmin();
				acpi = new AlienFan_SDK::Control();
				if (acpi->IsActivated() && acpi->Probe()) {
					conf->fan_conf->SetBoosts(acpi);
					eve->StartFanMon(acpi);
					// check for ACPI lights
					fxhl->UnblockUpdates(false);
					fxhl->FillAllDevs(acpi);
					fxhl->UnblockUpdates(true);
				} else {
					MessageBox(NULL, "Supported hardware not found. Fan control will be disabled!", "Error",
								MB_OK | MB_ICONHAND);
					delete acpi;
					acpi = NULL;
					conf->fanControl = false;
					CheckDlgButton(hDlg, IDC_FANCONTROL, BST_UNCHECKED);
				}
			} else {
				// Stop all fan services
				if (acpi && acpi->IsActivated()) {
					eve->StopFanMon();
					fxhl->UnblockUpdates(false);
					fxhl->FillAllDevs(NULL);
					fxhl->UnblockUpdates(true);
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
				SetSlider(sTip1, conf->dimmingPower);
				if (conf->IsDimmed())
					fxhl->ChangeState();
			}
		} break;
		} break;
	default: return false;
	}
	return true;
}