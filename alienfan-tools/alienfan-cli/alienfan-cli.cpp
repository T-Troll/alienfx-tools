#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <vector>
#include <combaseapi.h>
#include <PowrProf.h>
#include "alienfan-SDK.h"
#include "ConfigFan.h"

#pragma comment(lib, "PowrProf.lib")

//#include <initguid.h>
//#include <propkeydef.h>
//#include <propvarutil.h>
//#include <functiondiscoverykeys.h>
//#include <sensorsapi.h>
//#include <sensors.h>
//
//#pragma comment(lib,"sensorsapi.lib")
//#pragma comment(lib, "Propsys.lib")
//#pragma comment(lib, "PortableDeviceGuids.lib")

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

string ReadFromESIF(HANDLE proc, HANDLE g_hChildStd_IN_Wr, HANDLE g_hChildStd_OUT_Rd, string command, bool wait) {
    DWORD written;
    string outpart;
    byte e_command[] = "echo noop\n";
    if (WaitForSingleObject(proc, 0) != WAIT_TIMEOUT)
        return "";
    WriteFile(g_hChildStd_IN_Wr, command.c_str(), (DWORD)command.length(), &written, NULL);
    FlushProcessWriteBuffers();
    while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && written) {
        char* buffer = new char[written + 1]{ 0 };
        ReadFile(g_hChildStd_OUT_Rd, buffer, written, &written, NULL);
        outpart += buffer;
        delete[] buffer;
    }
    while (wait && outpart.find("</result>") == string::npos && outpart.find("ESIF_E") == string::npos
        && outpart.find("Error") == string::npos) {
        while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && !written) {
            for (int i = 0; (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && !written) && i < 40; i++) {
                if (WaitForSingleObject(proc, 0) != WAIT_TIMEOUT)
                    return "";
                WriteFile(g_hChildStd_IN_Wr, e_command, sizeof(e_command) - 1, &written, NULL);
                FlushProcessWriteBuffers();
            }
            Sleep(5);
        }
        while (PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &written, NULL) && written) {
            char* buffer = new char[written + 1]{ 0 };
            ReadFile(g_hChildStd_OUT_Rd, buffer, written, &written, NULL);
            outpart += buffer;
            delete[] buffer;
        }
    }
    if (!wait || outpart.find("ESIF_E") != string::npos || outpart.find("Error") != string::npos)
        return "";
    else {
        size_t firstpos = outpart.find("<result>"),
            lastpos = outpart.find("</result>");
        return outpart.substr(firstpos, lastpos - firstpos + 9);
    }
}

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

void UpdateBoost(byte fanID) {
    fan_overboost* fo = &fan_conf.boosts[fanID];
    fo->maxBoost = max(bestBoostPoint.maxBoost, 100);
    fo->maxRPM = max(bestBoostPoint.maxRPM, fo->maxRPM);
}

