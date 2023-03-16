#include "Common.h"
#include <windowsx.h>
#include <WinInet.h>

using namespace std;

#pragma comment(lib,"Wininet.lib")
#pragma comment(lib,"Version.lib")

extern HWND mDlg;
extern bool needUpdateFeedback, isNewVersion;

//int versionTag, linkControl;
//
//INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	UNREFERENCED_PARAMETER(lParam);
//	switch (message)
//	{
//	case WM_INITDIALOG: {
//		SetDlgItemText(hDlg, versionTag, ("Version: " + GetAppVersion()).c_str());
//	} break;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDOK: case IDCANCEL:
//		{
//			EndDialog(hDlg, LOWORD(wParam));
//		} break;
//		}
//		break;
//	case WM_NOTIFY:
//		if (LOWORD(wParam) == linkControl) {
//			switch (((LPNMHDR)lParam)->code)
//			{
//			case NM_CLICK:
//			case NM_RETURN:
//			{
//				ShellExecute(NULL, "open", "https://github.com/T-Troll/alienfx-tools", NULL, NULL, SW_SHOWNORMAL);
//			} break;
//			} break;
//		}
//		break;
//	default: return (INT_PTR)FALSE;
//	}
//	return (INT_PTR)TRUE;
//}
//
//void OpenAbout(int res, int vt, int link) {
//	versionTag = vt; linkControl = link;
//	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(res), mDlg, About);
//}

DWORD WINAPI Blinker(LPVOID lparam) {
	int howmany = (int)(ULONGLONG)lparam << 1;
	for (int i = 0; i < howmany; i++) {
		keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
		keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		Sleep(300);
	}
	return 0;
}

void BlinkNumLock(int howmany) {
	CreateThread(NULL, 0, Blinker, (LPVOID)(ULONGLONG)howmany, 0, NULL);
}

bool EvaluteToAdmin(HWND dlg) {
	// Evaluation attempt...
	if (!IsUserAnAdmin()) {
		char szPath[MAX_PATH];
		GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
		// Launch itself as admin
		SHELLEXECUTEINFO sei{ sizeof(sei), 0, NULL, "runas", szPath, NULL, NULL, SW_NORMAL };
		if (ShellExecuteEx(&sei)) {
			// Quit this instance
			if (dlg)
				SendMessage(dlg, WM_CLOSE, 0, 0);
			else
				_exit(1);
		}
		return false;
	}
	return true;
}

bool DoStopService(bool flag, bool kind) {
	bool rCode = false;
	if (flag && EvaluteToAdmin()) {
		SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
		SC_HANDLE schService = schSCManager ? OpenService(schSCManager, "AWCCService", SERVICE_ALL_ACCESS) : NULL;
		SERVICE_STATUS  serviceStatus;
		if (schSCManager && schService) {
			rCode = kind ? ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus) :
				StartService(schService, 0, NULL);
			CloseServiceHandle(schService);
		}
	}
	return rCode;
}

void ResetDPIScale(LPWSTR cmdLine) {
	HKEY dpiKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &dpiKey, NULL) == ERROR_SUCCESS) {
		char pathBuffer[2048];
		GetModuleFileNameA(NULL, pathBuffer, 2047);
		const char setValue[] = "~ GDIDPISCALING DPIUNAWARE";
		RegSetValueEx(dpiKey, pathBuffer, 0, REG_SZ, (byte*)setValue, (DWORD)strlen(setValue));
		RegCloseKey(dpiKey);
	}
	if (wcslen(cmdLine)) {
		Sleep(_wtoi(cmdLine) * 1000);
	}
}

void ShowNotification(NOTIFYICONDATA* niData, string title, string message) {
	//QUERY_USER_NOTIFICATION_STATE cns;
	//SHQueryUserNotificationState(&cns);
	//if (cns == QUNS_ACCEPTS_NOTIFICATIONS) {
		strcpy_s(niData->szInfoTitle, title.c_str());
		strcpy_s(niData->szInfo, message.c_str());
		niData->uFlags |= NIF_INFO;
		//niData->dwInfoFlags = NIIF_ERROR;
		if (!Shell_NotifyIcon(NIM_MODIFY, niData))
			Shell_NotifyIcon(NIM_ADD, niData);
		niData->uFlags &= ~NIF_INFO;
	//}
}

