#ifndef _DEBUG_STATE_H
#define _DEBUG_STATE_H

#include "_global.h"

struct ProcessInfo
{
	HANDLE hProcess;
	HANDLE hThread;
	DWORD ProcessId;
	DWORD MainThreadId;
};

struct DebugState
{
	ProcessInfo process;
};

#endif //_DEBUG_STATE_H