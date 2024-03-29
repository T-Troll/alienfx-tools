#pragma once
#include <wtypes.h>
class ThreadHelper
{
private:
	HANDLE tHandle = NULL;
public:
	void (*func)(LPVOID);
	HANDLE tEvent;
	int delay, priority;
	LPVOID param;
	ThreadHelper(LPVOID function, LPVOID param, int delay = 250, int prt = THREAD_PRIORITY_LOWEST);
	~ThreadHelper();
	void Stop();
	void Start();
};

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

