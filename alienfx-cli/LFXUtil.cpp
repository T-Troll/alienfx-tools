// UniLight by HunterZ

#include "LFXUtil.h"
#include "LFX2.h"
#include <tchar.h>
#include <windows.h>
#include <iostream>

namespace
{
	HINSTANCE hLibrary;
	HINSTANCE cLibrary;
	LFX2INITIALIZE _LFX_Initialize;
	LFX2RELEASE _LFX_Release;
	LFX2RESET _LFX_Reset;
	LFX2UPDATE _LFX_Update;
	LFX2UPDATEDEFAULT _LFX_UpdateDefault;
	LFX2GETNUMDEVICES _LFX_GetNumDevices;
	LFX2GETDEVDESC _LFX_GetDeviceDescription;
	LFX2GETNUMLIGHTS _LFX_GetNumLights;
	LFX2GETLIGHTDESC _LFX_GetLightDescription;
	LFX2GETLIGHTLOC _LFX_GetLightLocation;
	LFX2GETLIGHTCOL _LFX_GetLightColor;
	LFX2SETLIGHTCOL _LFX_SetLightColor;
	LFX2LIGHT _LFX_Light;
	LFX2SETLIGHTACTIONCOLOR _LFX_SetLightActionColor;
	LFX2SETLIGHTACTIONCOLOREX _LFX_SetLightActionColorEx;
	LFX2ACTIONCOLOR _LFX_ActionColor;
	LFX2ACTIONCOLOREX _LFX_ActionColorEx;
	LFX2SETTIMING _LFX_SetTiming;
	LFX2GETVERSION _LFX_GetVersion;

	bool initialized(false);

}

namespace LFXUtil
{

	int LFXUtilC::InitLFX()
	{
		if (initialized)
			return -1;// ResultT(true, _T("Already initialized"));

		// Dell is stupid and forces us to manually load the DLL and bind its functions
		hLibrary = LoadLibrary(_T(LFX_DLL_NAME));
		if (!hLibrary)
			return 0;// ResultT(false, _T("LoadLibrary() failed"));

		_LFX_Initialize = (LFX2INITIALIZE)GetProcAddress(hLibrary, LFX_DLL_INITIALIZE);
		_LFX_Release = (LFX2RELEASE)GetProcAddress(hLibrary, LFX_DLL_RELEASE);
		_LFX_Reset = (LFX2RESET)GetProcAddress(hLibrary, LFX_DLL_RESET);
		_LFX_Update = (LFX2UPDATE)GetProcAddress(hLibrary, LFX_DLL_UPDATE);
		_LFX_UpdateDefault = (LFX2UPDATEDEFAULT)GetProcAddress(hLibrary, LFX_DLL_UPDATEDEFAULT);
		_LFX_GetNumDevices = (LFX2GETNUMDEVICES)GetProcAddress(hLibrary, LFX_DLL_GETNUMDEVICES);
		_LFX_GetDeviceDescription = (LFX2GETDEVDESC)GetProcAddress(hLibrary, LFX_DLL_GETDEVDESC);
		_LFX_GetNumLights = (LFX2GETNUMLIGHTS)GetProcAddress(hLibrary, LFX_DLL_GETNUMLIGHTS);
		_LFX_GetLightDescription = (LFX2GETLIGHTDESC)GetProcAddress(hLibrary, LFX_DLL_GETLIGHTDESC);
		_LFX_GetLightLocation = (LFX2GETLIGHTLOC)GetProcAddress(hLibrary, LFX_DLL_GETLIGHTLOC);
		_LFX_GetLightColor = (LFX2GETLIGHTCOL)GetProcAddress(hLibrary, LFX_DLL_GETLIGHTCOL);
		_LFX_SetLightColor = (LFX2SETLIGHTCOL)GetProcAddress(hLibrary, LFX_DLL_SETLIGHTCOL);
		_LFX_Light = (LFX2LIGHT)GetProcAddress(hLibrary, LFX_DLL_LIGHT);
		_LFX_SetLightActionColor = (LFX2SETLIGHTACTIONCOLOR)GetProcAddress(hLibrary, LFX_DLL_SETLIGHTACTIONCOLOR);
		_LFX_SetLightActionColorEx = (LFX2SETLIGHTACTIONCOLOREX)GetProcAddress(hLibrary, LFX_DLL_SETLIGHTACTIONCOLOREX);
		_LFX_ActionColor = (LFX2ACTIONCOLOR)GetProcAddress(hLibrary, LFX_DLL_ACTIONCOLOR);
		_LFX_ActionColorEx = (LFX2ACTIONCOLOREX)GetProcAddress(hLibrary, LFX_DLL_ACTIONCOLOREX);
		_LFX_SetTiming = (LFX2SETTIMING)GetProcAddress(hLibrary, LFX_DLL_SETTIMING);
		_LFX_GetVersion = (LFX2GETVERSION)GetProcAddress(hLibrary, LFX_DLL_GETVERSION);

		if (_LFX_Initialize() != LFX_SUCCESS)
			return 1;// ResultT(false, _T("_LFX_Initialize() failed"));

		initialized = true;
		return -1;// ResultT(true, _T("InitFX() success"));
	}

