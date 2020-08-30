#include "DaramCam.h"

#include <dxgi.h>
#include <dxgi1_2.h>

#include "External Libraries\DXGIManager\DXGIManager.h"

#pragma intrinsic(memcpy)

class DCDXGIScreenCapturer : public DCScreenCapturer
{
public:
	DCDXGIScreenCapturer ( DCDXGIScreenCapturerRange range );
	virtual ~DCDXGIScreenCapturer ();

public:
	virtual void Capture ( bool reverse = false ) noexcept;
	virtual DCBitmap * GetCapturedBitmap () noexcept;

private:
	DXGIManager* dxgiManager;
	DCBitmap capturedBitmap;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DARAMCAM_EXPORTS DCScreenCapturer * DCCreateDXGIScreenCapturer ( DCDXGIScreenCapturerRange range )
{
	return new DCDXGIScreenCapturer ( range );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DCDXGIScreenCapturer::DCDXGIScreenCapturer ( DCDXGIScreenCapturerRange range )
	: capturedBitmap ( 0, 0 )
{
	dxgiManager = new DXGIManager;

	CaptureSource captureSource = CSDesktop;
	switch ( range )
	{
	case DCDXGIScreenCapturerRange_MainMonitor: captureSource = CSMonitor1; break;
	case DCDXGIScreenCapturerRange_SubMonitors: captureSource = CSMonitor2; break;
	}
	dxgiManager->SetCaptureSource ( captureSource );

	RECT outputRect;
	dxgiManager->GetOutputRect ( outputRect );
	capturedBitmap.SetIsDirectMode(false);
	capturedBitmap.Resize ( outputRect.right - outputRect.left, outputRect.bottom - outputRect.top, 4 );
}

DCDXGIScreenCapturer::~DCDXGIScreenCapturer ()
{
	delete dxgiManager;
}

void DCDXGIScreenCapturer::Capture ( bool reverse ) noexcept
{
	int cnt;
	for (cnt = 0; cnt < 10 && FAILED(dxgiManager->GetOutputBits(capturedBitmap.GetByteArray(), reverse)); cnt++)
		Sleep(20);
	if (cnt == 10)
		dxgiManager->SetCaptureSource(dxgiManager->GetCaptureSource());
}

DCBitmap * DCDXGIScreenCapturer::GetCapturedBitmap () noexcept { return &capturedBitmap; }