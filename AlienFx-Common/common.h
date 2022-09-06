#pragma once
//#include <libloaderapi.h>
#include <ShlObj.h>
#include <WinInet.h>
#include <string>
#include <vector>

using namespace std;

bool EvaluteToAdmin();
bool DoStopService(bool flag, bool kind);
void ResetDPIScale();
DWORD WINAPI CUpdateCheck(LPVOID);
bool WindowsStartSet(bool, string);
string GetAppVersion();
HWND CreateToolTip(HWND, HWND);
void SetToolTip(HWND, string);
void SetSlider(HWND, int);
void UpdateCombo(HWND ctrl, vector<string> items, int sel = 0, vector<int> val = {});
void ShowNotification(NOTIFYICONDATA* niData, string title, string message, bool type);
void SetBitMask(WORD& val, WORD mask, bool state);