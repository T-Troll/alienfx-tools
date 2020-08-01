#include "FXHelper.h"

FXHelper::FXHelper(ConfigHandler* conf) {
	//freq = freqp;
	lfx = conf->lfx;
	config = conf;
	//lastUpdate = GetTickCount64();
	done = 0;
	stopped = 0;
	lastLights = 0;
	for (unsigned i = 0; i < 50; i++)
		updates[i].lastUpdate = 0;
	lfx->InitLFX();
	lfx->FillInfo();
};
FXHelper::~FXHelper() {
};
void FXHelper::StartFX() {
	//done = 0;
	//stopped = 0;
	lfx->Reset();
	lfx->Update();
};
void FXHelper::StopFX() {
	done = 1;
	//while (!stopped)
	//	Sleep(100);
	lfx->Reset();
	lfx->Update();
	lfx->Release();
};

int FXHelper::Refresh(UCHAR* img)
{
	unsigned i = 0;
	for (i = 0; i < config->mappings.size(); i++) {
		mapping map = config->mappings[i];
		//double power = 0.0;
		Colorcode fin = { 0 };
		//from.ci = map.colorfrom.ci; to.ci = map.colorto.ci;
		// here need to check less bars...
		for (int j = 0; j < map.map.size(); j++) {
			fin.cs.red += img[4 * map.map[j]];
			fin.cs.green += img[4 * map.map[j] +1];
			fin.cs.blue += img[4 * map.map[j] +2 ];
			fin.cs.brightness += img[4 * map.map[j] +3];
		}
		if (map.map.size() > 0) {
			fin.cs.red /= map.map.size();
			fin.cs.green /= map.map.size();
			fin.cs.blue /= map.map.size();
			fin.cs.brightness /= map.map.size();
		}
		updates[i].color = fin;
		updates[i].devid = map.devid;
		updates[i].lightid = map.lightid;
	}
	lastLights = i;
	return 0;
}

int FXHelper::UpdateLights() {
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
}
