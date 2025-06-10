#include "CustomMutex.h"

CustomMutex::CustomMutex()
{
	InitializeCriticalSection(&mHandle);
}

CustomMutex::~CustomMutex()
{
	DeleteCriticalSection(&mHandle);
}

void CustomMutex::lock() {
	EnterCriticalSection(&mHandle);
}

void CustomMutex::unlock() {
	LeaveCriticalSection(&mHandle);
}