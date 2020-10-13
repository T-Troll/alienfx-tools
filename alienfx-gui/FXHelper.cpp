#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	//lastUpdate = 0;
	devList = AlienFX_SDK::Functions::AlienFXEnumDevices(AlienFX_SDK::Functions::vid);
	pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
	if (pid != -1)
	{
		bool result = AlienFX_SDK::Functions::Reset(false);
		if (result) {
			//std::cout << "Reset faled with " << std::hex << GetLastError() << std::endl;
			result = AlienFX_SDK::Functions::IsDeviceReady();
		}
		AlienFX_SDK::Functions::LoadMappings();
	}
};
FXHelper::~FXHelper() {
	AlienFX_SDK::Functions::SaveMappings();
	AlienFX_SDK::Functions::AlienFXClose();
	//lfx->Release();
};
void FXHelper::StartFX() {
	//done = 0;
	//stopped = 0;
	//lfx->Reset();
	//lfx->Update();
	AlienFX_SDK::Functions::Reset(false);
};
void FXHelper::StopFX() {
	//while (!stopped)
	//	Sleep(100);
	//lfx->Reset();
	//lfx->Update();
	AlienFX_SDK::Functions::Reset(false);
};

std::vector<int> FXHelper::GetDevList() {
	return devList;
}

void FXHelper::TestLight(int id)
{
	while (devbusy) Sleep(20);
	devbusy = true;
	/*if (id != lastTest) {
		if (lastTest != (-1))
			AlienFX_SDK::Functions::SetColor(lastTest, 0, 0, 0);
		lastTest = id;
	}*/
	AlienFX_SDK::Functions::SetColor(id, config->testColor.cs.red, config->testColor.cs.green, config->testColor.cs.blue);
	AlienFX_SDK::Functions::UpdateColors();
	devbusy = false;
}

void FXHelper::SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD, long cTemp, long cBatt, bool force)
{
	std::vector <lightset>::iterator Iter;
	Colorcode fin;
	while (devbusy) Sleep(20);
	devbusy = true;
	for (Iter = config->mappings.begin(); Iter != config->mappings.end(); Iter++) {
		if (Iter->devid == pid) {
			Colorcode c2 = Iter->eve[0].map.c2;
			if (Iter->eve[2].flags) {
				// counter
				double coeff = 0.0;
				Colorcode cfin = Iter->eve[0].flags ? Iter->eve[0].map.c1 : Iter->eve[2].map.c1;
				switch (Iter->eve[2].source) {
				case 0: if (!force && lCPU == cCPU) continue; coeff = cCPU / 100.0; break;
				case 1: if (!force && lRAM == cRAM) continue; coeff = cRAM / 100.0; break;
				case 2: if (!force && lHDD == cHDD) continue; coeff = cHDD / 100.0; break;
				case 3: if (!force && lGPU == cGPU) continue; coeff = cGPU / 100.0; break;
				case 4: if (!force && lNET == cNet) continue; coeff = cNet / 100.0; break;
				case 5: if (!force && lTemp == cTemp) continue; coeff = (cTemp > 273) ? (cTemp - 273.0) / 100.0 : 0; break;
				case 6: if (!force && lBatt == cBatt) continue; coeff = cBatt / 100.0; break;
				}
				fin.cs.red = cfin.cs.red * (1 - coeff) + Iter->eve[2].map.c2.cs.red * coeff;
				fin.cs.green = cfin.cs.green * (1 - coeff) + Iter->eve[2].map.c2.cs.green * coeff;
				fin.cs.blue = cfin.cs.blue * (1 - coeff) + Iter->eve[2].map.c2.cs.blue * coeff;
				SetLight(Iter->lightid, Iter->eve[1].source, 0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue,
					0, 0, 0, c2.cs.red, c2.cs.green, c2.cs.blue);
				continue;
			}
			if (Iter->eve[3].flags) {
				// indicator
				long indi = 0;
				switch (Iter->eve[3].source) {
				case 0: if (!force && lHDD == cHDD) continue; indi = cHDD; break;
				case 1: if (!force && lNET == cNet) continue; indi = cNet; break;
				case 2: if (!force && lTemp == cTemp) continue; indi = (cTemp - 273) > 90; break;
				}
				fin = (indi > 0) ? Iter->eve[3].map.c2 : Iter->eve[0].flags ? Iter->eve[0].map.c1 : Iter->eve[3].map.c1;
				SetLight(Iter->lightid, Iter->eve[1].source, 0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue,
					0, 0, 0, c2.cs.red, c2.cs.green, c2.cs.blue);
			}
		}
	}
	AlienFX_SDK::Functions::UpdateColors();
	devbusy = false;
	lCPU = cCPU; lRAM = cRAM; lGPU = cGPU; lHDD = cHDD; lNET = cNet; lTemp = cTemp; lBatt = cBatt;
	if (config->autoRefresh) Refresh();
}

