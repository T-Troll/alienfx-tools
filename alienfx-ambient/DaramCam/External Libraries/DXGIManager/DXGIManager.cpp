#include "DXGIManager.h"

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment ( lib, "d3d11.lib" )

#pragma intrinsic(memcpy) 

DXGIPointerInfo::DXGIPointerInfo(BYTE* pPointerShape, UINT uiPointerShapeBufSize, DXGI_OUTDUPL_FRAME_INFO fi, DXGI_OUTDUPL_POINTER_SHAPE_INFO psi)
	:	m_pPointerShape(pPointerShape),
		m_uiPointerShapeBufSize(uiPointerShapeBufSize),
		m_FI(fi),
		m_PSI(psi)
{
}

DXGIPointerInfo::~DXGIPointerInfo()
{
	if(m_pPointerShape)
	{
		delete [] m_pPointerShape;
	}
}

BYTE* DXGIPointerInfo::GetBuffer()
{
	return m_pPointerShape;
}

UINT DXGIPointerInfo::GetBufferSize()
{
	return m_uiPointerShapeBufSize;
}

DXGI_OUTDUPL_FRAME_INFO& DXGIPointerInfo::GetFrameInfo()
{
	return m_FI;
}

DXGI_OUTDUPL_POINTER_SHAPE_INFO& DXGIPointerInfo::GetShapeInfo()
{
	return m_PSI;
}

DXGIOutputDuplication::DXGIOutputDuplication(IDXGIAdapter1* pAdapter,
	ID3D11Device* pD3DDevice,
	ID3D11DeviceContext* pD3DDeviceContext,
	IDXGIOutput1* pDXGIOutput1,
	IDXGIOutputDuplication* pDXGIOutputDuplication)
	:	m_Adapter(pAdapter),
		m_D3DDevice(pD3DDevice),
		m_D3DDeviceContext(pD3DDeviceContext),
		m_DXGIOutput1(pDXGIOutput1),
		m_DXGIOutputDuplication(pDXGIOutputDuplication)
{
}

HRESULT DXGIOutputDuplication::GetDesc(DXGI_OUTPUT_DESC& desc)
{
	return m_DXGIOutput1->GetDesc(&desc);
}

HRESULT DXGIOutputDuplication::AcquireNextFrame(IDXGISurface1** pDXGISurface, DXGIPointerInfo*& pDXGIPointer)
{
	DXGI_OUTDUPL_FRAME_INFO fi;
	CComPtr<IDXGIResource> spDXGIResource;
	HRESULT hr = m_DXGIOutputDuplication->AcquireNextFrame(20, &fi, &spDXGIResource);
	if(FAILED(hr))
	{
		return hr;
	}

	CComQIPtr<ID3D11Texture2D> spTextureResource = spDXGIResource;

	D3D11_TEXTURE2D_DESC desc;
	spTextureResource->GetDesc ( &desc );

	D3D11_TEXTURE2D_DESC texDesc = { 0, };
	texDesc.Width = desc.Width;
	texDesc.Height = desc.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.Format = desc.Format;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.MiscFlags = 0;

	CComPtr<ID3D11Texture2D> m_spD3D11Texture2D;
	hr = m_D3DDevice->CreateTexture2D ( &texDesc, NULL, &m_spD3D11Texture2D );
	if ( FAILED ( hr ) )
		return hr;

	m_D3DDeviceContext->CopyResource(m_spD3D11Texture2D, spTextureResource);

	CComQIPtr<IDXGISurface1> spDXGISurface = m_spD3D11Texture2D;

	*pDXGISurface = spDXGISurface.Detach();
	
	// Updating mouse pointer, if visible
	if(fi.PointerPosition.Visible)
	{
		BYTE* pPointerShape = new BYTE[fi.PointerShapeBufferSize];

		DXGI_OUTDUPL_POINTER_SHAPE_INFO psi = {};
		UINT uiPointerShapeBufSize = fi.PointerShapeBufferSize;
		hr = m_DXGIOutputDuplication->GetFramePointerShape(uiPointerShapeBufSize, pPointerShape, &uiPointerShapeBufSize, &psi);
		if(hr == DXGI_ERROR_MORE_DATA)
		{
			pPointerShape = new BYTE[uiPointerShapeBufSize];
	
			hr = m_DXGIOutputDuplication->GetFramePointerShape(uiPointerShapeBufSize, pPointerShape, &uiPointerShapeBufSize, &psi);
		}

		if(hr == S_OK)
		{
			if((psi.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME ||
			    psi.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR ||
			    psi.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR) &&
				psi.Width <= 128 && psi.Height <= 128)
			{
				// Here we can obtain pointer shape
				if(pDXGIPointer)
				{
					delete pDXGIPointer;
				}

				pDXGIPointer = new DXGIPointerInfo(pPointerShape, uiPointerShapeBufSize, fi, psi);
				
				pPointerShape = NULL;
			}

			DXGI_OUTPUT_DESC outDesc;
			GetDesc(outDesc);

			if(pDXGIPointer)
			{
				pDXGIPointer->GetFrameInfo().PointerPosition.Position.x = outDesc.DesktopCoordinates.left + fi.PointerPosition.Position.x;
				pDXGIPointer->GetFrameInfo().PointerPosition.Position.y = outDesc.DesktopCoordinates.top + fi.PointerPosition.Position.y;
			}
		}

		if(pPointerShape)
		{
			delete [] pPointerShape;
		}
	}

	return hr;
}

