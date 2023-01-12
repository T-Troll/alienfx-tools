#include "alienfx-gui.h"

HHOOK dEvent;
HWND kDlg;
AlienFX_SDK::Afx_light* keySetLight;

string GetKeyName(WORD vkcode) {
	string res;
	UINT scancode = MapVirtualKey(vkcode & 0xff, MAPVK_VK_TO_VSC_EX) | (vkcode & KF_EXTENDED);
	res.resize(128);
	res.resize(GetKeyNameText(scancode << 16, (LPSTR)res.c_str(), 127));
	res.shrink_to_fit();
	if (res.empty())
		res = "Unknown key";
	return res;
}

LRESULT CALLBACK DetectKeyProc(int nCode, WPARAM wParam, LPARAM lParam) {

	if (wParam == WM_KEYUP) {
		LPKBDLLHOOKSTRUCT keyblock = (LPKBDLLHOOKSTRUCT)lParam;
		//keySetLight->name = GetKeyName(keyblock->vkCode | ((keyblock->flags & 1) << 8));
		keySetLight->name.resize(128);
		keySetLight->name.resize(GetKeyNameText(
			(keyblock->scanCode | ((keyblock->flags & 1) << 8)) << 16,
			(LPSTR)keySetLight->name.c_str(), 127));
		keySetLight->name.shrink_to_fit();
		if (keySetLight->name.empty())
			keySetLight->name = "Unknown key";
		keySetLight->scancode = (WORD)(keyblock->vkCode | ((keyblock->flags & 1) << 8));
		SendMessage(kDlg, WM_CLOSE, 0, 0);
	}

	return true;
}

BOOL CALLBACK KeyPressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_INITDIALOG:
		kDlg = hDlg;
		dEvent = SetWindowsHookEx(WH_KEYBOARD_LL, DetectKeyProc, NULL, 0);
		break;
	case WM_CLOSE:
		UnhookWindowsHookEx(dEvent);
		EndDialog(hDlg, IDCLOSE);
		break;
	default: return false;
	}
	return true;
}