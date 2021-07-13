// Taken from UniLight by HunterZ

#pragma once
#include <vector>
#include "LFXDecl.h"

struct deviceinfo {
	char desc[256];
	unsigned id;
	unsigned char type;
	unsigned lights;
};

struct lightinfo {
	unsigned id;
	LFX_POSITION pos;
	char desc[256];
};

struct ColorS
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char brightness;
};

union ColorU
{
	struct ColorS cs;
	unsigned int ci;
};

// utilities for working with Dell LightFX/AlienFX API
namespace LFXUtil
{
	// class for working with Dell LightFX/AlienFX API
	// a class is used here because we need to call cleanup functions on program exit
	// lazy initialization is used, so it's safe to instantiate this at any point
	class LFXUtilC
	{
		private:
			unsigned numDev = 0;
			std::vector<deviceinfo> devlist;
			std::vector<std::vector<lightinfo>> lightlist;
		public:
			int InitLFX();
			void Release();
			virtual ~LFXUtilC();

			int Update();
			int Reset();

			// returns true on success, false on failure
			int SetLFXColor(unsigned zone, unsigned color);
			int SetOneLFXColor(unsigned dev, unsigned light, unsigned *color);
			int SetLFXAction(unsigned action, unsigned dev, unsigned light, unsigned *color,
				unsigned *color2);
			int SetLFXZoneAction(unsigned action, unsigned zone, unsigned color,
				unsigned color2);
			int SetTempo(unsigned tempo);
			int GetStatus();

			int FillInfo();
			unsigned GetNumDev();
			deviceinfo *GetDevInfo(unsigned devId);
			lightinfo *GetLightInfo(unsigned devId, unsigned lightId);
	};
}
