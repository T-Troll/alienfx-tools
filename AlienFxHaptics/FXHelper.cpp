#include "FXHelper.h"

FXHelper::FXHelper(int* freqp, ConfigHandler* conf) {
	freq = freqp;
	config = conf;
	done = 0;
	stopped = 0;
	afx_dev = new AlienFX_SDK::Functions();
	std::vector<int> devList = afx_dev->AlienFXEnumDevices(afx_dev->vid);
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
		//afx_dev->LoadMappings();
		conf->lastActive = pid;
	}
	FadeToBlack();
};
FXHelper::~FXHelper() {
	FadeToBlack();
	afx_dev->AlienFXClose();
	delete afx_dev;
};

int FXHelper::GetPID() {
	return pid;
}

int FXHelper::Refresh(int numbars)
{
	unsigned i = 0;
	if (!afx_dev->IsDeviceReady()) {
#ifdef _DEBUG
		OutputDebugString("Device not ready!\n");
#endif
		return 1;
	}
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		if (map.devid == pid && afx_dev->GetFlags(pid, map.lightid) == 0 && map.map.size() > 0) {
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
			afx_dev->SetColor(map.lightid,
				fin.cs.red, fin.cs.green, fin.cs.blue);
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
		if (map.devid == pid && afx_dev->GetFlags(pid, map.lightid) == 0) {
			afx_dev->SetColor(map.lightid, 0, 0, 0);
		}
	}
	afx_dev->UpdateColors();
}
