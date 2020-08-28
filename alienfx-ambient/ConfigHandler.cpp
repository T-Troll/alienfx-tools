#include "ConfigHandler.h"

ConfigHandler::ConfigHandler() {
    DWORD  dwDisposition;

    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxambient"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey1,
        &dwDisposition);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxambient\\Mappings"),
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
}
int ConfigHandler::Load() {
    int size = 4;

    RegGetValue(hKey1,
        NULL,
        TEXT("MaxColors"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &maxcolors,
        (LPDWORD) &size);
    if (!maxcolors) { // no key
        maxcolors = 20;
    }
    RegGetValue(hKey1,
        NULL,
        TEXT("Mode"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &mode,
        (LPDWORD)&size);
    RegGetValue(hKey1,
        NULL,
        TEXT("Divider"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &divider,
        (LPDWORD)&size);
    if (!divider) divider = 8;
    unsigned vindex = 0, inarray[30];
    char name[255];
    unsigned ret = 0;
    do {
        DWORD len = 255, lend = 25 * 4; mapping map;
        ret = RegEnumValueA(
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
            unsigned ret2 = sscanf_s((char *) name, "%d-%d", &map.devid, &map.lightid);
            if (ret2 != EOF) {
                //map.colorfrom.ci = inarray[0];
                //map.colorto.ci = inarray[1];
                //map.lowcut = inarray[2];
                //map.hicut = inarray[3];
                for (unsigned i = 0; i < (lend / 4); i++)
                    map.map.push_back(inarray[i]);
                mappings.push_back(map);
            }
            vindex++;
        }
    } while (ret == ERROR_SUCCESS);
	return 0;
}
int ConfigHandler::Save() {
    char name[256];
    unsigned out[30];

    RegSetValueEx(
        hKey1,
        TEXT("MaxColors"),
        0,
        REG_DWORD,
        (BYTE *) &maxcolors,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Mode"),
        0,
        REG_DWORD,
        (BYTE*)&mode,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Divider"),
        0,
        REG_DWORD,
        (BYTE*)&divider,
        4
    );
    for (int i = 0; i < mappings.size(); i++) {
        //preparing name
        sprintf_s((char *)name, 255, "%d-%d", mappings[i].devid, mappings[i].lightid);
        //preparing binary....
        int j, size;
        for (j = 0; j < mappings[i].map.size(); j++) {
            out[j] = mappings[i].map[j];
        }
        size = j * sizeof(unsigned);
        RegSetValueExA(
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
