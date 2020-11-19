#include "ConfigHandler.h"
#include <algorithm>

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

bool ConfigHandler::sortMappings(lightset i, lightset j) { return (i.lightid < j.lightid); };

void ConfigHandler::updateProfileByID(int id, std::string name, std::string app, DWORD flags) {
    for (std::vector <profile>::iterator Iter = profiles.begin();
        Iter != profiles.end(); Iter++) {
        if (Iter->id == id) {
            // update profile..
            if (name != "")
                Iter->name = name;
            if (app != "")
                Iter->triggerapp = app;
            if (flags != -1)
                Iter->flags = flags;
            return;
        }
    }
    profile prof;
    // update data...
    prof.id = id;
    if (name != "")
        prof.name = name;
    if (app != "")
        prof.triggerapp = app;
    if (flags != -1)
        prof.flags = flags;
    profiles.push_back(prof);
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
    ret = RegGetValue(hKey1,
        NULL,
        TEXT("GammaCorrection"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &gammaCorrection,
        (LPDWORD)&size);
    if (ret != ERROR_SUCCESS)
        gammaCorrection = 1;
    ret = RegGetValue(hKey1,
        NULL,
        TEXT("ProfileAutoSwitch"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &enableProf,
        (LPDWORD)&size);
    if (ret != ERROR_SUCCESS)
        enableProf = 1;
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

    unsigned vindex = 0;
    BYTE inarray[380];
    char name[256]; int pid = -1, appid = -1;
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
        unsigned ret2 = sscanf_s((char*)name, "Profile-%d", &pid);
        if (ret == ERROR_SUCCESS && ret2 == 1) {
            char* profname = new char[lend];
            profname[0] = 0;
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
            updateProfileByID(pid, profname, "", -1);
            delete profname;
        }
        ret2 = sscanf_s((char*)name, "Profile-flags-%d", &pid);
        if (ret == ERROR_SUCCESS && ret2 == 1) {
            DWORD pFlags;
            ret = RegEnumValueA(
                hKey4,
                vindex,
                name,
                &len,
                NULL,
                NULL,
                (LPBYTE)&pFlags,
                &lend
            );
            updateProfileByID(pid, "", "", pFlags);
        }
        ret2 = sscanf_s((char*)name, "Profile-app-%d-%d", &pid, &appid);
        if (ret == ERROR_SUCCESS && ret2 == 2) {
            char* profname = new char[lend];
            profname[0] = 0;
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
            updateProfileByID(pid, "", profname, -1);
            delete profname;
        }
        vindex++;
    } while (ret == ERROR_SUCCESS);
    vindex = 0; ret = 0;
    do {
        DWORD len = 255, lend = 380; lightset map;
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
            unsigned ret2 = sscanf_s((char*)name, "Set-%d-%d-%d", &map.devid, &map.lightid, &profid);
            if (ret2 == 3) {
                BYTE* inPos = inarray;
                for (int i = 0; i < 4; i++) {
                    map.eve[i].fs.s = *((int*)inPos);
                    inPos += 4;
                    map.eve[i].source = *inPos;
                    inPos++;
                    BYTE mapSize = *inPos;
                    inPos++;
                    map.eve[i].map.clear();
                    for (int j = 0; j < mapSize; j++) {
                        map.eve[i].map.push_back(*((AlienFX_SDK::afx_act*)inPos));
                        inPos += sizeof(AlienFX_SDK::afx_act);
                    }
                }
                for (int i = 0; i < profiles.size(); i++)
                    if (profiles[i].id == profid) {
                        profiles[i].lightsets.push_back(map);
                        break;
                    }
            }
            else {
                ret2 = sscanf_s((char*)name, "%d-%d-%d", &map.devid, &map.lightid, &profid);
                if (ret2 == 3) {
                    AlienFX_SDK::afx_act action, action2;
                    unsigned* inar = (unsigned*)inarray;
                    Colorcode ccd;
                    for (int i = 0; i < 4; i++) {
                        map.eve[i].fs.s = inar[i * 10];
                        map.eve[i].source = inar[i * 10 + 1];
                        action.type = inar[i * 10 + 2];
                        action2.type = inar[i * 10 + 3];
                        ccd.ci = inar[i * 10 + 4];
                        action.r = ccd.cs.red;
                        action.g = ccd.cs.green;
                        action.b = ccd.cs.blue;
                        ccd.ci = inar[i * 10 + 5];
                        action2.r = ccd.cs.red;
                        action2.g = ccd.cs.green;
                        action2.b = ccd.cs.blue;
                        action.tempo = inar[i * 10 + 6];
                        action2.tempo = inar[i * 10 + 7];
                        action.time = inar[i * 10 + 8];
                        action2.time = inar[i * 10 + 9];
                        map.eve[i].map.push_back(action);
                        map.eve[i].map.push_back(action2);
                    }
                    for (int i = 0; i < profiles.size(); i++)
                        if (profiles[i].id == profid) {
                            profiles[i].lightsets.push_back(map);
                            break;
                        }
                }
            }
            vindex++;
        }
    } while (ret == ERROR_SUCCESS);
    stateDimmed = dimmed;
    stateOn = lightsOn;
    monState = enableMon;
    // set active profile...
    if (profiles.size() > 0) {
        for (int i = 0; i < profiles.size(); i++) {
            std::sort(profiles[i].lightsets.begin(), profiles[i].lightsets.end(), ConfigHandler::sortMappings);
            if (profiles[i].id == activeProfile) {
                mappings = profiles[i].lightsets;
                if (profiles[i].flags & 0x2)
                    monState = 0;
            }
        }
    }
    else {
        // need new profile
        profile prof;
        prof.id = 0;
        prof.flags = 1;
        prof.name = "Default";
        prof.lightsets = mappings;
        std::sort(mappings.begin(), mappings.end(), ConfigHandler::sortMappings);
        profiles.push_back(prof);
    }
    if (profiles.size() == 1)
        profiles[0].flags = profiles[0].flags | 0x1;
	return 0;
}
int ConfigHandler::Save() {
    char name[256];
    BYTE* out;
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
        TEXT("GammaCorrection"),
        0,
        REG_DWORD,
        (BYTE*)&gammaCorrection,
        4
    );
    RegSetValueEx(
        hKey1,
        TEXT("ProfileAutoSwitch"),
        0,
        REG_DWORD,
        (BYTE*)&enableProf,
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
        sprintf_s((char*)name, 255, "Profile-flags-%d", profiles[j].id);
        RegSetValueExA(
            hKey4,
            name,
            0,
            REG_DWORD,
            (BYTE*)&profiles[j].flags,
            sizeof(DWORD)
        );
        sprintf_s((char*)name, 255, "Profile-app-%d-0", profiles[j].id);
        RegSetValueExA(
            hKey4,
            name,
            0,
            REG_SZ,
            (BYTE*)profiles[j].triggerapp.c_str(),
            profiles[j].triggerapp.length()
        );
        for (int i = 0; i < profiles[j].lightsets.size(); i++) {
            //preparing name
            lightset cur = profiles[j].lightsets[i];
            sprintf_s((char*)name, 255, "Set-%d-%d-%d", cur.devid, cur.lightid, profiles[j].id);
            //preparing binary....
            UINT size = 6 * 4;// (UINT)mappings[i].map.size();
            for (int j = 0; j < 4; j++)
                size += cur.eve[j].map.size() * (sizeof(AlienFX_SDK::afx_act));
            out = (BYTE*)malloc(size);
            BYTE* outPos = out;
            Colorcode ccd;
            for (int j = 0; j < 4; j++) {
                *((int*)outPos) = cur.eve[j].fs.s;
                outPos += 4;
                *outPos = (BYTE) cur.eve[j].source;
                outPos++;
                *outPos = (BYTE) cur.eve[j].map.size();
                outPos++;
                for (int k = 0; k < cur.eve[j].map.size(); k++) {
                    memcpy(outPos, &cur.eve[j].map.at(k), sizeof(AlienFX_SDK::afx_act));
                    outPos += sizeof(AlienFX_SDK::afx_act);
                }
                /*out[j * 10 + 2] = cur.eve[j].map[0].type;
                out[j * 10 + 3] = cur.eve[j].map[1].type;
                ccd.cs.red = cur.eve[j].map[0].r;
                ccd.cs.green = cur.eve[j].map[0].g;
                ccd.cs.blue = cur.eve[j].map[0].b;
                out[j * 10 + 4] = ccd.ci;
                ccd.cs.red = cur.eve[j].map[1].r;
                ccd.cs.green = cur.eve[j].map[1].g;
                ccd.cs.blue = cur.eve[j].map[1].b;
                out[j * 10 + 5] = ccd.ci;
                out[j * 10 + 6] = cur.eve[j].map[0].tempo;
                out[j * 10 + 7] = cur.eve[j].map[1].tempo;
                out[j * 10 + 8] = cur.eve[j].map[0].time;
                out[j * 10 + 9] = cur.eve[j].map[1].time;*/
            }
            RegSetValueExA(
                hKey3,
                name,
                0,
                REG_BINARY,
                (BYTE*)out,
                size
            );
            free(out);
        }
    }
	return 0;
}