HRESULT DXGIOutputDuplication::ReleaseFrame()
{
	return m_DXGIOutputDuplication->ReleaseFrame();
}

bool DXGIOutputDuplication::IsPrimary()
{
	DXGI_OUTPUT_DESC outdesc;
	m_DXGIOutput1->GetDesc(&outdesc);
			
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(outdesc.Monitor, &mi);
	if(mi.dwFlags & MONITORINFOF_PRIMARY)
	{
		return true;
	}
	return false;
}

DXGIManager::DXGIManager()
	: m_pBuf ( NULL ),
	  m_pDXGIPointer ( NULL ),
	  m_bInitialized ( false ),
	  m_CaptureSource ( CSUndefined )
{
	SetRect(&m_rcCurrentOutput, 0, 0, 0, 0);
}

DXGIManager::~DXGIManager()
{

	if(m_pBuf)
	{
		delete [] m_pBuf;
		m_pBuf = NULL;
	}

	if(m_pDXGIPointer)
	{
		delete m_pDXGIPointer;
		m_pDXGIPointer = NULL;
	}
}

HRESULT DXGIManager::SetCaptureSource(CaptureSource cs)
{
	m_CaptureSource = cs;

	Init ();

	RECT rcShare = { 0, 0, 0, 0 };

	m_outputDupls.clear ();
	switch ( m_CaptureSource )
	{
	case CSMonitor1:
		{
			// Return the one with IsPrimary
			for ( auto iter = m_vOutputs.begin (); iter != m_vOutputs.end (); iter++ )
			{
				DXGIOutputDuplication& out = *iter;
				if ( out.IsPrimary () )
				{
					DXGI_OUTPUT_DESC outDesc;
					out.GetDesc ( outDesc );
					RECT rcOutCoords = outDesc.DesktopCoordinates;

					UnionRect ( &rcShare, &rcShare, &rcOutCoords );

					m_outputDupls.push_back ( &out );
					break;
				}
			}
		}
		break;

	case CSMonitor2:
		{
			// Return the first with !IsPrimary
			for ( auto iter = m_vOutputs.begin (); iter != m_vOutputs.end (); iter++ )
			{
				DXGIOutputDuplication& out = *iter;
				if ( !out.IsPrimary () )
				{
					DXGI_OUTPUT_DESC outDesc;
					out.GetDesc ( outDesc );
					RECT rcOutCoords = outDesc.DesktopCoordinates;

					UnionRect ( &rcShare, &rcShare, &rcOutCoords );

					m_outputDupls.push_back ( &out );
					break;
				}
			}
		}
		break;

	case CSDesktop:
		{
			// Return all m_outputDupls
			for ( auto iter = m_vOutputs.begin (); iter != m_vOutputs.end (); iter++ )
			{
				DXGIOutputDuplication& out = *iter;

				DXGI_OUTPUT_DESC outDesc;
				out.GetDesc ( outDesc );
				RECT rcOutCoords = outDesc.DesktopCoordinates;

				UnionRect ( &rcShare, &rcShare, &rcOutCoords );

				m_outputDupls.push_back ( &out );
			}
		}
		break;
	}

	CopyRect ( &m_rcCalcedRect, &rcShare );

	return S_OK;
}

