#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <vector>
#include "alienfan-SDK.h"
#include "..\alienfan-gui\ConfigHelper.h"

AlienFan_SDK::Control *acpi = NULL;
ConfigHelper *conf = NULL;

int SetFanSteady(short num, byte boost, bool downtrend = false) {
    bool run2 = true;
    acpi->SetFanValue(num, boost, true);
    // Check the trend...
    cout << "Stabilize Fan#" << num << " at boost " << (int) boost << ":" << endl;
    int oldMids = 0, midRPMs = 0;
    int maxRPM = 0, minRPM = 10000;
    do {
        oldMids = midRPMs;
        //midRPMs = 0;
        maxRPM = 0, minRPM = 10000;
        for (int i = 0; i < 9; i++) {
            Sleep(1000);
            int cRPM = acpi->GetFanRPM(num);
            if (cRPM < minRPM) minRPM = cRPM;
            if (cRPM > maxRPM) maxRPM = cRPM;
            //midRPMs += cRPM;
        }
        midRPMs = minRPM + (maxRPM - minRPM) / 2;
        cout << "\r" << midRPMs << " RPM (" << minRPM << "-" << maxRPM << ")";
    } while (minRPM != maxRPM && abs(oldMids - midRPMs) > 50 && (!downtrend || oldMids - midRPMs < 100));
    midRPMs = downtrend && oldMids - midRPMs >= 100 ? -1 : maxRPM;
    cout << " done, " << (midRPMs < 0 ? minRPM : midRPMs) << " RPM" << endl;
    return midRPMs;
}

void CheckFanOverboost(short num) {
    acpi->Unlock();
    int steps = 8;
    int boost = 100, badBoost = 0;
    int runrpm = 0, rpm = 0;
    bool run = true;
    cout << "Checking fan #" << num << endl;
    rpm = SetFanSteady(num, boost);
    do {
        //cout << "Starting step " << steps << endl;
        // Check for overrun
        do {
            boost += steps;
            if (boost != badBoost) {
                runrpm = SetFanSteady(num, boost, true);// acpi->GetFanRPM(num);
                run = runrpm > rpm;
                if (runrpm > rpm) rpm = runrpm;
            } else
                run = false;
        } while (run);
        badBoost = boost;
        boost -= steps; run = true;
        steps = steps >> 1;
        /*if (steps > 0)
            int trpm = SetFanSteady(num, boost);*/
    } while (steps > 0);
    cout << "Final boost - " << boost << ", " << rpm << " RPM" << endl << endl;
    acpi->SetFanValue(num, boost, true);
    if (num >= conf->boosts.size())
        conf->boosts.resize(num + 1);
    else
        if (conf->boosts[num].maxRPM >= rpm)
            return;
    conf->boosts[num] = {(byte)boost,(USHORT)rpm};

}

int main(int argc, char* argv[])
{
    cout << "AlienFan-Overboost v1.5.1.0" << endl;
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

