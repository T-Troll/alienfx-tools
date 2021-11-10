#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <vector>
#include "alienfan-SDK.h"
#include "..\alienfan-gui\ConfigHelper.h"

AlienFan_SDK::Control *acpi = NULL;
ConfigHelper *conf = NULL;

int SetFanSteady(short num, byte boost) {
    bool run2 = true;
    acpi->SetFanValue(num, boost, true);
    int runrpm = 0, newrpm = -1;
    do {
        Sleep(7000);
        runrpm = acpi->GetFanRPM(num);
        cout << ".";// << runrpm;
        run2 = abs(runrpm - newrpm) > 30;// runrpm != newrpm;
        newrpm = runrpm;
    } while (run2);
    return newrpm;
}

void CheckFanOverboost(short num) {
    acpi->Unlock();
    int steps = 8;
    int boost = 100 - steps, newrpm = -1;
    int runrpm = 0, rpm = newrpm;
    bool run2 = true;
    bool run = true;
    cout << "Checking fan #" << num << endl;
    do {
        cout << "Preparting step " << steps;
        newrpm = SetFanSteady(num, boost);
        cout << " done, " << newrpm << " RPM" << endl;
        // Check for overrun
        if (!newrpm) boost -= steps;
        do {
            boost += steps;
            newrpm = acpi->GetFanRPM(num);
            acpi->SetFanValue(num, boost, true);
            cout << "Boost " << boost;
            Sleep(5000);
            newrpm = -1;// acpi->GetFanRPM(num);
            run2 = true;
            do {
                Sleep(5000);
                cout << ".";
                runrpm = acpi->GetFanRPM(num);
                run2 = runrpm != newrpm && runrpm > rpm - 100;
                newrpm = runrpm;
            } while (run2);
            if (newrpm > rpm)
                rpm = newrpm;
            else
                run = false;
            cout << " " << newrpm << " RPM" << endl;
        } while (run);
        boost -= steps; run = true;
        //newrpm = acpi->GetFanRPM(num);
        //acpi->SetFanValue(num, boost, true);
        steps = steps >> 1;
    } while (steps > 0);
    cout << "Final boost - " << boost << ", " << rpm << " RPM" << endl << endl;
    acpi->SetFanValue(num, 0);
    if (num >= conf->boosts.size())
        conf->boosts.resize(num + 1);
    else
        if (conf->boosts[num].maxRPM >= rpm)
            return;
    conf->boosts[num] = {(byte)boost,(USHORT)rpm};

}

int main(int argc, char* argv[])
{
    cout << "AlienFan-Overboost v1.5.0.0" << endl;
    cout << "Usage: AlienFan-Overboost [fan ID [Manual boost]]" << endl;

    conf = new ConfigHelper();

    acpi = new AlienFan_SDK::Control();

    if (acpi->IsActivated()) {
        if (acpi->Probe()) {
            conf->SetBoosts(acpi);
            if (argc > 1) {
                // one fan
                int fanID = atoi(argv[1]);
                if (fanID < acpi->HowManyFans())
                    if (argc > 2) {
                        // manual fan set
                        int newBoost = atoi(argv[2]);
                        acpi->Unlock();
                        cout << "Boost for fan " << fanID << " will be set to " << newBoost;
                        int newrpm = SetFanSteady(fanID, newBoost);
                        if (fanID >= conf->boosts.size())
                            conf->boosts.resize(fanID + 1);
                        acpi->SetFanValue(fanID, 0);
                        conf->boosts[fanID] = {(byte)newBoost,(USHORT)newrpm};
                        cout << " done, " << newrpm << " RPM" << endl;
                    } else
                        CheckFanOverboost(fanID);
                else
                    cout << "Incorrect fan ID!" << endl;
            } else {
                // all fans
                for (int i = 0; i < acpi->HowManyFans(); i++)
                    CheckFanOverboost(i);
            }
            cout << "Done!" << endl;
        } else
            cout << "Supported device not found!" << endl;
    } else
        cout << "Error: can't load driver!" << endl;

    delete acpi;
    delete conf;

    return 0;
}

