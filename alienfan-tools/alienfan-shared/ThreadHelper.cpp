#include "ThreadHelper.h"

DWORD WINAPI ThreadFunc(LPVOID lpParam);

ThreadHelper::ThreadHelper(LPVOID function, LPVOID param, int delay, int prt) {
	this->delay = delay;
	priority = prt;
	func = (void (*)(LPVOID))function;
	this->param = param;
	tEvent = CreateEvent(NULL, false, false, NULL);
	tHandle = CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
}

ThreadHelper::~ThreadHelper()
{
	SetEvent(tEvent);
	WaitForSingleObject(tHandle, delay << 1);
	CloseHandle(tEvent);
	CloseHandle(tHandle);
}

DWORD WINAPI ThreadFunc(LPVOID lpParam) {
	ThreadHelper* src = (ThreadHelper*)lpParam;
	SetThreadPriority(GetCurrentThread(), src->priority);
	while (WaitForSingleObject(src->tEvent, src->delay) == WAIT_TIMEOUT) {
		src->func(src->param);
	}
	return 0;
}