CaptureSource DXGIManager::GetCaptureSource()
{
	return m_CaptureSource;
}

HRESULT DXGIManager::Init()
{
	if(m_bInitialized)
		return S_OK;

	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&m_spDXGIFactory1) );
	if( FAILED(hr) )
	{
		return hr;
	}

	// Getting all adapters
	vector<CComPtr<IDXGIAdapter1>> vAdapters;

	CComPtr<IDXGIAdapter1> spAdapter; 
	for(int i=0; m_spDXGIFactory1->EnumAdapters1(i, &spAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		vAdapters.push_back(spAdapter);
		spAdapter.Release();
	}

	// Iterating over all adapters to get all m_outputDupls
	for(vector<CComPtr<IDXGIAdapter1>>::iterator AdapterIter = vAdapters.begin();
		AdapterIter != vAdapters.end();
		++AdapterIter)
	{
		vector<CComPtr<IDXGIOutput>> vOutputs;

		CComPtr<IDXGIOutput> spDXGIOutput;
		for(int i=0; (*AdapterIter)->EnumOutputs(i, &spDXGIOutput) != DXGI_ERROR_NOT_FOUND; ++i)
		{ 
			DXGI_OUTPUT_DESC outputDesc;
			spDXGIOutput->GetDesc(&outputDesc);

			if(outputDesc.AttachedToDesktop)
			{
				vOutputs.push_back(spDXGIOutput);
			}

			spDXGIOutput.Release();
		}

		if(vOutputs.size() == 0)
			continue;

		// Creating device for each adapter that has the output
		CComPtr<ID3D11Device> spD3D11Device;
		CComPtr<ID3D11DeviceContext> spD3D11DeviceContext;
		D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_1;
		hr = D3D11CreateDevice((*AdapterIter), D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &spD3D11Device, &fl, &spD3D11DeviceContext);
		if( FAILED(hr) )
		{
			return hr;
		}

		for(std::vector<CComPtr<IDXGIOutput>>::iterator OutputIter = vOutputs.begin(); 
			OutputIter != vOutputs.end(); 
			++OutputIter)
		{
			CComQIPtr<IDXGIOutput1> spDXGIOutput1 = *OutputIter;
			CComQIPtr<IDXGIDevice1> spDXGIDevice = spD3D11Device;
			if(!spDXGIOutput1 || !spDXGIDevice)
				continue;

			CComPtr<IDXGIOutputDuplication> spDXGIOutputDuplication;
			hr = spDXGIOutput1->DuplicateOutput(spDXGIDevice, &spDXGIOutputDuplication);
			if( FAILED(hr) )
				continue;

			m_vOutputs.push_back(
				DXGIOutputDuplication((*AdapterIter),
					spD3D11Device,
					spD3D11DeviceContext,
					spDXGIOutput1,
					spDXGIOutputDuplication));
		}
	}

	m_bInitialized = true;

	return S_OK;
}

HRESULT DXGIManager::GetOutputRect(RECT& rc)
{
	CopyRect(&rc, &m_rcCalcedRect);
	return S_OK;
}

