#pragma once
#include <WTypesbase.h>
class CustomMutex
{
private:
	HANDLE mutexEvent;
public:
	CustomMutex();
	~CustomMutex();
	void lock();
	void unlock();
};

