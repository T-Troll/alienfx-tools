#include "toolkit.h"
#include <wininet.h>
#pragma comment(lib,"Wininet.lib")

//namespace AlienFX_TOOLS
//{
HINSTANCE hInst;
bool isNewVersion = false;

void RedrawButton(HWND hDlg, unsigned id, BYTE r, BYTE g, BYTE b) {
	RECT rect;
	HBRUSH Brush = NULL;
	HWND tl = GetDlgItem(hDlg, id);
	GetWindowRect(tl, &rect);
	HDC cnt = GetWindowDC(tl);
	rect.bottom -= rect.top;
	rect.right -= rect.left;
	rect.top = rect.left = 0;
	// BGR!
	Brush = CreateSolidBrush(RGB(r, g, b));
	FillRect(cnt, &rect, Brush);
	DrawEdge(cnt, &rect, EDGE_RAISED, BF_RECT);
	DeleteObject(Brush);
	ReleaseDC(tl, cnt);
}

HWND CreateToolTip(HWND hwndParent, HWND oldTip)
{
	// Create a tooltip.
	if (oldTip) {
		DestroyWindow(oldTip);
	} 
	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
							WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							hwndParent, NULL, hInst, NULL);

	SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	TOOLINFO ti = { 0 };
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = hwndParent;
	ti.hinst = hInst;
	ti.lpszText = (LPTSTR)"0";

	GetClientRect(hwndParent, &ti.rect);

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	return hwndTT;
}

void SetSlider(HWND tt, int value) {
		TOOLINFO ti = { 0 };
	ti.cbSize = sizeof(ti);
	if (tt) {
		SendMessage(tt, TTM_ENUMTOOLS, 0, (LPARAM) &ti);
		string toolTip = to_string(value);
		ti.lpszText = (LPTSTR) toolTip.c_str();
		SendMessage(tt, TTM_SETTOOLINFO, 0, (LPARAM) &ti);
	}

}

string GetAppVersion() {

	HRSRC hResInfo;
	DWORD dwSize;
	HGLOBAL hResData;
	LPVOID pRes, pResCopy;
	UINT uLen;
	VS_FIXEDFILEINFO* lpFfi;

	string res = "";

	hResInfo = FindResource(hInst, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	if (hResInfo) {
		dwSize = SizeofResource(hInst, hResInfo);
		hResData = LoadResource(hInst, hResInfo);
		if (hResData) {
			pRes = LockResource(hResData);
			pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
			if (pResCopy) {
				CopyMemory(pResCopy, pRes, dwSize);

				VerQueryValue(pResCopy, TEXT("\\"), (LPVOID*)&lpFfi, &uLen);

				DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
				DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

				res = to_string(HIWORD(dwFileVersionMS)) + "."
					+ to_string(LOWORD(dwFileVersionMS)) + "."
					+ to_string(HIWORD(dwFileVersionLS)) + "."
					+ to_string(LOWORD(dwFileVersionLS));

				LocalFree(pResCopy);
			}
			FreeResource(hResData);
		}
	}
	return res;
}

DWORD WINAPI CUpdateCheck(LPVOID lparam) {
	NOTIFYICONDATA* niData = (NOTIFYICONDATA*) lparam;
	HINTERNET session, req;
	char buf[2048];
	DWORD byteRead;
	if (session = InternetOpen("alienfx-tools", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) {
		if (req = InternetOpenUrl(session, "https://api.github.com/repos/t-troll/alienfx-tools/tags?per_page=1",
								  NULL, 0, 0, NULL)) {
			if (InternetReadFile(req, buf, 2047, &byteRead)) {
				buf[byteRead] = 0;
				string res = buf;
				size_t pos = res.find("\"name\":"), 
					posf = res.find("\"", pos + 8);
				res = res.substr(pos + 8, posf - pos - 8);
				size_t dotpos = res.find(".", 1+ res.find(".", 1 + res.find(".")));
				if (res.find(".", 1+ res.find(".", 1 + res.find("."))) == string::npos)
					res += ".0";
				if (res != GetAppVersion()) {
					// new version detected!
					niData->uFlags |= NIF_INFO;
					strcpy_s(niData->szInfoTitle, "Update avaliable!");
					strcpy_s(niData->szInfo, ("Version " + res + " released at GitHub.").c_str());
					Shell_NotifyIcon(NIM_MODIFY, niData);
					niData->uFlags &= ~NIF_INFO;
					isNewVersion = true;
				}
			}
			InternetCloseHandle(req);
		}
		InternetCloseHandle(session);
	}
	return 0;
}

//}