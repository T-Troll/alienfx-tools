#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(int* freqp, ConfigHandler* conf) {
	freq = freqp;
	config = conf;
	done = 0;
	stopped = 0;
	std::vector<int> devList = AlienFX_SDK::Functions::AlienFXEnumDevices(AlienFX_SDK::Functions::vid);
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
		//AlienFX_SDK::Functions::LoadMappings();
		conf->lastActive = pid;
	}
	FadeToBlack();
};
FXHelper::~FXHelper() {
	FadeToBlack();
	AlienFX_SDK::Functions::AlienFXClose();
};

int FXHelper::GetPID() {
	return pid;
}

int FXHelper::Refresh(int numbars)
{
	unsigned i = 0;
	if (!AlienFX_SDK::Functions::IsDeviceReady()) {
#ifdef _DEBUG
		OutputDebugString("Device not ready!\n");
#endif
		return 1;
	}
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		if (map.devid == pid && AlienFX_SDK::Functions::GetFlags(pid, map.lightid) == 0 && map.map.size() > 0) {
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
			AlienFX_SDK::Functions::SetColor(map.lightid,
				fin.cs.red, fin.cs.green, fin.cs.blue);
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
		if (map.devid == pid && AlienFX_SDK::Functions::GetFlags(pid, map.lightid) == 0) {
			AlienFX_SDK::Functions::SetColor(map.lightid, 0, 0, 0);
		}
	}
	AlienFX_SDK::Functions::UpdateColors();
}
