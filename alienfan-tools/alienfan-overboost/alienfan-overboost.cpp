#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <vector>
#include "alienfan-SDK.h"
#include "ConfigFan.h"

AlienFan_SDK::Control *acpi = NULL;
ConfigFan *conf = NULL;

int SetFanSteady(short num, byte boost, bool downtrend = false) {
    bool run2 = true;
    acpi->SetFanValue(num, boost, true);
    // Check the trend...
    printf("Stabilize Fan#%d at boost %d:\n", num, boost);
    int oldMids = 0, midRPMs = 0;
    int maxRPM = 0, minRPM = 10000;
    do {
        oldMids = midRPMs;

        maxRPM = 0, minRPM = 10000;
        for (int i = 0; i < 9; i++) {
            Sleep(1000);
            int cRPM = acpi->GetFanRPM(num);
            if (cRPM < minRPM) minRPM = cRPM;
            if (cRPM > maxRPM) maxRPM = cRPM;
        }
        midRPMs = minRPM + (maxRPM - minRPM) / 2;
        printf("\r%d RPM (%d-%d)", midRPMs, minRPM, maxRPM);
    } while (minRPM != maxRPM && abs(oldMids - midRPMs) > 50 && (!downtrend || oldMids - midRPMs < 100));
    midRPMs = downtrend && oldMids - midRPMs >= 100 ? -1 : maxRPM;
    printf(" done, %d RPM\n", (midRPMs < 0 ? minRPM : midRPMs));
    return midRPMs;
}

void UpdateBoost(byte id, byte boost, USHORT rpm) {
    bool noBoost = true;
    for (auto iter = conf->boosts.begin(); iter != conf->boosts.end(); iter++)
        if (iter->fanID == id) {
            iter->maxBoost = boost;
            iter->maxRPM = rpm;
            noBoost = false;
            break;
        }
    if (noBoost)
        conf->boosts.push_back({ id, boost, rpm });
}

void CheckFanOverboost(byte num) {
    acpi->Unlock();
    int steps = 8;
    int boost = 100, badBoost = 0;
    int runrpm = 0, rpm = 0;
    bool run = true;
    printf("Checking fan #%d\n", num);
    rpm = SetFanSteady(num, boost);
    do {
        //printf("Starting step " << steps << endl;
        // Check for overrun
        do {
            boost += steps;
            if (boost != badBoost) {
                runrpm = SetFanSteady(num, boost, true);
                run = runrpm > rpm;
                if (runrpm > rpm) rpm = runrpm;
            } else
                run = false;
        } while (run);
        badBoost = boost;
        boost -= steps; run = true;
        steps = steps >> 1;
    } while (steps > 0);
    printf("Final boost - %d (at %d RPM)\n\n", boost, rpm);
    acpi->SetFanValue(num, boost, true);
    bool noBoost = true;
    UpdateBoost(num, boost, rpm);
}

int main(int argc, char* argv[])
{
    printf("AlienFan-Overboost v5.8.1\n");
    printf("Usage: AlienFan-Overboost [fan ID [Manual boost]]\n");

    conf = new ConfigFan();

    acpi = new AlienFan_SDK::Control();

    if (acpi->IsActivated()) {
        if (acpi->Probe()) {
            conf->SetBoosts(acpi);
            if (argc > 1) {
                // one fan
                byte fanID = atoi(argv[1]);
                if (fanID < acpi->HowManyFans())
                    if (argc > 2) {
                        // manual fan set
                        byte newBoost = atoi(argv[2]);
                        acpi->Unlock();
                        printf("Boost for fan #%d will be set to %d.\n", fanID, newBoost);
                        int newrpm = SetFanSteady(fanID, newBoost);
                        UpdateBoost(fanID, newBoost, newrpm);
                        printf("Done, %d RPM\n", newrpm);
                    } else
                        CheckFanOverboost(fanID);
                else
                    printf("Incorrect fan ID!\n");
            } else {
                // all fans
                for (int i = 0; i < acpi->HowManyFans(); i++)
                    CheckFanOverboost(i);
            }
            printf("Done!\n");
        } else
            printf("Error: Supported device not found!\n");
    } else
        printf("Error: can't load driver!\n");

    delete acpi;
    delete conf;

    return 0;
}

