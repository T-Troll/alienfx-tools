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
	WaitForSingleObject(mutexEvent, 5000);
	ResetEvent(mutexEvent);
}

void CustomMutex::unlock()
{
	SetEvent(mutexEvent);
}

