#ifndef _DEBUGGER_THREADS_H
#define _DEBUGGER_THREADS_H

#include "Debugger.Global.h"

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

		ThreadInfo();
		ThreadInfo(DWORD dwThreadId, LPVOID lpThreadLocalBase, LPVOID lpStartAddress);
		~ThreadInfo();
	};

	typedef std::map<DWORD, ThreadInfo> ThreadMap;
};

#endif //_DEBUGGER_THREADS_H