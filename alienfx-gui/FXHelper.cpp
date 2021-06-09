#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	afx_dev.LoadMappings();
	FillDevs();
};
FXHelper::~FXHelper() {
	afx_dev.SaveMappings();
	if (devList.size() > 0) {
		for (int i = 0; i < devs.size(); i++)
			devs[i]->AlienFXClose();
		devs.clear();
	}
}

AlienFX_SDK::Functions* FXHelper::LocateDev(int pid)
{
	for (int i = 0; i < devs.size(); i++)
		if (devs[i]->GetPID() == pid)
			return devs[i];
	return nullptr;
}
void FXHelper::FillDevs()
{
	devList = GetDevList();
	if (devs.size() > 0) {
		for (int i = 0; i < devs.size(); i++)
			devs[i]->AlienFXClose();
		devs.clear();
	}
	for (int i = 0; i < devList.size(); i++) {
		AlienFX_SDK::Functions* dev = new AlienFX_SDK::Functions();
		int pid = dev->AlienFXInitialize(dev->vid, devList[i]);
		if (pid != -1)
			devs.push_back(dev);
	}
}

std::vector<int> FXHelper::GetDevList() {
	std::vector<int> devList = afx_dev.AlienFXEnumDevices(afx_dev.vid);
	return devList;
}

void FXHelper::TestLight(int did, int id)
{
	AlienFX_SDK::Functions* dev = LocateDev(did);
	if (dev != NULL) {
		bool dev_ready = false;// afx_dev->IsDeviceReady();
		for (int c_count = 0; c_count < 15 && !(dev_ready = dev->IsDeviceReady()); c_count++)
			Sleep(20);
		if (!dev_ready) return;

		int r = (config->testColor.cs.red * config->testColor.cs.red) >> 8,
			g = (config->testColor.cs.green * config->testColor.cs.green) >> 8,
			b = (config->testColor.cs.blue * config->testColor.cs.blue) >> 8;
		if (id != lastTest) {
			if (lastTest >= 0)
				dev->SetColor(lastTest, 0, 0, 0);
			lastTest = id;
		}
		dev->SetColor(id, r, g, b);
		dev->UpdateColors();
	}
}

void FXHelper::ResetPower(int did)
{
	AlienFX_SDK::Functions* dev = LocateDev(did);
	if (dev != NULL)
		dev->SetPowerAction(63, 0, 0, 0, 0, 0, 0, true);
}

void FXHelper::SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force)
{
	std::vector <lightset>::iterator Iter;
//#ifdef _DEBUG
	//char buff[2048];
	//sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
	//sprintf_s(buff, 2047, "CounterUpdate: S%d,", afx_dev->AlienfxGetDeviceStatus());
	//OutputDebugString(buff);
//#endif

	if (config->autoRefresh) Refresh();

	bStage = !bStage;
	int lFlags = 0;
	bool wasChanged = false;
	int whatChanged = -1;
	bool tHDD = force || (lHDD && !cHDD) || (!lHDD && cHDD),
		 tNet = force || (lNET && !cNet) || (!lNET && cNet);
	for (Iter = config->active_set.begin(); Iter != config->active_set.end(); Iter++)
		if ((Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)
			&& (lFlags = afx_dev.GetFlags(Iter->devid, Iter->lightid)) != (-1)) {
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
				case 5: if (!force && (lTemp == cTemp || lTemp < ccut && cTemp < ccut)) continue; coeff = cTemp; break;
				case 6: if (!force && (lBatt == cBatt || lBatt < ccut && cBatt < ccut)) continue; coeff = cBatt; break;
				}
				coeff = coeff > ccut ? (coeff - ccut) / (100.0 - ccut) : 0.0;
				fin.r = (BYTE) (fin.r * (1 - coeff) + Iter->eve[2].map[1].r * coeff);
				fin.g = (BYTE) (fin.g * (1 - coeff) + Iter->eve[2].map[1].g * coeff);
				fin.b = (BYTE) (fin.b * (1 - coeff) + Iter->eve[2].map[1].b * coeff);
				whatChanged = 2;
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
					((lTemp <= ccut && cTemp <= ccut) ||
						(cTemp > ccut && lTemp > ccut))) continue; 
					indi = cTemp - ccut; break;
				case 3: if (!force && !blink &&
					((lRAM <= ccut && cRAM <= ccut) || (lRAM > ccut && cRAM > ccut))) continue; 
					indi = cRAM - ccut; break;
				}
				fin = indi > 0 ? 
						blink ?
							bStage ? Iter->eve[3].map[1] : fin 
							: Iter->eve[3].map[1]
						: fin;
				whatChanged = 3;
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
			if (SetLight(Iter->devid, Iter->lightid, lFlags, actions)) {// , force);
				switch (whatChanged) {
					case 2:
						switch (Iter->eve[2].source) {
						case 0: lCPU = cCPU; break;
						case 1: lRAM = cRAM; break;
						case 2: lHDD = cHDD; break;
						case 3: lGPU = cGPU; break;
						case 4: lNET = cNet; break;
						case 5: lTemp = cTemp; break;
						case 6: lBatt = cBatt; break;
						}
						break;
					case 3:
						switch (Iter->eve[3].source) {
						case 0: lHDD = cHDD; break;
						case 1: lNET = cNet; break;
						case 2: lTemp = cTemp; break;
						case 3: lRAM = cRAM; break;
						}
						break;
				}
			}
		}
	if (wasChanged) {
		UpdateColors();
		//lCPU = cCPU; lRAM = cRAM; lGPU = cGPU; lHDD = cHDD; lNET = cNet; lTemp = cTemp; lBatt = cBatt;
	}
}

