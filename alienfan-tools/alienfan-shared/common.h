#pragma once
#include <libloaderapi.h>
#include <ShlObj.h>
#include <WinInet.h>

#pragma comment(lib,"Wininet.lib")

extern HWND mDlg;
extern string GetAppVersion();
extern bool needUpdateFeedback, isNewVersion;

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
                EndDialog(mDlg, 1);
            else
                _exit(1);  // Quit itself
        }
    }
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

DWORD WINAPI CUpdateCheck(LPVOID lparam) {
	NOTIFYICONDATA* niData = (NOTIFYICONDATA*)lparam;
	HINTERNET session, req;
	char buf[2048];
	DWORD byteRead;
	bool isConnectionFailed = false;
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
					res = res.substr(pos + 8, posf - pos - 8);
					size_t dotpos = res.find(".", 1 + res.find(".", 1 + res.find(".")));
					if (res.find(".", 1 + res.find(".", 1 + res.find("."))) == string::npos)
						res += ".0";
					if (res != GetAppVersion()) {
						// new version detected!
						strcpy_s(niData->szInfoTitle, "Update available!");
						strcpy_s(niData->szInfo, ("Latest version is " + res).c_str());
						isNewVersion = true;
					}
				}
			}
			else
				isConnectionFailed = true;
			InternetCloseHandle(req);
		}
		else
			isConnectionFailed = true;
		InternetCloseHandle(session);
	}
	else
		isConnectionFailed = true;
	if (needUpdateFeedback && !isNewVersion) {
		if (isConnectionFailed) {
			strcpy_s(niData->szInfoTitle, "Update check failed!");
			strcpy_s(niData->szInfo, "Can't connect to GitHub for update check.");
		}
		else {
			strcpy_s(niData->szInfoTitle, "You are up to date!");
			strcpy_s(niData->szInfo, "You are using latest version.");
		}
	}
	niData->uFlags |= NIF_INFO;
	Shell_NotifyIcon(NIM_MODIFY, niData);
	niData->uFlags &= ~NIF_INFO;
	return 0;
}