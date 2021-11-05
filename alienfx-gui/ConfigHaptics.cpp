#include "ConfigHaptics.h"
#include <algorithm>
#include <string>

using namespace std;

ConfigHaptics::ConfigHaptics() {
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxhaptics"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey1,
        NULL);
    RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Alienfxhaptics\\Mappings"),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey2,
        NULL);
    Load();
}
ConfigHaptics::~ConfigHaptics() {
    Save();
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
}

void ConfigHaptics::GetReg(char *name, DWORD *value, DWORD defValue) {
    DWORD size = sizeof(DWORD);
    if (RegGetValueA(hKey1, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
        *value = defValue;
}

void ConfigHaptics::SetReg(char *text, DWORD value) {
    RegSetValueEx( hKey1, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

int ConfigHaptics::Load() {
    int size = 4;

    GetReg("Bars", &numbars, 20);
    GetReg("Axis", &showAxis);
    GetReg("Input", &inpType);

    unsigned vindex = 0,
        *inarray = new unsigned[numbars+5];
    char name[255];
    LSTATUS ret = 0;
    do {
        DWORD len = 255, lend = (numbars+5) * sizeof(DWORD); haptics_map map;
        // get id(s)...
        if ((ret = RegEnumValueA( hKey2, vindex, name, &len, NULL, NULL, (LPBYTE)inarray, &lend )) == ERROR_SUCCESS) {
            vindex++;
            //Old format, not used anymore
            //if (sscanf_s(name, "%u-%u", &map.devid, &map.lightid) == 2) {
            //    map.colorfrom.ci = inarray[0];
            //    map.colorto.ci = inarray[1];
            //    map.lowcut = inarray[2];
            //    map.hicut = inarray[3];
            //    map.flags = 0;
            //    for (unsigned i = 4; i < (lend / 4); i++)
            //        map.map.push_back(inarray[i]);
            //    mappings.push_back(map);
            //    continue;
            //}
            if (sscanf_s(name, "Map%u-%u", &map.devid, &map.lightid) == 2) {
                map.colorfrom.ci = inarray[0];
                map.colorto.ci = inarray[1];
                map.lowcut = inarray[2];
                map.hicut = inarray[3];
                map.flags = inarray[4];
                for (unsigned i = 5; i < (lend / sizeof(unsigned)); i++)
                    map.map.push_back(inarray[i]);
                mappings.push_back(map);
                continue;
            }
        }
    } while (ret == ERROR_SUCCESS);
    delete[] inarray;
    //std::sort(mappings.begin(), mappings.end(), sortMappings);
	return 0;
}
int ConfigHaptics::Save() {

    SetReg("Bars", numbars);
    SetReg("Axis", showAxis);
    SetReg("Input", inpType);

    RegDeleteTreeA(hKey1, "Mappings");
    RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxhaptics\\Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey2, NULL);
    for (int i = 0; i < mappings.size(); i++) {
        if (mappings[i].map.size()) {
            //preparing name
            string name = "Map" + to_string(mappings[i].devid) + "-" + to_string(mappings[i].lightid);
            //preparing binary....
            unsigned *out = new unsigned[mappings[i].map.size() + 5];
            out[0] = mappings[i].colorfrom.ci;
            out[1] = mappings[i].colorto.ci;
            out[2] = mappings[i].lowcut;
            out[3] = mappings[i].hicut;
            out[4] = mappings[i].flags;

            for (int j = 0; j < mappings[i].map.size(); j++) {
                out[j + 5] = mappings[i].map[j];
            }
            RegSetValueExA( hKey2, name.c_str(), 0, REG_BINARY, (BYTE *) out, (DWORD) (mappings[i].map.size() + 5) * sizeof(unsigned) );
            delete[] out;
        }
    }
	return 0;
}

//bool ConfigHaptics::sortMappings(haptics_map i, haptics_map j)
//{
//    return i.lightid < j.lightid;
//}



