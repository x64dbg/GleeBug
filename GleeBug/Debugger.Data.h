#ifndef _DEBUGGER_DATA_H
#define _DEBUGGER_DATA_H

#include "_global.h"

namespace GleeBug
{
	struct ProcessInfo
	{
		HANDLE hProcess;
		HANDLE hThread;
		DWORD ProcessId;
		DWORD MainThreadId;
	};
}

#endif //_DEBUGGER_DATA_H