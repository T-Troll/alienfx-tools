#include "ConfigHaptics.h"
#include <string>

using namespace std;

ConfigHaptics::ConfigHaptics() {
    RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxhaptics"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMainKey, NULL);
    RegCreateKeyEx(hMainKey, TEXT("Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMappingKey, NULL);
    Load();
}
ConfigHaptics::~ConfigHaptics() {
    //Save();
    RegCloseKey(hMainKey);
    RegCloseKey(hMappingKey);
}

void ConfigHaptics::GetReg(char *name, DWORD *value, DWORD defValue) {
    DWORD size = sizeof(DWORD);
    if (RegGetValueA(hMainKey, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
        *value = defValue;
}

void ConfigHaptics::SetReg(char *text, DWORD value) {
    RegSetValueEx( hMainKey, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

void ConfigHaptics::Load() {

    //GetReg("Bars", &numbars, 20);
    GetReg("Axis", &showAxis, 1);
    GetReg("Input", &inpType);

    unsigned vindex = 0;
    DWORD inarray[25]{0};// = new DWORD[numbars + 5];
    char name[255];
    LSTATUS ret = 0;
    do {
        DWORD len = 255, lend = 25/*(numbars+5)*/ * sizeof(DWORD); haptics_map map;
        // get id(s)...
        if ((ret = RegEnumValueA( hMappingKey, vindex, name, &len, NULL, NULL, (LPBYTE)inarray, &lend )) == ERROR_SUCCESS) {
            vindex++;
            if (sscanf_s(name, "Map%u-%u", &map.devid, &map.lightid) == 2 && (map.devid || map.lightid)) {
                map.colorfrom.ci = inarray[0];
                map.colorto.ci = inarray[1];
                map.lowcut = (byte) inarray[2];
                map.hicut = (byte) inarray[3];
                map.flags = (byte) inarray[4];
                for (unsigned i = 5; i < (lend / sizeof(DWORD)); i++)
                    map.map.push_back((byte)inarray[i]);
                haptics.push_back(map);
                continue;
            }
        }
    } while (ret == ERROR_SUCCESS);
    //delete[] inarray;
}
void ConfigHaptics::Save() {

    //SetReg("Bars", numbars);
    SetReg("Axis", showAxis);
    SetReg("Input", inpType);

    RegDeleteTreeA(hMainKey, "Mappings");
    RegCreateKeyEx(hMainKey, TEXT("Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMappingKey, NULL);
    for (int i = 0; i < haptics.size(); i++) {
        if (haptics[i].map.size()) {
            //preparing name
            string name = "Map" + to_string(haptics[i].devid) + "-" + to_string(haptics[i].lightid);
            //preparing binary....
            unsigned *out = new unsigned[haptics[i].map.size() + 5];
            out[0] = haptics[i].colorfrom.ci;
            out[1] = haptics[i].colorto.ci;
            out[2] = haptics[i].lowcut;
            out[3] = haptics[i].hicut;
            out[4] = haptics[i].flags;

            for (int j = 0; j < haptics[i].map.size(); j++) {
                out[j + 5] = haptics[i].map[j];
            }
            RegSetValueExA( hMappingKey, name.c_str(), 0, REG_BINARY, (BYTE *) out, (DWORD) (haptics[i].map.size() + 5) * sizeof(unsigned) );
            delete[] out;
        }
    }
}