HRESULT DXGIManager::GetOutputBits(BYTE* pBits, bool reverse )
{
	HRESULT hr = S_OK;

	RECT rcOutput;
	hr = GetOutputRect(rcOutput);
	if( FAILED(hr) )
		return hr;

	DWORD dwOutputWidth = rcOutput.right - rcOutput.left;
	DWORD dwOutputHeight = rcOutput.bottom - rcOutput.top;

	BYTE* pBuf = pBits;

	vector<DXGIOutputDuplication*> & vOutputs = GetOutputDuplication();
	for(auto iter = vOutputs.begin(); iter != vOutputs.end(); ++iter)
	{
		DXGIOutputDuplication* out = *iter;
	
		DXGI_OUTPUT_DESC outDesc;
		hr = out->GetDesc(outDesc);
		if( FAILED(hr) )
			break;
		RECT rcOutCoords = outDesc.DesktopCoordinates;

		CComPtr<IDXGISurface1> spDXGISurface1;
		hr = out->AcquireNextFrame(&spDXGISurface1, m_pDXGIPointer);
		if( FAILED(hr) )
			break;

		DXGI_MAPPED_RECT map;
		hr = spDXGISurface1->Map(&map, DXGI_MAP_READ);
		if( FAILED(hr) )
		{
			out->ReleaseFrame ();
			break;
		}

		RECT rcDesktop = outDesc.DesktopCoordinates;
		DWORD dwWidth = rcDesktop.right - rcDesktop.left;
		DWORD dwHeight = rcDesktop.bottom - rcDesktop.top;

		OffsetRect(&rcDesktop, -rcOutput.left, -rcOutput.top);

		DWORD dwMapPitchPixels = map.Pitch/4;

		switch(outDesc.Rotation)
		{
			case DXGI_MODE_ROTATION_IDENTITY:
				{
					// Just copying
					DWORD dwStripe = dwWidth*4;
					for(DWORD i=0; i<dwHeight; ++i)
					{
						memcpy(pBuf + (rcDesktop.left + (i + rcDesktop.top)*dwOutputWidth)*4, map.pBits + (reverse ? dwHeight - i - 1 : i)*map.Pitch, dwStripe);
					}
				}
				break;
			case DXGI_MODE_ROTATION_ROTATE90:
				{
					// Rotating at 90 degrees
					DWORD* pSrc = (DWORD*)map.pBits;
					DWORD* pDst = (DWORD*)pBuf;
					for( DWORD j=0; j<dwHeight; ++j)
					{
						for( DWORD i=0; i<dwWidth; ++i)
						{
							*(pDst + (rcDesktop.left + (j + rcDesktop.top)*dwOutputWidth) + i) = *(pSrc + (reverse ? dwHeight - j - 1 : j) + dwMapPitchPixels*(dwWidth - i - 1));
						}
					}
				}
				break;
			case DXGI_MODE_ROTATION_ROTATE180:
				{
					// Rotating at 180 degrees
					DWORD* pSrc = (DWORD*)map.pBits;
					DWORD* pDst = (DWORD*)pBuf;
					for( DWORD j=0; j<dwHeight; ++j)
					{
						for( DWORD i=0; i<dwWidth; ++i)
						{
							*(pDst + (rcDesktop.left + (j + rcDesktop.top)*dwOutputWidth) + i) = *(pSrc + (dwWidth - i - 1) + dwMapPitchPixels*(reverse ? j : (dwHeight - j - 1)));
						}
					}
				}
				break;
			case DXGI_MODE_ROTATION_ROTATE270:
				{
					// Rotating at 270 degrees
					DWORD* pSrc = (DWORD*)map.pBits;
					DWORD* pDst = (DWORD*)pBuf;
					for( DWORD j=0; j<dwHeight; ++j)
					{
						for( DWORD i=0; i<dwWidth; ++i)
						{
							*(pDst + (rcDesktop.left + (j + rcDesktop.top)*dwOutputWidth) + i) = *(pSrc + (reverse ? j : (dwHeight - j - 1)) + dwMapPitchPixels*i);
						}
					}
				}
				break;
		}
		
		spDXGISurface1->Unmap();

		out->ReleaseFrame();
	}

	if(FAILED(hr))
		return hr;

	DrawMousePointer(pBuf, rcOutput, rcOutput);
	
	return hr;
}

