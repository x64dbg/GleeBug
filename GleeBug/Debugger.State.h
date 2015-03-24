#ifndef _DEBUG_STATE_H
#define _DEBUG_STATE_H

#include "_global.h"

namespace Debugger
{
	struct ProcessInfo
	{
		HANDLE hProcess;
		HANDLE hThread;
		DWORD ProcessId;
		DWORD MainThreadId;
	};

	struct DebugState
	{
		ProcessInfo Process;
		DEBUG_EVENT DebugEvent;
		DWORD ContinueStatus;
		bool BreakDebugger;
	};
};

#endif //_DEBUG_STATE_H