#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	afx_dev = new AlienFX_SDK::Functions();
	devList = afx_dev->AlienFXEnumDevices(afx_dev->vid);
	afx_dev->LoadMappings();
	pid = 0;
	if (conf->lastActive != 0)
		for (int i = 0; i < devList.size(); i++)
			if (devList[i] == conf->lastActive) {
				pid = conf->lastActive;
				break;
			}
	if (pid == 0)
		pid = afx_dev->AlienFXInitialize(afx_dev->vid);
	else
		pid = afx_dev->AlienFXInitialize(afx_dev->vid, pid);

	if (pid != -1)
	{
		int count;
		for (count = 0; count < 5 && !afx_dev->IsDeviceReady(); count++)
			Sleep(20);
		if (count == 5)
			afx_dev->Reset(0);
		conf->lastActive = pid;
	}
};
FXHelper::~FXHelper() {
	if (pid != -1) {
		afx_dev->SaveMappings();
		afx_dev->AlienFXClose();
	}
	delete afx_dev;
};

std::vector<int> FXHelper::GetDevList() {
	devList = afx_dev->AlienFXEnumDevices(afx_dev->vid);
	return devList;
}

void FXHelper::TestLight(int id)
{
	bool dev_ready = afx_dev->IsDeviceReady();
	int c_count = 0;
	while (!dev_ready) {
		c_count++;
		if (c_count > 5) return;
		Sleep(20);
		dev_ready = afx_dev->IsDeviceReady();
	}
	int r = (config->testColor.cs.red * config->testColor.cs.red) >> 8,
		g = (config->testColor.cs.green * config->testColor.cs.green) >> 8,
		b = (config->testColor.cs.blue * config->testColor.cs.blue) >> 8;
	if (id != lastTest) {
		if (lastTest >= 0)
			afx_dev->SetColor(lastTest, 0, 0, 0);
		lastTest = id;
	}
	afx_dev->SetColor(id, r, g, b);
	afx_dev->UpdateColors();
}

void FXHelper::ResetPower()
{
	afx_dev->SetPowerAction(63, 0, 0, 0, 0, 0, 0, true);
}

