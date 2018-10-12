// UniLight by HunterZ

#include "LFXUtil.h"

#include "LFX2.h"
#include <tchar.h>
#include <windows.h>

namespace
{
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

	ResultT InitLFX()
	{
		if (initialized)
			return ResultT(true, _T("Already initialized"));

		// Dell is stupid and forces us to manually load the DLL and bind its functions
		hLibrary = LoadLibrary(_T(LFX_DLL_NAME));
		if (!hLibrary)
			return ResultT(false, _T("LoadLibrary() failed"));

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
			return ResultT(false, _T("_LFX_Initialize() failed"));

		initialized = true;
		return ResultT(true, _T("InitFX() success"));
	}
}

namespace LFXUtil
{
	LFXUtilC::~LFXUtilC()
	{
		if (initialized)
		{
			_LFX_Release();
			FreeLibrary(hLibrary);
			initialized = false;
		}
	}

	ResultT LFXUtilC::SetLFXColor(unsigned char red, unsigned char green, unsigned char blue)
	{
		// perform lazy initialization
		// this should support a device being plugged in after the program has already started running
		// abort if initialization fails
		const ResultT& result(InitLFX());
		if (!result.first)
			return result;

		// Reset the state machine and await light settings
		if (_LFX_Reset() != LFX_SUCCESS)
			return ResultT(false, _T("_LFX_Reset() failed"));

		// Set all lights to color
		static ColorU color;
		color.cs.red = red;
		color.cs.green = green;
		color.cs.blue = blue;
		color.cs.brightness = 0xFF;
		if (_LFX_Light(LFX_ALL, color.ci) != LFX_SUCCESS)
			return ResultT(false, _T("_LFX_Light() failed"));

		// Update the state machine, which causes the physical color change
		if (_LFX_Update() != LFX_SUCCESS)
			return ResultT(false, _T("_LFX_Update() failed"));

		return ResultT(true, _T("SetLFXColor() success"));
	}
}
