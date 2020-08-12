#include "FXHelper.h"
//#include "../AlienFX-SDK/AlienFX_SDK/AlienFX_SDK.h"

//using namespace AlienFX_SDK;
//#ifdef _DEBUG
//#pragma comment(lib, "opencvworld343d.lib")
//#else
//#pragma comment(lib, "opencvworld343.lib")
//#endif

FXHelper::FXHelper(ConfigHandler* conf) {
	//freq = freqp;
	lfx = conf->lfx;
	config = conf;
	lastUpdate = 0;
	//lastLights = 0;
	//for (unsigned i = 0; i < 50; i++)
	//	updates[i].lastUpdate = 0;
	//lfx->InitLFX();
	//lfx->FillInfo();
	//lfx->Release();
	//pid = AlienFX_SDK::Functions::AlienFXInitialize(AlienFX_SDK::Functions::vid);
};
FXHelper::~FXHelper() {
	//AlienFX_SDK::Functions::AlienFXClose();
	lfx->Release();
};
void FXHelper::StartFX() {
	//done = 0;
	//stopped = 0;
	lfx->Reset();
	lfx->Update();
	//AlienFX_SDK::Functions::Reset(false);
};
void FXHelper::StopFX() {
	//while (!stopped)
	//	Sleep(100);
	lfx->Reset();
	lfx->Update();
	//AlienFX_SDK::Functions::Reset(false);
};

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	if (updates.size() < config->mappings.size())
		updates.resize(config->mappings.size());
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		Colorcode fin = { 0 };
		for (int j = 0; j < map.map.size(); j++) {
			fin.cs.red += img[3 * map.map[j]];
			fin.cs.green += img[3 * map.map[j] +1];
			fin.cs.blue += img[3 * map.map[j] +2 ];
			//fin.cs.brightness += 0.2126 * img[3 * map.map[j]] 
			//	+ 0.7152 * img[3 * map.map[j] + 1] 
			//	+ 0.0722 * img[3 * map.map[j] + 2];// img[4 * map.map[j] + 3];
		}
		if (map.map.size() > 0) {
			fin.cs.red /= map.map.size();
			fin.cs.green /= map.map.size();
			fin.cs.blue /= map.map.size();
			//fin.cs.brightness /= map.map.size();
		}
		fin.cs.brightness = (0.2126 * fin.cs.red
			+ 0.7152 * fin.cs.green
			+ 0.0722 * fin.cs.blue) / 4 + 192;
		//updates[i].color = fin;
		//updates[i].devid = map.devid;
		//updates[i].lightid = map.lightid;
		if (abs(fin.cs.red - updates[i].color.cs.red) > 10 ||
			abs(fin.cs.blue - updates[i].color.cs.blue) > 10 ||
			abs(fin.cs.green - updates[i].color.cs.green) > 10) {
			lfx->SetOneLFXColor(map.devid, map.lightid, &fin.ci);
			lfx->Update();
			Sleep(50);
			//if (AlienFX_SDK::Functions::IsDeviceReady()) {
			//	AlienFX_SDK::Functions::SetColor(map.lightid + 1, fin.cs.red, fin.cs.blue, fin.cs.green);
				updates[i].color = fin;
			//}
		}
	}
	//lastLights = i;
	//AlienFX_SDK::Functions::UpdateColors();
	return 0;
}

