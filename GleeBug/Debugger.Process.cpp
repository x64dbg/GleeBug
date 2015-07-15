#include "Debugger.Process.h"

namespace GleeBug
{
    ProcessInfo::ProcessInfo()
    {
        this->thread = nullptr;
        this->systemBreakpoint = false;
        this->hProcess = INVALID_HANDLE_VALUE;
    }

    ProcessInfo::ProcessInfo(uint32 dwProcessId, HANDLE hProcess, uint32 dwMainThreadId)
    {
        this->systemBreakpoint = false;
        this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
        this->dwProcessId = dwProcessId;
        this->dwMainThreadId = dwMainThreadId;
    }

    bool ProcessInfo::MemRead(ptr address, const size_t size, void* buffer)
    {
        return !!ReadProcessMemory(this->hProcess, reinterpret_cast<const void*>(address), buffer, size, nullptr);
    }

    bool ProcessInfo::MemWrite(ptr address, const size_t size, const void* buffer)
    {
        return !!WriteProcessMemory(this->hProcess, reinterpret_cast<void*>(address), buffer, size, nullptr);
    }
};