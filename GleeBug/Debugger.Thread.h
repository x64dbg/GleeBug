#ifndef _DEBUGGER_THREADS_H
#define _DEBUGGER_THREADS_H

#include "_global.h"

namespace GleeBug
{
	/**
	\brief Thread information structure.
	*/
	struct ThreadInfo
	{
		DWORD dwThreadId;
		HANDLE hThread;
		ULONG_PTR lpThreadLocalBase;
		ULONG_PTR lpStartAddress;

		ThreadInfo(DWORD dwThreadId, HANDLE hThread, LPVOID lpThreadLocalBase, LPVOID lpStartAddress)
		{
			this->dwThreadId = dwThreadId;
			this->hThread = hThread;
			this->lpThreadLocalBase = (ULONG_PTR)lpThreadLocalBase;
			this->lpStartAddress = (ULONG_PTR)lpStartAddress;
		}
	};

	typedef std::map<DWORD, ThreadInfo> ThreadMap;
};

#endif //_DEBUGGER_THREADS_H