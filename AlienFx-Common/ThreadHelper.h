#pragma once
#include <wtypes.h>
class ThreadHelper
{
private:
	HANDLE tHandle = NULL;
public:
	void (*func)(LPVOID param);
	LPVOID param;
	HANDLE tEvent;
	int delay, priority;
	ThreadHelper(LPVOID function, LPVOID param, int delay = 250, int prt = THREAD_PRIORITY_LOWEST);
	~ThreadHelper();
	void Stop();
	void Start();
};

