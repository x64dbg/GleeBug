#include "Debugger.Process.h"

namespace GleeBug
{
    Process::Process(HANDLE hProcess, uint32 dwProcessId, uint32 dwMainThreadId, const CREATE_PROCESS_DEBUG_INFO & createProcessInfo) :
        hProcess(hProcess),
        dwProcessId(dwProcessId),
        dwMainThreadId(dwMainThreadId),
        createProcessInfo(createProcessInfo),
        thread(nullptr),
        systemBreakpoint(false),
        permanentDep(false)
    {
        for (int i = 0; i < HWBP_COUNT; i++)
            hardwareBreakpoints[i].enabled = false;

        // DEP is disabled if lpFlagsDep == 0
        typedef BOOL(WINAPI * GETPROCESSDEPPOLICY)(
            _In_  HANDLE  /*hProcess*/,
            _Out_ LPDWORD /*lpFlags*/,
            _Out_ PBOOL   /*lpPermanent*/
            );
        static auto GPDP = GETPROCESSDEPPOLICY(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessDEPPolicy"));
        if (GPDP)
        {
            DWORD lpFlags;
            BOOL bPermanent;
            if (GPDP(hProcess, &lpFlags, &bPermanent))
                permanentDep = lpFlags && bPermanent;
#ifdef _WIN64
            else if (GetLastError() == ERROR_NOT_SUPPORTED)
                permanentDep = true;
#endif
        }
    }

    void Process::StepOver(const StepCallback & cbStep)
    {
        auto gip = thread->registers.Gip();
        unsigned char data[16];
        if (MemReadSafe(gip, data, sizeof(data)))
        {
            mCapstone.Disassemble(gip, data);
            if(mCapstone.GetId() == X86_INS_CALL)
            {
                SetBreakpoint(gip + mCapstone.Size(), [cbStep](const BreakpointInfo & info)
                {
                    cbStep();
                }, true, SoftwareType::ShortInt3);
                return;
            }
        }
        thread->StepInto(cbStep);
    }
};