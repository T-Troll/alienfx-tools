// alienfan-tools.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <vector>
#include "alienfan-SDK.h"
#include <shellapi.h>

using namespace std;

AlienFan_SDK::Control *InitAcpi() {
    AlienFan_SDK::Control *cAcpi = new AlienFan_SDK::Control();

    if (!cAcpi->IsActivated()) {
        // Driver can't start, let's do kernel hack...
        delete cAcpi;
        char currentPath[MAX_PATH];
        GetModuleFileName(NULL, currentPath, MAX_PATH);
        string cpath = currentPath;
        cpath.resize(cpath.find_last_of("\\"));
        string shellcom = "-prv 6 -scv 3 -drvn HwAcc -map \"" + cpath + "\\HwAcc.sys\"",
            shellapp = "\"" + cpath + "\\KDU\\kdu.exe\"";

        SHELLEXECUTEINFO shBlk = {0};
        shBlk.cbSize = sizeof(SHELLEXECUTEINFO);
        shBlk.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
        shBlk.lpFile = shellapp.c_str();
        shBlk.lpParameters = shellcom.c_str();
        shBlk.lpVerb = "runas";
        shBlk.nShow = SW_HIDE;

        if (ShellExecuteEx(&shBlk)) {
            WaitForSingleObject(shBlk.hProcess, INFINITE);
            CloseHandle(shBlk.hProcess);
            cAcpi = new AlienFan_SDK::Control();
        } else {
            cAcpi = NULL;
        }
    }

    return cAcpi;
}

void Usage() {
    cout << "Usage: alienfan-cli [command[=value{,value}] [command...]]" << endl
        << "Avaliable commands: " << endl
        << "usage, help\t\t\tShow this usage" << endl
        //<< "dump\t\t\t\tDump all ACPI values avaliable (for debug and new hardware support)" << endl
        << "rpm\t\t\t\tShow fan(s) RPMs" << endl
        << "temp\t\t\t\tShow known temperature sensors values" << endl
        << "unlock\t\t\t\tUnclock fan controls" << endl
        << "getpower\t\t\tDisplay current power state" << endl
        << "power=<value>\t\t\tSet TDP to this level" << endl
        << "gpu=<value>\t\t\tSet GPU power limit" << endl
        << "getfans\t\t\t\tShow current fan boost level (0..100 - in percent)" << endl
        << "setfans=<fan1>[,<fan2>]\t\tSet fans boost level (0..100 - in percent)" << endl
        << "direct=<id>,<subid>[,val,val]\tIssue direct interface command (for testing)" << endl
        << "directgpu=<id>,<value>\t\tIssue direct GPU interface command (for testing)" << endl
        << "  Power level can be in 0..N - according to power states detected" << endl
        << "  GPU power limit can be in 0..4 - 0 - no limit, 4 - max. limit" << endl
        << "  Number of fan boost values should be the same as a number fans detected" << endl
        << "  All values in \"direct\" commands should be hex, not decimal!" << endl;
}

