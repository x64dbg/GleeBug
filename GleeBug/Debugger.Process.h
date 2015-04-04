#ifndef _DEBUGGER_PROCESS_H
#define _DEBUGGER_PROCESS_H

#include "Debugger.Global.h"
#include "Debugger.Thread.h"
#include "Debugger.Dll.h"

namespace GleeBug
{
	/**
	\brief Process information structure.
	*/
	struct ProcessInfo
	{
		HANDLE hProcess;
		DWORD dwProcessId;
		DWORD dwMainThreadId;

		ThreadInfo* curThread;
		bool systemBreakpoint;

		ThreadMap threads;
		DllMap dlls;

		ProcessInfo();
		ProcessInfo(DWORD dwProcessId, DWORD dwMainThreadId);
		~ProcessInfo();
		bool MemRead(ULONG_PTR address, const size_t size, void* buffer);
		bool MemWrite(ULONG_PTR address, const size_t size, const void* buffer);
	};

	typedef std::map<DWORD, ProcessInfo> ProcessMap;
};

#endif //_DEBUGGER_PROCESS_H