void DXGIManager::DrawMousePointer(BYTE* pDesktopBits, RECT rcDesktop, RECT rcDest)
{
	//if(!m_pDXGIPointer)
		return;

	DWORD dwDesktopWidth = rcDesktop.right - rcDesktop.left;
	DWORD dwDesktopHeight = rcDesktop.bottom - rcDesktop.top;

	int PtrX = m_pDXGIPointer->GetFrameInfo().PointerPosition.Position.x - rcDesktop.left;
	int PtrY = m_pDXGIPointer->GetFrameInfo().PointerPosition.Position.y - rcDesktop.top;
	switch(m_pDXGIPointer->GetShapeInfo().Type)
	{
		case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
			{
				unique_ptr<Bitmap> bmpBitmap(new Bitmap( dwDesktopWidth, dwDesktopHeight, dwDesktopWidth *4, PixelFormat32bppARGB, pDesktopBits));
				unique_ptr<Graphics> graphics(Graphics::FromImage(bmpBitmap.get()));
				unique_ptr<Bitmap> bmpPointer(new Bitmap(m_pDXGIPointer->GetShapeInfo().Width, m_pDXGIPointer->GetShapeInfo().Height, m_pDXGIPointer->GetShapeInfo().Width*4, PixelFormat32bppARGB, m_pDXGIPointer->GetBuffer()));
				
				graphics->DrawImage(bmpPointer.get(), PtrX, PtrY);
			}
			break;
		case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
		case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
			{
				RECT rcPointer;
	
				if(m_pDXGIPointer->GetShapeInfo().Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME)
				{
					SetRect(&rcPointer, PtrX, PtrY, PtrX + m_pDXGIPointer->GetShapeInfo().Width, PtrY + m_pDXGIPointer->GetShapeInfo().Height/2);
				}
				else
				{
					SetRect(&rcPointer, PtrX, PtrY, PtrX + m_pDXGIPointer->GetShapeInfo().Width, PtrY + m_pDXGIPointer->GetShapeInfo().Height);
				}

				RECT rcDesktopPointer;
				IntersectRect(&rcDesktopPointer, &rcPointer, &rcDesktop);

				CopyRect(&rcPointer, &rcDesktopPointer);
				OffsetRect(&rcPointer, -PtrX, -PtrY);

				BYTE* pShapeBuffer = m_pDXGIPointer->GetBuffer();
				UINT* pDesktopBits32 = (UINT*)pDesktopBits;

				if(m_pDXGIPointer->GetShapeInfo().Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME)
				{
					for(int j = rcPointer.top, jDP = rcDesktopPointer.top;
						j<rcPointer.bottom && jDP<rcDesktopPointer.bottom; 
						++j, ++jDP)
					{
						for(int i = rcPointer.left, iDP = rcDesktopPointer.left;
							i<rcPointer.right && iDP<rcDesktopPointer.right; 
							++i, ++iDP)
						{
							BYTE Mask = 0x80 >> (i % 8);
							BYTE AndMask = pShapeBuffer[i/8 + (m_pDXGIPointer->GetShapeInfo().Pitch)*j] & Mask;
							BYTE XorMask = pShapeBuffer[i/8 + (m_pDXGIPointer->GetShapeInfo().Pitch)*(j + m_pDXGIPointer->GetShapeInfo().Height / 2)] & Mask;

							UINT AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
							UINT XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;

							pDesktopBits32[jDP*dwDesktopWidth + iDP] = (pDesktopBits32[jDP*dwDesktopWidth + iDP] & AndMask32) ^ XorMask32;
						}
					}
				}
				else
				{
					UINT* pShapeBuffer32 = (UINT*)pShapeBuffer;
					for(int j = rcPointer.top, jDP = rcDesktopPointer.top;
						j<rcPointer.bottom && jDP<rcDesktopPointer.bottom; 
						++j, ++jDP)
					{
						for(int i = rcPointer.left, iDP = rcDesktopPointer.left;
							i<rcPointer.right && iDP<rcDesktopPointer.right; 
							++i, ++iDP)
						{
							// Set up mask
							UINT MaskVal = 0xFF000000 & pShapeBuffer32[i + (m_pDXGIPointer->GetShapeInfo().Pitch/4)*j];
							if (MaskVal)
							{
								// Mask was 0xFF
								pDesktopBits32[jDP*dwDesktopWidth + iDP] = (pDesktopBits32[jDP*dwDesktopWidth + iDP] ^ pShapeBuffer32[i + (m_pDXGIPointer->GetShapeInfo().Pitch/4)*j]) | 0xFF000000;
							}
							else
							{
								// Mask was 0x00 - replacing pixel
								pDesktopBits32[jDP*dwDesktopWidth + iDP] = pShapeBuffer32[i + (m_pDXGIPointer->GetShapeInfo().Pitch/4)*j];
							}
						}
					}
				}
			}
			break;
	}
}

vector<DXGIOutputDuplication*> & DXGIManager::GetOutputDuplication()
{
	return m_outputDupls;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    int *Count = (int*)dwData;
    ++(*Count);
    return TRUE;
}

int DXGIManager::GetMonitorCount()
{
    int Count = 0;
    if (EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&Count))
        return Count;
    return -1;
}
