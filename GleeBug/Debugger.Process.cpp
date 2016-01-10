#include "Debugger.Process.h"

namespace GleeBug
{
    Process::Process(HANDLE hProcess, uint32 dwProcessId, uint32 dwMainThreadId) :
        hProcess(hProcess),
        dwProcessId(dwProcessId),
        dwMainThreadId(dwMainThreadId),
        thread(nullptr),
        systemBreakpoint(false)
    {
        for (int i = 0; i < HWBP_COUNT; i++)
            hardwareBreakpoints[i].enabled = false;
    }
};