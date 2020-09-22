#include "FXHelper.h"
#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

//using namespace AlienFX_SDK;
//#ifdef _DEBUG
//#pragma comment(lib, "opencvworld343d.lib")
//#else
//#pragma comment(lib, "opencvworld343.lib")
//#endif

FXHelper::FXHelper(ConfigHandler* conf) {
	config = conf;
	//lastUpdate = 0;
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

void FXHelper::UpdateLight(int id) {
	if (id < config->mappings.size()) {
		mapping map = config->mappings[id];
		if (map.mode == 0) {
			AlienFX_SDK::Functions::SetColor(map.lightid, map.c1.cs.red, map.c1.cs.green, map.c1.cs.blue);
		}
		else {
			AlienFX_SDK::Functions::SetAction(map.lightid,
				map.mode, map.length1, map.speed1, map.c1.cs.red, map.c1.cs.green, map.c1.cs.blue,
				map.mode, map.length2, map.speed2, map.c2.cs.red, map.c2.cs.green, map.c2.cs.blue
			);
		}
		AlienFX_SDK::Functions::UpdateColors();
	}
}

int FXHelper::Refresh()
{
	unsigned i = 0;
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		if (map.mode == 0) {
			AlienFX_SDK::Functions::SetColor(map.lightid, map.c1.cs.red, map.c1.cs.green, map.c1.cs.blue);
		}
		else {
			AlienFX_SDK::Functions::SetAction(map.lightid,
				map.mode, map.length1, map.speed1, map.c1.cs.red, map.c1.cs.green, map.c1.cs.blue,
				map.mode, map.length2, map.speed2, map.c2.cs.red, map.c2.cs.green, map.c2.cs.blue
			);
		}
	}
	AlienFX_SDK::Functions::UpdateColors();
	return 0;
}


