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

fan_overboost bestBoostPoint;

int SetFanSteady(byte boost, bool downtrend = false) {
    printf("Probing Fan#%d at boost %d: ", bestBoostPoint.fanID, boost);
    acpi->SetFanBoost(bestBoostPoint.fanID, boost, true);
    // Check the trend...
    int pRpm, maxRPM, bRpm = acpi->GetFanRPM(bestBoostPoint.fanID), fRpm;
    Sleep(3000);
    fRpm = acpi->GetFanRPM(bestBoostPoint.fanID);
    do {
        pRpm = bRpm;
        bRpm = fRpm;
        Sleep(3000);
        fRpm = acpi->GetFanRPM(bestBoostPoint.fanID);
        //printf("\rProbing Fan#%d at boost %d: %4d-%4d-%4d (%+d, %+d)        ", bestBoostPoint.fanID, boost, pRpm, bRpm, fRpm, bRpm - pRpm, fRpm - bRpm);
        printf("\rProbing Fan#%d at boost %d: %4d (%+d)   \r", bestBoostPoint.fanID, boost, fRpm, fRpm - bRpm);
        maxRPM = max(bRpm, fRpm);
        bestBoostPoint.maxRPM = max(bestBoostPoint.maxRPM, maxRPM);
    } while ((fRpm > bRpm || bRpm < pRpm || fRpm != pRpm) && (!downtrend || !(fRpm < bRpm && bRpm < pRpm)));
    printf("Probing Fan#%d at boost %d done, %d RPM ", bestBoostPoint.fanID, boost, maxRPM);
    return maxRPM;
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
    acpi->boosts[bestBoostPoint.fanID] = max(bestBoostPoint.maxBoost, 100);
    acpi->maxrpm[bestBoostPoint.fanID] = max(bestBoostPoint.maxRPM, acpi->maxrpm[bestBoostPoint.fanID]);
    //fan_conf->Save();
}

