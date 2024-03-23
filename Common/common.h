#pragma once
#include <ShlObj.h>
#include <string>
#include <vector>

using namespace std;

bool EvaluteToAdmin(HWND dlg = NULL);
bool DoStopService(bool flag, bool kind);
void ResetDPIScale(LPWSTR cmdLine);
DWORD WINAPI CUpdateCheck(LPVOID);
bool WindowsStartSet(bool, string);
string GetAppVersion();
HWND CreateToolTip(HWND, HWND);
void SetToolTip(HWND, string);
void SetSlider(HWND, int);
void UpdateCombo(HWND ctrl, vector<string> items, int sel = 0, vector<int> val = {});
void ShowNotification(NOTIFYICONDATA* niData, string title, string message);
void SetBitMask(WORD& val, WORD mask, bool state);
bool AddTrayIcon(NOTIFYICONDATA* iconData, bool needCheck);
//void OpenAbout(int res, int vt, int link);
void BlinkNumLock(int howmany);


