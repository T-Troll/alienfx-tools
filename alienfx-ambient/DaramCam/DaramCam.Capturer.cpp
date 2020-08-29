#include "DaramCam.h"

DCScreenCapturer::~DCScreenCapturer ()
{

}

DCAudioCapturer::~DCAudioCapturer ()
{

}

unsigned DCAudioCapturer::GetByterate () noexcept
{
	return GetChannels () * GetSamplerate () * ( GetBitsPerSample () / 8 );
}