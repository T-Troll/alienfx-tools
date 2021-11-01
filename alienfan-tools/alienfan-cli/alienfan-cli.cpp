
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <vector>
#include "alienfan-SDK.h"
#include "alienfan-low.h"

using namespace std;

void Usage() {
    cout << "Usage: alienfan-cli [command[=value{,value}] [command...]]\n\
Avaliable commands: \n\
usage, help\t\t\tShow this usage\n\
rpm\t\t\t\tShow fan(s) RPMs\n\
persent\t\t\tShow fan(s) RPM in perecent of maximum\n\
temp\t\t\t\tShow known temperature sensors values\n\
unlock\t\t\t\tUnclock fan controls\n\
getpower\t\t\tDisplay current power state\n\
power=<value>\t\t\tSet TDP to this level\n\
gpu=<value>\t\t\tSet GPU power limit\n\
getfans\t\t\t\tShow current fan boost level (0..100 - in percent)\n\
setfans=<fan1>[,<fan2>]\t\tSet fans boost level (0..100 - in percent)\n\
setfandirect=<fanid>,<value>\tSet fan with selected ID to given value\n\
resetcolor\t\t\tReset color system\n\
setcolor=<mask>,r,g,b\t\tSet light(s) defined by mask to color\n\
setcolormode=<dim>,<flag>\tSet light system brightness and mode\n\
direct=<id>,<subid>[,val,val]\tIssue direct interface command (for testing)\n\
directgpu=<id>,<value>\t\tIssue direct GPU interface command (for testing)\n\
\tPower level can be in 0..N - according to power states detected\n\
\tGPU power limit can be in 0..4 - 0 - no limit, 4 - max. limit\n\
\tNumber of fan boost values should be the same as a number fans detected\n\
\tBrighness for ACPI lights can only have 10 values - 1,3,4,6,7,9,10,12,13,15\n\
\tAll values in \"direct\" commands should be hex, not decimal!\n";
}

int main(int argc, char* argv[])
{
    std::cout << "AlienFan-cli v1.4.0.0\n";

    AlienFan_SDK::Control *acpi = new AlienFan_SDK::Control();

    bool supported = false;

    if (acpi->IsActivated()) {

        AlienFan_SDK::Lights *lights = new AlienFan_SDK::Lights(acpi);

        if (supported = acpi->Probe()) {
            cout << "Supported hardware v" << acpi->GetVersion() << " detected, " << acpi->HowManyFans() << " fans, "
                << acpi->sensors.size() << " sensors, " << acpi->HowManyPower() << " power states."
                << " Light control: ";
            if (lights->IsActivated())
                cout << "enabled";
            else
                cout << "disabled";
            cout << endl;
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
                if (command == "percent" && supported) {
                    int prms = 0;
                    for (int i = 0; i < acpi->HowManyFans(); i++)
                        if ((prms = acpi->GetFanPercent(i)) >= 0)
                            cout << "Fan#" << i << ": " << prms << endl;
                        else {
                            cout << "RPM percent reading failed!" << endl;
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

                if (command == "resetcolor" && lights->IsActivated()) { // Reset color system for Aurora
                    if (lights->Reset())
                        cout << "Lights reset complete" << endl;
                    else
                        cout << "Lights reset failed" << endl;
                    continue;
                }
                if (command == "setcolor" && lights->IsActivated()) { // Set light color for Aurora
                    if (args.size() < 4) {
                        cout << "SetColor: incorrect arguments (should be mask,r,g,b)" << endl;
                        continue;
                    }
                    byte mask = atoi(args[0].c_str()),
                        r = atoi(args[1].c_str()),
                        g = atoi(args[2].c_str()),
                        b = atoi(args[3].c_str());
                    if (lights->SetColor(mask, r, g, b))
                        cout << "SetColor complete." << endl;
                    else
                        cout << "SetColor failed." << endl;
                    lights->Update();
                    continue;
                }
                if (command == "setcolormode" && lights->IsActivated()) { // set effect (?) for Aurora
                    if (args.size() < 2) {
                        cout << "SetColorMode: incorrect arguments (should be mode,(0..1))" << endl;
                        continue;
                    }
                    byte num = atoi(args[0].c_str()),
                        mode = atoi(args[1].c_str());
                    if (lights->SetMode(num, mode)) {
                        cout << "SetColorMode complete." << endl;
                    } else
                        cout << "SetColorMode failed." << endl;
                    lights->Update();
                    continue;
                }
                if (command == "test") { // pseudo block for test modules
                    continue;
                }
                cout << "Unknown command - " << command << ", use \"usage\" or \"help\" for information" << endl;
            }
        }

        delete lights;

    } else {
        cout << "System configuration issue - see readme.md for details!" << endl;
    }

    delete acpi;

}
