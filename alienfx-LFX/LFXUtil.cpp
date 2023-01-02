// UniLight by HunterZ

#include "LFXUtil.h"
#include "LFX2.h"
#include <wtypes.h>
#include <tchar.h>
#include <string>

namespace
{
	HINSTANCE hLibrary;
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
			return -1;

		// Dell is stupid and forces us to manually load the DLL and bind its functions
		hLibrary = LoadLibrary(_T(LFX_DLL_NAME));
		if (!hLibrary)
			return 0;

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

		int res = _LFX_Initialize();

		if (res == LFX_FAILURE)
			return 1;
		if (res == LFX_ERROR_NODEVS)
			return 2;

		initialized = true;
		return -1;
	}

	void LFXUtilC::Release()
	{
		_LFX_Release();
	}

	int LFXUtilC::Update()
	{
		if (_LFX_Update() != LFX_SUCCESS)
			return 0;

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

	int LFXUtilC::SetTempo(unsigned tempo) {
		if (_LFX_SetTiming(tempo) != LFX_SUCCESS)
			return 0;
		return 1;
	}

	int LFXUtilC::SetLFXColor(unsigned zone, unsigned color)
	{
		if (_LFX_Light(zone, color) != LFX_SUCCESS)
			return 0;
		return 1;
	}

	int LFXUtilC::SetLFXZoneAction(unsigned action, unsigned zone, unsigned color, unsigned color2)
	{
		// Set all lights to color

		if (_LFX_ActionColorEx(zone, action, color, color2) != LFX_SUCCESS)
			return 0;

		return 1;
	}

	int LFXUtilC::SetOneLFXColor(unsigned dev, unsigned light, unsigned color)
	{
		// perform lazy initialization
		// this should support a device being plugged in after the program has already started running

		if (_LFX_SetLightColor(dev, light, (PLFX_COLOR)&color) != LFX_SUCCESS)
			return 0;

		return 1;
	}

	int LFXUtilC::SetLFXAction(unsigned action, unsigned dev, unsigned light, unsigned color, unsigned color2) {

		if (_LFX_SetLightActionColorEx(dev, light, action, (PLFX_COLOR)&color, (PLFX_COLOR)&color2) != LFX_SUCCESS)
			return 0;

		return 0;

	}

	int LFXUtilC::GetStatus()
	{

		char vdesc[256];

		if (!_LFX_GetVersion(vdesc, 256))
			printf("Dell API version %s detected\n", vdesc);
		else {
			printf("Old Dell API detected\n");
		}

		Update();

		unsigned numDev;

		_LFX_GetNumDevices(&numDev);

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

			printf("Device #%d - %s (%s), %d lights\n", i, desc, type.c_str(), numLights);
			for (unsigned j = 0; j < numLights; j++) {
				LFX_COLOR color{0}; LFX_POSITION pos{0};
				_LFX_GetLightColor(i, j, &color),
				_LFX_GetLightLocation(i, j, &pos);
				printf("  Light #%d - %s, Position (%d,%d,%d), Color (%d,%d,%d,%d)\n", j,
				        (_LFX_GetLightDescription(i, j, desc, 256) ? "Unknown" : desc),
						pos.x, pos.y, pos.z,
						color.red, color.green, color.blue, color.brightness);
			}

		}

		return 1;
	}

	int LFXUtilC::FillInfo()
	{

		_LFX_GetNumDevices(&numDev);

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

		return 1;
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
