#pragma once
#include <wtypes.h>

class CustomMutex
{
private:
	CRITICAL_SECTION mHandle;
public:
	CustomMutex();
	~CustomMutex();
	void lock();
	void unlock();
};