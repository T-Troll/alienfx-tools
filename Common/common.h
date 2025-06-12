#pragma once
#include <ShlObj.h>
#include <string>
#include <vector>

using namespace std;

bool EvaluteToAdmin(HWND dlg = NULL);
bool DoStopAWCC(bool flag, bool kind);
void ResetDPIScale(LPWSTR cmdLine);
DWORD WINAPI CUpdateCheck(LPVOID);
bool WindowsStartSet(bool, string);
string GetAppVersion(bool isFile = true);
HWND CreateToolTip(HWND, HWND);
void SetToolTip(HWND, string);
void SetSlider(HWND, int);
void UpdateCombo(HWND ctrl, const string* items, int sel = 0, vector<int> val = {});
void UpdateCombo(HWND ctrl, const char* items[], int sel = 0, const int val[] = NULL);
void ShowNotification(NOTIFYICONDATA* niData, string title, string message);
void SetBitMask(WORD& val, WORD mask, bool state);
bool AddTrayIcon(NOTIFYICONDATA* iconData, bool needCheck);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void BlinkNumLock(int howmany);
bool WarningBox(HWND hDlg, const char* msg);


