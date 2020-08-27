#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

FXHelper::FXHelper(int* freqp, ConfigHandler* conf) {
	freq = freqp;
	config = conf;
	//lastUpdate = GetTickCount64();
	done = 0;
	stopped = 0;
	int isInit = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
	//std::cout << "PID: " << std::hex << isInit << std::endl;
	if (isInit != -1)
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
	AlienFX_SDK::Functions::AlienFXClose();
};
void FXHelper::StartFX() {
	//done = 0;
	//stopped = 0;
	//lfx->Reset();
	//lfx->Update();
};
void FXHelper::StopFX() {
	done = 1;
	//while (!stopped)
	//	Sleep(100);
	//lfx->Reset();
	//lfx->Update();
	//lfx->Release();
};

int FXHelper::Refresh(int numbars)
{
	unsigned i = 0;
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		double power = 0.0;
		Colorcode from, to, fin;
		from.ci = map.colorfrom.ci; to.ci = map.colorto.ci;
		// here need to check less bars...
		for (int j = 0; j < map.map.size(); j++) {
			power += (freq[map.map[j]] > map.lowcut ? freq[map.map[j]] < map.hicut ? freq[map.map[j]] - map.lowcut : map.hicut - map.lowcut: 0 );
		}
		if (map.map.size() > 0)
			power = power / (map.map.size() * (map.hicut - map.lowcut));
		fin.cs.red = (unsigned char)((1 - power) * from.cs.red + power * to.cs.red);
		fin.cs.green = (unsigned char)((1 - power) * from.cs.green + power * to.cs.green);
		fin.cs.blue = (unsigned char)((1 - power) * from.cs.blue + power * to.cs.blue);
		//it's a bug into LightFX - r and b are inverted in this call!
		fin.cs.brightness = (unsigned char)((1 - power) * from.cs.brightness + power * to.cs.brightness);
		AlienFX_SDK::Functions::SetColor(map.lightid,
			fin.cs.red, fin.cs.green, fin.cs.blue);
	}
	AlienFX_SDK::Functions::UpdateColors();
	return 0;
}

/*int FXHelper::UpdateLights() {
	if (done) { stopped = 1; return 1; }
	ULONGLONG cTime = GetTickCount64();
	ULONGLONG oldTime = cTime;
	int uIndex = 0;
	for (int j = 0; j < lastLights; j++) {
		if (oldTime > updates[j].lastUpdate) {
			oldTime = updates[j].lastUpdate;
			uIndex = j;
		}
	}
	//if (cTime - oldTime > 50) {
		lfx->SetOneLFXColor(updates[uIndex].devid, updates[uIndex].lightid, &updates[uIndex].color.ci);
		lfx->Update();
		updates[uIndex].lastUpdate = cTime;
	//}
	return 0;
}*/
