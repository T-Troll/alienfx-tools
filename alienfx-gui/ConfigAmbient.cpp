#include "ConfigAmbient.h"
#include <algorithm>
#include <string>

using namespace std;

ConfigAmbient::ConfigAmbient() {

    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxambient"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey1,
        NULL);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxambient\\Mappings"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey2,
        NULL);
    Load();
}
ConfigAmbient::~ConfigAmbient() {
    Save();
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
}
int ConfigAmbient::Load() {
    int size = 4;

    RegGetValue(hKey1,
        NULL,
        TEXT("Shift"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &shift,
        (LPDWORD)&size);
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
    unsigned ret = RegGetValue(hKey1,
        NULL,
        TEXT("GammaCorrection"),
        RRF_RT_DWORD | RRF_ZEROONFAILURE,
        NULL,
        &gammaCorrection,
        (LPDWORD)&size);
    if (ret != ERROR_SUCCESS)
        gammaCorrection = 1;
    if (!divider || divider > 32) divider = 16;
    unsigned vindex = 0, inarray[12*4];
    char name[256];
    ret = 0;
    do {
        DWORD len = 255, lend = 12 * 4; mapping map;
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
            unsigned ret2 = sscanf_s(name, "%d-%d", &map.devid, &map.lightid);
            if (ret2 == 2) {
                if (lend > 0) {
                    for (unsigned i = 0; i < (lend / 4); i++)
                        map.map.push_back(inarray[i]);
                    mappings.push_back(map);
                }
            }
            vindex++;
        }
    } while (ret == ERROR_SUCCESS);
    //std::sort(mappings.begin(), mappings.end(), sortMappings);
	return 0;
}
int ConfigAmbient::Save() {

    unsigned out[12*4];

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
    );
    RegSetValueEx(
        hKey1,
        TEXT("GammaCorrection"),
        0,
        REG_DWORD,
        (BYTE*)&gammaCorrection,
        4
    );
    for (int i = 0; i < mappings.size(); i++) {
        //preparing name
        string name = to_string(mappings[i].devid) + "-" + to_string(mappings[i].lightid);
        //preparing binary....
        UINT j, size = (UINT) mappings[i].map.size();
        if (size > 0) {
            for (j = 0; j < size; j++) {
                out[j] = mappings[i].map[j];
            }
            size *= sizeof(unsigned);
            RegSetValueExA(
                hKey2,
                name.c_str(),
                0,
                REG_BINARY,
                (BYTE*)out,
                size
            );
        }
    }
	return 0;
}

//bool ConfigAmbient::sortMappings(mapping i, mapping j)
//{
//    return i.lightid < j.lightid;
//}
