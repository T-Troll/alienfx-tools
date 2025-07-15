#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <vector>
#include <combaseapi.h>
#include <PowrProf.h>
#include "alienfan-SDK.h"
#include "ConfigFan.h"

#pragma comment(lib, "PowrProf.lib")

using namespace std;

struct ARG {
    string str;
    int num;
};

AlienFan_SDK::Control acpi;
ConfigFan fan_conf;

string command;
vector<ARG> args;
fan_overboost bestBoostPoint;

bool CheckArgs(size_t minArgs, size_t maxZeroArg, bool print = true) {
    if (minArgs > args.size()) {
        if (print) printf("%s: Incorrect arguments count (at least %zd needed)\n", command.c_str(), minArgs);
    }
    else
        if (args.size() && (maxZeroArg <= args[0].num || args[0].num < 0))
            printf("%s: Incorrect argument value %d (should be in [0..%zd])\n", command.c_str(), args[0].num, maxZeroArg - 1);
        else
            return true;
    return false;
}

int SetFanSteady(byte fanID, byte boost, bool downtrend = false) {
    printf("Probing Fan#%d at boost %d: ", fanID, boost);
    acpi.SetFanBoost(fanID, boost);
    // Check the trend...
    int pRpm, maxRPM, bRpm = acpi.GetFanRPM(fanID), fRpm;
    Sleep(3000);
    fRpm = acpi.GetFanRPM(fanID);
    do {
        pRpm = bRpm;
        bRpm = fRpm;
        Sleep(3000);
        fRpm = acpi.GetFanRPM(fanID);
        //printf("\rProbing Fan#%d at boost %d: %4d-%4d-%4d (%+d, %+d)        ", bestBoostPoint.fanID, boost, pRpm, bRpm, fRpm, bRpm - pRpm, fRpm - bRpm);
        printf("\rProbing Fan#%d at boost %d: %4d (%+d)   \r", fanID, boost, fRpm, fRpm - bRpm);
        maxRPM = max(bRpm, fRpm);
        bestBoostPoint.maxRPM = max(bestBoostPoint.maxRPM, maxRPM);
    } while ((fRpm > bRpm || bRpm < pRpm || fRpm != pRpm) && (!downtrend || !(fRpm < bRpm && bRpm < pRpm)));
    printf("Probing Fan#%d at boost %d done, %d RPM ", fanID, boost, maxRPM);
    return maxRPM;
}

