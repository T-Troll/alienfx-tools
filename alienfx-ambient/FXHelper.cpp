#include "FXHelper.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	afx_dev.LoadMappings();
	FillDevs();
};
FXHelper::~FXHelper() {
	FadeToBlack();
	if (devList.size() > 0) {
		for (int i = 0; i < devs.size(); i++)
			devs[i]->AlienFXClose();
		devs.clear();
	}
};

AlienFX_SDK::Functions* FXHelper::LocateDev(int pid)
{
	for (int i = 0; i < devs.size(); i++)
		if (devs[i]->GetPID() == pid)
			return devs[i];
	return nullptr;
}
void FXHelper::FillDevs()
{
	devList = afx_dev.AlienFXEnumDevices(afx_dev.vid);
	if (devs.size() > 0) {
		for (int i = 0; i < devs.size(); i++)
			devs[i]->AlienFXClose();
		devs.clear();
	}
	for (int i = 0; i < devList.size(); i++) {
		AlienFX_SDK::Functions* dev = new AlienFX_SDK::Functions();
		int pid = dev->AlienFXInitialize(dev->vid, devList[i]);
		if (pid != -1) {
			devs.push_back(dev);
			dev->Reset(true);
		}
	}
}

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
		Colorcode fin = { 0 };
		unsigned r = 0, g = 0, b = 0, size = (unsigned)map.map.size();
		if (dev && afx_dev.GetFlags(map.devid, map.lightid) == 0 && dev->IsDeviceReady()) {
			dev->SetColor(map.lightid, 0, 0, 0);
		}
	}
	UpdateColors();
}

void FXHelper::UpdateColors()
{
	for (int i = 0; i < devs.size(); i++)
		devs[i]->UpdateColors();
}