	void LFXUtilC::Release()
	{
		_LFX_Release();
	}

	int LFXUtilC::Update()
	{
		if (_LFX_Update() != LFX_SUCCESS)
			return 0;// ResultT(false, _T("_LFX_Update() failed"));

		return 1;
	}

	int LFXUtilC::Reset() {
		if (_LFX_Reset() != LFX_SUCCESS)
			return 0;
		return 1;
	}

	unsigned LFXUtilC::GetNumDev() {
		return numDev;
	}

	LFXUtilC::~LFXUtilC()
	{
		if (initialized)
		{
			_LFX_Release();
			FreeLibrary(hLibrary);
			initialized = false;
		}
	}

	int LFXUtilC::SetLFXColor(unsigned zone, unsigned color)
	{
		// perform lazy initialization
		// this should support a device being plugged in after the program has already started running
		// abort if initialization fails
		//const ResultT& result(InitLFX());
		//if (!result.first)
		//	return result;

		// Reset the state machine and await light settings
		//if (_LFX_Reset() != LFX_SUCCESS)
		//	return ResultT(false, _T("_LFX_Reset() failed"));

		// Set all lights to color
		/*static ColorU color;
		color.cs.red = red;
		color.cs.green = green;
		color.cs.blue = blue;
		color.cs.brightness = br;*/
		if (_LFX_Light(zone, color) != LFX_SUCCESS)
			return 0;// ResultT(false, _T("_LFX_Light() failed"));

		// Update the state machine, which causes the physical color change
		//if (_LFX_Update() != LFX_SUCCESS)
		//	return 0;// ResultT(false, _T("_LFX_Update() failed"));

		return 1;// ResultT(true, _T("SetLFXColor() success"));
	}

	int LFXUtilC::SetLFXZoneAction(unsigned action, unsigned zone, unsigned color, unsigned color2)
	{
		// Set all lights to color
		/*static ColorU color, color2;
		color.cs.red = red;
		color.cs.green = green;
		color.cs.blue = blue;
		color.cs.brightness = br;
		color2.cs.red = r2;
		color2.cs.green = g2;
		color2.cs.blue = b2;
		color2.cs.brightness = br2;*/

		if (_LFX_ActionColorEx(zone, action, color, color2) != LFX_SUCCESS)
			return 0;// ResultT(false, _T("_LFX_Light() failed"));

		// Update the state machine, which causes the physical color change
		//if (_LFX_Update() != LFX_SUCCESS)
		//	return 0;// ResultT(false, _T("_LFX_Update() failed"));

		return 1;// ResultT(true, _T("SetLFXColor() success"));
	}

	int LFXUtilC::SetOneLFXColor(unsigned dev, unsigned light, unsigned *color)
	{
		// perform lazy initialization
		// this should support a device being plugged in after the program has already started running
		// abort if initialization fails
		//const ResultT& result(InitLFX());
		//if (!result.first)
		//	return result;

		// Reset the state machine and await light settings
		//if (_LFX_Reset() != LFX_SUCCESS)
		//	return ResultT(false, _T("_LFX_Reset() failed"));

		// Set one light to color
		/*static LFX_COLOR color;
		color.red = red;
		color.green = green;
		color.blue = blue;
		color.brightness = br;*/
		if (_LFX_SetLightColor(dev, light, (PLFX_COLOR)color) != LFX_SUCCESS)
			return 0;// ResultT(false, _T("_LFX_Light() failed"));

		// Update the state machine, which causes the physical color change
		//if (_LFX_Update() != LFX_SUCCESS)
		//	return 0;//ResultT(false, _T("_LFX_Update() failed"));

		return 1;//ResultT(true, _T("SetLFXColor() success"));
	}

