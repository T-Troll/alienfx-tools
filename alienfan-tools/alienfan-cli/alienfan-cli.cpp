
#define WIN32_LEAN_AND_MEAN
//#include <iostream>
#include <stdio.h>
#include <vector>
#include <combaseapi.h>
#include <PowrProf.h>
#include "alienfan-SDK.h"
#include "ConfigFan.h"

#pragma comment(lib, "PowrProf.lib")

using namespace std;

struct ARG {
    int num;
    string str;
};

AlienFan_SDK::Control* acpi = new AlienFan_SDK::Control();

ConfigFan* fan_conf = new ConfigFan();

bool CheckArgs(string cName, int minArgs, size_t nargs) {
    if (minArgs > nargs) {
        printf("%s: Incorrect arguments (should be %d)\n", cName.c_str(), minArgs);
        return false;
    }
    return true;
}

fan_overboost bestBoostPoint, lastBoostPoint;

int SetFanSteady(byte boost, bool downtrend = false) {
    printf("Probing Fan#%d at boost %d: ", bestBoostPoint.fanID, boost);
    acpi->SetFanValue(bestBoostPoint.fanID, boost, true);
    // Check the trend...
    int fRpm, fDelta = -1, oDelta, bRpm = acpi->GetFanRPM(bestBoostPoint.fanID);
    lastBoostPoint = { (byte)bestBoostPoint.fanID, boost, (USHORT)bRpm };
    Sleep(5000);
    do {
        oDelta = fDelta;
        Sleep(5000);
        fRpm = acpi->GetFanRPM(bestBoostPoint.fanID);
        fDelta = fRpm - bRpm;
        printf("\rProbing Fan#%d at boost %d: %d-%d (%+d)   ", bestBoostPoint.fanID, boost, bRpm, fRpm, fDelta);
        lastBoostPoint.maxRPM = max(bRpm, fRpm);
        bestBoostPoint.maxRPM = max(bestBoostPoint.maxRPM, lastBoostPoint.maxRPM);
        bRpm = fRpm;
    } while ((fDelta > 0 || oDelta < 0 ) && (!downtrend || !(fDelta < -40 && oDelta < -40)));
    printf("\rProbing Fan#%d at boost %d done, %d RPM ", bestBoostPoint.fanID, boost, lastBoostPoint.maxRPM);
    return lastBoostPoint.maxRPM;
}

void UpdateBoost() {
    auto pos = find_if(fan_conf->boosts.begin(), fan_conf->boosts.end(),
        [](auto t) {
            return t.fanID == bestBoostPoint.fanID;
        });
    if (pos != fan_conf->boosts.end()) {
        pos->maxBoost = bestBoostPoint.maxBoost;
        pos->maxRPM = max(bestBoostPoint.maxRPM, pos->maxRPM);
    }
    else {
        fan_conf->boosts.push_back(bestBoostPoint);
    }
    acpi->boosts[bestBoostPoint.fanID] = bestBoostPoint.maxBoost;
    acpi->maxrpm[bestBoostPoint.fanID] = max(bestBoostPoint.maxRPM, acpi->maxrpm[bestBoostPoint.fanID]);
    fan_conf->Save();
}

