#pragma once
#include <wtypes.h>
#include <string>

using namespace std;

DWORD GetRegData(HKEY key, int vindex, char* name, byte** data);
string GetRegString(byte* data, int len);