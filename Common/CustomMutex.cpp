#include "CustomMutex.h"

CustomMutex::CustomMutex()
{
	mutexEvent = CreateEvent(NULL, true, true, NULL);
}

CustomMutex::~CustomMutex()
{
	CloseHandle(mutexEvent);
}

void CustomMutex::lock()
{
	WaitForSingleObject(mutexEvent, INFINITE);
	ResetEvent(mutexEvent);
}

void CustomMutex::unlock()
{
	SetEvent(mutexEvent);
}