void FXHelper::SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force)
{
	std::vector <lightset>::iterator Iter;
#ifdef _DEBUG
	//char buff[2048];
	//sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
	//sprintf_s(buff, 2047, "CounterUpdate: S%d,", afx_dev->AlienfxGetDeviceStatus());
	//OutputDebugString(buff);
#endif

	if (config->autoRefresh) Refresh();
	bool dev_ready = afx_dev->IsDeviceReady();
	int c_count = 0;
	/*if (!dev_ready) {
#ifdef _DEBUG
		OutputDebugString(TEXT("SetCounter: device busy!\n"));
#endif
		return;
	}*/
	while (!dev_ready) {
		if (!force) return;
		c_count++;
		if (c_count > 20) {
#ifdef _DEBUG
			OutputDebugString(TEXT("SetCounter device busy!\n"));
#endif
			return;
		}
		Sleep(20);
		dev_ready = afx_dev->IsDeviceReady();
	}

	bStage = !bStage;
	int lFlags = 0;
	bool wasChanged = false;
	bool tHDD = force || (lHDD && !cHDD) || (!lHDD && cHDD),
		 tNet = force || (lNET && !cNet) || (!lNET && cNet);
	for (Iter = config->active_set.begin(); Iter != config->active_set.end(); Iter++)
		if (Iter->devid == pid 
		&& (Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)
			&& (lFlags = afx_dev->GetFlags(pid, Iter->lightid)) != (-1)) {
			int mIndex = lFlags && Iter->eve[0].map.size() > 1 && activeMode != MODE_AC && activeMode != MODE_CHARGE ? 1 : 0;
			AlienFX_SDK::afx_act fin = Iter->eve[0].fs.b.flags ? Iter->eve[0].map[mIndex] : Iter->eve[2].fs.b.flags ?
				Iter->eve[2].map[0] : Iter->eve[3].map[0];
			fin.type = 0;
			if (Iter->eve[2].fs.b.flags) {
				// counter
				double coeff = 0.0, ccut = Iter->eve[2].fs.b.cut;
				switch (Iter->eve[2].source) {
				case 0: if (!force && (lCPU == cCPU || lCPU < ccut && cCPU < ccut)) continue; coeff = cCPU; break;
				case 1: if (!force && (lRAM == cRAM || lRAM < ccut && cRAM < ccut)) continue; coeff = cRAM; break;
				case 2: if (!force && (lHDD == cHDD || lHDD < ccut && cHDD < ccut)) continue; coeff = cHDD; break;
				case 3: if (!force && (lGPU == cGPU || lGPU < ccut && cGPU < ccut)) continue; coeff = cGPU; break;
				case 4: if (!force && (lNET == cNet || lNET < ccut && cNet < ccut)) continue; coeff = cNet; break;
				case 5: if (!force && (lTemp == cTemp || lTemp < ccut && cTemp < ccut)) continue; coeff = (cTemp > 273) ? cTemp - 273.0 : 0; break;
				case 6: if (!force && (lBatt == cBatt || lBatt < ccut && cBatt < ccut)) continue; coeff = cBatt; break;
				}
				coeff = coeff > ccut ? (coeff - ccut) / (100.0 - ccut) : 0.0;
				fin.r = (BYTE) (fin.r * (1 - coeff) + Iter->eve[2].map[1].r * coeff);
				fin.g = (BYTE) (fin.g * (1 - coeff) + Iter->eve[2].map[1].g * coeff);
				fin.b = (BYTE) (fin.b * (1 - coeff) + Iter->eve[2].map[1].b * coeff);
			}
			if (Iter->eve[3].fs.b.flags) {
				// indicator
				long indi = 0, ccut = Iter->eve[3].fs.b.cut;
				bool blink = Iter->eve[3].fs.b.proc;
				switch (Iter->eve[3].source) {
				case 0: if (!tHDD && !blink) continue;
					indi = cHDD; break;
				case 1: if (!tNet && !blink) continue;
					indi = cNet; break;
				case 2: if (!force && !blink &&
					((lTemp <= 273 + ccut && cTemp <= 273 + ccut) ||
						(cTemp > 273 + ccut && lTemp > 273 + ccut))) continue; indi = cTemp - 273 - ccut; break;
				case 3: if (!force && !blink &&
					((lRAM <= ccut && cRAM <= ccut) || (lRAM > ccut && cRAM > ccut))) continue; indi = cRAM - ccut; break;
				}
				fin = indi > 0 ? 
						blink ?
							bStage ? Iter->eve[3].map[1] : fin 
							: Iter->eve[3].map[1]
						: fin;
			}
			wasChanged = true;
			std::vector<AlienFX_SDK::afx_act> actions;
			if (lFlags)
				if (activeMode != MODE_AC && activeMode != MODE_CHARGE) {
					actions.push_back(Iter->eve[0].map[0]);
					actions.push_back(fin);
				}
				else {
					actions.push_back(fin);
					actions.push_back(Iter->eve[0].map.size() > 1 ? Iter->eve[0].map[1] : Iter->eve[0].map[0]);
				}
			else
				actions.push_back(fin);
			SetLight(Iter->lightid, lFlags, actions);// , force);
		}
	if (wasChanged) {
		afx_dev->UpdateColors();
		lCPU = cCPU; lRAM = cRAM; lGPU = cGPU; lHDD = cHDD; lNET = cNet; lTemp = cTemp; lBatt = cBatt;
	}
}

