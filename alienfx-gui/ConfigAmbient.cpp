#include "ConfigAmbient.h"
#include <string>
#include <winreg.h>
#include <winerror.h>

using namespace std;

ConfigAmbient::ConfigAmbient() {

    RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxambient"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMainKey, NULL);
    RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxambient\\Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMappingKey, NULL);
    Load();
}
ConfigAmbient::~ConfigAmbient() {
    Save();
    RegCloseKey(hMainKey);
    RegCloseKey(hMappingKey);
}

void ConfigAmbient::GetReg(char *name, DWORD *value, DWORD defValue) {
    DWORD size = sizeof(DWORD);
    if (RegGetValueA(hMainKey, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
        *value = defValue;
}

void ConfigAmbient::SetReg(char *text, DWORD value) {
    RegSetValueEx(hMainKey, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
}

void ConfigAmbient::Load() {
    int size = 4;

    GetReg("Shift", &shift);
    GetReg("Mode", &mode);
    //GetReg("Divider", &divider);
    GetReg("GammaCorrection", &gammaCorrection, 1);
    unsigned /*vindex = 0,*/ inarray[12 * sizeof(DWORD)]{0};
    char name[256];
    //LSTATUS ret = 0;
    DWORD len = 255, lend = 12 * sizeof(DWORD); zone map;
    for (int vindex = 0; RegEnumValueA(hMappingKey, vindex, name, &len, NULL, NULL, (LPBYTE) inarray, &lend) == ERROR_SUCCESS; vindex++) {
        // get id(s)...
        if (sscanf_s(name, "%d-%hd", &map.devid, &map.lightid) == 2) {
            if (lend > 0) {
                for (unsigned i = 0; i < (lend / 4); i++)
                    map.map.push_back(inarray[i]);
                zones.push_back(map);
            }
        }
        len = 255; lend = 12 * sizeof(DWORD); map.map.clear();
    }
}
void ConfigAmbient::Save() {

    DWORD out[12 * sizeof(DWORD)]{0};

    SetReg("Shift", shift);
    SetReg("Mode", mode);
    //SetReg("Divider", divider);
    SetReg("GammaCorrection", gammaCorrection);
    for (int i = 0; i < zones.size(); i++) {
        //preparing name
        string name = to_string(zones[i].devid) + "-" + to_string(zones[i].lightid);
        //preparing binary....
        int size = (int) zones[i].map.size();
        if (size > 0) {
            for (int j = 0; j < size; j++) {
                out[j] = zones[i].map[j];
            }
            size *= sizeof(DWORD);
            RegSetValueExA( hMappingKey, name.c_str(), 0, REG_BINARY, (BYTE*)out, size );
        }
    }
}