void CheckFanOverboost(byte num) {
    int cSteps = 8, boost = 100, cBoost = 100, rpm, oldBoost = acpi->GetFanBoost(num, true);
    printf("Checking Fan#%d:\n", num);
    bestBoostPoint = { (byte)num, 100, 0 };
    rpm = SetFanSteady(boost);
    printf("    \n");
    for (int steps = cSteps; steps; steps = steps >> 1) {
        // Check for uptrend
        while ((boost+=steps) != cBoost)
        {
            if (SetFanSteady(boost, true) > rpm) {
                rpm = bestBoostPoint.maxRPM;
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
    for (int steps = cSteps; steps; steps = steps >> 1) {
        // Check for downtrend
        boost -= steps;
        while (boost > 100 && SetFanSteady(boost) >= bestBoostPoint.maxRPM - 55) {
            bestBoostPoint.maxBoost = boost;
            boost -= steps;
            printf("(New best: %d @ %d RPM)\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
        }
        if (boost > 100)
            printf("(Step back)\n");
        boost = bestBoostPoint.maxBoost;
    }
    printf("Final boost - %d, %d RPM\n\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
    acpi->SetFanBoost(num, oldBoost, true);
    UpdateBoost();
}

void Usage() {
    printf("Usage: alienfan-cli [command[=value{,value}] [command...]]\n\
Available commands: \n\
rpm[=id]\t\t\tShow fan(s) RPM\n\
percent[=id]\t\t\tShow fan(s) RPM in percent of maximum\n\
temp[=id]\t\t\tShow known temperature sensors values\n\
unlock\t\t\t\tUnlock fan controls\n\
getpower\t\t\tDisplay current power state\n\
setpower=<power mode>\t\tSet CPU power to this mode\n\
setperf=<ac>,<dc>\t\tSet CPU performance boost\n\
getfans[=<mode>]\t\tShow current fan boost level (0..100 - in percent) with selected mode\n\
setfans=<fan1>[,<fanN>][,mode]\tSet fans boost level (0..100 - in percent) with selected mode\n\
setover[=fanID[,boost]]\t\tSet overboost for selected fan to boost (manual or auto)\n\
setgmode=<mode>\t\t\tSet G-mode on/off (1-on, 0-off)\n\
gmode\t\t\t\tShow G-mode state\n\
resetcolor\t\t\tReset color system\n\
setcolor=<mask>,r,g,b\t\tSet light(s) defined by mask to color\n\
setbrightness=<dim>,<flag>\tSet light system brightness and mode\n\
\tPower mode can be in 0..N - according to power states detected\n\
\tPerformance boost can be in 0..4 - disabled, enabled, aggressive, efficient, efficient aggressive\n\
\tNumber of fan boost values should be the same as a number of fans detected\n\
\tMode can be 0 or absent for set cooked value, 1 for raw value\n\
\tBrightness for ACPI lights can only have 10 values - 1,3,4,6,7,9,10,12,13,15\n");
// \tGPU power limit can be in 0..4 - 0 - no limit, 4 - max. limit\n\
// direct=<id>,<subid>[,val,val]\tIssue direct interface command (for testing)\n\
// directgpu=<id>,<value>\t\tIssue direct GPU interface command (for testing)\n\
// setgpu=<value>\t\t\tSet GPU power limit\n\
//\n\
//\tAll values in \"direct\" commands should be hex, not decimal!

}

int main(int argc, char* argv[])
{
    printf("AlienFan-CLI v7.2.0\n");

    AlienFan_SDK::Lights* lights = NULL;

    if (acpi->Probe()) {
        lights = new AlienFan_SDK::Lights(acpi);

        printf("Supported hardware (%d) detected, %d fans, %d sensors, %d power states%s%s.\n",
            acpi->GetSystemID(), (int)acpi->fans.size(), (int)acpi->sensors.size(), (int)acpi->powers.size(),
            (acpi->GetDeviceFlags() & DEV_FLAG_GMODE ? ", G-Mode" : ""),
            (lights->IsActivated() ? ", Lights" : ""));

        fan_conf->SetBoostsAndNames(acpi);
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

            if (command == "rpm") {
                if (args.size() > 0 && args[0].num < acpi->fans.size()) {
                    printf("%d\n", acpi->GetFanRPM(args[0].num));
                }
                else {
                    for (int i = 0; i < acpi->fans.size(); i++)
                        printf("Fan#%d: %d\n", i, acpi->GetFanRPM(i));
                }
                continue;
            }
            if (command == "percent") {
                if (args.size() > 0 && args[0].num < acpi->fans.size()) {
                    printf("%d\n", acpi->GetFanPercent(args[0].num));
                }
                else {
                    for (int i = 0; i < acpi->fans.size(); i++)
                        printf("Fan#%d: %d%%\n", i, acpi->GetFanPercent(i));
                }
                continue;
            }
            if (command == "temp") {
                if (args.size() > 0 && args[0].num < acpi->sensors.size()) {
                    printf("%d\n", acpi->GetTempValue(args[0].num));
                }
                else {
                    for (int i = 0; i < acpi->sensors.size(); i++) {
                        auto sname = fan_conf->sensors.find(i);
                        printf("%s: %d\n", (sname == fan_conf->sensors.end() ? acpi->sensors[i].name.c_str() : sname->second.c_str()), acpi->GetTempValue(i));
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
            if (command == "gmode") {
                int res = acpi->GetGMode();
                printf("G-mode state is %s\n", res ? res > 0 ? "On" : "Error" : "Off");
                continue;
            }
            if (command == "setpower" && CheckArgs(command, 1, args.size())) {

                if (args[0].num < acpi->powers.size()) {
                    printf("Power set to %d (result %d)\n", args[0].num, acpi->SetPower(acpi->powers[args[0].num]));
                }
                else
                    printf("Power: incorrect value (should be 0..%d)\n", (int)acpi->powers.size());
                continue;
            }
            //if (command == "setgpu" && CheckArgs(command, 1, args.size())) {

            //    printf("GPU limit set to %d (result %d)\n", args[0].num, acpi->SetGPU(args[0].num));
            //    continue;
            //}
            if (command == "setperf" && CheckArgs(command, 2, args.size())) {
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
                for (int i = 0; i < acpi->fans.size(); i++)
                    printf("Fan#%d boost %d\n", i, acpi->GetFanBoost(i, direct));
                continue;
            }
            if (command == "setfans" && CheckArgs(command, (int)acpi->fans.size(), args.size())) {

                bool direct = args.size() > acpi->fans.size() ? args[acpi->fans.size()].num : false;
                for (int i = 0; i < acpi->fans.size(); i++) {
                    printf("Fan#%d boost set to %d (result %d)\n", i, args[i].num, acpi->SetFanBoost(i, args[i].num, direct));
                }
                continue;
            }
            if (command == "setover") {
                int oldMode = acpi->GetPower();
                acpi->Unlock();
                if (args.size()) {
                    if (args[0].num < acpi->fans.size())
                        if (args.size() > 1) {
                            // manual fan set
                            acpi->Unlock();
                            bestBoostPoint = { (byte)args[0].num, (byte)args[1].num, 0 };
                            acpi->SetFanBoost(bestBoostPoint.fanID, 100, true);
                            Sleep(2000);
                            SetFanSteady(bestBoostPoint.maxBoost);
                            UpdateBoost();
                            acpi->SetFanBoost(bestBoostPoint.fanID, 0);
                            printf("\nBoost for fan #%d set to %d @ %d RPM.\n",
                                args[0].num, bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
                        }
                        else
                            // auto fan set
                            CheckFanOverboost(args[0].num);
                    else
                        printf("Incorrect fan ID (should be 0..%d)!\n", (int)acpi->fans.size() - 1);
                }
                else {
                    // all fans
                    for (int i = 0; i < acpi->fans.size(); i++)
                        CheckFanOverboost(i);
                }
                if (oldMode >= 0)
                acpi->SetPower(acpi->powers[oldMode]);
                printf("Done!\n");
                continue;
            }
            if (command == "setgmode" && CheckArgs(command, 1, args.size())) {
                printf("G-mode set result %d\n", acpi->SetGMode(args[0].num));
                if (!args[0].num)
                    acpi->SetPower(acpi->powers[fan_conf->prof.powerStage]);
                continue;
            }

            if (command == "resetcolor" && lights->IsActivated()) { // Reset color system for Aurora
                if (lights->Reset())
                    printf("Lights reset complete\n");
                else
                    printf("Lights reset failed\n");
                continue;
            }
            if (command == "setcolor" && lights->IsActivated() && CheckArgs(command, 4, args.size())) { // Set light color for Aurora

                printf("SetColor result %d.\n", lights->SetColor(args[0].num, args[1].num, args[2].num, args[3].num));
                lights->Update();
                continue;
            }
            if (command == "setbrightness" && lights->IsActivated() && CheckArgs(command, 2, args.size())) { // set brightness for Aurora

                printf("SetBrightness result %d.\n", lights->SetMode(args[0].num, args[1].num));
                lights->Update();
                continue;
            }
            if (command == "dump") { // dump WMI functions
                BSTR name;
                // Command dump
                acpi->m_AWCCGetObj->GetObjectText(0, &name);
                wprintf(L"Names: %s\n", name);
                continue;
            }
            //if (command == "test") { // dump WMI functions
            //    // SSD temperature sensors
            //    IWbemClassObject* driveObject = NULL, * instObj = NULL;
            //    IWbemLocator* m_WbemLocator;
            //    IWbemServices* m_WbemServices = NULL;
            //    CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&m_WbemLocator);
            //    m_WbemLocator->ConnectServer((BSTR)L"root\\Microsoft\\Windows\\Storage\\Providers_v2", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_WbemServices);
            //    m_WbemLocator->Release();
            //    if (m_WbemServices->GetObject((BSTR)L"MSFT_PhysicalDiskToStorageReliabilityCounter", NULL, nullptr, &driveObject, nullptr) == S_OK) {
            //        //BSTR name;
            //        VARIANT m_instancePath;
            //        // Command dump
            //        //m_ESIFObject->GetObjectText(0, &name);
            //        //wprintf(L"Names: %s\n", name);
            //        IEnumWbemClassObject* enum_obj;
            //        m_WbemServices->CreateInstanceEnum((BSTR)L"MSFT_PhysicalDiskToStorageReliabilityCounter", WBEM_FLAG_FORWARD_ONLY/*WBEM_FLAG_RETURN_IMMEDIATELY*/, NULL, &enum_obj);
            //        IWbemClassObject* spInstance;
            //        ULONG uNumOfInstances = 0;
            //        enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
            //        while (uNumOfInstances) {
            //            spInstance->Get((BSTR)L"StorageReliabilityCounter", 0, &m_instancePath, 0, 0);
            //            m_WbemServices->GetObject(m_instancePath.bstrVal, NULL, nullptr, &instObj, nullptr);
            //            instObj->Get((BSTR)L"Temperature", 0, &m_instancePath, 0, 0);
            //            wprintf(L"Temp: %d\n", m_instancePath.uintVal);
            //            instObj->Release();
            //            spInstance->Release();
            //            enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
            //        }
            //        enum_obj->Release();
            //        //IWbemClassObject* m_outParameters = NULL;
            //        //if (acpi->m_WbemServices->ExecMethod(acpi->m_instancePath.bstrVal,
            //        //    (BSTR)L"MemoryOCControl", 0, NULL, NULL, &m_outParameters, NULL) == S_OK && m_outParameters) {
            //        //    //m_InParamaters->Release();
            //        //    VARIANT result;
            //        //    m_outParameters->Get(L"Data", 0, &result, nullptr, nullptr);
            //        //    m_outParameters->Release();
            //        //    printf("Result - %d", result.uintVal);
            //        //}
            //    }
            //    continue;
            //}
            printf("Unknown command - %s, use \"usage\" or \"help\" for information\n", command.c_str());
        }
    }
    else {
        printf("Supported hardware not found!\n");
    }

    if (argc < 2)
    {
        Usage();
    }

    delete lights;
    delete acpi;
    delete fan_conf;

}
