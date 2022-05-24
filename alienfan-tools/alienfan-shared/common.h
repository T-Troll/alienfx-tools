#pragma once
//#include <libloaderapi.h>
#include <ShlObj.h>
#include <WinInet.h>
#include <string>

using namespace std;

void EvaluteToAdmin();
void ResetDPIScale();
DWORD WINAPI CUpdateCheck(LPVOID);
bool WindowsStartSet(bool, string);
string GetAppVersion();
HWND CreateToolTip(HWND, HWND);
void SetToolTip(HWND, string);

