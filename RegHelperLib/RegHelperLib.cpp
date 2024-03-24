#include "RegHelperLib.h"

DWORD GetRegData(HKEY key, int vindex, char* name, byte** data) {
	DWORD len, lend;
	if (*data) {
		delete[] * data;
		*data = NULL;
	}
	if (RegEnumValue(key, vindex, name, &(len = 255), NULL, NULL, NULL, &lend) == ERROR_SUCCESS) {
		*data = new byte[lend];
		RegEnumValue(key, vindex, name, &(len = 255), NULL, NULL, *data, &lend);
		return lend;
	}
	return 0;
}

string GetRegString(byte* data, int len) {
	string res;
	if (!data[len - 1])
		len--;
	res.resize(len);
	memcpy((void*)res.data(), data, len);
	return res;
}
