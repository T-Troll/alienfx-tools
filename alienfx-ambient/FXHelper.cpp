#include "FXHelper.h"

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	unsigned shift = 256 - config->shift;
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		AlienFX_SDK::Functions* dev = LocateDev(map.devid);
		Colorcode fin = { 0 };
		unsigned r = 0, g = 0, b = 0, size = (unsigned) map.map.size();
		if (dev && !afx_dev.GetFlags(map.devid, map.lightid) && size > 0) {
			for (unsigned j = 0; j < size; j++) {
				r += img[3 * map.map[j] + 2];
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
			// TODO: expand group!
			if (dev->IsDeviceReady())
				dev->SetColor(map.lightid, fin.cs.red, fin.cs.green, fin.cs.blue);

		}
	}
	UpdateColors();
	return 0;
}

void FXHelper::FadeToBlack()
{
	for (int i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		AlienFX_SDK::Functions* dev = LocateDev(map.devid);
		if (dev && !afx_dev.GetFlags(map.devid, map.lightid) && dev->IsDeviceReady()) {
			dev->SetColor(map.lightid, 0, 0, 0);
		}
	}
	UpdateColors();
}

//void FXHelper::UpdateColors()
//{
//	for (int i = 0; i < devs.size(); i++)
//		devs[i]->UpdateColors();
//}
