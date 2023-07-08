#include "Debugger.Process.h"
#include "Debugger.Thread.Registers.h"
#include "Zydis/Zydis.h"

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
        for(int i = 0; i < HWBP_COUNT; i++)
            hardwareBreakpoints[i].internal.hardware.enabled = false;
    }

    static bool IsRepeated(const ZydisDecodedInstruction & info)
    {
        // https://www.felixcloutier.com/x86/rep:repe:repz:repne:repnz
        // TODO: allow extracting the affected range
        switch(info.mnemonic)
        {
        // INS
        case ZYDIS_MNEMONIC_INSB:
        case ZYDIS_MNEMONIC_INSW:
        case ZYDIS_MNEMONIC_INSD:
        // OUTS
        case ZYDIS_MNEMONIC_OUTSB:
        case ZYDIS_MNEMONIC_OUTSW:
        case ZYDIS_MNEMONIC_OUTSD:
        // MOVS
        case ZYDIS_MNEMONIC_MOVSB:
        case ZYDIS_MNEMONIC_MOVSW:
        case ZYDIS_MNEMONIC_MOVSD:
        case ZYDIS_MNEMONIC_MOVSQ:
        // LODS
        case ZYDIS_MNEMONIC_LODSB:
        case ZYDIS_MNEMONIC_LODSW:
        case ZYDIS_MNEMONIC_LODSD:
        case ZYDIS_MNEMONIC_LODSQ:
        // STOS
        case ZYDIS_MNEMONIC_STOSB:
        case ZYDIS_MNEMONIC_STOSW:
        case ZYDIS_MNEMONIC_STOSD:
        case ZYDIS_MNEMONIC_STOSQ:
        // CMPS
        case ZYDIS_MNEMONIC_CMPSB:
        case ZYDIS_MNEMONIC_CMPSW:
        case ZYDIS_MNEMONIC_CMPSD:
        case ZYDIS_MNEMONIC_CMPSQ:
        // SCAS
        case ZYDIS_MNEMONIC_SCASB:
        case ZYDIS_MNEMONIC_SCASW:
        case ZYDIS_MNEMONIC_SCASD:
        case ZYDIS_MNEMONIC_SCASQ:
            return (info.attributes & (ZYDIS_ATTRIB_HAS_REP | ZYDIS_ATTRIB_HAS_REPZ | ZYDIS_ATTRIB_HAS_REPNZ)) != 0;
        }
        return false;
    }

    void Process::StepOver(const StepCallback & cbStep)
    {
        auto gip = Registers(thread->hThread, CONTEXT_CONTROL).Gip();
        unsigned char data[16];
        if(MemReadSafe(gip, data, sizeof(data)))
        {
            ZydisDisassembledInstruction instruction;
            if(ZYAN_SUCCESS(ZydisDisassembleIntel(
                                GleeArchValue(ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_MACHINE_MODE_LONG_COMPAT_32),
                                gip,
                                data,
                                sizeof(data),
                                &instruction
                            )))
            {
                bool stepOver = false;
                switch(instruction.info.mnemonic)
                {
                case ZYDIS_MNEMONIC_CALL:
                case ZYDIS_MNEMONIC_PUSHF:
                case ZYDIS_MNEMONIC_PUSHFD:
                case ZYDIS_MNEMONIC_PUSHFQ:
                    stepOver = true;
                    break;
                default:
                    stepOver = IsRepeated(instruction.info);
                    break;
                }
                if(stepOver)
                {
                    SetBreakpoint(gip + instruction.info.length, [cbStep](const BreakpointInfo & info)
                    {
                        cbStep();
                    }, true, SoftwareType::ShortInt3);
                    return;
                }
            }
        }
        thread->StepInto(cbStep);
    }
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