void CheckFanOverboost(byte num) {
    int rpm, crpm, cSteps = 8, boost, oldBoost = acpi.GetFanBoost(num), downScale;
    fan_overboost* fo = &fan_conf.boosts[num];
    printf("Checking Fan#%d:\n", num);
    bestBoostPoint = *fo;
    SetFanSteady(num, 100);
    boost = fo->maxBoost;
    rpm = fo->maxRPM;
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
        while (boost > 100 && SetFanSteady(num, boost) > bestBoostPoint.maxRPM - downScale) {
            bestBoostPoint.maxBoost = boost;
            boost -= steps;
            printf("(New best: %d @ %d RPM)\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
        }
        if (boost > 100)
            printf("(Step back)\n");
        boost = bestBoostPoint.maxBoost;
    }
    printf("Final boost - %d, %d RPM\n\n", bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
    acpi.SetFanBoost(num, oldBoost);
    UpdateBoost(num);
}

const char* GetFanType(int index) {
    const char* res;
    switch (acpi.fans[index].type) {
    case 0: res = " CPU"; break;
    case 1: res = " GPU"; break;
    default: res = "";
    }
    return res;
}

void Usage() {
    printf("Usage: alienfan-cli command[=value{,value}] {command...}\n\
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
setcolor=id,r,g,b\t\tSet light to color\n\
setbrightness=<brightness>\tSet lights brightness\n\
\tPower mode can be in 0..N - according to power states detected\n\
\tPerformance boost can be in 0..4 - disabled, enabled, aggressive, efficient, efficient aggressive\n\
\tNumber of fan boost values should be the same as a number of fans detected\n\
\tMode can absent for cooked value, and \"raw\" for raw value\n\
\tBrightness can be in 0..15\n");
}

int main(int argc, char* argv[])
{
    printf("AlienFan-CLI v7.9.2.1\n");

    AlienFan_SDK::Lights* lights = NULL;

    if (acpi.Probe()) {
        lights = new AlienFan_SDK::Lights(&acpi);
        printf("Supported hardware (%d) detected, %d fans, %d sensors, %d power states%s%s.\n",
            acpi.GetSystemID(), (int)acpi.fans.size(), (int)acpi.sensors.size(), (int)acpi.powers.size(),
            (acpi.GetDeviceFlags() & DEV_FLAG_GMODE ? ", G-Mode" : ""),
            (lights->isActivated ? ", Lights" : ""));
        fan_conf.SetSensorNames(&acpi);
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
            for (int i = 0; i < acpi.fans.size(); i++)
                if (args.empty() || args[0].num == i)
                    printf("Fan%s %d: %d\n", GetFanType(i), i, acpi.GetFanRPM(i));
            continue;
        }
        if (command == "maxrpm") {
            for (int i = 0; i < acpi.fans.size(); i++)
                if (args.empty() || args[0].num == i)
                    printf("Fan%s %d: %d\n", GetFanType(i), i, acpi.GetMaxRPM(i));
            continue;
        }
        if (command == "percent") {
            for (int i = 0; i < acpi.fans.size(); i++)
                if (args.empty() || args[0].num == i) {
                    auto maxboost = fan_conf.boosts.find(i);
                    printf("%s %d: %d%%\n", GetFanType(i), i, maxboost == fan_conf.boosts.end() ?
                        acpi.GetFanPercent(i) : (acpi.GetFanRPM(i) * 100) / maxboost->second.maxRPM);
                }
            continue;
        }
        if (command == "temp") {
            for (int i = 0; i < acpi.sensors.size(); i++)
                if (args.empty() || i == args[0].num) {
                    auto sname = fan_conf.sensors.find(acpi.sensors[i].sid);
                    printf("%s: %d\n", (sname == fan_conf.sensors.end() ? acpi.sensors[i].name.c_str() :
                        sname->second.c_str()), acpi.GetTempValue(i));
                }
            continue;
        }
        if (command == "unlock") {
            printf("%s", acpi.Unlock() < 0 ? "Unlock failed!\n" : "Unlocked.\n");
            continue;
        }
        if (command == "gmode") {
            int res = acpi.GetGMode();
            printf("G-mode state is %s\n", res ? res > 0 ? "On" : "Error" : "Off");
            continue;
        }
        if (command == "setpower" && CheckArgs(1, acpi.powers.size())) {
            acpi.SetPower(acpi.powers[args[0].num]);
            printf("Power mode set to %s(%d)\n", fan_conf.powers[acpi.powers[args[0].num]].c_str(), acpi.powers[args[0].num]);
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
            int res = acpi.GetPower();
            if (res >= 0)
                printf("Power mode: %s(%d)\n", fan_conf.powers[acpi.powers[res]].c_str(), acpi.powers[res]);
            continue;
        }
        if (command == "getfans") {
            int rawboost;
            bool single = CheckArgs(1, acpi.fans.size(), false) && args[0].str != "raw";
            for (int i = 0; i < acpi.fans.size(); i++)
                if (!single || i == args[0].num) {
                    rawboost = acpi.GetFanBoost(i);
                    printf("Fan%s %d boost %d (%d)\n", GetFanType(i), i,
                        args.size() && args.back().str == "raw" ? rawboost : (rawboost * 100) / fan_conf.GetFanScale(i), rawboost);
                }
            continue;
        }
        if (command == "setfans") {
            if (CheckArgs(acpi.fans.size(), 256))
                for (int i = 0; i < acpi.fans.size(); i++)
                    printf("Fan%s %d boost set to %d (%d)\n", GetFanType(i), i, args[i].num,
                        acpi.SetFanBoost(i, args.back().str == "raw" ? args[i].num : (args[i].num * fan_conf.GetFanScale(i)) / 100));
            continue;
        }
        if (command == "setover") {
            int oldMode = acpi.GetPower();
            acpi.Unlock();
            if (CheckArgs(2, acpi.fans.size(), false)) {
                byte fanID = args[0].num;
                // manual fan set
                bestBoostPoint = { (byte)args[1].num, 0 };
                SetFanSteady(fanID, bestBoostPoint.maxBoost);
                UpdateBoost(fanID);
                printf("\nBoost for Fan%s %d set to %d @ %d RPM.\n", GetFanType(fanID),
                    fanID, bestBoostPoint.maxBoost, bestBoostPoint.maxRPM);
            }
            else {
                // all fans
                for (int i = 0; i < acpi.fans.size(); i++)
                    if (args.empty() || i == args[0].num)
                        CheckFanOverboost(i);
            }
            if (oldMode >= 0)
                acpi.SetPower(acpi.powers[oldMode]);
            continue;
        }
        if (command == "setgmode" && CheckArgs(1, 2)) {
            if ((acpi.GetDeviceFlags() & DEV_FLAG_GMODE) && acpi.GetGMode() != args[0].num) {
                if (acpi.GetSystemID() == 2933 && args[0].num) // G5 5510 fix
                    acpi.SetPower(0xa0);
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
            wprintf(L"Names:\n%s\n", name);
            continue;
        }

        if (command == "test") { // Test
            while (!acpi.DPTFdone)
                Sleep(200);
            for (int i = 0; i < acpi.sensors.size(); i++)
                if (acpi.sensors[i].type == 3) {
                    auto sname = fan_conf.sensors.find(acpi.sensors[i].sid);
                    printf("%s: %d\n", (sname == fan_conf.sensors.end() ? acpi.sensors[i].name.c_str() :
                        sname->second.c_str()), acpi.GetTempValue(i));
                }
            // Find UF path
            //string wdName;
            //wdName.resize(2048);
            //wdName.resize(GetWindowsDirectory((LPSTR)wdName.data(), 2047));
            //wdName += "\\system32\\DriverStore\\FileRepository\\";
            //WIN32_FIND_DATA file;
            //HANDLE search_handle = FindFirstFile((wdName + "dptf_cpu*").c_str(), &file);
            //if (search_handle != INVALID_HANDLE_VALUE)
            //{
            //    wdName += string(file.cFileName) + "\\esif_uf.exe";
            //    FindClose(search_handle);
            //}
            //// Start process with UF client
            //SECURITY_ATTRIBUTES attr{ sizeof(SECURITY_ATTRIBUTES), NULL, true };
            //STARTUPINFO sinfo{ sizeof(STARTUPINFO), 0 };
            //HANDLE g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd;
            //CreatePipe(&g_hChildStd_OUT_Rd, &sinfo.hStdOutput, &attr, 0);
            //CreatePipe(&sinfo.hStdInput, &g_hChildStd_IN_Wr, &attr, 0);
            //PROCESS_INFORMATION proc;
            ////AllocConsole();
            //sinfo.dwFlags = STARTF_USESTDHANDLES;
            //sinfo.hStdError = sinfo.hStdOutput;
            ////setvbuf(stdin, NULL, _IONBF, 4096);
            ////setvbuf(stdout, NULL, _IONBF, 4096);
            ////_setmode(_fileno(stdout), _O_BINARY);
            //bool res = CreateProcess(NULL/*(LPSTR)wdName.c_str()*/, (LPSTR)(wdName + " client").c_str(),
            //    NULL, NULL, true,
            //    0/*CREATE_NEW_CONSOLE*/, NULL, NULL, &sinfo, &proc);
            ////WaitForInputIdle(proc.hProcess, INFINITE);
            //// Execute commands
            //string parts = ReadFromESIF(proc.hProcess, g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, "format xml\nparticipants\n", true);
            //size_t pos = 0;
            //while (parts.find("<participant>", pos) != string::npos) {
            //    pos = parts.find("<UpId>", pos);
            //    size_t fPos = parts.find("</UpId>", pos);
            //    int sID = atoi(parts.substr(pos + 6, fPos - pos - 6).c_str());
            //    pos = parts.find("<desc>", fPos);
            //    fPos = parts.find("</desc>", pos);
            //    string name = parts.substr(pos + 6, fPos - pos - 6);
            //    printf("Sensor %d (%s) - ", sID, name.c_str());
            //    string val = ReadFromESIF(proc.hProcess, g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, "getp_part " + to_string(sID)+ " 14\n", true);
            //    if (val.empty())
            //        printf("Incorrect\n");
            //    else {
            //        size_t vPos = val.find("<value>"), vFpos = val.find("</value>", vPos);
            //        printf("%s\n", val.substr(vPos + 7, vFpos - vPos - 7).c_str());
            //    }
            //}
            //ReadFromESIF(proc.hProcess, g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, "exit\n", false);

            //CloseHandle(sinfo.hStdInput);
            //CloseHandle(g_hChildStd_IN_Wr);
            //CloseHandle(g_hChildStd_OUT_Rd);
            //CloseHandle(sinfo.hStdOutput);

            //CloseHandle(proc.hProcess);
            //CloseHandle(proc.hThread);
            // FreeConsole();

            continue;
        }
        printf("Unknown command - %s, run without parameters for help.\n", command.c_str());
    }

    if (argc < 2)
    {
        Usage();
    }

    if (lights)
        delete lights;
}
