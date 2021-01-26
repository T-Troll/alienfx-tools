#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	devList = AlienFX_SDK::Functions::AlienFXEnumDevices(AlienFX_SDK::Functions::vid);
	AlienFX_SDK::Functions::LoadMappings();
	pid = 0;
	if (conf->lastActive != 0)
		for (int i = 0; i < devList.size(); i++)
			if (devList[i] == conf->lastActive) {
				pid = conf->lastActive;
				break;
			}
	if (pid == 0)
		pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
	else
		pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid, pid);

	if (pid != -1)
	{
		int count;
		for (count = 0; count < 5 && !AlienFX_SDK::Functions::IsDeviceReady(); count++)
			Sleep(20);
		if (count == 5)
			AlienFX_SDK::Functions::Reset(false);
		AlienFX_SDK::Functions::LoadMappings();
		conf->lastActive = pid;
	}
};
FXHelper::~FXHelper() {
	FadeToBlack();
	AlienFX_SDK::Functions::AlienFXClose();
};

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	unsigned shift = 256 - config->shift;
	if (!AlienFX_SDK::Functions::IsDeviceReady()) return 1;
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		Colorcode fin = { 0 };
		unsigned r = 0, g = 0, b = 0, size = (unsigned) map.map.size();
		if (map.devid == pid && AlienFX_SDK::Functions::GetFlags(pid, map.lightid) == 0 && size > 0) {
			for (unsigned j = 0; j < size; j++) {
				r += img[3 * map.map[j]+2];
				g += img[3 * map.map[j] + 1];
				b += img[3 * map.map[j]];
			}

			// Brightness correction...
			fin.cs.red = (r * shift) / (256 * size);
			fin.cs.green = (g * shift) / (256 * size);
			fin.cs.blue = (b *shift) / (256 * size);

			// Gamma correction...
			if (config->gammaCorrection) {
				fin.cs.red = ((int)fin.cs.red * fin.cs.red) >> 8;
				fin.cs.green = ((int)fin.cs.green * fin.cs.green) >> 8;
				fin.cs.blue = ((int)fin.cs.blue * fin.cs.blue) >> 8;
			}
			AlienFX_SDK::Functions::SetColor(map.lightid, fin.cs.red, fin.cs.green, fin.cs.blue);

		}
	}
	AlienFX_SDK::Functions::UpdateColors();
	return 0;
}

void FXHelper::FadeToBlack()
{
	if (!AlienFX_SDK::Functions::IsDeviceReady()) return;
	for (int i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		Colorcode fin = { 0 };
		unsigned r = 0, g = 0, b = 0, size = (unsigned)map.map.size();
		if (map.devid == pid && AlienFX_SDK::Functions::GetFlags(pid, map.lightid) == 0 && size > 0) {
			AlienFX_SDK::Functions::SetColor(map.lightid, 0, 0, 0);
		}
	}
	AlienFX_SDK::Functions::UpdateColors();
}


