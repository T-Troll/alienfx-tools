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
    if (RegGetValue(hMainKey, NULL, name, RRF_RT_DWORD | RRF_ZEROONFAILURE, NULL, value, &size) != ERROR_SUCCESS)
        *value = defValue;
}

void ConfigHaptics::SetReg(char *text, DWORD value) {
    RegSetValueEx( hMainKey, text, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD) );
}

haptics_map *ConfigHaptics::FindHapMapping(DWORD devid, DWORD lid) {
    auto res = find_if(haptics.begin(), haptics.end(), [lid, devid](haptics_map ls) {
        return ls.lightid == lid && ls.devid == devid;
        });
    return res == haptics.end() ? nullptr : &(*res);
}

void ConfigHaptics::Load() {

    GetReg("Axis", &showAxis, 1);
    GetReg("Input", &inpType);

    unsigned vindex = 0;
    DWORD inarray[25]{0};
    char name[255];
    LSTATUS ret = 0;
    do {
        DWORD len = 255, lend = 25 * sizeof(DWORD); haptics_map map; freq_map freq;
        // get id(s)...
        if ((ret = RegEnumValue( hMappingKey, vindex, name, &len, NULL, NULL, (LPBYTE)inarray, &lend )) == ERROR_SUCCESS) {
            vindex++; int gIndex;
            if (sscanf_s(name, "Map%u-%u", &map.devid, &map.lightid) == 2 && (map.devid || map.lightid)) {
                freq.colorfrom.ci = inarray[0];
                freq.colorto.ci = inarray[1];
                freq.lowcut = (byte) inarray[2];
                freq.hicut = (byte) inarray[3];
                map.flags = (byte) inarray[4];
                for (unsigned i = 5; i < (lend / sizeof(DWORD)); i++)
                    freq.freqID.push_back((byte)inarray[i]);
                map.freqs.push_back(freq);
                haptics.push_back(map);
                continue;
            }
            if (sscanf_s(name, "Light%u-%u-%u", &map.devid, &map.lightid, &gIndex) == 3) {
                byte *bArray = (byte *) (inarray + 2);
                haptics_map *cmap = FindHapMapping(map.devid, map.lightid);
                if (!cmap) {
                    haptics.push_back({map.devid, map.lightid});
                    cmap = &haptics.back();
                }
                freq.colorfrom.ci = inarray[0];
                freq.colorto.ci = inarray[1];
                freq.lowcut = bArray[0];
                freq.hicut = bArray[1];
                cmap->flags = bArray[2];
                for (unsigned i = 3; i + 2*sizeof(DWORD) < lend; i++)
                    if (bArray[i] < NUMBARS)
                        freq.freqID.push_back((byte) bArray[i]);
                cmap->freqs.push_back(freq);
                continue;
            }
        }
    } while (ret == ERROR_SUCCESS);
}
void ConfigHaptics::Save() {

    SetReg("Axis", showAxis);
    SetReg("Input", inpType);

    RegDeleteTree(hMainKey, "Mappings");
    RegCreateKeyEx(hMainKey, TEXT("Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMappingKey, NULL);
    for (auto i = haptics.begin(); i < haptics.end(); i++) {

        if (i->freqs.size()) {
            for (int j = 0; j < i->freqs.size(); j++) {
                string name = "Light" + to_string(i->devid) + "-" + to_string(i->lightid) + "-" + to_string(j);
                byte *out = new byte[i->freqs[j].freqID.size() + 2 * sizeof(DWORD) + 3];
                DWORD *dOut = (DWORD *) out; byte *bOut = out + 2 * sizeof(DWORD);
                dOut[0] = i->freqs[j].colorfrom.ci;
                dOut[1] = i->freqs[j].colorto.ci;
                bOut[0] = i->freqs[j].lowcut;
                bOut[1] = i->freqs[j].hicut;
                bOut[2] = i->flags;
                bOut += 3;
                for (int k = 0; k < i->freqs[j].freqID.size(); k++)
                    bOut[k] = i->freqs[j].freqID[k];
                RegSetValueEx( hMappingKey, name.c_str(), 0, REG_BINARY, out, (DWORD) (i->freqs[0].freqID.size() + 2 * sizeof(DWORD) + 3));
                delete[] out;
            }
        }
    }
}



