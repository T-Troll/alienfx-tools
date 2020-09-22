#include "ConfigHandler.h"

ConfigHandler::ConfigHandler() {
    DWORD  dwDisposition;

    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey1,
        &dwDisposition);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui\\Colors"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey2,
        &dwDisposition);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui\\Events"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey3,
        &dwDisposition);
}
ConfigHandler::~ConfigHandler() {
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    RegCloseKey(hKey3);
}
int ConfigHandler::Load() {
    int size = 4;

    /*RegGetValue(hKey1,
        NULL,
        TEXT("MaxColors"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &maxcolors,
        (LPDWORD) &size);
    if (!maxcolors) { // no key
        maxcolors = 12;
    }
    RegGetValue(hKey1,
        NULL,
        TEXT("Shift"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &shift,
        (LPDWORD)&size);
    if (!shift) { // no key
        shift = 40;// 0;
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
    if (!divider) divider = 8;*/
    unsigned vindex = 0, inarray[8];
    char name[256];
    unsigned ret = 0;
    do {
        DWORD len = 255, lend = 8 * 4; mapping map;
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
            if (ret2 == 2) {
                map.mode = inarray[0];
                map.mode2 = inarray[1];
                map.c1.ci = inarray[2];
                map.c2.ci = inarray[3];
                map.speed1 = inarray[4];
                map.speed2 = inarray[5];
                map.length1 = inarray[6];
                map.length2 = inarray[7];
                /*if (lend > 0) {
                    for (unsigned i = 0; i < (lend / 4); i++)
                        map.map.push_back(inarray[i]);
                    mappings.push_back(map);
                }*/
            }
            vindex++;
        }
    } while (ret == ERROR_SUCCESS);
	return 0;
}
int ConfigHandler::Save() {
    char name[256];
    unsigned out[8];

    /*RegSetValueEx(
        hKey1,
        TEXT("MaxColors"),
        0,
        REG_DWORD,
        (BYTE *) &maxcolors,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Shift"),
        0,
        REG_DWORD,
        (BYTE*)&shift,
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
    );*/
    for (int i = 0; i < mappings.size(); i++) {
        //preparing name
        sprintf_s((char*)name, 255, "%d-%d", mappings[i].devid, mappings[i].lightid);
        //preparing binary....
        UINT size = 8 * 4;// (UINT)mappings[i].map.size();
        out[0] = mappings[i].mode;
        out[1] = mappings[i].mode2;
        out[2] = mappings[i].c1.ci;
        out[3] = mappings[i].c2.ci;
        out[4] = mappings[i].speed1;
        out[5] = mappings[i].speed2;
        out[6] = mappings[i].length1;
        out[7] = mappings[i].length2;
 
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
