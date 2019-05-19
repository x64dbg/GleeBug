#include "Debugger.Process.h"
#include "Debugger.Thread.Registers.h"

#define ZYDIS_EXPORTS
#define ZYDIS_ENABLE_FEATURE_IMPLICITLY_USED_REGISTERS
#define ZYDIS_ENABLE_FEATURE_AFFECTED_FLAGS
#include <Zydis/Zydis.h>

namespace GleeBug
{
    Process::Process(HANDLE hProcess, uint32 dwProcessId, uint32 dwMainThreadId, const CREATE_PROCESS_DEBUG_INFO & createProcessInfo) :
        hProcess(hProcess),
        dwProcessId(dwProcessId),
        dwMainThreadId(dwMainThreadId),
        createProcessInfo(createProcessInfo),
        thread(nullptr),
        systemBreakpoint(false),
        permanentDep(false),
        emulator(this)
    {
        for (int i = 0; i < HWBP_COUNT; i++)
            hardwareBreakpoints[i].internal.hardware.enabled = false;
    }

    void Process::StepOver(const StepCallback & cbStep)
    {
        auto gip = Registers(thread->hThread, CONTEXT_CONTROL).Gip();
        unsigned char data[16];
        if (MemReadSafe(gip, data, sizeof(data)))
        {
            ZydisInstructionInfo info;
            memset(&info, 0, sizeof(info));
            auto mode = GleeArchValue(ZYDIS_OPERATING_MODE_32BIT, ZYDIS_OPERATING_MODE_64BIT);
            auto status = ZydisDecode(mode, data, sizeof(data), gip, &info);
            auto stepOver = false;
            if(ZYDIS_SUCCESS(status))
            {
                switch(info.mnemonic)
                {
                case ZYDIS_MNEMONIC_CALL:
                case ZYDIS_MNEMONIC_PUSHF:
                case ZYDIS_MNEMONIC_PUSHFD:
                case ZYDIS_MNEMONIC_PUSHFQ:
                    stepOver = true;
                    break;
                default:
                    auto repAttributes = ZYDIS_ATTRIB_HAS_REP | ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPZ | ZYDIS_ATTRIB_HAS_REPNE | ZYDIS_ATTRIB_HAS_REPNZ;
                    stepOver = (info.attributes & repAttributes) != 0;
                }
            }
            if (stepOver)
            {
                SetBreakpoint(gip + info.length, [cbStep](const BreakpointInfo & info)
                {
                    cbStep();
                }, true, SoftwareType::ShortInt3);
                return;
            }
        }
        thread->StepInto(cbStep);
    }
};
