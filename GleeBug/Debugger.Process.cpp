#include "Debugger.Process.h"

namespace GleeBug
{
	ProcessInfo::ProcessInfo()
	{
		this->curThread = nullptr;
		this->systemBreakpoint = false;
		this->hProcess = INVALID_HANDLE_VALUE;
	}

	ProcessInfo::ProcessInfo(DWORD dwProcessId, HANDLE hProcess, DWORD dwMainThreadId)
	{
		this->systemBreakpoint = false;
		this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
		this->dwProcessId = dwProcessId;
		this->dwMainThreadId = dwMainThreadId;
	}

	bool ProcessInfo::MemRead(ULONG_PTR address, const size_t size, void* buffer)
	{
		return !!ReadProcessMemory(this->hProcess, (const void*)address, buffer, size, NULL);
	}

	bool ProcessInfo::MemWrite(ULONG_PTR address, const size_t size, const void* buffer)
	{
		return !!WriteProcessMemory(this->hProcess, (void*)address, buffer, size, NULL);
	}
};