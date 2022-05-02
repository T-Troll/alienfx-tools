#pragma once
#include <wtypes.h>
class ThreadHelper
{
private:
	HANDLE tHandle;

public:
	void (*func)(LPVOID param) = NULL;
	LPVOID param;
	HANDLE tEvent;
	int delay;
	ThreadHelper(LPVOID function, LPVOID param, int delay = 250);
	~ThreadHelper();
};

