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
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey2,
        &dwDisposition);
    //RegCreateKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey2);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui\\Events"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey3,
        &dwDisposition);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui\\Profiles"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey4,
        &dwDisposition);

    testColor.ci = 0;
    testColor.cs.green = 255;
}
ConfigHandler::~ConfigHandler() {
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    RegCloseKey(hKey3);
    RegCloseKey(hKey4);
}
int ConfigHandler::Load() {
    int size = 4, size_c = 4*16;

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
    ret = RegGetValue(hKey1,
        NULL,
        TEXT("Monitoring"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &enableMon,
        (LPDWORD)&size);
    if (ret != ERROR_SUCCESS)
        enableMon = 1;
    RegGetValue(hKey1,
        NULL,
        TEXT("OffWithScreen"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &offWithScreen,
        (LPDWORD)&size);
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
    RegGetValue(hKey1,
        NULL,
        TEXT("ActiveProfile"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &activeProfile,
        (LPDWORD)&size);
    RegGetValue(hKey1,
        NULL,
        TEXT("OffPowerButton"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &offPowerButton,
        (LPDWORD)&size);
    RegGetValue(hKey1,
        NULL,
        TEXT("CustomColors"),
        RRF_RT_REG_BINARY | RRF_ZEROONFAILURE,
        NULL,
        customColors,
        (LPDWORD)&size_c);
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
        DWORD len = 255, lend = 0; profile prof;
        ret = RegEnumValueA(
            hKey4,
            vindex,
            name,
            &len,
            NULL,
            NULL,
            NULL,
            &lend
        );
        lend++; len = 255;
        char* profname = new char[lend];
        ret = RegEnumValueA(
            hKey4,
            vindex,
            name,
            &len,
            NULL,
            NULL,
            (LPBYTE)profname,
            &lend
        );
        if (ret == ERROR_SUCCESS) {
            vindex++;
            unsigned ret2 = sscanf_s((char*)name, "Profile-%d", &prof.id);
            if (ret2 == 1) {
                prof.name = profname;
                profiles.push_back(prof);
            }
        }
        delete profname;
    } while (ret == ERROR_SUCCESS);
    vindex = 0; ret = 0;
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
            unsigned profid;
            unsigned ret2 = sscanf_s((char*)name, "%d-%d-%d", &map.devid, &map.lightid, &profid);
            switch (ret2) {
            case 3: { // new format
                for (int i = 0; i < 4; i++) {
                    map.eve[i].fs.s = inarray[i * 10];
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
                for (int i = 0; i < profiles.size(); i++)
                    if (profiles[i].id == profid) {
                        profiles[i].lightsets.push_back(map);
                        break;
                    }
            }
            case 2: { // old format
                for (int i = 0; i < 4; i++) {
                    map.eve[i].fs.s = inarray[i * 10];
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
            } break;
            }
            vindex++;
        }
    } while (ret == ERROR_SUCCESS);
    stateDimmed = dimmed;
    stateOn = lightsOn;
    // set active profile...
    if (profiles.size() > 0) {
        for (int i = 0; i < profiles.size(); i++)
            if (profiles[i].id == activeProfile){
                mappings = profiles[i].lightsets;
                break;
            }
    }
    else {
        // need new profile
        profile prof;
        prof.id = 0;
        prof.name = "Default";
        prof.lightsets = mappings;
        profiles.push_back(prof);
    }
	return 0;
}
int ConfigHandler::Save() {
    char name[256];
    unsigned out[40];
    DWORD dwDisposition;

    if (startWindows) {
        char pathBuffer[2048];
        GetModuleFileNameA(NULL, pathBuffer, 2047);
        RegSetValueExA(hKey2, "Alienfx GUI", 0, REG_SZ, (BYTE*)pathBuffer, strlen(pathBuffer)+1);
    }
    else {
        // remove key.
        RegSetValueExA(hKey2, "Alienfx GUI", 0, REG_SZ, (BYTE*)&"", 1);
    }

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
        TEXT("Monitoring"),
        0,
        REG_DWORD,
        (BYTE*)&enableMon,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("OffWithScreen"),
        0,
        REG_DWORD,
        (BYTE*)&offWithScreen,
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
        TEXT("OffPowerButton"),
        0,
        REG_DWORD,
        (BYTE*)&offPowerButton,
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
    RegSetValueEx(
        hKey1,
        TEXT("ActiveProfile"),
        0,
        REG_DWORD,
        (BYTE*)&activeProfile,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("CustomColors"),
        0,
        REG_BINARY,
        (BYTE*)customColors,
        4*16
    );
    // clear old profiles
    RegDeleteTreeA(hKey1, "Profiles");
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxgui\\Profiles"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,//KEY_WRITE,
        NULL,
        &hKey4,
        &dwDisposition);
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
    // set current profile mappings to current set!
    profiles[activeProfile].lightsets = mappings;
    for (int j = 0; j < profiles.size(); j++) {
        sprintf_s((char*)name, 255, "Profile-%d", profiles[j].id);
        RegSetValueExA(
            hKey4,
            name,
            0,
            REG_SZ,
            (BYTE*)profiles[j].name.c_str(),
            profiles[j].name.length()
        );
        for (int i = 0; i < profiles[j].lightsets.size(); i++) {
            //preparing name
            lightset cur = profiles[j].lightsets[i];
            sprintf_s((char*)name, 255, "%d-%d-%d", cur.devid, cur.lightid, profiles[j].id);
            //preparing binary....
            UINT size = 40 * 4;// (UINT)mappings[i].map.size();
            for (int j = 0; j < 4; j++) {
                out[j * 10] = cur.eve[j].fs.s;
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
    }
	return 0;
}
