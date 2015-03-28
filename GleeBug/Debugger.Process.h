#ifndef _DEBUGGER_PROCESS_H
#define _DEBUGGER_PROCESS_H

#include "_global.h"
#include "Debugger.Thread.h"

namespace GleeBug
{
	/**
	\brief Process information structure.
	*/
	struct ProcessInfo
	{
		HANDLE hProcess;
		HANDLE hThread;
		DWORD dwProcessId;
		DWORD dwMainThreadId;

		ThreadMap threads;
		ThreadInfo* curThread;

		ProcessInfo() {} //fixes a 'no default constructor available' error

		ProcessInfo(HANDLE hProcess, HANDLE hThread, DWORD dwProcessId, DWORD dwMainThreadId)
		{
			this->hProcess = hProcess;
			this->hThread = hThread;
			this->dwProcessId = dwProcessId;
			this->dwMainThreadId = dwMainThreadId;
			this->threads.clear();
		}
	};

	typedef std::map<DWORD, ProcessInfo> ProcessMap;
};

#endif //_DEBUGGER_PROCESS_H