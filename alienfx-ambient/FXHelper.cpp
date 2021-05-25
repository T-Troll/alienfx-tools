#include "FXHelper.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	afx_dev = new AlienFX_SDK::Functions();
	devList = afx_dev->AlienFXEnumDevices(afx_dev->vid);
	afx_dev->LoadMappings();
	pid = 0;
	if (conf->lastActive != 0)
		for (int i = 0; i < devList.size(); i++)
			if (devList[i] == conf->lastActive) {
				pid = conf->lastActive;
				break;
			}
	if (pid == 0)
		pid = afx_dev->AlienFXInitialize(afx_dev->vid);
	else
		pid = afx_dev->AlienFXInitialize(afx_dev->vid, pid);

	if (pid != -1)
	{
		int count;
		for (count = 0; count < 5 && !afx_dev->IsDeviceReady(); count++)
			Sleep(20);
		if (count == 5)
			afx_dev->Reset(false);
		afx_dev->LoadMappings();
		conf->lastActive = pid;
	}
};
FXHelper::~FXHelper() {
	FadeToBlack();
	afx_dev->AlienFXClose();
	delete afx_dev;
};

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	unsigned shift = 256 - config->shift;
	if (!afx_dev->IsDeviceReady()) return 1;
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		Colorcode fin = { 0 };
		unsigned r = 0, g = 0, b = 0, size = (unsigned) map.map.size();
		if (map.devid == pid && afx_dev->GetFlags(pid, map.lightid) == 0 && size > 0) {
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
			afx_dev->SetColor(map.lightid, fin.cs.red, fin.cs.green, fin.cs.blue);

		}
	}
	afx_dev->UpdateColors();
	return 0;
}

void FXHelper::FadeToBlack()
{
	if (!afx_dev->IsDeviceReady()) return;
	for (int i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		Colorcode fin = { 0 };
		unsigned r = 0, g = 0, b = 0, size = (unsigned)map.map.size();
		if (map.devid == pid && afx_dev->GetFlags(pid, map.lightid) == 0 && size > 0) {
			afx_dev->SetColor(map.lightid, 0, 0, 0);
		}
	}
	afx_dev->UpdateColors();
}


