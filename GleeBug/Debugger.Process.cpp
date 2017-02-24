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
            hardwareBreakpoints[i].internal.hardware.enabled = false;
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