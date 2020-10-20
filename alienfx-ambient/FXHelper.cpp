#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	//lastUpdate = 0;
	pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
	if (pid != -1)
	{
		int count;
		for (count = 0; count < 5 && !AlienFX_SDK::Functions::IsDeviceReady(); count++)
			Sleep(20);
		if (count == 5)
			AlienFX_SDK::Functions::Reset(false);
		AlienFX_SDK::Functions::LoadMappings();
	}
};
FXHelper::~FXHelper() {
	FadeToBlack();
	AlienFX_SDK::Functions::AlienFXClose();
};

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	unsigned shift = config->shift;
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
			fin.cs.red = r / size;
			fin.cs.green = g / size;
			fin.cs.blue = b / size;
			// Brightness correction...
			unsigned //cmax = fin.cs.red > fin.cs.green ? max(fin.cs.red, fin.cs.blue) : max(fin.cs.green, fin.cs.blue),
				cmin = fin.cs.red < fin.cs.green ? min(fin.cs.red, fin.cs.blue) : min(fin.cs.green, fin.cs.blue),
				//lght = (cmax + cmin) > shift * 2 ? (cmax + cmin) / 2 - shift : 0,
				//strn = (cmax - cmin) == 0 ? 0 : (cmax - cmin) / (255 - std::abs((cmax + cmin) - 255)),
				delta = cmin > shift ? shift : cmin;// lght - (strn * (255 - 2 * std::abs(lght - 255))) / 2;
			fin.cs.red -= delta;  //fin.cs.red < shift ? 0 : fin.cs.red - shift;
			fin.cs.green -= delta;  //fin.cs.green < shift ? 0 : fin.cs.green - shift;
			fin.cs.blue -= delta;  //fin.cs.blue < shift ? 0 : fin.cs.blue - shift;
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


