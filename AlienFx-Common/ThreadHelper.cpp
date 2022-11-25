#include "ThreadHelper.h"

DWORD WINAPI ThreadFunc(LPVOID lpParam);

ThreadHelper::ThreadHelper(LPVOID function, LPVOID param, int delay, int prt) {
	this->delay = delay;
	priority = prt;
	func = (void (*)(LPVOID))function;
	this->param = param;
	tEvent = CreateEvent(NULL, false, false, NULL);
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
		WaitForSingleObject(tHandle,
#ifdef _DEBUG
			INFINITE
#else
			delay << 4
#endif // _DEBUG
		);
		CloseHandle(tHandle);
		tHandle = NULL;
	}
}

void ThreadHelper::Start()
{
	if (!tHandle) {
		func(param);
		tHandle = CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
	}
}

DWORD WINAPI ThreadFunc(LPVOID lpParam) {
	ThreadHelper* src = (ThreadHelper*)lpParam;
	SetThreadPriority(GetCurrentThread(), src->priority);
	while (WaitForSingleObject(src->tEvent, src->delay) == WAIT_TIMEOUT) {
		src->func(src->param);
	}
	return 0;
}
