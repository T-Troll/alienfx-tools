#include "Common.h"
#include <windowsx.h>

using namespace std;

#pragma comment(lib,"Wininet.lib")

extern HWND mDlg;
extern bool needUpdateFeedback, isNewVersion, needRemove;

void EvaluteToAdmin() {
	// Evaluation attempt...
	char szPath[MAX_PATH];
	if (!IsUserAnAdmin()) {
		if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
		{
			// Launch itself as admin
			SHELLEXECUTEINFO sei{ sizeof(sei), 0, NULL, "runas", szPath, NULL, NULL, SW_NORMAL };
			ShellExecuteEx(&sei);
			if (mDlg)
				SendMessage(mDlg, WM_CLOSE, 0, 0);
			else
				_exit(1);  // Quit itself
		}
	}
}

bool DoStopService(bool flag, bool kind) {
	if (flag) {
		EvaluteToAdmin();

		SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);
		SC_HANDLE schService = schSCManager ? OpenService(schSCManager, "AWCCService", SERVICE_ALL_ACCESS) : NULL;
		SERVICE_STATUS  serviceStatus;
		bool rCode = false;
		if (!schSCManager || !schService)
			return false;
		if (kind) {
			// stop service
			rCode = (BOOLEAN)ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
		}
		else {
			// start service
			/*rCode = (BOOLEAN)*/StartService(schService, 0, NULL);
			/*if (!rCode && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
				rCode = true;*/
		}
		CloseServiceHandle(schService);
		return rCode;
	}
	return false;
}

void ResetDPIScale() {
	HKEY dpiKey;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &dpiKey, NULL) == ERROR_SUCCESS) {
		char pathBuffer[2048];
		GetModuleFileNameA(NULL, pathBuffer, 2047);
		const char setValue[] = "~ GDIDPISCALING DPIUNAWARE";
		RegSetValueEx(dpiKey, pathBuffer, 0, REG_SZ, (byte*)setValue, (DWORD)strlen(setValue));
		RegCloseKey(dpiKey);
	}
}

void ShowNotification(NOTIFYICONDATA* niData, string title, string message, bool type) {
	needRemove = type;
	strcpy_s(niData->szInfoTitle, title.c_str());
	strcpy_s(niData->szInfo, message.c_str());
	niData->uFlags |= NIF_INFO;
	Shell_NotifyIcon(NIM_MODIFY, niData);
	niData->uFlags &= ~NIF_INFO;
}

DWORD WINAPI CUpdateCheck(LPVOID lparam) {
	NOTIFYICONDATA* niData = (NOTIFYICONDATA*)lparam;
	HINTERNET session, req;
	char buf[2048];
	DWORD byteRead;
	bool isConnectionFailed = true;
	// Wait connection for a while
	if (!needUpdateFeedback)
		Sleep(10000);
	if (session = InternetOpen("alienfx-tools", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) {
		if (req = InternetOpenUrl(session, "https://api.github.com/repos/t-troll/alienfx-tools/tags?per_page=1",
			NULL, 0, 0, NULL)) {
			if (InternetReadFile(req, buf, 2047, &byteRead)) {
				buf[byteRead] = 0;
				string res = buf;
				size_t pos = res.find("\"name\":"),
					posf = res.find("\"", pos + 8);
				if (pos != string::npos) {
					isConnectionFailed = false;
					res = res.substr(pos + 8, posf - pos - 8);
					size_t dotpos = res.find(".", 1 + res.find(".", 1 + res.find(".")));
					if (res.find(".", 1 + res.find(".", 1 + res.find("."))) == string::npos)
						res += ".0";
					if (res != GetAppVersion()) {
						// new version detected!
						ShowNotification(niData, "Update available!", "Latest version is " + res, false);
						isNewVersion = true;
					} else
						if (needUpdateFeedback)
							ShowNotification(niData, "You are up to date!", "You are using latest version.", true);
				}
			}
			InternetCloseHandle(req);
		}
		InternetCloseHandle(session);
	}
	if (needUpdateFeedback && isConnectionFailed) {
		ShowNotification(niData, "Update check failed!", "Can't connect to GitHub for update check.", true);
	}
	return 0;
}

bool WindowsStartSet(bool kind, string name) {
	char pathBuffer[2048];
	if (kind) {
		GetModuleFileName(NULL, pathBuffer, 2047);
		//string tname = "Register-ScheduledTask -TaskName \"" + name + "\" -trigger $(New-ScheduledTaskTrigger -Atlogon) -settings $(New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0) -action $(New-ScheduledTaskAction -Execute '"
		//	+ string(pathBuffer) + /*"' -Argument '-d'*/"') -force -RunLevel Highest";
		return ShellExecute(NULL, "runas", "powershell.exe", ("Register-ScheduledTask -TaskName \"" + name + "\" -trigger $(New-ScheduledTaskTrigger -Atlogon) -settings $(New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0) -action $(New-ScheduledTaskAction -Execute '"
			+ string(pathBuffer) + /*"' -Argument '-d'*/"') -force -RunLevel Highest").c_str(), NULL, SW_HIDE) > (HINSTANCE)32;
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
	TOOLINFO ti{ sizeof(TOOLINFO) };
	if (tt) {
		SendMessage(tt, TTM_ENUMTOOLS, 0, (LPARAM)&ti);
		string toolTip = to_string(value);
		ti.lpszText = (LPTSTR)toolTip.c_str();
		SendMessage(tt, TTM_SETTOOLINFO, 0, (LPARAM)&ti);
	}
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