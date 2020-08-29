#include "DaramCam.h"

#pragma comment ( lib, "windowscodecs.lib" )
#pragma comment ( lib, "shlwapi.lib" )
#pragma comment ( lib, "dxgi.lib" )
#pragma comment ( lib, "d3d9.lib" )
#pragma comment ( lib, "OpenGL32.lib" )

#include <Psapi.h>
#pragma comment ( lib, "Kernel32.lib" )
#pragma comment ( lib, "Psapi.lib" )

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment ( lib, "GdiPlus.lib" )

ULONG_PTR g_gdiplusToken;
DARAMCAM_EXPORTS IWICImagingFactory * g_piFactory;

void DCStartup ()
{
#if _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	CoInitializeEx ( NULL, COINIT_APARTMENTTHREADED );

	//GdiplusStartupInput gdiplusStartupInput;
	//GdiplusStartup ( &g_gdiplusToken, &gdiplusStartupInput, NULL );

	//CoCreateInstance ( CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, ( LPVOID* ) &g_piFactory );
}

void DCShutdown ()
{
	if ( g_piFactory )
		g_piFactory->Release ();

	//GdiplusShutdown ( g_gdiplusToken );

	CoUninitialize ();
}

DARAMCAM_EXPORTS void DCGetProcesses ( DWORD * buffer, unsigned * bufferSize )
{
	EnumProcesses ( buffer, sizeof ( DWORD ) * 4096, ( DWORD* ) bufferSize );
}

DARAMCAM_EXPORTS void DCGetProcessName ( DWORD pId, char * nameBuffer, unsigned nameBufferSize )
{
	HANDLE hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pId );
	if ( hProcess != nullptr )
	{
		HMODULE hModule;
		DWORD cbNeeded;
		if ( EnumProcessModules ( hProcess, &hModule, sizeof ( HMODULE ), &cbNeeded ) )
			GetModuleBaseNameA ( hProcess, hModule, nameBuffer, nameBufferSize );
	}
	CloseHandle ( hProcess );
}

DARAMCAM_EXPORTS HWND DCGetActiveWindowFromProcess ( DWORD pId )
{
	struct ENUMWINDOWPARAM { DWORD process; HWND returnHWND; } lParam = { pId, 0 };
	EnumWindows ( [] ( HWND hWnd, LPARAM lParam )
	{
		DWORD pId;
		ENUMWINDOWPARAM * param = ( ENUMWINDOWPARAM* ) lParam;
		GetWindowThreadProcessId ( hWnd, &pId );
		if ( param->process != pId || !( GetWindow ( hWnd, GW_OWNER ) == 0 && IsWindowVisible ( hWnd ) ) || GetWindowLong(hWnd, WS_CHILD ) )
			return 1;

		RECT cr;
		GetClientRect ( hWnd, &cr );
		if ( cr.right - cr.left == 0 || cr.bottom - cr.top == 0 )
			return 1;

		bool hasChild = false;
		EnumChildWindows ( hWnd, [] ( HWND hWnd, LPARAM lParam )
		{
			if ( GetWindow ( hWnd, GW_OWNER ) == 0 && IsWindowVisible ( hWnd ) )
				return 1;
			RECT cr;
			GetClientRect ( hWnd, &cr );
			if ( cr.right - cr.left == 0 || cr.bottom - cr.top == 0 )
				return 1;

			bool * hasChild = ( bool* ) lParam;
			*hasChild = true;
			return 0;
		}, ( LPARAM ) &hasChild );

		if ( !param->returnHWND )
			param->returnHWND = hWnd;

		if ( hasChild )
			return 1;

		param->returnHWND = hWnd;

		return 0;
	}, ( LPARAM ) &lParam );
	return lParam.returnHWND;
}

DARAMCAM_EXPORTS BSTR DCGetDeviceName ( IMMDevice * pDevice )
{
	IPropertyStore * propStore;
	pDevice->OpenPropertyStore ( STGM_READ, &propStore );
	PROPVARIANT pv;
	propStore->GetValue ( { { 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 }, 14 }, &pv );
	propStore->Release ();
	return pv.bstrVal;
}

DARAMCAM_EXPORTS double DCGetCurrentTime ()
{
	LARGE_INTEGER performanceFrequency, getTime;
	QueryPerformanceFrequency ( &performanceFrequency );
	QueryPerformanceCounter ( &getTime );
	return ( getTime.QuadPart / ( double ) performanceFrequency.QuadPart );
}

DARAMCAM_EXPORTS void DCGetMultimediaDevices ( std::vector<CComPtr<IMMDevice>>& devices )
{
	IMMDeviceEnumerator *pEnumerator;
	IMMDeviceCollection * pCollection;

	CoCreateInstance ( CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, ( void** ) &pEnumerator );
	pEnumerator->EnumAudioEndpoints ( eCapture, DEVICE_STATE_ACTIVE, &pCollection );

	UINT collectionCount;
	pCollection->GetCount ( &collectionCount );
	for ( UINT i = 0; i < collectionCount; ++i )
	{
		IMMDevice* pDevice;
		pCollection->Item ( i, &pDevice );
		pDevice->AddRef ();
		devices.push_back ( pDevice );
	}

	pCollection->Release ();
	pEnumerator->Release ();
}
