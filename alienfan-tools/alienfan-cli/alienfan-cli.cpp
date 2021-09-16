
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <vector>
#include "alienfan-SDK.h"
#include "kdl.h"

using namespace std;

AlienFan_SDK::Control *InitAcpi() {
    AlienFan_SDK::Control *cAcpi = new AlienFan_SDK::Control();

    if (!cAcpi->IsActivated()) {
        // Driver can't start, let's do kernel hack...
        delete cAcpi;
        wchar_t currentPath[MAX_PATH];
        GetModuleFileNameW(NULL, currentPath, MAX_PATH);
        wstring cpath = currentPath;
        cpath.resize(cpath.find_last_of(L"\\"));
        cpath += L"\\HwAcc.sys";

        if (LoadKernelDriver((LPWSTR) cpath.c_str(), (LPWSTR) L"HwAcc")) {
            cAcpi = new AlienFan_SDK::Control();
        } else {
            cAcpi = NULL;
        }
    }

    return cAcpi;
}

void Usage() {
    cout << "Usage: alienfan - cli[command[=value{,value}][command...]]\n\
Avaliable commands: \n\
usage, help\t\t\tShow this usage\n\
rpm\t\t\t\tShow fan(s) RPMs\n\
temp\t\t\t\tShow known temperature sensors values\n\
unlock\t\t\t\tUnclock fan controls\n\
getpower\t\t\tDisplay current power state\n\
power=<value>\t\t\tSet TDP to this level\n\
gpu=<value>\t\t\tSet GPU power limit\n\
getfans\t\t\t\tShow current fan boost level (0..100 - in percent)\n\
setfans=<fan1>[,<fan2>]\t\tSet fans boost level (0..100 - in percent)\n\
setfandirect=<fanid>,<value>\tSet fan with selected ID to given value\n\
direct=<id>,<subid>[,val,val]\tIssue direct interface command (for testing)\n\
directgpu=<id>,<value>\t\tIssue direct GPU interface command (for testing)\n\
\tPower level can be in 0..N - according to power states detected\n\
\tGPU power limit can be in 0..4 - 0 - no limit, 4 - max. limit\n\
\tNumber of fan boost values should be the same as a number fans detected\n\
\tAll values in \"direct\" commands should be hex, not decimal!\n";
}

int main(int argc, char* argv[])
{
    std::cout << "AlienFan-cli v1.2.0\n";

    AlienFan_SDK::Control *acpi = InitAcpi();

    bool supported = false;

    if (acpi && acpi->IsActivated()) {

        if (supported = acpi->Probe()) {
            cout << "Supported hardware v" << acpi->GetVersion() << " detected, " << acpi->HowManyFans() << " fans, "
                << acpi->sensors.size() << " sensors, " << acpi->HowManyPower() << " power states." << endl;
        }
        else {
            cout << "Supported hardware not found!" << endl;
        }

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
                if (command == "rpm" && supported) {
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
                if (command == "temp" && supported) {
                    int res = 0;
                    for (int i = 0; i < acpi->sensors.size(); i++) {
                        if ((res = acpi->GetTempValue(i)) >= 0)
                            cout << acpi->sensors[i].name << ": " << res << endl;
                    }
                    continue;
                }
                if (command == "unlock" && supported) {
                    if (acpi->Unlock() >= 0)
                        cout << "Unlock successful." << endl;
                    else
                        cout << "Unlock failed!" << endl;
                    continue;
                }
                if (command == "power" && supported) {
                    if (args.size() < 1) {
                        cout << "Power: incorrect arguments" << endl;
                        continue;
                    }
                    BYTE unlockStage = atoi(args[0].c_str());
                    if (unlockStage < acpi->HowManyPower()) {
                        if (acpi->SetPower(unlockStage) != acpi->GetErrorCode())
                            cout << "Power set to " << (int) unlockStage << endl;
                        else
                            cout << "Power set failed!" << endl;
                    } else
                        cout << "Power: incorrect value (should be 0.." << acpi->HowManyPower() << ")" << endl;
                    continue;
                }
                if (command == "gpu" && supported) {
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
                if (command == "getpower" && supported) {
                    int cpower = acpi->GetPower();
                    if (cpower >= 0)
                        cout << "Current power mode: " << cpower << endl;
                    else
                        cout << "Getpower failed!" << endl;
                    continue;
                }
                if (command == "getfans" && supported) {
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
                if (command == "setfans" && supported) {
                    if (args.size() < acpi->HowManyFans()) {
                        cout << "Setfans: incorrect arguments (should be " << acpi->HowManyFans() << " of them)" << endl;
                        continue;
                    }
                    int prms = 0;
                    for (int i = 0; i < acpi->HowManyFans(); i++) {
                        BYTE boost = atoi(args[i].c_str());
                        if (boost < 101) {
                            if (acpi->SetFanValue(i, boost) >= 0)
                                cout << "Fan#" << i << " set to " << (int) boost << endl;
                            else {
                                cout << "Set fan level failed!" << endl;
                                break;
                            }
                        } else
                            cout << "Incorrect boost value for Fan#" << i << endl;
                    }
                    continue;
                }
                if (command == "setfandirect") {
                    if (args.size() < 2) {
                        cout << "Setfandirect: incorrect arguments (should be 2)" << endl;
                        continue;
                    }
                    byte fanID = atoi(args[0].c_str()),
                        boost = atoi(args[1].c_str());
                    if (fanID < acpi->HowManyFans()) {
                        if (acpi->SetFanValue(fanID, boost, true) >= 0)
                            cout << "Fan#" << (int)fanID << " set to " << (int) boost << endl;
                        else {
                            cout << "Set fan level failed!" << endl;
                            break;
                        }
                    } else
                        cout << "Setfandirect: incorrect fan id (should be in 0.." << acpi->HowManyFans() << ")" << endl;
                    continue;
                }
                if (command == "direct") {
                    int res = 0;
                    if (args.size() < 2) {
                        cout << "Direct: incorrect arguments (should be 2 or 3)" << endl;
                        continue;
                    }
                    AlienFan_SDK::ALIENFAN_COMMAND comm;
                    comm.com = (USHORT) strtoul(args[0].c_str(), NULL, 16);
                    comm.sub = (byte) strtoul(args[1].c_str(), NULL, 16);
                    byte value1 = 0, value2 = 0;
                    if (args.size() > 2)
                        value1 = (byte) strtol(args[2].c_str(), NULL, 16);
                    if (args.size() > 3)
                        value2 = (byte) strtol(args[3].c_str(), NULL, 16);
                    if ((res = acpi->RunMainCommand(comm, value1, value2)) != acpi->GetErrorCode())
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
                    USHORT command = (USHORT) strtoul(args[0].c_str(), NULL, 16);// atoi(args[0].c_str());
                    DWORD subcommand = strtoul(args[1].c_str(), NULL, 16);// atoi(args[1].c_str());
                    if ((res = acpi->RunGPUCommand(command, subcommand)) != acpi->GetErrorCode())
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
        cout << "System configuration issue - see readme.md for details!" << endl;
    }
    if (acpi)
        delete acpi;

}