DWORD WINAPI CUpdateCheck(LPVOID lparam) {
	NOTIFYICONDATA* niData = (NOTIFYICONDATA*)lparam;
	HINTERNET session, req;
	char* buf = new char[255];
	DWORD byteRead;
	bool isConnectionFailed = true;
	// Wait connection for a while
	if (!needUpdateFeedback)
		Sleep(10000);
	if (session = InternetOpen("alienfx-tools", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) {
		if (req = InternetOpenUrl(session, "https://api.github.com/repos/t-troll/alienfx-tools/tags?per_page=1",
			NULL, 0, 0, NULL)) {
			if (InternetReadFile(req, buf, 254, &byteRead)) {
				buf[byteRead] = 0;
				string res = buf;
				size_t pos = res.find("\"name\":");
				if (pos != string::npos) {
					isConnectionFailed = false;
					res = res.substr(pos + 8);
					res = res.substr(0, res.find("\""));
					int ndots = 0;
					for (auto i = res.begin(); i != res.end(); i++)
						if (*i == '.')
							ndots++;
					while (ndots++ < 3)
						res += ".0";
					if (res.compare(GetAppVersion()) > 0) {
						// new version detected!
						ShowNotification(niData, "Update available!", "Latest version is " + res);
						isNewVersion = true;
					} else
						if (needUpdateFeedback)
							ShowNotification(niData, "You are up to date!", "You are using latest version.");
				}
			}
			InternetCloseHandle(req);
		}
		InternetCloseHandle(session);
	}
	if (needUpdateFeedback && isConnectionFailed)
			ShowNotification(niData, "Update check failed!", "Can't connect to GitHub for update check.");
	delete[] buf;
	return 0;
}

bool WindowsStartSet(bool kind, string name) {
	if (kind) {
		char* pathBuffer = new char[2048];
		GetModuleFileName(NULL, pathBuffer, 2047);
		bool res = ShellExecute(NULL, "runas", "powershell.exe", ("Register-ScheduledTask -TaskName \"" + name + "\" -trigger $(New-ScheduledTaskTrigger -Atlogon) -settings $(New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0) -action $(New-ScheduledTaskAction -Execute '"
			+ string(pathBuffer) + /*"' -Argument '-d'*/"') -force -RunLevel Highest").c_str(), NULL, SW_HIDE) > (HINSTANCE)32;
		delete[] pathBuffer;
		return res;
	}
	else {
		return ShellExecute(NULL, "runas", "schtasks.exe", ("/delete /F /TN \"" + name + "\"").c_str(), NULL, SW_HIDE) > (HINSTANCE)32;
	}
}

string GetAppVersion() {

	HMODULE hInst = GetModuleHandle(NULL);

	HRSRC hResInfo = FindResource(hInst, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);

	string res;

	if (hResInfo) {
		DWORD dwSize = SizeofResource(hInst, hResInfo);
		HGLOBAL hResData = LoadResource(hInst, hResInfo);
		if (hResData) {
			LPVOID pRes = LockResource(hResData),
				pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
			if (pResCopy) {
				UINT uLen = 0;
				VS_FIXEDFILEINFO* lpFfi = NULL;

				CopyMemory(pResCopy, pRes, dwSize);

				VerQueryValue(pResCopy, TEXT("\\"), (LPVOID*)&lpFfi, &uLen);

				res = to_string(HIWORD(lpFfi->dwProductVersionMS)) + "."
					+ to_string(LOWORD(lpFfi->dwProductVersionMS)) + "."
					+ to_string(HIWORD(lpFfi->dwProductVersionLS)) + "."
					+ to_string(LOWORD(lpFfi->dwProductVersionLS));

				LocalFree(pResCopy);
			}
			FreeResource(hResData);
		}
	}
	return res;
}

HWND CreateToolTip(HWND hwndParent, HWND oldTip)
{
	// Create a tool tip.
	if (oldTip) DestroyWindow(oldTip);

	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		0, 0, 0, 0, hwndParent, NULL, GetModuleHandle(NULL), NULL);
	TOOLINFO ti{ sizeof(TOOLINFO), TTF_SUBCLASS, hwndParent };
	GetClientRect(hwndParent, &ti.rect);
	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	return hwndTT;
}

void SetToolTip(HWND tt, string value) {
	TOOLINFO ti{ sizeof(TOOLINFO) };
	if (tt) {
		SendMessage(tt, TTM_ENUMTOOLS, 0, (LPARAM)&ti);
		ti.lpszText = (LPTSTR)value.c_str();
		SendMessage(tt, TTM_SETTOOLINFO, 0, (LPARAM)&ti);
	}
}

void SetSlider(HWND tt, int value) {
	SetToolTip(tt, to_string(value));
}

void UpdateCombo(HWND ctrl, vector<string> items, int sel, vector<int> val) {
	ComboBox_ResetContent(ctrl);
	for (int i = 0; i < items.size(); i++) {
		ComboBox_AddString(ctrl, items[i].c_str());
		if (val.size()) {
			ComboBox_SetItemData(ctrl, i, val[i]);
			if (sel == val[i])
				ComboBox_SetCurSel(ctrl, i);
		} else
			if (sel == i)
				ComboBox_SetCurSel(ctrl, sel);
	}
}

void SetBitMask(WORD& val, WORD mask, bool state) {
	val = (val & ~mask) | (state * mask);
}

bool AddTrayIcon(NOTIFYICONDATA* iconData, bool needCheck) {
	if (!Shell_NotifyIcon(NIM_MODIFY, iconData) && !Shell_NotifyIcon(NIM_ADD, iconData))
			return false;
	if (needCheck)
		CreateThread(NULL, 0, CUpdateCheck, iconData, 0, NULL);
	return true;
}
