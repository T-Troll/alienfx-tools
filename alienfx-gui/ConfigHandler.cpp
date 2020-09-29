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
    /*RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui\\Colors"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey2,
        &dwDisposition);*/
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
    //RegCloseKey(hKey2);
    RegCloseKey(hKey3);
}
int ConfigHandler::Load() {
    int size = 4;

    RegGetValue(hKey1,
        NULL,
        TEXT("AutoStart"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &startWindows,
        (LPDWORD) &size);
    RegGetValue(hKey1,
        NULL,
        TEXT("Minimized"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &startMinimized,
        (LPDWORD)&size);
    RegGetValue(hKey1,
        NULL,
        TEXT("Refresh"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &autoRefresh,
        (LPDWORD)&size);
    unsigned ret = RegGetValue(hKey1,
        NULL,
        TEXT("LightsOn"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &lightsOn,
        (LPDWORD)&size);
    if (ret != ERROR_SUCCESS)
        lightsOn = 1;
    RegGetValue(hKey1,
        NULL,
        TEXT("Dimmed"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &dimmed,
        (LPDWORD)&size);
    RegGetValue(hKey1,
        NULL,
        TEXT("DimmedOnBattery"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &dimmedBatt,
        (LPDWORD)&size);
    ret = RegGetValue(hKey1,
        NULL,
        TEXT("DimmingPower"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &dimmingPower,
        (LPDWORD)&size);
    if (ret != ERROR_SUCCESS)
        dimmingPower = 92;

    unsigned vindex = 0, inarray[40];
    char name[256];
    ret = 0;
    do {
        DWORD len = 255, lend = 40*4; lightset map;
        ret = RegEnumValueA(
            hKey3,
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
                for (int i = 0; i < 4; i++) {
                    map.eve[i].flags = inarray[i*10];
                    map.eve[i].source = inarray[i * 10 + 1];
                    map.eve[i].map.mode = inarray[i * 10 + 2];
                    map.eve[i].map.mode2 = inarray[i * 10 + 3];
                    map.eve[i].map.c1.ci = inarray[i * 10 + 4];
                    map.eve[i].map.c2.ci = inarray[i * 10 + 5];
                    map.eve[i].map.speed1 = inarray[i * 10 + 6];
                    map.eve[i].map.speed2 = inarray[i * 10 + 7];
                    map.eve[i].map.length1 = inarray[i * 10 + 8];
                    map.eve[i].map.length2 = inarray[i * 10 + 9];
                }
                mappings.push_back(map);
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
    unsigned out[40];
    DWORD dwDisposition;

    RegSetValueEx(
        hKey1,
        TEXT("AutoStart"),
        0,
        REG_DWORD,
        (BYTE *) &startWindows,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Minimized"),
        0,
        REG_DWORD,
        (BYTE*)&startMinimized,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Refresh"),
        0,
        REG_DWORD,
        (BYTE*)&autoRefresh,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("LightsOn"),
        0,
        REG_DWORD,
        (BYTE*)&lightsOn,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("Dimmed"),
        0,
        REG_DWORD,
        (BYTE*)&dimmed,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("DimmedOnBattery"),
        0,
        REG_DWORD,
        (BYTE*)&dimmedBatt,
        4
    );
    RegSetValueEx(
            hKey1,
            TEXT("DimmingPower"),
            0,
            REG_DWORD,
            (BYTE*)&dimmingPower,
            4
        );
    // clear old events
    RegDeleteTreeA(hKey1, "Events");
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui\\Events"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey3,
        &dwDisposition);
    for (int i = 0; i < mappings.size(); i++) {
        //preparing name
        lightset cur = mappings[i];
        sprintf_s((char*)name, 255, "%d-%d", cur.devid, cur.lightid);
        //preparing binary....
        UINT size = 40*4;// (UINT)mappings[i].map.size();
        for (int j = 0; j < 4; j++) {
            out[j * 10] = cur.eve[j].flags;
            out[j * 10 + 1] = cur.eve[j].source;
            out[j * 10 + 2] = cur.eve[j].map.mode;
            out[j * 10 + 3] = cur.eve[j].map.mode2;
            out[j * 10 + 4] = cur.eve[j].map.c1.ci;
            out[j * 10 + 5] = cur.eve[j].map.c2.ci;
            out[j * 10 + 6] = cur.eve[j].map.speed1;
            out[j * 10 + 7] = cur.eve[j].map.speed2;
            out[j * 10 + 8] = cur.eve[j].map.length1;
            out[j * 10 + 9] = cur.eve[j].map.length2;
        }
        RegSetValueExA(
            hKey3,
            name,
            0,
            REG_BINARY,
            (BYTE*)out,
            size
        );
        /*size = mappings[i].events.size() * 12;
        for (int j = 0; j < mappings[i].events.size(); j++) {
            out[j * 3] = mappings[i].events[j].type;
            out[j * 3 + 1] = mappings[i].events[j].source;
            out[j * 3 + 2] = mappings[i].events[j].color.ci;
        }
        if (size > 0)
            RegSetValueExA(
                hKey3,
                name,
                0,
                REG_BINARY,
                (BYTE*)out,
                size
            );*/
    }
	return 0;
}
