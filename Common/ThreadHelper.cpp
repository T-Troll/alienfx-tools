#include "ThreadHelper.h"

DWORD WINAPI ThreadFunc(LPVOID lpParam);

ThreadHelper::ThreadHelper(LPVOID function, LPVOID param, int delay, int prt) {
	this->delay = delay;
	priority = prt;
	func = (void (*)(LPVOID))function;
	this->param = param;
	Start();
}

ThreadHelper::~ThreadHelper()
{
	Stop();
}

void ThreadHelper::Stop()
{
	if (tHandle) {
		SetEvent(tEvent);
		int code = WaitForSingleObject(tHandle, delay << 2);
		CloseHandle(tHandle);
		CloseHandle(tEvent);
		tHandle = NULL;
	}
}

void ThreadHelper::Start()
{
	if (!tHandle) {
		tEvent = CreateEvent(NULL, true, false, NULL);
		tHandle = CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
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
