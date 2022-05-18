#pragma once
//#include <libloaderapi.h>
#include <ShlObj.h>
#include <WinInet.h>
#include <string>

using namespace std;

void EvaluteToAdmin();
void ResetDPIScale();
DWORD WINAPI CUpdateCheck(LPVOID lparam);
bool WindowsStartSet(bool kind, string name);
string GetAppVersion();

