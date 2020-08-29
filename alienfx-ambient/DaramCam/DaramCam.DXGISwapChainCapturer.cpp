#include "DaramCam.h"

#include <dxgi.h>

class DCDXGISwapChainCapturer : public DCScreenCapturer
{
public:
	DCDXGISwapChainCapturer ( IUnknown * dxgiSwapChain );
	virtual ~DCDXGISwapChainCapturer ();

public:
	virtual void Capture ( bool reverse = false ) noexcept;
	virtual DCBitmap * GetCapturedBitmap () noexcept;

private:
	IDXGISwapChain * dxgiSwapChain;
	DCBitmap capturedBitmap;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DARAMCAM_EXPORTS DCScreenCapturer * DCCreateDXGISwapChainCapturer ( IUnknown * dxgiSwapChain )
{
	return new DCDXGISwapChainCapturer ( dxgiSwapChain );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DCDXGISwapChainCapturer::DCDXGISwapChainCapturer ( IUnknown * dxgiSwapChain )
	: capturedBitmap ( 0, 0 )
{
	dxgiSwapChain->QueryInterface<IDXGISwapChain> ( &this->dxgiSwapChain );

	DXGI_SWAP_CHAIN_DESC desc;
	this->dxgiSwapChain->GetDesc ( &desc );
	capturedBitmap.Resize ( desc.BufferDesc.Width, desc.BufferDesc.Height, 4 );
}

DCDXGISwapChainCapturer::~DCDXGISwapChainCapturer ()
{
	dxgiSwapChain->Release ();
}

void DCDXGISwapChainCapturer::Capture ( bool reverse ) noexcept
{
	IDXGISurface * surface;
	dxgiSwapChain->GetBuffer ( 0, __uuidof ( IDXGISurface ), ( void ** ) &surface );
	DXGI_MAPPED_RECT locked;
	surface->Map ( &locked, 0 );
	if ( !reverse )
		memcpy ( capturedBitmap.GetByteArray (), locked.pBits, capturedBitmap.GetByteArraySize () );
	else
	{
		unsigned height = capturedBitmap.GetHeight (), stride = capturedBitmap.GetStride ();
		for ( unsigned y = 0; y < height; ++y )
			memcpy ( capturedBitmap.GetByteArray () + ( y * stride ), locked.pBits + ( ( height - y - 1 ) * stride ), stride );
	}
	surface->Unmap ();
}

DCBitmap * DCDXGISwapChainCapturer::GetCapturedBitmap () noexcept { return &capturedBitmap; }
