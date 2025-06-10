#pragma once
#include <wtypes.h>

class ThreadHelper
{
public:
	void (*func)(LPVOID);
	HANDLE tEvent;
	HANDLE tHandle = NULL;
	int delay, priority;
	LPVOID param;
	ThreadHelper(LPVOID function, LPVOID param, int delay = 250, int prt = THREAD_PRIORITY_LOWEST);
	~ThreadHelper();
	void Stop();
	void Start();
};
;