void CheckFanOverboost(byte num) {
    int steps = 8, cSteps, boost = 100, cBoost = 100,
        rpm, oldBoost = acpi->GetFanValue(num, true);
    printf("Checking Fan#%d:\n", num);
    bestBoostPoint = { (byte)num, 100, 0 };
    rpm = SetFanSteady(boost);
    printf("    \n");
    for (int steps = 8; steps; steps = steps >> 1) {
        // Check for uptrend
        while ((boost+=steps) != cBoost)
        {
            SetFanSteady(boost, true);
            //printf("(Best: %d @ %d RPM)\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
            if (lastBoostPoint.maxRPM > rpm) {
                rpm = lastBoostPoint.maxRPM;
                cSteps = steps;
                bestBoostPoint.maxBoost = boost;
                printf("(New best: %d @ %d RPM)\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
            }
            else {
                printf("(Skipped)\n");
                break;
            }
        }
        boost = bestBoostPoint.maxBoost;
        cBoost = boost + steps;
    }
    printf("High check done, best %d @ %d RPM, starting low check:\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
    for (int steps = cSteps > 1 ? cSteps >> 1 : 1; steps; steps = steps >> 1) {
        // Check for uptrend
        boost -= steps;
        while (SetFanSteady(boost, true) >= bestBoostPoint.maxRPM - 60) {
            bestBoostPoint.maxBoost = boost;
            boost -= steps;
            printf("(New best: %d @ %d RPM)\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
        }
        printf("(Step back)\n");
        boost = bestBoostPoint.maxBoost;
    }
    printf("Final boost - %d, %d RPM\n\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
    acpi->SetFanValue(num, oldBoost, true);
    UpdateBoost();
}

void Usage() {
    printf("Usage: alienfan-cli [command[=value{,value}] [command...]]\n\
Avaliable commands: \n\
rpm[=id]\t\t\tShow fan(s) RPM\n\
persent[=id]\t\t\tShow fan(s) RPM in perecent of maximum\n\
temp[=id]\t\t\tShow known temperature sensors values\n\
unlock\t\t\t\tUnclock fan controls\n\
getpower\t\t\tDisplay current power state\n\
setpower=<power mode>\t\tSet CPU power to this mode\n\
setgpu=<value>\t\t\tSet GPU power limit\n\
setperf=<ac>,<dc>\t\tSet CPU performance boost\n\
getfans[=<mode>]\t\tShow current fan boost level (0..100 - in percent) with selected mode\n\
setfans=<fan1>[,<fanN>][,mode]\tSet fans boost level (0..100 - in percent) with selected mode\n\
setover[=fanID[,boost]]\t\tSet overboost for selected fan to boost (manual or auto)\n\
resetcolor\t\t\tReset color system\n\
setcolor=<mask>,r,g,b\t\tSet light(s) defined by mask to color\n\
setbrightness=<dim>,<flag>\tSet light system brightness and mode\n\
direct=<id>,<subid>[,val,val]\tIssue direct interface command (for testing)\n\
directgpu=<id>,<value>\t\tIssue direct GPU interface command (for testing)\n\
\tPower mode can be in 0..N - according to power states detected\n\
\tPerformance boost can be in 0..4 - disabled, enabled, aggressive, efficient, efficient aggressive\n\
\tGPU power limit can be in 0..4 - 0 - no limit, 4 - max. limit\n\
\tNumber of fan boost values should be the same as a number of fans detected\n\
\tMode can be 0 or absent for set cooked value, 1 for raw value\n\
\tBrighness for ACPI lights can only have 10 values - 1,3,4,6,7,9,10,12,13,15\n\
\tAll values in \"direct\" commands should be hex, not decimal!\n");
}

int main(int argc, char* argv[])
{
    printf("AlienFan-cli v6.0.0.5\n");

    bool supported = false;

    AlienFan_SDK::Lights* lights = NULL;

    if (acpi->IsActivated()) {

        if (supported = acpi->Probe()) {
            if (!(lights = new AlienFan_SDK::Lights(acpi))->IsActivated()) {
                delete lights;
                lights = NULL;
            }
            printf("Supported hardware v%d detected, %d fans, %d sensors, %d power states. Light control %s.\n",
                acpi->GetVersion(), (int)acpi->HowManyFans(), (int)acpi->sensors.size(), (int)acpi->HowManyPower(),
                (lights ? "enabled" : "disabled"));
            fan_conf->SetBoosts(acpi);
        }
        else {
           printf("Supported hardware not found!\n");
        }

        if (argc < 2)
        {
            Usage();
        } else {
            if (supported) {
                for (int cc = 1; cc < argc; cc++) {
                    string arg = string(argv[cc]);
                    size_t vid = arg.find_first_of('=');
                    string command = arg.substr(0, vid);
                    vector<ARG> args;
                    while (vid != string::npos) {
                        size_t argPos = arg.find(',', vid + 1);
                        if (argPos == string::npos)
                            // last one...
                            args.push_back({ 0, arg.substr(vid + 1, arg.length() - vid - 1) });
                        else
                            args.push_back({ 0, arg.substr(vid + 1, argPos - vid - 1) });
                        args.back().num = atoi(args.back().str.c_str());
                        vid = argPos;
                    }
                    //int prms = 0;
                    //cerr << "Executing " << command << " with " << values);
                    /*if (command == "usage" || command == "help" || command == "?") {
                        Usage();
                        continue;
                    }*/
                    if (command == "rpm") {
                        if (args.size() > 0 && args[0].num < acpi->HowManyFans()) {
                            printf("%d\n", acpi->GetFanRPM(args[0].num));
                        }
                        else {
                            for (int i = 0; i < acpi->HowManyFans(); i++)
                                   printf("Fan#%d: %d\n", i, acpi->GetFanRPM(i));
                        }
                        continue;
                    }
                    if (command == "percent") {
                        if (args.size() > 0 && args[0].num < acpi->HowManyFans()) {
                            printf("%d\n", acpi->GetFanPercent(args[0].num));
                        }
                        else {
                            for (int i = 0; i < acpi->HowManyFans(); i++)
                                    printf("Fan#%d: %d%%\n", i, acpi->GetFanPercent(i));
                        }
                        continue;
                    }
                    if (command == "temp") {
                        if (args.size() > 0 && args[0].num < acpi->HowManySensors()) {
                            printf("%d\n", acpi->GetTempValue(args[0].num));
                        }
                        else {
                            for (int i = 0; i < acpi->HowManySensors(); i++) {
                                printf("%s: %d\n", acpi->sensors[i].name.c_str(), acpi->GetTempValue(i));
                            }
                        }
                        continue;
                    }
                    if (command == "unlock") {
                        if (acpi->Unlock() >= 0)
                            printf("Unlock successful.\n");
                        else
                            printf("Unlock failed!\n");
                        continue;
                    }
                    if (command == "setpower" && CheckArgs(command, 1, args.size())) {

                        //BYTE unlockStage = atoi(args[0].c_str());
                        if (args[0].num < acpi->HowManyPower()) {
                            printf("Power set to %d (result %d)\n", args[0].num, acpi->SetPower(args[0].num));
                        }
                        else
                            printf("Power: incorrect value (should be 0..%d)\n", acpi->HowManyPower());
                        continue;
                    }
                    if (command == "setgpu" && CheckArgs(command, 1, args.size())) {

                        //BYTE gpuStage = atoi(args[0].c_str());
                        printf("GPU limit set to %d (result %d)\n", args[0].num, acpi->SetGPU(args[0].num));
                        continue;
                    }
                    if (command == "setperf" && CheckArgs(command, 2, args.size())) {
                        //DWORD acMode = atoi(args[0].c_str()),
                        //    dcMode = atoi(args[1].c_str());
                        if (args[0].num > 4 || args[1].num > 4)
                            printf("Incorrect value - should be 0..4\n");
                        else {
                            GUID* sch_guid, perfset;
                            IIDFromString(L"{be337238-0d82-4146-a960-4f3749d470c7}", &perfset);
                            PowerGetActiveScheme(NULL, &sch_guid);
                            PowerWriteACValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, args[0].num);
                            PowerWriteDCValueIndex(NULL, sch_guid, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &perfset, args[1].num);
                            PowerSetActiveScheme(NULL, sch_guid);
                            printf("CPU boost set to %d,%d\n", args[0].num, args[1].num);
                            LocalFree(sch_guid);
                        }
                        continue;
                    }
                    if (command == "getpower") {
                        printf("Current power mode: %d\n", acpi->GetPower());
                        continue;
                    }
                    if (command == "getfans") {
                        bool direct = args.size() ? args[0].num : false;
                        for (int i = 0; i < acpi->HowManyFans(); i++)
                            printf("Fan#%d boost %d\n", i, acpi->GetFanValue(i, direct));
                        continue;
                    }
                    if (command == "setfans" && CheckArgs(command, acpi->HowManyFans(), args.size())) {

                        bool direct = args.size() > acpi->HowManyFans() ? args[acpi->HowManyFans()].num : false;
                        for (int i = 0; i < acpi->HowManyFans(); i++) {
                            printf("Fan#%d boost set to %d (result %d)\n", i, args[i].num, acpi->SetFanValue(i, args[i].num, direct));
                        }
                        continue;
                    }
                    if (command == "setover") {
                        int oldMode = acpi->GetPower();
                        acpi->Unlock();
                        if (args.size()) {
                            // one fan
                            //byte fanID = atoi(args[0].c_str());
                            if (args[0].num < acpi->HowManyFans())
                                if (args.size() > 1) {
                                    // manual fan set
                                    acpi->Unlock();
                                    bestBoostPoint = { (byte)args[0].num, (byte)args[1].num, 0 };
                                    //printf("Boost for fan #%d will be set to %d.\n", args[0].num, bestBoostPoint.maxBoost);
                                    SetFanSteady(args[0].num, bestBoostPoint.maxBoost);
                                    UpdateBoost();
                                    printf("\nBoost for fan #%d set to %d @ %d RPM.\n",
                                        args[0].num, bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
                                }
                                else
                                    CheckFanOverboost(args[0].num);
                            else
                                printf("Incorrect fan ID (should be 0..%d)!\n", acpi->HowManyFans() - 1);
                        }
                        else {
                            // all fans
                            for (int i = 0; i < acpi->HowManyFans(); i++)
                                CheckFanOverboost(i);
                        }
                        acpi->SetPower(oldMode);
                        printf("Done!\n");
                        continue;
                    }
                    if (command == "direct" && CheckArgs(command, 2, args.size())) {
                        AlienFan_SDK::ALIENFAN_COMMAND comm;
                        comm.com = (byte)strtoul(args[0].str.c_str(), NULL, 16);
                        comm.sub = (byte)strtoul(args[1].str.c_str(), NULL, 16);
                        byte value1 = 0, value2 = 0;
                        if (args.size() > 2)
                            value1 = (byte)strtol(args[2].str.c_str(), NULL, 16);
                        if (args.size() > 3)
                            value2 = (byte)strtol(args[3].str.c_str(), NULL, 16);
                        printf("Direct call result: %d\n", acpi->RunMainCommand(comm, value1, value2));
                        continue;
                    }
                    if (command == "directgpu" && CheckArgs(command, 2, args.size())) {
                        USHORT command = (USHORT)strtoul(args[0].str.c_str(), NULL, 16);
                        DWORD subcommand = strtoul(args[1].str.c_str(), NULL, 16);
                        printf("DirectGPU call result: %d\n", acpi->RunGPUCommand(command, subcommand));
                        continue;
                    }

                    if (command == "resetcolor" && lights) { // Reset color system for Aurora
                        if (lights->Reset())
                            printf("Lights reset complete\n");
                        else
                            printf("Lights reset failed\n");
                        continue;
                    }
                    if (command == "setcolor" && lights && CheckArgs(command, 4, args.size())) { // Set light color for Aurora

                        //byte mask = atoi(args[0].c_str()),
                        //    r = atoi(args[1].c_str()),
                        //    g = atoi(args[2].c_str()),
                        //    b = atoi(args[3].c_str());
                        printf("SetColor result %d.\n", lights->SetColor(args[0].num, args[1].num, args[2].num, args[3].num));
                        lights->Update();
                        continue;
                    }
                    if (command == "setbrightness" && lights && CheckArgs(command, 2, args.size())) { // set brightness for Aurora

                        //byte num = atoi(args[0].c_str()),
                        //    mode = atoi(args[1].c_str());
                        printf("SetBrightness result %d.\n", lights->SetMode(args[0].num, args[1].num));
                        lights->Update();
                        continue;
                    }
                    //if (command == "test") { // pseudo block for test modules
                    //    continue;
                    //}
                    printf("Unknown command - %s, use \"usage\" or \"help\" for information\n", command.c_str());
                }
            }
        }

        delete lights;

    } else {
        printf("System configuration issue - see readme.md for details!\n");
    }

    delete fan_conf;
    delete acpi;

}