void FXHelper::UpdateColors()
{
	for (int i = 0; i < devs.size(); i++)
		devs[i]->UpdateColors();
}

bool FXHelper::SetLight(int did, int id, bool power, std::vector<AlienFX_SDK::afx_act> actions, bool force)
{
	// modify colors for dimmed...
	const unsigned delta = 256 - config->dimmingPower;
	AlienFX_SDK::Functions* dev = LocateDev(did);

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
	if (dev != NULL) {
		bool devReady = false;
		for (int wcount = 0; wcount < 5 && !(devReady = dev->IsDeviceReady()) && force; wcount++)
			Sleep(20);
		if (!devReady) {
#ifdef _DEBUG
			if (force)
				OutputDebugString("Forced light update skipped!\n");
#endif
			return false;
		}
		if (power && actions.size() > 1) {
			if (!config->block_power) {
#ifdef _DEBUG
				char buff[2048];
				//sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
				sprintf_s(buff, 2047, "Set power button to: %d,%d,%d\n", actions[0].r, actions[0].g, actions[0].b);
				OutputDebugString(buff);
#endif
				if (config->lightsOn && config->stateOn || !config->offPowerButton)
					dev->SetPowerAction(id, actions[0].r, actions[0].g, actions[0].b,
						actions[1].r, actions[1].g, actions[1].b, force);
				else
					dev->SetPowerAction(id, 0, 0, 0, 0, 0, 0, force);
			}
		}
		else
			if (config->lightsOn && config->stateOn) {
				if (actions[0].type == 0)
					dev->SetColor(id, actions[0].r, actions[0].g, actions[0].b);
				else {
					dev->SetAction(id, actions);
				}
			}
			else {
				dev->SetColor(id, 0, 0, 0);
			}
	}
	else
		return false;
	return true;
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

	/*bool dev_ready = false;
	for (int c_count = 0; c_count < 20 && !(dev_ready = afx_dev.IsDeviceReady()) && forced; c_count++) {
		Sleep(20);
	}
	if (!dev_ready) {
		#ifdef _DEBUG
				OutputDebugString(TEXT("Refresh: device busy!\n"));
		#endif
		return 1;
	}*/
	int lFlags = 0;
	std::vector<AlienFX_SDK::afx_act> actions; AlienFX_SDK::afx_act action;
	for (Iter = config->active_set.begin(); Iter != config->active_set.end(); Iter++) {
			actions = Iter->eve[0].map;
			lFlags = afx_dev.GetFlags(Iter->devid, Iter->lightid);
			if (config->monState && !forced) {
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
			SetLight(Iter->devid, Iter->lightid, lFlags, actions, forced);
	}
	UpdateColors();
	return 0;
}

int FXHelper::SetMode(int mode)
{
	int t = activeMode;
	activeMode = mode;
	return t == activeMode;
}