void FXHelper::SetLight(int id, bool power, std::vector<AlienFX_SDK::afx_act> actions, bool force)
{
	// modify colors for dimmed...
	const unsigned delta = 256 - config->dimmingPower;

	for (int i = 0; i < actions.size(); i++) {
		if (config->dimmed || config->stateDimmed ||
			(config->dimmedBatt && (activeMode & (MODE_BAT | MODE_LOW)))) {
			actions[i].r = (actions[i].r * delta) >> 8;
			actions[i].g = (actions[i].g * delta) >> 8;
			actions[i].b = (actions[i].b * delta) >> 8;
		}
		// gamma-correction...
		if (config->gammaCorrection) {
			actions[i].r = (actions[i].r * actions[i].r) >> 8;
			actions[i].g = (actions[i].g * actions[i].g) >> 8;
			actions[i].b = (actions[i].b * actions[i].b) >> 8;
		}
	}
	if (afx_dev->IsDeviceReady()) {
		if (power && actions.size() > 1) {
			if (!config->block_power) {
#ifdef _DEBUG
				char buff[2048];
				//sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
				sprintf_s(buff, 2047, "Set power button to: %d,%d,%d\n", actions[0].r, actions[0].g, actions[0].b);
				OutputDebugString(buff);
#endif
				if (config->lightsOn && config->stateOn || !config->offPowerButton)
					afx_dev->SetPowerAction(id, actions[0].r, actions[0].g, actions[0].b,
						actions[1].r, actions[1].g, actions[1].b, force);
				else
					afx_dev->SetPowerAction(id, 0, 0, 0, 0, 0, 0, force);
			}
		}
		else
			if (config->lightsOn && config->stateOn) {
				if (actions[0].type == 0)
					afx_dev->SetColor(id, actions[0].r, actions[0].g, actions[0].b);
				else {
					afx_dev->SetAction(id, actions);
				}
			}
			else {
				afx_dev->SetColor(id, 0, 0, 0);
			}
	}
	else {
#ifdef _DEBUG
		OutputDebugString(TEXT("SetLight: device busy!\n"));
#endif
	}
}

void FXHelper::RefreshState(bool force)
{
	Refresh(force);
	RefreshMon();
}

void FXHelper::RefreshMon()
{
	if (config->enableMon)
		SetCounterColor(lCPU, lRAM, lGPU, lNET, lHDD, lTemp, lBatt, true);
}

int FXHelper::Refresh(bool forced)
{
	std::vector <lightset>::iterator Iter;
	Colorcode fin;

	bool dev_ready = afx_dev->IsDeviceReady();
	int c_count = 0;
	while (!dev_ready) {
		if (!forced) {
#ifdef _DEBUG
			OutputDebugString("Refresh failed.\n");
#endif
			return 1;
		}
		c_count++;
		if (c_count > 20) {
#ifdef _DEBUG
			OutputDebugString("Forced refresh failed.\n");
#endif
			return 1;
		}
		Sleep(20);
		dev_ready = afx_dev->IsDeviceReady();
	}
	int lFlags = 0;
	std::vector<AlienFX_SDK::afx_act> actions; AlienFX_SDK::afx_act action;
	for (Iter = config->active_set.begin(); Iter != config->active_set.end(); Iter++) {
		if (Iter->devid == pid) {
			actions = Iter->eve[0].map;
			lFlags = afx_dev->GetFlags(pid, Iter->lightid);
			if (config->monState && !forced) {
				/*if (!Iter->eve[0].fs.b.flags) {
					c1.ci = 0; c2.ci = 0;
				}*/
				if (Iter->eve[1].fs.b.flags) {
					// use power event;
					if (!Iter->eve[0].fs.b.flags)
						actions = Iter->eve[1].map;
					else
						if (actions.size() < 2)
							actions.push_back(Iter->eve[1].map[1]);
					switch (activeMode) {
					case MODE_BAT:
						action = actions[0]; actions[0] = actions[1]; actions[1] = action;
						actions[0].type = 0;
						break;
					case MODE_LOW:
						action = actions[0]; actions[0] = actions[1]; actions[1] = action;
						actions[0].type = actions[1].type = 1;
						break;
					case MODE_CHARGE: 
						actions[0].type = actions[1].type = 2; 
						break;
					}
				}
				if ((Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)
					&& config->lightsOn && config->stateOn) continue;
			}
			SetLight(Iter->lightid, lFlags, actions, forced);
		}
	}
	afx_dev->UpdateColors();
	return 0;
}

int FXHelper::SetMode(int mode)
{
	int t = activeMode;
	activeMode = mode;
	return t == activeMode;
}