void CheckFanOverboost(byte num, byte boost) {
    int rpm = acpi.GetFanRPM(num), cSteps = 8,
        oldBoost = acpi.GetFanBoost(num), downScale, crpm;
    printf("Checking Fan#%d:\n", num);
    bestBoostPoint = { (byte)boost, (WORD)rpm };
    SetFanSteady(num, boost);
    if (boost == 100) {
        printf("    \n");
        for (int steps = cSteps; steps; steps = steps >> 1) {
            // Check for uptrend
            boost += steps;
            while ((crpm = SetFanSteady(num, boost, true)) > rpm)
            {
                rpm = bestBoostPoint.maxRPM;
                cSteps = steps;
                bestBoostPoint.maxBoost = boost;
                printf("(New best: %d @ %d RPM)\n", boost, bestBoostPoint.maxRPM);
                boost += steps;
            }
            printf("(Skipped)\n");
            boost = bestBoostPoint.maxBoost;
        }
        printf("High check done, best %d @ %d RPM, starting low check:\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
        // 100 rpm step patch
        downScale = (rpm / 100) * 100 == rpm ? 101 : 56;
        for (int steps = cSteps; steps; steps = steps >> 1) {
            // Check for downtrend
            boost -= steps;
            while (boost >= 100 && SetFanSteady(num, boost) > bestBoostPoint.maxRPM - downScale) {
                bestBoostPoint.maxBoost = boost;
                boost -= steps;
                printf("(New best: %d @ %d RPM)\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
            }
            if (boost > 100) {
                printf("(Step back)\n");
                boost = bestBoostPoint.maxBoost;
            }
        }
        bestBoostPoint.maxBoost = max(bestBoostPoint.maxBoost, 100);
    }
    printf("Final boost - %d, %d RPM\n\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
    acpi.SetFanBoost(num, oldBoost);
    fan_conf.UpdateBoost(num, bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
    fan_conf.Save();
}

void PrintFanType(int index, int val, const char* type) {
    const char* res;
    switch (acpi.fans[index].type) {
    case 1: res = " CPU"; break;
    case 6: res = " GPU"; break;
    default: res = "";
    }
    printf("Fan%s %d %s: %d\n", res, index, type, val);
}

inline void Usage() {
    printf("\nUsage: alienfan-cli command[=value{,value}] {command...}\n\
Available commands: \n\
rpm[=id]\t\t\tShow fan(s) RPM\n\
maxrpm[=id]\t\t\tBIOS fan max. RPM\n\
percent[=id]\t\t\tShow fan(s) RPM in percent of maximum\n\
temp[=id]\t\t\tShow known temperature sensors values\n\
unlock\t\t\t\tUnlock fan controls\n\
getpower\t\t\tDisplay current power state\n\
setpower=<power mode>\t\tSet CPU power to this mode\n\
setperf=<ac>,<dc>\t\tSet CPU performance boost\n\
getfans[=[fanID,]<mode>]\tShow fan boost level (0..100 - in percent) with selected mode\n\
setfans=<fan1>[,<fanN>][,mode]\tSet fans boost level (0..100 - in percent) with selected mode\n\
setover[=fanID[,boost]]\t\tSet overboost for selected fan to boost (manual or auto)\n\
setgmode=<mode>\t\t\tSet G-mode on/off (1-on, 0-off)\n\
gmode\t\t\t\tShow G-mode state\n\
toggledisk\t\t\tToggle disk sensors\n\
gettcc\t\t\t\tShow current TCC level\n\
settcc=<level>\t\t\tSet TCC level\n\
getxmp\t\t\t\tShow current memory XMP profile level\n\
setxmp=<level>\t\t\tSet memory XMP profile level\n");
#ifndef NOLIGHTS
    printf("setcolor = id, r, g, b\t\tSet light to color\n\
setbrightness=<brightness>\tSet lights brightness\n");
#endif
    printf("\tPower mode can be in 0..N - according to power states detected\n\
\tPerformance boost can be in 0..4 - disabled, enabled, aggressive, efficient, efficient aggressive\n\
\tNumber of fan boost values should be the same as a number of fans detected\n\
\tMode can absent for cooked value, and \"raw\" for raw value\n\
\tBrightness can be in 0..15\n");
}

int main(int argc, char* argv[])
{
    printf("AlienFan-CLI v9.3.0.2\n");
#ifndef NOLIGHTS
    AlienFan_SDK::Lights* lights = NULL;
#endif
    if (acpi.Probe(fan_conf.diskSensors)) {
#ifndef NOLIGHTS
        lights = new AlienFan_SDK::Lights(&acpi);
#endif
        printf("Supported hardware (%d) detected, %d fans, %d sensors, %d power states%s%s%s%s.\n",
            acpi.systemID, (int)acpi.fans.size(), (int)acpi.sensors.size(), (int)acpi.powers.size(),
            (acpi.isGmode ? ", G-Mode" : ""),
            (acpi.isTcc ? ", TCC" : ""),
            (acpi.isXMP ? ", XMP" : ""),
            (
#ifndef NOLIGHTS
                lights->isActivated ? ", Lights" :
#endif
                ""));
    }
    else {
        printf("%sHardware not compatible.\n", acpi.isAlienware ? "Alienware system, " : "");
    }


    if (argc < 2)
    {
        Usage();
    }
    else {
        for (int cc = 1; cc < argc; cc++) {
            string arg = argv[cc];
            size_t vid = arg.find_first_of('=');
            command = arg.substr(0, vid);
            args.clear();
            while (vid != string::npos) {
                size_t argPos = arg.find(',', vid + 1);
                args.push_back({ arg.substr(vid + 1, (argPos == string::npos ? arg.length() : argPos) - vid - 1) });
                args.back().num = atoi(args.back().str.c_str());
                vid = argPos;
            }

            if (command == "rpm") {
                for (int i = 0; i < acpi.fans.size(); i++)
                    if (args.empty() || args[0].num == i)
                        PrintFanType(i, acpi.GetFanRPM(i), "RPM");
                continue;
            }
            if (command == "maxrpm") {
                for (int i = 0; i < acpi.fans.size(); i++)
                    if (args.empty() || args[0].num == i)
                        PrintFanType(i, acpi.GetMaxRPM(i), "Max. RPM");
                continue;
            }
            if (command == "percent") {
                for (int i = 0; i < acpi.fans.size(); i++)
                    if (args.empty() || args[0].num == i) {
                        PrintFanType(i, (acpi.GetFanRPM(i) * 100) / (fan_conf.boosts[i].maxRPM ?
                            fan_conf.boosts[i].maxRPM : acpi.GetMaxRPM(i)), "Percent");
                    }
                continue;
            }
            if (command == "temp") {
                for (int i = 0; i < acpi.sensors.size(); i++)
                    if (args.empty() || i == args[0].num) {
                        printf("%s: %d\n", fan_conf.GetSensorName(&acpi.sensors[i]).c_str(), acpi.GetTempValue(i));
                    }
                continue;
            }
            if (command == "unlock") {
                printf("%s", acpi.SetPower(0) < 0 ? "Unlock failed!\n" : "Unlocked.\n");
                continue;
            }
            if (command == "gmode") {
                printf("G-mode is %s\n", acpi.GetGMode() ? "On" : "Off");
                continue;
            }
            if (command == "setpower" && CheckArgs(1, acpi.powers.size())) {
                if (acpi.SetPower(acpi.powers[args[0].num]) < 0)
                    printf("Power mode error.\n");
                else
                    printf("Power mode set to %s (%x)\n", fan_conf.GetPowerName(acpi.powers[args[0].num])->c_str(), acpi.powers[args[0].num]);
                continue;
            }
            if (command == "setperf" && CheckArgs(2, 5)) {
                GUID* sch_guid, perfset;
                IIDFromString(L"{be337238-0d82-4146-a960-4f3749d470c7}", &perfset);
                PowerGetActiveScheme(NULL, &sch_guid);
                PowerWriteACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, args[0].num);
                PowerWriteDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, args[1].num);
                PowerSetActiveScheme(NULL, sch_guid);
                printf("CPU boost set to %d,%d\n", args[0].num, args[1].num);
                LocalFree(sch_guid);
                continue;
            }
            if (command == "getpower") {
                int res = acpi.GetPower(true);
                if (res < 0)
                    printf("Power mode error.\n");
                else
                    printf("Power mode: %s (%x)\n", fan_conf.GetPowerName(res)->c_str(), res);
                continue;
            }
            if (command == "getfans") {
                int rawboost;
                bool single = CheckArgs(1, acpi.fans.size(), false) && args[0].str != "raw";
                for (int i = 0; i < acpi.fans.size(); i++)
                    if (!single || i == args[0].num) {
                        rawboost = acpi.GetFanBoost(i);
                        PrintFanType(i, args.size() && args.back().str == "raw" ? rawboost : (rawboost * 100) / fan_conf.GetFanScale(i), "boost");
                    }
                continue;
            }
            if (command == "setfans" && CheckArgs(acpi.fans.size(), 256)) {
                for (int i = 0; i < acpi.fans.size(); i++)
                    if (acpi.SetFanBoost(i, args.back().str == "raw" ? args[i].num : (args[i].num * fan_conf.GetFanScale(i)) / 100) >= 0)
                        PrintFanType(i, args[i].num, "boost set");
                continue;
            }
            if (command == "setover") {
                int oldMode = acpi.GetPower(), boost = CheckArgs(2, acpi.fans.size(), false) ? args[1].num : 100;
                acpi.Unlock();
                for (int i = 0; i < acpi.fans.size(); i++)
                    if (args.empty() || i == args[0].num)
                        CheckFanOverboost(i, boost);
                acpi.SetPower(acpi.powers[oldMode]);
                continue;
            }
            if (command == "setgmode" && CheckArgs(1, 2) && acpi.isGmode /*&& acpi.GetGMode() != args[0].num*/) {
                acpi.SetGMode(args[0].num);
                if (!args[0].num)
                    acpi.SetPower(acpi.powers[fan_conf.prof.powerStage]);
                printf("G-mode %s\n", args[0].num ? "On" : "Off");
                continue;
            }
#ifndef NOLIGHTS
            if (command == "setcolor" && lights->isActivated && CheckArgs(4, 255)) { // Set light color for Aurora
                printf("SetColor result %d.\n", lights->SetColor(args[0].num, args[1].num, args[2].num, args[3].num));
                continue;
            }
            if (command == "setbrightness" && lights->isActivated && CheckArgs(1, 15)) { // set brightness for Aurora
                printf("SetBrightness result %d.\n", lights->SetBrightness(args[0].num));
                continue;
            }
#endif
            if (command == "toggledisk") {
                fan_conf.diskSensors = !fan_conf.diskSensors;
                printf("Disk sensors %s\n", fan_conf.diskSensors ? "ON" : "OFF");
                fan_conf.Save();
                continue;
            }
#ifndef ALIENFAN_SDK_V1
            if (command == "gettcc") {
                printf("Current TCC is %d (max %d)\n", acpi.GetTCC(), acpi.maxTCC);
                continue;
            }
            if (command == "settcc" && CheckArgs(1, 255)) {
                if (args[0].num <= acpi.maxTCC && acpi.maxTCC - args[0].num <= acpi.maxOffset) {
                    acpi.SetTCC(args[0].num);
                    printf("Current TCC set to %d\n", args[0].num);
                }
                else
                    printf("Incorrect TCC value - should be in [%d..%d]\n", acpi.maxTCC - acpi.maxOffset, acpi.maxTCC);

                continue;
            }
            if (command == "getxmp") {
                printf("Current XMP profile is %d\n", acpi.GetXMP());
                continue;
            }
            if (command == "setxmp" && CheckArgs(1, 3)) {
                if (!acpi.SetXMP(args[0].num))
                    printf("XMP profile set to %d\n", args[0].num);
                else
                    printf("XMP switch locked");
                continue;
            }
            if (command == "dump" && acpi.isAlienware) { // dump WMI functions
                BSTR name;
                // Command dump
                acpi.m_AWCCGetObj->GetObjectText(0, &name);
                wprintf(L"Names:\n%s\n", name);
                continue;
            }
            if (command == "probe" && acpi.isAlienware && CheckArgs(1, 2)) { // manual detection dump
                VARIANT result{ VT_I4 };
                result.intVal = -1;
                IWbemClassObject* m_outParameters = NULL;
                VARIANT parameters = { VT_I4 };
                parameters.uintVal = AlienFan_SDK::ALIENFAN_INTERFACE{ (byte)args[0].num, 0, 0, 0 }.args;
                acpi.m_InParamaters->Put((BSTR)L"arg2", NULL, &parameters, 0);
                if (acpi.m_WbemServices->ExecMethod(acpi.m_instancePath.bstrVal,
                    (BSTR)L"Set_OCUIBIOSControl", 0, NULL, acpi.m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
                    m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
                    m_outParameters->Release();
                }
                printf("Subcommand %d - result %x\n", args[0].num, result.intVal);
                continue;
            }
            if (command == "test") { // Test
                IWbemClassObject* m_outParameters = NULL;
                VARIANT result{ VT_I4 };
                result.intVal = -1;
                if (acpi.m_WbemServices->ExecMethod(acpi.m_instancePath.bstrVal,
                    (BSTR)L"Return_OverclockingReport", 0, NULL, NULL, &m_outParameters, NULL) == S_OK && m_outParameters) {
                    m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
                    m_outParameters->Release();
                }
                printf("Subcommand %d - result %x\n", 0, result.intVal);
                continue;
            }
            printf("Unknown command - %s, run without parameters for help.\n", command.c_str());
#else
            //if (command == "getcharge" && acpi.isCharge) {
            //    printf("Charge is %s\n", acpi.GetCharge() & 0x10 ? "Off" : "On");
            //    continue;
            //}
            //if (command == "setcharge" && acpi.isCharge && CheckArgs(1, 255)) {
            //    printf("Charge set to %d\n", acpi.SetCharge(args[0].num ? 0 : 0x10));
            //    continue;
            //}
#endif // !ALIENFAN_SDK_V1
        }
    }
#ifndef NOLIGHTS
    if (lights)
        delete lights;
#endif
}
