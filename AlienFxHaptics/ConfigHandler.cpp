#include "ConfigHandler.h"

ConfigHandler::ConfigHandler() {
    DWORD  dwDisposition;
    //TCHAR  szData[] = TEXT("USR:SOFTWARE\\Alienfxhaptics\\Settings");
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxhaptics"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey1,
        &dwDisposition);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxhaptics\\Mappings"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey2,
        &dwDisposition);
}
ConfigHandler::~ConfigHandler() {
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    //RegCloseKey(hKey3);
}
int ConfigHandler::Load() {
    int size = 4;

    RegGetValue(hKey1,
        NULL,
        TEXT("Bars"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &numbars,
        (LPDWORD) &size);
    if (!numbars) { // no key
        numbars = 20;
    }
    RegGetValue(hKey1,
        NULL,
        TEXT("Power"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &res,
        (LPDWORD)&size);
    if (!res) {
        res = 10000;
    }
    RegGetValue(hKey1,
        NULL,
        TEXT("Input"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &inpType,
        (LPDWORD)&size);
    unsigned vindex = 0, inarray[30];
    TCHAR name[255];
    unsigned ret = 0;
    do {
        DWORD len = 255, lend = 25 * 4; mapping map;
        ret = RegEnumValue(
            hKey2,
            vindex,
            name,
            &len,
            NULL,
            NULL,
            (LPBYTE)inarray,
            &lend
        );
        // get id(s)...
        if (ret == ERROR_SUCCESS) {
            unsigned ret2 = sscanf_s(name, "%d-%d", &map.devid, &map.lightid);
            if (ret2 == 2) {
                map.colorfrom.ci = inarray[0];
                map.colorto.ci = inarray[1];
                map.lowcut = inarray[2];
                map.hicut = inarray[3];
                for (unsigned i = 4; i < (lend / 4); i++)
                    map.map.push_back(inarray[i]);
                mappings.push_back(map);
            }
            vindex++;
        }
    } while (ret == ERROR_SUCCESS);
	return 0;
}
int ConfigHandler::Save() {
    TCHAR name[256];
    unsigned out[30];

    RegSetValueEx(
        hKey1,
        TEXT("Bars"),
        0,
        REG_DWORD,
        (BYTE *) &numbars,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Power"),
        0,
        REG_DWORD,
        (BYTE*)&res,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Input"),
        0,
        REG_DWORD,
        (BYTE*)&inpType,
        4
    );
    for (int i = 0; i < mappings.size(); i++) {
        //preparing name
        sprintf_s(name, 255, "%d-%d", mappings[i].devid, mappings[i].lightid);
        //preparing binary....
        out[0] = mappings[i].colorfrom.ci;
        out[1] = mappings[i].colorto.ci;
        out[2] = mappings[i].lowcut;
        out[3] = mappings[i].hicut;
        int j, size;
        for (j = 0; j < mappings[i].map.size(); j++) {
            out[j + 4] = mappings[i].map[j];
        }
        size = (j + 4) * sizeof(unsigned);
        RegSetValueEx(
            hKey2,
            name,
            0,
            REG_BINARY,
            (BYTE*)out,
            size
        );
    }
	return 0;
}
