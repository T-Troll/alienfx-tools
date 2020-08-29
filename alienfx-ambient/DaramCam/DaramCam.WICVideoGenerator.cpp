#include "DaramCam.h"
#include "DaramCam.Internal.h"

#pragma comment ( lib, "winmm.lib" )

#include <concurrent_queue.h>
#include <ppltasks.h>

struct WICVG_CONTAINER { IWICBitmapSource * bitmapSource; UINT deltaTime; };

class DCWICVideoGenerator : public DCVideoGenerator
{
	friend DWORD WINAPI WICVG_Progress ( LPVOID vg );
	friend DWORD WINAPI WICVG_Capturer ( LPVOID vg );
public:
	DCWICVideoGenerator ( unsigned frameTick );
	virtual ~DCWICVideoGenerator ();

public:
	virtual void Begin ( IStream * stream, DCScreenCapturer * capturer );
	virtual void End ();

public:
	void SetGenerateSize ( const SIZE* size );

private:
	IStream * stream;
	DCScreenCapturer * capturer;

	HANDLE threadHandles [ 2 ];
	bool threadRunning;

	unsigned frameTick;
	const SIZE* resize;

	Concurrency::concurrent_queue<WICVG_CONTAINER> capturedQueue;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DARAMCAM_EXPORTS DCVideoGenerator * DCCreateWICVideoGenerator ( unsigned frameTick )
{
	return new DCWICVideoGenerator ( frameTick );
}

DARAMCAM_EXPORTS void DCSetSizeToWICVideoGenerator ( DCVideoGenerator * generator, const SIZE * size )
{
	dynamic_cast< DCWICVideoGenerator* >( generator )->SetGenerateSize ( size );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DCWICVideoGenerator::DCWICVideoGenerator ( unsigned _frameTick )
	: resize ( nullptr )
{
	frameTick = _frameTick;
}

DCWICVideoGenerator::~DCWICVideoGenerator () { }

DWORD WINAPI WICVG_Progress ( LPVOID vg )
{
	DCWICVideoGenerator * videoGen = ( DCWICVideoGenerator* ) vg;

	IWICStream * piStream;
	IWICBitmapEncoder * piEncoder;

	g_piFactory->CreateStream ( &piStream );
	piStream->InitializeFromIStream ( videoGen->stream );
	
	g_piFactory->CreateEncoder ( GUID_ContainerFormatGif, NULL, &piEncoder );
	HRESULT hr = piEncoder->Initialize ( piStream, WICBitmapEncoderNoCache );

	IWICMetadataQueryWriter * queryWriter;
	piEncoder->GetMetadataQueryWriter ( &queryWriter );
	PROPVARIANT propValue = { 0, };

	memset ( &propValue, 0, sizeof ( propValue ) );
	propValue.vt = VT_UI1 | VT_VECTOR;
	propValue.caub.cElems = 11;
	propValue.caub.pElems = new UCHAR [ 11 ];
	memcpy ( propValue.caub.pElems, "NETSCAPE2.0", 11 );
	queryWriter->SetMetadataByName ( L"/appext/Application", &propValue );
	delete [] propValue.caub.pElems;

	// From: http://www.pixcl.com/WIC-and-Animated-GIF-Files.htm
	memset ( &propValue, 0, sizeof ( propValue ) );
	propValue.vt = VT_UI1 | VT_VECTOR;
	propValue.caub.cElems = 5;
	propValue.caub.pElems = new UCHAR [ 5 ];
	*( propValue.caub.pElems ) = 3; // must be > 1,
	*( propValue.caub.pElems + 1 ) = 1; // defines animated GIF
	*( propValue.caub.pElems + 2 ) = 0; // LSB 0 = infinite loop.
	*( propValue.caub.pElems + 3 ) = 0; // MSB of iteration count value
	*( propValue.caub.pElems + 4 ) = 0; // NULL == end of data
	queryWriter->SetMetadataByName ( TEXT ( "/appext/Data" ), &propValue );
	delete [] propValue.caub.pElems;

	memset ( &propValue, 0, sizeof ( propValue ) );
	propValue.vt = VT_UI2;
	propValue.uiVal = ( USHORT ) videoGen->capturer->GetCapturedBitmap ()->GetWidth ();
	queryWriter->SetMetadataByName ( TEXT ( "/logscrdesc/Width" ), &propValue );
	memset ( &propValue, 0, sizeof ( propValue ) );
	propValue.vt = VT_UI2;
	propValue.uiVal = ( USHORT ) videoGen->capturer->GetCapturedBitmap ()->GetHeight ();
	queryWriter->SetMetadataByName ( TEXT ( "/logscrdesc/Height" ), &propValue );
	PropVariantClear ( &propValue );

	queryWriter->Release ();

	IWICBitmapFrameEncode *piBitmapFrame = NULL;
	IPropertyBag2 *pPropertybag = NULL;

	memset ( &propValue, 0, sizeof ( propValue ) );
	propValue.vt = VT_UI2;

	while ( videoGen->threadRunning || !videoGen->capturedQueue.empty () )
	{
		if ( videoGen->capturedQueue.empty () )
		{
			Sleep ( 0 );
			continue;
		}

		hr = piEncoder->CreateNewFrame ( &piBitmapFrame, &pPropertybag );
		if ( hr != S_OK )
		{
			videoGen->threadRunning = false;
			return -1;
		}
		hr = piBitmapFrame->Initialize ( pPropertybag );

		WICVG_CONTAINER container;
		if ( !videoGen->capturedQueue.try_pop ( container ) )
		{
			Sleep ( 1 );
			continue;
		}
		UINT imgWidth, imgHeight;
		container.bitmapSource->GetSize ( &imgWidth, &imgHeight );
		piBitmapFrame->SetSize ( imgWidth, imgHeight );
		piBitmapFrame->WriteSource ( container.bitmapSource, nullptr );
		container.bitmapSource->Release ();

		piBitmapFrame->GetMetadataQueryWriter ( &queryWriter );
		piBitmapFrame->Commit ();

		propValue.uiVal = ( WORD ) container.deltaTime;
		queryWriter->SetMetadataByName ( TEXT ( "/grctlext/Delay" ), &propValue );
		queryWriter->Release ();

		piBitmapFrame->Release ();

		Sleep ( 0 );
	}

	piEncoder->Commit ();

	if ( piEncoder )
		piEncoder->Release ();

	if ( piStream )
		piStream->Release ();

	return 0;
}

DWORD WINAPI WICVG_Capturer ( LPVOID vg )
{
	DCWICVideoGenerator * videoGen = ( DCWICVideoGenerator* ) vg;

	DWORD lastTick, currentTick;
	lastTick = currentTick = timeGetTime ();
	while ( videoGen->threadRunning )
	{
		if ( ( currentTick = timeGetTime () ) - lastTick >= videoGen->frameTick )
		{
			videoGen->capturer->Capture ();
			DCBitmap * bitmap = videoGen->capturer->GetCapturedBitmap ();

			WICVG_CONTAINER container;
			container.deltaTime = ( currentTick - lastTick ) / 10;
			IWICBitmap * wicBitmap = bitmap->ToWICBitmap ();
			if ( videoGen->resize != nullptr )
			{
				IWICBitmapScaler * wicBitmapScaler;
				g_piFactory->CreateBitmapScaler ( &wicBitmapScaler );
				wicBitmapScaler->Initialize ( wicBitmap, videoGen->resize->cx, videoGen->resize->cy, WICBitmapInterpolationModeFant );
				container.bitmapSource = wicBitmapScaler;
				wicBitmap->Release ();
			}
			else
			{
				container.bitmapSource = wicBitmap;
			}

			videoGen->capturedQueue.push ( container );

			lastTick = currentTick;
		}

		Sleep ( 0 );
	}

	return 0;
}

void DCWICVideoGenerator::Begin ( IStream * _stream, DCScreenCapturer * _capturer )
{
	stream = _stream;
	capturer = _capturer;

	threadRunning = true;
	threadHandles [ 0 ] = CreateThread ( nullptr, 0, WICVG_Progress, this, 0, nullptr );
	threadHandles [ 1 ] = CreateThread ( nullptr, 0, WICVG_Capturer, this, 0, nullptr );
}

void DCWICVideoGenerator::End ()
{
	threadRunning = false;
	WaitForMultipleObjects ( 2, threadHandles, true, INFINITE );
}

void DCWICVideoGenerator::SetGenerateSize ( const SIZE * size )
{
	resize = size;
}