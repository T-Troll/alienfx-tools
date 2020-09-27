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
	if (id != lastTest) {
		if (lastTest != (-1))
			AlienFX_SDK::Functions::SetColor(lastTest, 0, 0, 0);
		lastTest = id;
		AlienFX_SDK::Functions::SetColor(id, 0, 255, 0);
	}
	AlienFX_SDK::Functions::UpdateColors();
}

void FXHelper::UpdateLight(lightset* map, bool update) {
	if (map != NULL) {
		if (map->eve[2].flags || map->eve[3].flags) return;
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
			case MODE_LOW: mode1 = mode2 = 1; break;
			case MODE_CHARGE: mode1 = mode2 = 2; break;
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
		if (update)
			AlienFX_SDK::Functions::UpdateColors();
	}
}

int FXHelper::Refresh()
{
	unsigned i = 0;
	for (i = 0; i < config->mappings.size(); i++) {
		UpdateLight(&config->mappings[i], false);	
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


