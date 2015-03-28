#include "Debugger.Thread.h"

namespace GleeBug
{
	ThreadInfo::ThreadInfo()
	{
		this->hThread = INVALID_HANDLE_VALUE;
	}

	ThreadInfo::ThreadInfo(DWORD dwThreadId, LPVOID lpThreadLocalBase, LPVOID lpStartAddress)
	{
		this->dwThreadId = dwThreadId;
		this->hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
		this->lpThreadLocalBase = (ULONG_PTR)lpThreadLocalBase;
		this->lpStartAddress = (ULONG_PTR)lpStartAddress;
	}

	ThreadInfo::~ThreadInfo()
	{
		if (this->hThread != INVALID_HANDLE_VALUE)
			CloseHandle(hThread);
	}
};