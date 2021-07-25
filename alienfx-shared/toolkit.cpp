#include "toolkit.h"
#include "AlienFX_SDK.h"

//namespace AlienFX_TOOLS
//{
HINSTANCE hInst;

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

void SetSlider(HWND tt, char* buff, int value) {
	TOOLINFO ti = { 0 };
	ti.cbSize = sizeof(ti);
	ti.lpszText = (LPTSTR)buff;
	if (tt) {
		SendMessage(tt, TTM_ENUMTOOLS, 0, (LPARAM)&ti);
		_itoa_s(value, buff, 4, 10);
		ti.lpszText = (LPTSTR)buff;
		SendMessage(tt, TTM_SETTOOLINFO, 0, (LPARAM)&ti);
	}
}
//}