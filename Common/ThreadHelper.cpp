#include "ThreadHelper.h"

DWORD WINAPI ThreadFunc(LPVOID lpParam);

ThreadHelper::ThreadHelper(LPVOID function, LPVOID param, int delay, int prt) {
	this->delay = delay;
	priority = prt;
	func = (void (*)(LPVOID))function;
	this->param = param;
	tEvent = CreateEvent(NULL, true, false, NULL);
	Start();
}

ThreadHelper::~ThreadHelper()
{
	Stop();
	CloseHandle(tEvent);
}

void ThreadHelper::Stop()
{
	if (tHandle) {
		SetEvent(tEvent);
		WaitForSingleObject(tHandle, INFINITE/*delay << 2*/);
		CloseHandle(tHandle);		
		tHandle = NULL;
	}
}

void ThreadHelper::Start()
{
	if (!tHandle) {
		tHandle = CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
		if (tHandle)
			SetThreadPriority(tHandle, priority);
	}
}

DWORD WINAPI ThreadFunc(LPVOID lpParam) {
	ThreadHelper* src = (ThreadHelper*)lpParam;
	do {
		src->func(src->param);
	} while (WaitForSingleObject(src->tEvent, src->delay) == WAIT_TIMEOUT);
	return 0;
}


