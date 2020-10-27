#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	devList = AlienFX_SDK::Functions::AlienFXEnumDevices(AlienFX_SDK::Functions::vid);
	pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
	if (pid != -1)
	{
		int count;
		for (count = 0; count < 5 && !AlienFX_SDK::Functions::IsDeviceReady(); count++)
			Sleep(20);
		if (count == 5)
			AlienFX_SDK::Functions::Reset(0);
		AlienFX_SDK::Functions::LoadMappings();
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
			Colorcode fin = Iter->eve[0].fs.b.flags ? Iter->eve[0].map.c1 : Iter->eve[2].fs.b.flags ?
				Iter->eve[2].map.c1 : Iter->eve[3].map.c1;
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
				fin.cs.red = fin.cs.red * (1 - coeff) + Iter->eve[2].map.c2.cs.red * coeff;
				fin.cs.green = fin.cs.green * (1 - coeff) + Iter->eve[2].map.c2.cs.green * coeff;
				fin.cs.blue = fin.cs.blue * (1 - coeff) + Iter->eve[2].map.c2.cs.blue * coeff;
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
							bStage ? Iter->eve[3].map.c2 : fin 
							: Iter->eve[3].map.c2 
						: fin;
			}
			wasChanged = true;
			if (lFlags)
				if (activeMode != MODE_AC && activeMode != MODE_CHARGE)
					SetLight(Iter->lightid, lFlags, 0, 0, 0, Iter->eve[0].map.c1.cs.red, Iter->eve[0].map.c1.cs.green, Iter->eve[0].map.c1.cs.blue,
						0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue);
				else
					SetLight(Iter->lightid, lFlags, 0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue,
						0, 0, 0, Iter->eve[0].map.c2.cs.red, Iter->eve[0].map.c2.cs.green, Iter->eve[0].map.c2.cs.blue);
			else
				SetLight(Iter->lightid, lFlags, 0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue,
					0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue);
		}
	if (wasChanged)
		//Refresh(false, false);
		AlienFX_SDK::Functions::UpdateColors();
	lCPU = cCPU; lRAM = cRAM; lGPU = cGPU; lHDD = cHDD; lNET = cNet; lTemp = cTemp; lBatt = cBatt;
}

void FXHelper::SetLight(int id, bool power, int mode1, int length1, int speed1, int r, int g, int b, int mode2, int length2, int speed2, int r2, int g2, int b2)
{
	// modify colors for dimmed...
	const BYTE delta = (BYTE) config->dimmingPower;

	if (config->lightsOn && config->stateOn) {
		if (config->dimmed ||
			(config->dimmedBatt && (activeMode & (MODE_BAT | MODE_LOW)))) {
			r = r < delta ? 0 : r - delta;
				g = g < delta ? 0 : g - delta;
				b = b < delta ? 0 : b - delta;
				r2 = r2 < delta ? 0 : r2 - delta;
				g2 = g2 < delta ? 0 : g2 - delta;
				b2 = b2 < delta ? 0 : b2 - delta;
		}
		// gamma-correction...
		if (config->gammaCorrection) {
			r = (r * r) >> 8;
			g = (g * g) >> 8;
			b = (b * b) >> 8;
			r2 = (r2 * r2) >> 8;
			g2 = (g2 * g2) >> 8;
			b2 = (b2 * b2) >> 8;
		}

		if (power)
			AlienFX_SDK::Functions::SetPowerAction(id, r, g, b, r2, g2, b2);
		else 
			if (mode1 == 0 && mode2 == 0)
				AlienFX_SDK::Functions::SetColor(id, r, g, b);
			else
				AlienFX_SDK::Functions::SetAction(id,
					mode1, length1, speed1, r, g, b,
					mode2, length2, speed2, r2, g2, b2);
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
	for (int i = 0; i < 15 && !AlienFX_SDK::Functions::IsDeviceReady(); i++) Sleep(20);
	if (!AlienFX_SDK::Functions::IsDeviceReady()) return 1;
	int lFlags = 0;
	for (Iter = config->mappings.begin(); Iter != config->mappings.end(); Iter++) {
		if (Iter->devid == pid && (!(lFlags = AlienFX_SDK::Functions::GetFlags(pid, Iter->lightid)) || forced)) {
			Colorcode c1 = Iter->eve[0].map.c1, c2 = Iter->eve[0].map.c2;
			int mode1 = Iter->eve[0].map.mode, mode2 = Iter->eve[0].map.mode2;
			if (config->enableMon && !forced) {
				if (!Iter->eve[0].fs.b.flags) {
					c1.ci = 0; c2.ci = 0;
				}
				if (Iter->eve[1].fs.b.flags) {
					// use power event;
					c2 = Iter->eve[1].map.c2;
					c1 = Iter->eve[1].map.c1;
					switch (activeMode) {
					case MODE_AC: if (Iter->eve[0].fs.b.flags) {
						c1 = Iter->eve[0].map.c1; c2 = Iter->eve[0].map.c2;
					}
								else {
						mode1 = mode2 = 0; //c1 = Iter->eve[1].map.c1;
					} break;
					case MODE_BAT: mode1 = mode2 = 0; c1 = c2; break;
					case MODE_LOW: mode1 = mode2 = 1; c1 = c2; break;
					case MODE_CHARGE: mode1 = mode2 = 2; /*c1 = Iter->eve[0].map.c1;*/ break;
					}
				}
				if ((Iter->eve[2].fs.b.flags || Iter->eve[3].fs.b.flags)
					&& config->lightsOn && config->stateOn) continue;
			}
			SetLight(Iter->lightid, lFlags,
				mode1, Iter->eve[0].map.length1, Iter->eve[0].map.speed1, c1.cs.red, c1.cs.green, c1.cs.blue,
				mode2, Iter->eve[0].map.length2, Iter->eve[0].map.speed2, c2.cs.red, c2.cs.green, c2.cs.blue
			);
		}
		//UpdateLight(&config->mappings[i], false);	
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


