#include "ConfigAmbient.h"
#include <string>
#include <winreg.h>
#include <winerror.h>

using namespace std;

ConfigAmbient::ConfigAmbient() {

    RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Alienfxambient"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMainKey, NULL);
    RegCreateKeyEx(hMainKey, TEXT("Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMappingKey, NULL);
    Load();
}
ConfigAmbient::~ConfigAmbient() {
    //Save();
    RegCloseKey(hMainKey);
    RegCloseKey(hMappingKey);
}

void ConfigAmbient::GetReg(char *name, DWORD *value, DWORD defValue) {
    DWORD size = sizeof(DWORD);
    if (RegGetValue(hMainKey, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
        *value = defValue;
}

void ConfigAmbient::SetReg(char *text, DWORD value) {
    RegSetValueEx(hMainKey, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
}

void ConfigAmbient::Load() {

    DWORD fGrid;

    GetReg("Shift", &shift);
    GetReg("Mode", &mode);
    GetReg("Grid", &fGrid, 0x30004);

    grid = { (byte)LOWORD(fGrid), (byte)HIWORD(fGrid) };

    DWORD inarray[12 * sizeof(DWORD)]{0};
    char name[256];
    DWORD len = 255, lend = 12 * sizeof(DWORD); zone map;
    for (int vindex = 0; RegEnumValue(hMappingKey, vindex, name, &len, NULL, NULL, (LPBYTE) inarray, &lend) == ERROR_SUCCESS; vindex++) {
        // get id(s)...
        if (sscanf_s(name, "%d-%d", &map.devid, &map.lightid) == 2) {
            if (lend > 0) {
                for (unsigned i = 0; i < (lend / sizeof(DWORD)); i++)
                    map.map.push_back((byte)inarray[i]);
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
    SetReg("Grid", MAKELPARAM(grid.x, grid.y));

    RegDeleteTree(hMainKey, "Mappings");
    RegCreateKeyEx(hMainKey, TEXT("Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMappingKey, NULL);
    for (int i = 0; i < zones.size(); i++) {
        //preparing name
        string name = to_string(zones[i].devid) + "-" + to_string(zones[i].lightid);
        //preparing binary....
        int size = (int) zones[i].map.size();
        if (size) {
            for (int j = 0; j < size; j++) {
                out[j] = zones[i].map[j];
            }
            size *= sizeof(DWORD);
            RegSetValueEx( hMappingKey, name.c_str(), 0, REG_BINARY, (BYTE*)out, size );
        }
    }
}