int main(int argc, char* argv[])
{
    std::cout << "AlienFan-cli v1.1.0.2\n";

    AlienFan_SDK::Control *acpi = InitAcpi();// new AlienFan_SDK::Control();

    if (acpi && acpi->IsActivated()) {

        if (acpi->Probe()) {
            cout << "Supported hardware detected, " << acpi->HowManyFans() << " fans, " 
                << acpi->sensors.size() << " sensors, " << acpi->HowManyPower() << " power states." << endl;

            if (argc < 2) 
            {
                Usage();
            } else {

                for (int cc = 1; cc < argc; cc++) {
                    string arg = string(argv[cc]);
                    size_t vid = arg.find_first_of('=');
                    string command = arg.substr(0, vid);
                    string values;
                    vector<string> args;
                    if (vid != string::npos) {
                        size_t vpos = 0;
                        values = arg.substr(vid + 1, arg.size());
                        while (vpos < values.size()) {
                            size_t tvpos = values.find(',', vpos);
                            args.push_back(values.substr(vpos, tvpos - vpos));
                            vpos = tvpos == string::npos ? values.size() : tvpos + 1;
                        }
                    }
                    //cerr << "Executing " << command << " with " << values << endl;
                    if (command == "usage" || command == "help" || command == "?") {
                        Usage();
                        continue;
                    }
                    if (command == "rpm") {
                        int prms = 0;
                        for (int i = 0; i < acpi->HowManyFans(); i++)
                            if ((prms = acpi->GetFanRPM(i)) >= 0)
                                cout << "Fan#" << i << ": " << prms << endl;
                            else {
                                cout << "RPM reading failed!" << endl;
                                break;
                            }
                        continue;
                    }
                    if (command == "temp") {
                        int res = 0;
                        for (int i = 0; i < acpi->sensors.size(); i++) {
                            if ((res = acpi->GetTempValue(i)) >= 0)
                                cout << acpi->sensors[i].name << ": " << res << endl;
                        }
                        continue;
                    }
                    if (command == "unlock") {
                        if (acpi->Unlock() >= 0)
                            cout << "Unlock successful." << endl;
                        else
                            cout << "Unlock failed!" << endl;
                        continue;
                    }
                    if (command == "power") {
                        if (args.size() < 1) {
                            cout << "Power: incorrect arguments" << endl;
                            continue;
                        }
                        BYTE unlockStage = atoi(args[0].c_str());
                        if (unlockStage < acpi->HowManyPower()) {
                            if (acpi->SetPower(unlockStage) >= 0)
                                cout << "Power set to " << (int) unlockStage << endl;
                            else
                                cout << "Power set failed!" << endl;
                        } else
                            cout << "Power: incorrect value (should be 0.." << acpi->HowManyPower() << ")" << endl;
                        continue;
                    }
                    if (command == "gpu") {
                        if (args.size() < 1) {
                            cout << "GPU: incorrect arguments" << endl;
                            continue;
                        }
                        BYTE gpuStage = atoi(args[0].c_str());
                        if (acpi->SetGPU(gpuStage) >= 0)
                            cout << "GPU limit set to " << (int) gpuStage << endl;
                        else
                            cout << "GPU limit set failed!" << endl;
                        continue;
                    }
                    if (command == "getpower") {
                        int cpower = acpi->GetPower();
                        if (cpower >= 0)
                            cout << "Current power mode: " << cpower << endl;
                        else
                            cout << "Getpower failed!" << endl;
                        continue;
                    }
                    if (command == "getfans") {
                        int prms = 0;
                        for (int i = 0; i < acpi->HowManyFans(); i++)
                            if ((prms = acpi->GetFanValue(i)) >= 0)
                                cout << "Fan#" << i << " now at " << prms << endl;
                            else {
                                cout << "Get fan settings failed!" << endl;
                                break;
                            }
                        continue;
                    }
                    if (command == "setfans") {
                        if (args.size() < acpi->HowManyFans()) {
                            cout << "Setfans: incorrect arguments (should be " << acpi->HowManyFans() << ")" << endl;
                            continue;
                        }
                        int prms = 0;
                        for (int i = 0; i < acpi->HowManyFans(); i++) {
                            BYTE boost = atoi(args[i].c_str());
                            if (acpi->SetFanValue(i, boost))
                                cout << "Fan#" << i << " set to " << (int) boost << endl;
                            else {
                                cout << "Set fan level failed!" << endl;
                                break;
                            }
                        }
                        continue;
                    }
                    if (command == "direct") {
                        int res = 0;
                        if (args.size() < 2) {
                            cout << "Direct: incorrect arguments (should be 2 or 3)" << endl;
                            continue;
                        }
                        USHORT command = (USHORT) strtol(args[0].c_str(), NULL, 16);
                        byte subcommand = (byte) strtol(args[1].c_str(), NULL, 16),
                            value1 = 0, value2 = 0;
                        if (args.size() > 2)
                            value1 = (byte) strtol(args[2].c_str(), NULL, 16);
                        if (args.size() > 3)
                            value2 = (byte) strtol(args[3].c_str(), NULL, 16);
                        if ((res = acpi->RunMainCommand(command, subcommand, value1, value2)) >= 0)
                            cout << "Direct call result: " << res << endl;
                        else {
                            cout << "Direct call failed!" << endl;
                            break;
                        }
                        continue;
                    }
                    if (command == "directgpu") {
                        int res = 0;
                        if (args.size() < 2) {
                            cout << "DirectGPU: incorrect arguments (should be 2)" << endl;
                            continue;
                        } 
                        USHORT command = (USHORT) strtol(args[0].c_str(), NULL, 16);// atoi(args[0].c_str());
                        DWORD subcommand = strtol(args[1].c_str(), NULL, 16);// atoi(args[1].c_str());
                        if ((res = acpi->RunGPUCommand(command, subcommand)) >= 0)
                            cout << "DirectGPU call result: " << res << endl;
                        else {
                            cout << "DirectGPU call failed!" << endl;
                            break;
                        }
                        continue;
                    }
                    //if (command == "test") { // pseudo block for tes modules
                    //    continue;
                    //}
                    cout << "Unknown command - " << command << ", use \"usage\" or \"help\" for information" << endl;
                }
            }
        } else {
            cout << "Supported hardware not found!" << endl;
            acpi->UnloadService();
        }
    } else {
        cout << "System configuration issue - see readme.md for details!" << endl;
    }
    delete acpi;

}
