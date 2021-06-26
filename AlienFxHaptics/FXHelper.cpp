#include "FXHelper.h"

FXHelper::FXHelper(int* freqp, ConfigHandler* conf) {
	freq = freqp;
	config = conf;
	afx_dev.LoadMappings();
	FillDevs();
	FadeToBlack();
};
FXHelper::~FXHelper() {
	FadeToBlack();
	if (devList.size() > 0) {
		for (int i = 0; i < devs.size(); i++)
			devs[i]->AlienFXClose();
		devs.clear();
	}
};

int FXHelper::Refresh()
{
	for (unsigned i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		AlienFX_SDK::Functions* dev = LocateDev(map.devid);
		if (dev && afx_dev.GetFlags(map.devid, map.lightid) == 0 && map.map.size() > 0) {
			double power = 0.0;
			Colorcode from, to, fin;
			from.ci = map.colorfrom.ci; to.ci = map.colorto.ci;
			// here need to check less bars...
			for (int j = 0; j < map.map.size(); j++)
				power += (freq[map.map[j]] > map.lowcut ? freq[map.map[j]] < map.hicut ? freq[map.map[j]] - map.lowcut : map.hicut - map.lowcut : 0);
			if (map.map.size() > 1)
				power = power / (map.map.size() * (map.hicut - map.lowcut));
			fin.cs.red = (unsigned char)((1 - power) * from.cs.red + power * to.cs.red);
			fin.cs.green = (unsigned char)((1 - power) * from.cs.green + power * to.cs.green);
			fin.cs.blue = (unsigned char)((1 - power) * from.cs.blue + power * to.cs.blue);
			fin.cs.red = (fin.cs.red * fin.cs.red) >> 8;
			fin.cs.green = (fin.cs.green * fin.cs.green) >> 8;
			fin.cs.blue = (fin.cs.blue * fin.cs.blue) >> 8;
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
		if (dev && afx_dev.GetFlags(map.devid, map.lightid) == 0 && dev->IsDeviceReady()) {
			dev->SetColor(map.lightid, 0, 0, 0);
		}
	}
	UpdateColors();
}

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

void FXHelper::UpdateColors()
{
	for (int i = 0; i < devs.size(); i++)
		devs[i]->UpdateColors();
}