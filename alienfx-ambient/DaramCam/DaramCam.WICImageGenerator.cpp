#include "DaramCam.h"
#include "DaramCam.Internal.h"

class DARAMCAM_EXPORTS DCWICImageGenerator : public DCImageGenerator
{
public:
	DCWICImageGenerator ( DCWICImageType imageType );
	virtual ~DCWICImageGenerator ();

public:
	virtual void Generate ( IStream * stream, DCBitmap * bitmap );

public:
	void SetGenerateSize ( const SIZE* size );

private:
	GUID containerGUID;
	const SIZE* resize;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DARAMCAM_EXPORTS DCImageGenerator * DCCreateWICImageGenerator ( DCWICImageType imageType )
{
	return new DCWICImageGenerator ( imageType );
}

DARAMCAM_EXPORTS void DCSetSizeToWICImageGenerator ( DCImageGenerator * generator, const SIZE * size )
{
	dynamic_cast< DCWICImageGenerator* >( generator )->SetGenerateSize ( size );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

DCWICImageGenerator::DCWICImageGenerator ( DCWICImageType imageType )
	: resize ( nullptr )
{
	switch ( imageType )
	{
	case DCWICImageType_BMP: containerGUID = GUID_ContainerFormatBmp; break;
	case DCWICImageType_JPEG: containerGUID = GUID_ContainerFormatJpeg; break;
	case DCWICImageType_PNG: containerGUID = GUID_ContainerFormatPng; break;
	case DCWICImageType_TIFF: containerGUID = GUID_ContainerFormatTiff; break;
	}
}

DCWICImageGenerator::~DCWICImageGenerator () { }

void DCWICImageGenerator::Generate ( IStream * stream, DCBitmap * bitmap )
{
	IWICStream * piStream = nullptr;
	g_piFactory->CreateStream ( &piStream );

	piStream->InitializeFromIStream ( stream );

	IWICBitmapEncoder * piEncoder;
	g_piFactory->CreateEncoder ( containerGUID, NULL, &piEncoder );
	piEncoder->Initialize ( piStream, WICBitmapEncoderNoCache );

	IWICBitmapFrameEncode *piBitmapFrame = NULL;
	IPropertyBag2 *pPropertybag = NULL;
	piEncoder->CreateNewFrame ( &piBitmapFrame, &pPropertybag );
	piBitmapFrame->Initialize ( pPropertybag );

	auto wicBitmap = bitmap->ToWICBitmap ();
	IWICBitmapSource * wicBitmapSource;
	if ( resize != nullptr )
	{
		IWICBitmapScaler * wicBitmapScaler;
		g_piFactory->CreateBitmapScaler ( &wicBitmapScaler );
		wicBitmapScaler->Initialize ( wicBitmap, resize->cx, resize->cy, WICBitmapInterpolationModeFant );
		wicBitmapSource = wicBitmapScaler;
		wicBitmap->Release ();
	}
	else
		wicBitmapSource = wicBitmap;

	UINT width, height;
	wicBitmapSource->GetSize ( &width, &height );

	piBitmapFrame->SetSize ( width, height );

	WICPixelFormatGUID formatGUID = bitmap->GetColorDepth () == 3 ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA;
	piBitmapFrame->SetPixelFormat ( &formatGUID );

	WICRect wicRect = { 0, 0, ( int ) width, ( int ) height };
	piBitmapFrame->WriteSource ( wicBitmapSource, &wicRect );
	wicBitmapSource->Release ();

	piBitmapFrame->Commit ();
	piBitmapFrame->Release ();

	piEncoder->Commit ();
	piEncoder->Release ();

	piStream->Release ();
}

void DCWICImageGenerator::SetGenerateSize ( const SIZE * size )
{
	resize = size;
}