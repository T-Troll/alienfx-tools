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
		if (!result) {
			//std::cout << "Reset faled with " << std::hex << GetLastError() << std::endl;
			return;
		}
		result = AlienFX_SDK::Functions::IsDeviceReady();
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
	if (id != lastTest) {
		if (lastTest != (-1))
			AlienFX_SDK::Functions::SetColor(lastTest, 0, 0, 0);
		lastTest = id;
	}
	AlienFX_SDK::Functions::SetColor(id, 0, 255, 0);
	AlienFX_SDK::Functions::UpdateColors();
	devbusy = false;
}

void FXHelper::SetCounterColor(long cCPU, long cRAM, long cGPU, long cNet, long cHDD)
{
	lCPU = cCPU; lRAM = cRAM; lGPU = cGPU; lHDD = cHDD; lNET = cNet;
	if (config->autoRefresh) Refresh();
	std::vector <lightset>::iterator Iter;
	Colorcode fin;
	while (devbusy) Sleep(20);
	devbusy = true;
	for (Iter = config->mappings.begin(); Iter != config->mappings.end(); Iter++) {
		if (Iter->devid == pid) {
			if (Iter->eve[2].flags) {
				// counter
				double coeff = 0.0;
				switch (Iter->eve[2].source) {
				case 0: coeff = cCPU / 100.0; break;
				case 1: coeff = cRAM / 100.0; break;
				case 2: coeff = cHDD / 100.0; break;
				case 3: coeff = cGPU / 100.0; break;
				case 4: coeff = cNet / 100.0; break;
				}
				fin.cs.red = Iter->eve[0].map.c1.cs.red * (1 - coeff) + Iter->eve[2].map.c2.cs.red * coeff;
				fin.cs.green = Iter->eve[0].map.c1.cs.green * (1 - coeff) + Iter->eve[2].map.c2.cs.green * coeff;
				fin.cs.blue = Iter->eve[0].map.c1.cs.blue * (1 - coeff) + Iter->eve[2].map.c2.cs.blue * coeff;
				SetLight(Iter->lightid, 0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue);
			}
			if (Iter->eve[3].flags) {
				// indicator
				long indi = 0;
				switch (Iter->eve[3].source) {
				case 0: indi = cHDD; break;
				case 1: indi = cNet; break;
				}
				fin = (indi > 0) ? Iter->eve[3].map.c2 : Iter->eve[0].map.c1;
				SetLight(Iter->lightid, 0, 0, 0, fin.cs.red, fin.cs.green, fin.cs.blue);
			}
		}
	}
	AlienFX_SDK::Functions::UpdateColors();
	devbusy = false;
}

void FXHelper::SetLight(int id, int mode1, int length1, int speed1, BYTE r, BYTE g, BYTE b, int mode2, int length2, int speed2, BYTE r2, BYTE g2, BYTE b2)
{
	// modify colors for dimmed...
	const BYTE delta = config->dimmingPower;

	if (config->lightsOn) {
		if (config->dimmed ||
			(config->dimmedBatt && (activeMode & (MODE_BAT | MODE_LOW)))) {
			r = r < delta ? 0 : r - delta;
				g = g < delta ? 0 : g - delta;
				b = b < delta ? 0 : b - delta;
				r2 = r2 < delta ? 0 : r2 - delta;
				g2 = g2 < delta ? 0 : g2 - delta;
				b2 = b2 < delta ? 0 : b2 - delta;
		}

		if (mode1 == 0) {
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

/*void FXHelper::UpdateLight(lightset* map, bool update) {
	if (map != NULL && map->devid == pid) {
		while (devbusy) Sleep(20);
		devbusy = true;
		if (config->lightsOn) {
			if (map->eve[2].flags || map->eve[3].flags) {
				devbusy = false;
				return;
			}
			mapping* mmap = &map->eve[0].map;
			Colorcode c1 = mmap->c1, c2 = mmap->c2;
			if (!map->eve[0].flags)
				c1.ci = c2.ci = 0;
			int mode1 = mmap->mode, mode2 = mmap->mode2;
			if (map->eve[1].flags) {
				// use power event;
				c2 = map->eve[1].map.c2;
				switch (activeMode) {
				case MODE_AC: mode1 = mode2 = 0; c1 = mmap->c1; break;
				case MODE_BAT: mode1 = mode2 = 0; c1 = c2; break;
				case MODE_LOW: mode1 = mode2 = 1; c1 = c2; break;
				case MODE_CHARGE: mode1 = mode2 = 2; c1 = mmap->c1; break;
				}
			}
			if (mode1 == 0) {
				AlienFX_SDK::Functions::SetColor(map->lightid, c1.cs.red, c1.cs.green, c1.cs.blue);
			}
			else {
				AlienFX_SDK::Functions::SetAction(map->lightid,
					mode1, mmap->length1, mmap->speed1, c1.cs.red, c1.cs.green, c1.cs.blue,
					mode2, mmap->length2, mmap->speed2, c2.cs.red, c2.cs.green, c2.cs.blue
				);
			}
		}
		else {
			AlienFX_SDK::Functions::SetColor(map->lightid, 0, 0, 0);
		}
		if (update)
			AlienFX_SDK::Functions::UpdateColors();
		devbusy = false;
	}
}*/

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
			if (!Iter->eve[0].flags)
				c1.ci = c2.ci = 0;
			int mode1 = Iter->eve[0].map.mode, mode2 = Iter->eve[0].map.mode2;
			if (Iter->eve[1].flags) {
				// use power event;
				c2 = Iter->eve[1].map.c2;
				switch (activeMode) {
				case MODE_AC: mode1 = mode2 = 0; c1 = Iter->eve[0].map.c1; break;
				case MODE_BAT: mode1 = mode2 = 0; c1 = c2; break;
				case MODE_LOW: mode1 = mode2 = 1; c1 = c2; break;
				case MODE_CHARGE: mode1 = mode2 = 2; c1 = Iter->eve[0].map.c1; break;
				}
			}
			if ((Iter->eve[2].flags || Iter->eve[3].flags) && config->lightsOn) continue;
			/*if (Iter->eve[2].flags) {
				// counter
				double coeff = 0.0;
				switch (Iter->eve[2].source) {
				case 0: coeff = lCPU / 100.0; break;
				case 1: coeff = lRAM / 100.0; break;
				case 2: coeff = lHDD / 100.0; break;
				case 3: coeff = lGPU / 100.0; break;
				case 4: coeff = lNET / 100.0; break;
				}
				c1.cs.red = Iter->eve[0].map.c1.cs.red * (1 - coeff) + Iter->eve[2].map.c2.cs.red * coeff;
				c1.cs.green = Iter->eve[0].map.c1.cs.green * (1 - coeff) + Iter->eve[2].map.c2.cs.green * coeff;
				c1.cs.blue = Iter->eve[0].map.c1.cs.blue * (1 - coeff) + Iter->eve[2].map.c2.cs.blue * coeff;
				mode1 = 0;
			}
			if (Iter->eve[3].flags) {
				// indicator
				long indi = 0;
				switch (Iter->eve[3].source) {
				case 0: indi = lHDD; break;
				case 1: indi = lNET; break;
				}
				c1 = (indi > 0) ? Iter->eve[3].map.c2 : Iter->eve[0].map.c1;
				mode1 = 0;
			}*/
			SetLight(Iter->lightid,
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


