#pragma once
#include <wtypes.h>
//#include <synchapi.h>

class CustomMutex
{
private:
	//CRITICAL_SECTION mHandle;
	SRWLOCK mHandle;
public:
	CustomMutex();
	//~CustomMutex();
	void lockRead();
	void lockWrite();
	void unlockRead();
	void unlockWrite();
};