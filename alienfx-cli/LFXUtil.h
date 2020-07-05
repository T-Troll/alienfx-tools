// Taken from UniLight by HunterZ

#pragma once

#include "alienfx-cli.h"

// utilities for working with Dell LightFX/AlienFX API
namespace LFXUtil
{
	// class for working with Dell LightFX/AlienFX API
	// a class is used here because we need to call cleanup functions on program exit
	// lazy initialization is used, so it's safe to instantiate this at any point
	class LFXUtilC
	{
		public:
			ResultT InitLFX();
			void Release();
			virtual ~LFXUtilC();

			// set LFX color to given RGB value
			// returns true on success, false on failure
			ResultT SetLFXColor(unsigned zone, unsigned char red, unsigned char green, unsigned char blue, unsigned char br);
			ResultT SetOneLFXColor(unsigned dev, unsigned light, unsigned char red, unsigned char green, unsigned char blue, unsigned char br);
			ResultT SetLFXAction(unsigned action, unsigned dev, unsigned light, unsigned char red, unsigned char green, unsigned char blue, unsigned char br,
				unsigned char r2, unsigned char g2, unsigned char b2, unsigned char br2);
			ResultT SetLFXZoneAction(unsigned action, unsigned zone, unsigned char red, unsigned char green, unsigned char blue, unsigned char br,
				unsigned char r2, unsigned char g2, unsigned char b2, unsigned char br2);
			ResultT GetStatus();
	};
}
