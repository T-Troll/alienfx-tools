#include "CustomMutex.h"

CustomMutex::CustomMutex()
{
	//InitializeCriticalSection(&mHandle);
	InitializeSRWLock(&mHandle);
}

//CustomMutex::~CustomMutex()
//{
//	//DeleteCriticalSection(&mHandle);
//}



void CustomMutex::lockRead()
{
	AcquireSRWLockShared(&mHandle);
}

void CustomMutex::lockWrite() {
	//EnterCriticalSection(&mHandle);
	AcquireSRWLockExclusive(&mHandle);
}

void CustomMutex::unlockRead()
{
	ReleaseSRWLockShared(&mHandle);
}

void CustomMutex::unlockWrite() {
	//LeaveCriticalSection(&mHandle);
	ReleaseSRWLockExclusive(&mHandle);
}