void FXHelper::SetLight(int id, bool power, int mode1, int length1, int speed1, BYTE r, BYTE g, BYTE b, int mode2, int length2, int speed2, BYTE r2, BYTE g2, BYTE b2)
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

		if (power) {
			//AlienFX_SDK::Functions::UpdateColors();
			//AlienFX_SDK::Functions::Reset(false);
			AlienFX_SDK::Functions::SetPowerAction(id,
				r, g, b,
				r2, g2, b2
			);
		} else 
			if (mode1 == 0 && mode2 == 0) {
				AlienFX_SDK::Functions::SetColor(id, r, g, b);
			}
			else {
				AlienFX_SDK::Functions::SetAction(id,
					mode1, length1, speed1, r, g, b,
					mode2, length2, speed2, r2, g2, b2
				);
			}
	}
	else {
		AlienFX_SDK::Functions::SetColor(id, 0, 0, 0);
	}
}

void FXHelper::RefreshState()
{
	if (config->enableMon)
		SetCounterColor(lCPU, lRAM, lGPU, lNET, lHDD, lTemp, lBatt, true);
	Refresh();
}

int FXHelper::Refresh()
{
	unsigned i = 0;
	std::vector <lightset>::iterator Iter;
	Colorcode fin;
	while (devbusy) Sleep(20);
	devbusy = true;
	for (Iter = config->mappings.begin(); Iter != config->mappings.end(); Iter++) {
		if (Iter->devid == pid) {
			Colorcode c1 = Iter->eve[0].map.c1, c2 = Iter->eve[0].map.c2;
			int mode1 = Iter->eve[0].map.mode, mode2 = Iter->eve[0].map.mode2;
			if (config->enableMon) {
				if (!Iter->eve[0].flags) {
					c1.ci = 0; c2.ci = 0;
				}
				if (Iter->eve[1].flags) {
					// use power event;
					c2 = Iter->eve[1].map.c2;
					c1 = Iter->eve[1].map.c1;
					switch (activeMode) {
					case MODE_AC: if (Iter->eve[0].flags) {
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
				if ((Iter->eve[2].flags || Iter->eve[3].flags)
					&& config->lightsOn && config->stateOn) continue;
			}
			SetLight(Iter->lightid, Iter->eve[1].source,
				mode1, Iter->eve[0].map.length1, Iter->eve[0].map.speed1, c1.cs.red, c1.cs.green, c1.cs.blue,
				mode2, Iter->eve[0].map.length2, Iter->eve[0].map.speed2, c2.cs.red, c2.cs.green, c2.cs.blue
			);
		}
		//UpdateLight(&config->mappings[i], false);	
	}
	AlienFX_SDK::Functions::UpdateColors();
	devbusy = false;
	return 0;
}

int FXHelper::SetMode(int mode)
{
	int t = activeMode;
	activeMode = mode;
	return t;
}


