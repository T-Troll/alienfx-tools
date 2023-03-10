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
	if (WaitForSingleObject(mutexEvent, 5000) == WAIT_OBJECT_0)
		ResetEvent(mutexEvent);
	else
		int i = 0;
}

void CustomMutex::unlock()
{
	SetEvent(mutexEvent);
}

