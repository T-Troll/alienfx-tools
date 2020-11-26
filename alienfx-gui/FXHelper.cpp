#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	devList = AlienFX_SDK::Functions::AlienFXEnumDevices(AlienFX_SDK::Functions::vid);
	AlienFX_SDK::Functions::LoadMappings();
	pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
	if (pid != -1)
	{
		int count;
		for (count = 0; count < 5 && !AlienFX_SDK::Functions::IsDeviceReady(); count++)
			Sleep(20);
		if (count == 5)
			AlienFX_SDK::Functions::Reset(0);
	}
};
FXHelper::~FXHelper() {
	if (pid != -1) {
		AlienFX_SDK::Functions::SaveMappings();
		AlienFX_SDK::Functions::AlienFXClose();
	}
};

std::vector<int> FXHelper::GetDevList() {
	return devList;
}

void FXHelper::TestLight(int id)
{
	for (int i = 0; i < 25 && !AlienFX_SDK::Functions::IsDeviceReady(); i++) Sleep(20);
	if (!AlienFX_SDK::Functions::IsDeviceReady()) return;
	int r = (config->testColor.cs.red * config->testColor.cs.red) >> 8,
		g = (config->testColor.cs.green * config->testColor.cs.green) >> 8,
		b = (config->testColor.cs.blue * config->testColor.cs.blue) >> 8;
	AlienFX_SDK::Functions::SetColor(id, r, g, b);
		// config->testColor.cs.red, config->testColor.cs.green, config->testColor.cs.blue);
	AlienFX_SDK::Functions::UpdateColors();
}

void FXHelper::SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force)
{
	std::vector <lightset>::iterator Iter;
#ifdef _DEBUG
	char buff[2048];
	sprintf_s(buff, 2047, "CPU: %d, RAM: %d, HDD: %d, NET: %d, GPU: %d, Temp: %d, Batt:%d\n", cCPU, cRAM, cHDD, cNet, cGPU, cTemp, cBatt);
	//sprintf_s(buff, 2047, "CounterUpdate: S%d,", AlienFX_SDK::Functions::AlienfxGetDeviceStatus());
	OutputDebugString(buff);
#endif
	if (config->autoRefresh) Refresh();
	if (force)
		for (int i = 0; i < 15 && !AlienFX_SDK::Functions::IsDeviceReady(); i++) Sleep(20);
	if (!AlienFX_SDK::Functions::IsDeviceReady()) return;
	bStage = !bStage;
	int lFlags = 0;
	bool wasChanged = false;
	bool tHDD = force || (lHDD && !cHDD) || (!lHDD && cHDD),
		 tNet = force || (lNET && !cNet) || (!lNET && cNet);
	for (Iter = config->mappings.begin(); Iter != config->mappings.end(); Iter++)
		if (Iter->devid == pid 
		&& (Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)
			&& (lFlags = AlienFX_SDK::Functions::GetFlags(pid, Iter->lightid)) != (-1)) {
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
				fin.r = fin.r * (1 - coeff) + Iter->eve[2].map[1].r * coeff;
				fin.g = fin.g * (1 - coeff) + Iter->eve[2].map[1].g * coeff;
				fin.b = fin.b * (1 - coeff) + Iter->eve[2].map[1].b * coeff;
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
			SetLight(Iter->lightid, lFlags, actions, force);
		}
	if (wasChanged)
		AlienFX_SDK::Functions::UpdateColors();
	lCPU = cCPU; lRAM = cRAM; lGPU = cGPU; lHDD = cHDD; lNET = cNet; lTemp = cTemp; lBatt = cBatt;
}

void FXHelper::SetLight(int id, bool power, std::vector<AlienFX_SDK::afx_act> actions, bool force)// mode1, BYTE length1, BYTE speed1, BYTE r, BYTE g, BYTE b,
	//BYTE mode2, BYTE length2, BYTE speed2, BYTE r2, BYTE g2, BYTE b2, bool force)
{
	// modify colors for dimmed...
	const unsigned delta = 256 - config->dimmingPower;

	if (config->lightsOn && config->stateOn) {
		for (int i = 0; i < actions.size(); i++) {
			if (config->dimmed || config->stateDimmed ||
				(config->dimmedBatt && (activeMode & (MODE_BAT | MODE_LOW)))) {
				actions[i].r = (actions[i].r * delta) >> 8;
				actions[i].g = (actions[i].g * delta) >> 8;
				actions[i].b = (actions[i].b * delta) >> 8;
				/*r2 = (r2 * delta) >> 8;
				g2 = (g2 * delta) >> 8;
				b2 = (b2 * delta) >> 8;*/
			}
			// gamma-correction...
			if (config->gammaCorrection) {
				actions[i].r = (actions[i].r * actions[i].r) >> 8;
				actions[i].g = (actions[i].g * actions[i].g) >> 8;
				actions[i].b = (actions[i].b * actions[i].b) >> 8;
				/*r2 = (r2 * r2) >> 8;
				g2 = (g2 * g2) >> 8;
				b2 = (b2 * b2) >> 8;*/
			}
		}
		if (power && actions.size() > 1)
			AlienFX_SDK::Functions::SetPowerAction(id, actions[0].r, actions[0].g, actions[0].b,
				actions[1].r, actions[1].g, actions[1].b, force);
		else 
			if (actions[0].type == 0)
				AlienFX_SDK::Functions::SetColor(id, actions[0].r, actions[0].g, actions[0].b);
			else {
				AlienFX_SDK::Functions::SetAction(id, actions);
			}
	}
	else {
		if (!power)
			AlienFX_SDK::Functions::SetColor(id, 0, 0, 0);
		else
			if (config->offPowerButton)
				AlienFX_SDK::Functions::SetPowerAction(id, 0, 0, 0, 0, 0, 0);
	}
}

void FXHelper::RefreshState()
{
	if (config->enableMon)
		SetCounterColor(lCPU, lRAM, lGPU, lNET, lHDD, lTemp, lBatt, true);
	Refresh();
}

int FXHelper::Refresh(bool forced)
{
	std::vector <lightset>::iterator Iter;
	Colorcode fin;
	for (int i = 0; i < 15 && forced && !AlienFX_SDK::Functions::IsDeviceReady(); i++) Sleep(20);
	if (!AlienFX_SDK::Functions::IsDeviceReady()) return 1;
	int lFlags = 0;
	std::vector<AlienFX_SDK::afx_act> actions; AlienFX_SDK::afx_act action;
	for (Iter = config->mappings.begin(); Iter != config->mappings.end(); Iter++) {
		if (Iter->devid == pid) {
			actions = Iter->eve[0].map;
			lFlags = AlienFX_SDK::Functions::GetFlags(pid, Iter->lightid);
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
	AlienFX_SDK::Functions::UpdateColors();
	return 0;
}

int FXHelper::SetMode(int mode)
{
	int t = activeMode;
	activeMode = mode;
	return t;
}


