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

haptics_map *ConfigHaptics::FindHapMapping(DWORD devid, DWORD lid) {
    for (auto i = haptics.begin(); i < haptics.end(); i++)
        if (i->devid == devid && i->lightid == lid)
            return &(*i);
    return NULL;
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
        if ((ret = RegEnumValueA( hMappingKey, vindex, name, &len, NULL, NULL, (LPBYTE)inarray, &lend )) == ERROR_SUCCESS) {
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

    RegDeleteTreeA(hMainKey, "Mappings");
    RegCreateKeyEx(hMainKey, TEXT("Mappings"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMappingKey, NULL);
    for (auto i = haptics.begin(); i < haptics.end(); i++) {
        // TODO: save all freqs!
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
                RegSetValueExA( hMappingKey, name.c_str(), 0, REG_BINARY, out, (DWORD) (i->freqs[0].freqID.size() + 2 * sizeof(DWORD) + 3));
                delete[] out;
            }
            //preparing name
            //string name = "Map" + to_string(i->devid) + "-" + to_string(i->lightid);
            ////preparing binary....
            //DWORD *out = new DWORD[i->freqs[0].freqID.size() + 5];
            //out[0] = i->freqs[0].colorfrom.ci;
            //out[1] = i->freqs[0].colorto.ci;
            //out[2] = i->freqs[0].lowcut;
            //out[3] = i->freqs[0].hicut;
            //out[4] = i->flags;

            //for (int j = 0; j < i->freqs[0].freqID.size(); j++) {
            //    out[j + 5] = i->freqs[0].freqID[j];
            //}
            //RegSetValueExA( hMappingKey, name.c_str(), 0, REG_BINARY, (BYTE *) out, (DWORD) (i->freqs[0].freqID.size() + 5) * sizeof(DWORD) );
            //delete[] out;
        }
    }
}



