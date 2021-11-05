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

void ConfigAmbient::GetReg(char *name, DWORD *value, DWORD defValue) {
    DWORD size = sizeof(DWORD);
    if (RegGetValueA(hKey1, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
        *value = defValue;
}

void ConfigAmbient::SetReg(char *text, DWORD value) {
    RegSetValueEx( hKey1, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

int ConfigAmbient::Load() {
    int size = 4;

    GetReg("Shift", &shift);
    GetReg("Mode", &mode);
    //GetReg("Divider", &divider);
    GetReg("GammaCorrection", &gammaCorrection, 1);
    //if (!divider || divider > 32) divider = 16;
    unsigned vindex = 0, inarray[12*4];
    char name[256];
    LSTATUS ret = 0;
    do {
        DWORD len = 255, lend = 12 * sizeof(DWORD); mapping map;
        // get id(s)...
        if ((ret = RegEnumValueA( hKey2, vindex, name, &len, NULL, NULL, (LPBYTE)inarray, &lend )) == ERROR_SUCCESS) {
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

    SetReg("Shift", shift);
    SetReg("Mode", mode);
    //SetReg("Divider", divider);
    SetReg("GammaCorrection", gammaCorrection);
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
            RegSetValueExA( hKey2, name.c_str(), 0, REG_BINARY, (BYTE*)out, size );
        }
    }
	return 0;
}

//bool ConfigAmbient::sortMappings(mapping i, mapping j)
//{
//    return i.lightid < j.lightid;
//}
