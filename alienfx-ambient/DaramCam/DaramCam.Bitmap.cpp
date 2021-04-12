#include "DaramCam.h"
#include "DaramCam.Internal.h"

#pragma intrinsic(memcpy)

DCBitmap::DCBitmap ( unsigned _width, unsigned _height, unsigned _colorDepth )
	: byteArray ( nullptr )
{
	Resize ( _width, _height, _colorDepth );
}

DCBitmap::~DCBitmap ()
{
	if ( byteArray && !directMode )
		delete [] byteArray;
	byteArray = nullptr;
}

unsigned char * DCBitmap::GetByteArray () noexcept { return byteArray; }
unsigned DCBitmap::GetWidth () noexcept { return width; }
unsigned DCBitmap::GetHeight () noexcept { return height; }
unsigned DCBitmap::GetColorDepth () noexcept { return colorDepth; }
unsigned DCBitmap::GetStride () noexcept { return stride; }

unsigned DCBitmap::GetByteArraySize () noexcept
{
	return colorDepth == 3 ? ( ( ( stride + 3 ) / 4 ) * 4 ) * height : stride * height;
}

IWICBitmap * DCBitmap::ToWICBitmap ()
{
	IWICBitmap * bitmap;
	if ( FAILED ( g_piFactory->CreateBitmapFromMemory ( width, height, colorDepth == 3 ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA,
		stride, GetByteArraySize (), GetByteArray (), &bitmap ) ) )
		return nullptr;
	return bitmap;
}

bool DCBitmap::GetIsDirectMode () noexcept
{
	return directMode;
}

void DCBitmap::SetIsDirectMode ( bool mode ) noexcept
{
	if ( mode && !directMode )
	{
		if ( byteArray )
			delete [] byteArray;
		byteArray = nullptr;
	}
	else if ( !mode && directMode )
	{
		byteArray = new unsigned char [ GetByteArraySize () ];
	}
	directMode = mode;
}

void DCBitmap::SetDirectBuffer ( void * buffer ) noexcept
{
	if ( !directMode ) return;
	byteArray = ( unsigned char * ) buffer;
}

void DCBitmap::Resize ( unsigned _width, unsigned _height, unsigned _colorDepth )
{
	if ( _width == width && _height == _height && colorDepth == _colorDepth ) return;
	if (byteArray && !directMode) {
		delete[] byteArray;
		byteArray = nullptr;
	}
	if ( ( _width == 0 && _height == 0 ) || ( _colorDepth != 3 && _colorDepth != 4 ) )
	{
		byteArray = nullptr;
		return;
	}

	width = _width; height = _height; colorDepth = _colorDepth;
	stride = colorDepth == 3 ? ( _width * ( colorDepth * 8 ) + 7 ) / 8 : width * colorDepth;
	if ( !directMode )
		byteArray = new unsigned char [ GetByteArraySize () ];
}

COLORREF DCBitmap::GetColorRef ( unsigned x, unsigned y )
{
	unsigned basePos = ( x * colorDepth ) + ( y * stride );
	return RGB ( byteArray [ basePos + 0 ], byteArray [ basePos + 1 ], byteArray [ basePos + 2 ] );
}

void DCBitmap::SetColorRef ( COLORREF colorRef, unsigned x, unsigned y )
{
	unsigned basePos = ( x * colorDepth ) + ( y * stride );
	memcpy ( byteArray + basePos, &colorRef, colorDepth );
}

DCBitmap * DCBitmap::Clone ()
{
	DCBitmap * ret = new DCBitmap ( width, height, colorDepth );
	memcpy ( ret->byteArray, byteArray, GetByteArraySize () );
	return ret;
}

void DCBitmap::CopyFrom ( HDC hDC, HBITMAP hBitmap, bool reverse )
{
	BITMAPINFO bmpInfo = { 0, };
	bmpInfo.bmiHeader.biSize = sizeof ( BITMAPINFOHEADER );
	GetDIBits ( hDC, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS );
	if ( width != bmpInfo.bmiHeader.biWidth || height != bmpInfo.bmiHeader.biHeight )
		Resize ( bmpInfo.bmiHeader.biWidth, bmpInfo.bmiHeader.biHeight, 3 );
	bmpInfo.bmiHeader.biBitCount = 24;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	for ( unsigned i = 0; i < height; ++i )
		GetDIBits ( hDC, hBitmap, i, 1, ( byteArray + ( ( reverse ? i : ( height - 1 - i ) ) * stride ) ), &bmpInfo, DIB_RGB_COLORS );
}
bool DCBitmap::CopyFrom ( IWICBitmapSource * wicBitmapSource )
{
	if ( wicBitmapSource == nullptr ) return false;
	
	GUID pixelFormat;
	UINT srcColorDepth;
	
	wicBitmapSource->GetPixelFormat ( &pixelFormat );
	if ( pixelFormat == GUID_WICPixelFormat24bppBGR ) srcColorDepth = 3;
	else if ( pixelFormat == GUID_WICPixelFormat32bppBGRA ) srcColorDepth = 4;
	else return false;
	
	UINT srcWidth, srcHeight;
	wicBitmapSource->GetSize ( &srcWidth, &srcHeight );

	if ( width != srcWidth || height != srcHeight || colorDepth != srcColorDepth )
		Resize ( srcWidth, srcHeight, srcColorDepth );

	WICRect wicRect = { 0, 0, ( int ) srcWidth, ( int ) srcHeight };
	wicBitmapSource->CopyPixels ( &wicRect, stride, GetByteArraySize (), GetByteArray () );

	return true;
}
