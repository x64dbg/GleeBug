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
};