	int LFXUtilC::SetLFXAction(unsigned action, unsigned dev, unsigned light, unsigned *color, unsigned *color2) {
		/*static LFX_COLOR color, color2;
		color.red = red;
		color.green = green;
		color.blue = blue;
		color.brightness = br;
		color2.red = r2;
		color2.green = g2;
		color2.blue = b2;
		color2.brightness = br2;*/

		if (_LFX_SetLightActionColorEx(dev, light, action, (PLFX_COLOR)color, (PLFX_COLOR)color2) != LFX_SUCCESS)
			return 0;//ResultT(false, _T("_LFX_Light() failed"));

		// Update the state machine, which causes the physical color change
		//if (_LFX_Update() != LFX_SUCCESS)
		//	return 0;//ResultT(false, _T("_LFX_Update() failed"));

		return 0;//ResultT(true, _T("SetLFXColor() success"));

	}

	int LFXUtilC::GetStatus()
	{

		char vdesc[256];

		if (_LFX_GetVersion(vdesc, 256) == 0)
			std::cout << "API version " << vdesc << " detected\n";
		else {
			std::cout << "Old API detected";
//			return 0;//ResultT(false, _T("No API detected"));
		}

		Update();

		unsigned numDev;

		_LFX_GetNumDevices(&numDev);
		std::cout << "Devices found: " << numDev << "\n";

		for (unsigned i = 0; i < numDev; i++) {
			char desc[256]; unsigned char id; unsigned numLights;
			_LFX_GetDeviceDescription(i, desc, 256, &id);
			_LFX_GetNumLights(i, &numLights);

			std::string type = "Unknown";

			switch (id) {
				case LFX_DEVTYPE_DESKTOP: type = "Desktop"; break;
				case LFX_DEVTYPE_NOTEBOOK: type = "Notebook"; break;
				case LFX_DEVTYPE_DISPLAY: type = "Display"; break;
				case LFX_DEVTYPE_MOUSE: type = "Mouse"; break;
				case LFX_DEVTYPE_KEYBOARD: type = "Keyboard"; break;
				case LFX_DEVTYPE_GAMEPAD: type = "Gamepad"; break;
				case LFX_DEVTYPE_SPEAKER: type = "Speaker"; break;
				case LFX_DEVTYPE_OTHER: type = "Other"; break;
			}

			std::cout << "Device #" << i << ", Name: " << desc << ", Type: " << type << ", Lights: " << numLights << "\n";
			for (unsigned j = 0; j < numLights; j++) {
				char ldesc[256]; LFX_COLOR color; LFX_POSITION pos;
				std::cout << "  Light #" << j << ", Name: ";
				if (_LFX_GetLightDescription(i, j, ldesc, 256) == 0)
					std::cout << ldesc << ", Position ";
				else
					std::cout << "Unknown, Position ";
				if (_LFX_GetLightLocation(i, j, &pos) == 0)
					std::cout << "(" << (unsigned)pos.x << ", " << (unsigned)pos.y << ", " << (unsigned)pos.z << "), Color ";
				else
					std::cout << "Unknown, Color ";
				if (_LFX_GetLightColor(i, j, &color) == 0)
					std::cout << "(" << (unsigned)color.red << "," << (unsigned)color.green << "," << (unsigned)color.blue
					<< "), Brightness: " << (unsigned)color.brightness;
				else
					std::cout << "Unknown";
				std::cout << "\n";
			}

		}

		return 1;//ResultT(true, _T("Ok"));
	}

	int LFXUtilC::FillInfo()
	{

		_LFX_GetNumDevices(&numDev);
		//std::cout << "Devices found: " << numDev << "\n";

		for (unsigned i = 0; i < numDev; i++) {
			deviceinfo d;
			_LFX_GetDeviceDescription(i, d.desc, 256, &d.type);
			_LFX_GetNumLights(i, &d.lights);
			d.id = i;
			devlist.push_back(d);

			std::string type = "Unknown";

			std::vector<lightinfo> d_l;

			for (unsigned j = 0; j < d.lights; j++) {
				lightinfo l;
				l.id = j;
				_LFX_GetLightDescription(i, j, l.desc, 256);
				_LFX_GetLightLocation(i, j, &l.pos);
				d_l.push_back(l);
			}
			lightlist.push_back(d_l);

		}

		return 1;//ResultT(true, _T("Ok"));
	}

	deviceinfo *LFXUtilC::GetDevInfo(unsigned id) {
		if (id < numDev)
			return &devlist[id];
		else
			return NULL;
	}

	lightinfo *LFXUtilC::GetLightInfo(unsigned id, unsigned lid) {
		if (id < numDev && lid < lightlist[id].size())
			return &(lightlist[id][lid]);
		else
			return NULL;
	}

}
