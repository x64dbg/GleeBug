#include "Debugger.Process.h"

namespace GleeBug
{
    ProcessInfo::ProcessInfo(uint32 dwProcessId, HANDLE hProcess, uint32 dwMainThreadId)
    {
        this->systemBreakpoint = false;
        this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
        this->dwProcessId = dwProcessId;
        this->dwMainThreadId = dwMainThreadId;
    }

    bool ProcessInfo::MemRead(ptr address, void* buffer, const size_t size) const
    {
        return !!ReadProcessMemory(this->hProcess, reinterpret_cast<const void*>(address), buffer, size, nullptr);
    }

    bool ProcessInfo::MemWrite(ptr address, const void* buffer, const size_t size) const
    {
        return !!WriteProcessMemory(this->hProcess, reinterpret_cast<void*>(address), buffer, size, nullptr);
    }
};