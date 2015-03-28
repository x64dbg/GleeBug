#include "Debugger.Process.h"

namespace GleeBug
{
	ProcessInfo::ProcessInfo()
	{
		this->curThread = nullptr;
		this->hProcess = INVALID_HANDLE_VALUE;
	}

	ProcessInfo::ProcessInfo(DWORD dwProcessId, DWORD dwMainThreadId)
	{
		this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
		this->dwProcessId = dwProcessId;
		this->dwMainThreadId = dwMainThreadId;
		this->threads.clear();
	}

	ProcessInfo::~ProcessInfo()
	{
		if (this->hProcess != INVALID_HANDLE_VALUE)
			CloseHandle(hProcess);
	}
};