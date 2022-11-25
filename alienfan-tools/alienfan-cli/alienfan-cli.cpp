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
    acpi.SetFanBoost(fanID, boost, true);
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

void UpdateBoost(byte fanID) {
    fan_conf.boosts[fanID].maxBoost = max(bestBoostPoint.maxBoost, 100);
    fan_conf.boosts[fanID].maxRPM = max(bestBoostPoint.maxRPM, fan_conf.boosts[fanID].maxRPM);

    acpi.boosts[fanID] = max(bestBoostPoint.maxBoost, 100);
    acpi.maxrpm[fanID] = max(fan_conf.boosts[fanID].maxRPM, acpi.maxrpm[fanID]);

    fan_conf.Save();
}

void CheckFanOverboost(byte num) {
    int cSteps = 8, boost = 100, cBoost = 100, rpm, oldBoost = acpi.GetFanBoost(num, true);
    printf("Checking Fan#%d:\n", num);
    bestBoostPoint = { 100, 0 };
    rpm = SetFanSteady(num, boost);
    printf("    \n");
    for (int steps = cSteps; steps; steps = steps >> 1) {
        // Check for uptrend
        while ((boost+=steps) != cBoost)
        {
            if (SetFanSteady(num, boost, true) > rpm) {
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
        while (boost > 100 && SetFanSteady(num, boost) >= bestBoostPoint.maxRPM - 55) {
            bestBoostPoint.maxBoost = boost;
            boost -= steps;
            printf("(New best: %d @ %d RPM)\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
        }
        if (boost > 100)
            printf("(Step back)\n");
        boost = bestBoostPoint.maxBoost;
    }
    printf("Final boost - %d, %d RPM\n\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
    acpi.SetFanBoost(num, oldBoost, true);
    UpdateBoost(num);
}

const char* GetFanType(int index) {
    const char* res;
    switch (acpi.fans[index].type) {
    case 0: res = "CPU"; break;
    case 1: res = "GPU"; break;
    default: res = "Fan";
    }
    return res;
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
getfans[=[fanID,]<mode>]\t\tShow fan boost level (0..100 - in percent) with selected mode\n\
setfans=<fan1>[,<fanN>][,mode]\tSet fans boost level (0..100 - in percent) with selected mode\n\
setover[=fanID[,boost]]\t\tSet overboost for selected fan to boost (manual or auto)\n\
setgmode=<mode>\t\t\tSet G-mode on/off (1-on, 0-off)\n\
gmode\t\t\t\tShow G-mode state\n\
setcolor=id,r,g,b\t\tSet light to color\n\
setbrightness=<brightness>\tSet lights brightness\n\
\tPower mode can be in 0..N - according to power states detected\n\
\tPerformance boost can be in 0..4 - disabled, enabled, aggressive, efficient, efficient aggressive\n\
\tNumber of fan boost values should be the same as a number of fans detected\n\
\tMode can absent for cooked value, and \"raw\" for raw value\n\
\tBrightness can be in 0..15\n");
// \tGPU power limit can be in 0..4 - 0 - no limit, 4 - max. limit\n\
// direct=<id>,<subid>[,val,val]\tIssue direct interface command (for testing)\n\
// directgpu=<id>,<value>\t\tIssue direct GPU interface command (for testing)\n\
// setgpu=<value>\t\t\tSet GPU power limit\n\
//\n\
//\tAll values in \"direct\" commands should be hex, not decimal!

}

int main(int argc, char* argv[])
{
    printf("AlienFan-CLI v7.7.1\n");

    AlienFan_SDK::Lights* lights = new AlienFan_SDK::Lights(&acpi);

    if (acpi.Probe()) {
        printf("Supported hardware (%d) detected, %d fans, %d sensors, %d power states%s%s.\n",
            acpi.GetSystemID(), (int)acpi.fans.size(), (int)acpi.sensors.size(), (int)acpi.powers.size(),
            (acpi.GetDeviceFlags() & DEV_FLAG_GMODE ? ", G-Mode" : ""),
            (lights->isActivated ? ", Lights" : ""));
        fan_conf.SetBoostsAndNames(&acpi);
    }
    else {
        if (acpi.GetDeviceFlags() & DEV_FLAG_AWCC)
            printf("Alienware system, ");
        printf("Hardware not compatible.\n");
    }

    for (int cc = 1; cc < argc; cc++) {
        string arg = string(argv[cc]);
        size_t vid = arg.find_first_of('=');
        command = arg.substr(0, vid);
        args.clear();
        if (vid != arg.length() - 1)
            while (vid != string::npos) {
                size_t argPos = arg.find(',', vid + 1);
                args.push_back({ arg.substr(vid + 1, (argPos == string::npos ? arg.length() : argPos) - vid - 1) });
                args.back().num = atoi(args.back().str.c_str());
                vid = argPos;
            }

        if (command == "rpm") {
            if (CheckArgs(1, acpi.fans.size(), false))
                printf("%s %d: %d\n", GetFanType(args[0].num), args[0].num, acpi.GetFanRPM(args[0].num));
            else
                for (int i = 0; i < acpi.fans.size(); i++)
                    printf("%s %d: %d\n", GetFanType(i), i, acpi.GetFanRPM(i));
            continue;
        }
        if (command == "percent") {
            if (CheckArgs(0, acpi.fans.size()))
                printf("%s %d: %d%%\n", GetFanType(args[0].num), args[0].num, acpi.GetFanPercent(args[0].num));
            else
                for (int i = 0; i < acpi.fans.size(); i++)
                    printf("%s %d: %d%%\n", GetFanType(i), i, acpi.GetFanPercent(i));
            continue;
        }
        if (command == "temp") {
            if (CheckArgs(0, acpi.sensors.size()))
                printf("%d\n", acpi.GetTempValue(args[0].num));
            else
                for (int i = 0; i < acpi.sensors.size(); i++) {
                    auto sname = fan_conf.sensors.find(acpi.sensors[i].sid);
                    printf("%s: %d\n", (sname == fan_conf.sensors.end() ? acpi.sensors[i].name.c_str() :
                        sname->second.c_str()), acpi.GetTempValue(i));
                }
            continue;
        }
        if (command == "unlock") {
            printf("%s", acpi.Unlock() < 0 ? "Unlock failed!\n" : "Unlock successful.\n");
            continue;
        }
        if (command == "gmode") {
            int res = acpi.GetGMode();
            printf("G-mode state is %s\n", res ? res > 0 ? "On" : "Error" : "Off");
            continue;
        }
        if (command == "setpower" && CheckArgs(1, acpi.powers.size())) {
            printf("Power set to %d (result %d)\n", args[0].num, acpi.SetPower(acpi.powers[args[0].num]));
            continue;
        }
        //if (command == "setgpu" && CheckArgs(command, 1, args.size())) {

        //    printf("GPU limit set to %d (result %d)\n", args[0].num, acpi->SetGPU(args[0].num));
        //    continue;
        //}
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
            printf("Current power mode: %d\n", acpi.GetPower());
            continue;
        }
        if (command == "getfans") {
            if (CheckArgs(1, acpi.fans.size(), false) && args[0].str != "raw")
                printf("%s %d boost - %d\n", GetFanType(args[0].num), args[0].num, acpi.GetFanBoost(args[0].num, args.back().str == "raw"));
            else
                for (int i = 0; i < acpi.fans.size(); i++)
                    printf("%s %d boost %d\n", GetFanType(i), i, acpi.GetFanBoost(i, args.size() && args.back().str == "raw"));
            continue;
        }
        if (command == "setfans") {
            if (CheckArgs(acpi.fans.size(), 256))
                for (int i = 0; i < acpi.fans.size(); i++)
                    printf("%s %d boost set to %d (result %d)\n", GetFanType(i), i, args[i].num,
                        acpi.SetFanBoost(i, args[i].num, args.back().str == "raw"));
            continue;
        }
        if (command == "setover") {
            int oldMode = acpi.GetPower();
            acpi.Unlock();
            if (CheckArgs(1, acpi.fans.size(), false)) {
                byte fanID = args[0].num;
                if (args.size() > 1) {
                    // manual fan set
                    bestBoostPoint = { (byte)args[1].num, 0 };
                    SetFanSteady(fanID, bestBoostPoint.maxBoost);
                    UpdateBoost(fanID);
                    printf("\nBoost for %s %d set to %d @ %d RPM.\n", GetFanType(fanID),
                        fanID, bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
                }
                else
                    // auto fan set
                    CheckFanOverboost(fanID);
            }
            else {
                // all fans
                for (int i = 0; i < acpi.fans.size(); i++)
                    CheckFanOverboost(i);
            }
            if (oldMode >= 0)
                acpi.SetPower(acpi.powers[oldMode]);
            continue;
        }
        if (command == "setgmode" && CheckArgs(1, 2)) {
            if ((acpi.GetDeviceFlags() & DEV_FLAG_GMODE) && acpi.GetGMode() != args[0].num) {
                if (acpi.GetSystemID() == 2933 && args[0].num) // G5 5510 fix
                    acpi.SetPower(acpi.powers[1]);
                acpi.SetGMode(args[0].num);
                if (!args[0].num)
                    acpi.SetPower(acpi.powers[fan_conf.prof.powerStage]);
                printf("G-mode %s\n", args[0].num ? "On" : "Off");
            }
            continue;
        }
        if (command == "setcolor" && lights->isActivated && CheckArgs(4, 255)) { // Set light color for Aurora
            printf("SetColor result %d.\n", lights->SetColor(args[0].num, args[1].num, args[2].num, args[3].num));
            continue;
        }
        if (command == "setbrightness" && lights->isActivated && CheckArgs(1, 15)) { // set brightness for Aurora
            printf("SetBrightness result %d.\n", lights->SetBrightness(args[0].num));
            continue;
        }

        if (command == "dump" && (acpi.GetDeviceFlags() & DEV_FLAG_AWCC)) { // dump WMI functions
            BSTR name;
            // Command dump
            acpi.m_AWCCGetObj->GetObjectText(0, &name);
            wprintf(L"Names: %s\n", name);
            continue;
        }
        //if (command == "thermal" && (acpi.GetDeviceFlags() & DEV_FLAG_AWCC)) { // test thermal command
        //    acpi.m_AWCCGetObj->GetMethod((BSTR)L"GetThermalInfo", NULL, &acpi.m_InParamaters, nullptr);
        //    VARIANT parameters = { VT_I4 };
        //    if (acpi.m_InParamaters) {
        //        for (byte i = 0; i < 10; i++) {
        //            IWbemClassObject* m_outParameters = NULL;
        //            parameters.uintVal = AlienFan_SDK::ALIENFAN_INTERFACE{ i, 0, 0 }.args;
        //            if (acpi.m_InParamaters && acpi.m_WbemServices->ExecMethod(acpi.m_instancePath.bstrVal,
        //                (BSTR)L"GetThermalInfo", 0, NULL, acpi.m_InParamaters, &m_outParameters, NULL) == S_OK && m_outParameters) {
        //                VARIANT result{ VT_I4 };
        //                m_outParameters->Get(L"argr", 0, &result, nullptr, nullptr);
        //                m_outParameters->Release();
        //                printf("Thermal for %d is %d\n", i, result.intVal);
        //            }
        //            else
        //                printf("Thermal for %d failed\n", i);
        //        }
        //    }
        //    else
        //        printf("Thermal InParam failed\n");
        //    continue;
        //}
        //if (command == "test") { // dump WMI functions
        //    IWbemClassObject* driveObject = NULL;
        //    IWbemLocator* m_WbemLocator;
        //    IWbemServices* m_WbemServices = NULL;
        //    //IEnumWbemClassObject* enum_obj;
        //    CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&m_WbemLocator);
        //    m_WbemLocator->ConnectServer((BSTR)L"root\\WMI", nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &m_WbemServices);
        //    m_WbemLocator->Release();
        //    if (m_WbemServices->GetObject((BSTR)L"WMI_FanSpeedControl", NULL, nullptr, &driveObject, nullptr) == S_OK) {
        //        BSTR name;
        //        VARIANT /*m_instancePath, */wSpeed{ VT_UI2 };
        //        IWbemClassObject* m_InParamaters = NULL;
        //        IWbemClassObject* m_outParameters = NULL;
        //        IWbemClassObject* ECInst = NULL;
        //        // Command dump
        //        driveObject->GetObjectText(0, &name);
        //        wprintf(L"Names: %s\n", name);
        //        //m_WbemServices->CreateInstanceEnum((BSTR)L"WMI_FanSpeedControl", WBEM_FLAG_FORWARD_ONLY, NULL, &enum_obj);
        //        //IWbemClassObject* spInstance;
        //        //ULONG uNumOfInstances = 0;
        //        //enum_obj->Next(10000, 1, &spInstance, &uNumOfInstances);
        //        //driveObject->Get(L"InstanceName", 0, &m_instancePath, nullptr, nullptr);
        //        //spInstance->Release();
        //        //enum_obj->Release();
        //        driveObject->GetMethod((BSTR)L"ECSetCPUFan", NULL, &m_InParamaters, nullptr);
        //        //m_InParamaters->SpawnInstance(0, &ECInst);
        //        wSpeed.iVal = 0;
        //        m_InParamaters->Put(L"wSpeed", NULL, &wSpeed, NULL);
        //        IErrorInfo* errinfo;
        //        m_WbemServices->ExecMethod((BSTR)L"WMI_FanSpeedControl",
        //            (BSTR)L"ECSetCPUFan", 0, NULL, m_InParamaters, NULL, NULL);
        //        GetErrorInfo(0, &errinfo);
        //        errinfo->GetDescription(&name);
        //        m_InParamaters->Get(L"wSpeed", 0, &wSpeed, nullptr, nullptr);
        //        if (m_outParameters) {
        //            m_outParameters->Get(L"wSpeed", 0, &wSpeed, nullptr, nullptr);
        //            m_outParameters->Release();
        //            printf("Result %d\n", wSpeed.iVal);
        //        }
        //        else
        //            printf("Failed\n");
        //    }
        //    continue;
        //}
        printf("Unknown command - %s, run without parameters for help.\n", command.c_str());
    }

    if (argc < 2)
    {
        Usage();
    }

    delete lights;
}
