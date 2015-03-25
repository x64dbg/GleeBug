#ifndef _DEBUGGER_DATA_H
#define _DEBUGGER_DATA_H

#include "_global.h"

namespace GleeBug
{
	/**
	\brief Process information structure.
	*/
	struct ProcessInfo
	{
		HANDLE hProcess;
		HANDLE hThread;
		DWORD ProcessId;
		DWORD MainThreadId;
	};
}

#endif //_DEBUGGER_DATA_H