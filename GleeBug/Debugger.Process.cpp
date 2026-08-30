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
        default:
            return false;
        }
    }

    static ZydisMachineMode GetMachineMode(const Registers & registers)
    {
#ifdef _WIN64
        return registers.Is32BitMode() ? ZYDIS_MACHINE_MODE_LONG_COMPAT_32 : ZYDIS_MACHINE_MODE_LONG_64;
#else
        return ZYDIS_MACHINE_MODE_LEGACY_32;
#endif // _WIN64
    }

    void Process::StepOver(const StepCallback & cbStep)
    {
        Registers registers(thread->hThread, CONTEXT_CONTROL);
        auto gip = registers.Gip();
        auto machineMode = GetMachineMode(registers);
        unsigned char data[16];
        if(MemReadSafe(gip, data, sizeof(data)))
        {
            ZydisDisassembledInstruction instruction;
            if(ZYAN_SUCCESS(ZydisDisassembleIntel(
                                machineMode,
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

    void Process::StepInternal(const StepCallback & cbStep)
    {
        Registers registers(thread->hThread, CONTEXT_CONTROL);
        registers.TrapFlag.Set();
        thread->isInternalStepping = true;

        // Check if we're currently stepping on a pushf instruction
        auto isPushf = false;
        auto pointerSize = registers.PointerSize();
        {
            auto gip = registers.Gip();
            unsigned char data[16];
            if(MemReadSafe(gip, data, sizeof(data)))
            {
                ZydisDisassembledInstruction instruction;
                if(ZYAN_SUCCESS(ZydisDisassembleIntel(
                                    GetMachineMode(registers),
                                    gip,
                                    data,
                                    sizeof(data),
                                    &instruction
                                )))
                {
                    switch(instruction.info.mnemonic)
                    {
                    case ZYDIS_MNEMONIC_PUSHF:
                    case ZYDIS_MNEMONIC_PUSHFD:
                    case ZYDIS_MNEMONIC_PUSHFQ:
                        isPushf = true;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        if(isPushf)
        {
            thread->cbInternalStep = [this, cbStep, pointerSize]()
            {
                // Remove the trap flag from the stack using the execution
                // mode's pointer width (PUSHFD is four bytes under WoW64).
                auto gsp = Registers(this->thread->hThread).Gsp();
                uint64 data = 0;
                if(MemReadUnsafe(gsp, &data, pointerSize))
                {
                    data &= ~uint64(Registers::F::Trap);
                    MemWriteUnsafe(gsp, &data, pointerSize);
                }

                cbStep();
            };
        }
        else
        {
            thread->cbInternalStep = cbStep;
        }
    }
};