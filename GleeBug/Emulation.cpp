#include "GleeBug.h"
#include "Emulation.h"
#include "Emulation.Instructions.h"
#include "Debugger.Thread.Registers.h"
#include "Debugger.Thread.Registers.Flag.h"

#define ZYDIS_EXPORTS
#define ZYDIS_ENABLE_FEATURE_IMPLICITLY_USED_REGISTERS
#define ZYDIS_ENABLE_FEATURE_AFFECTED_FLAGS
#include <Zydis/Zydis.h>
#include <Zydis/Decoder.h>
#include <Zydis/Formatter.h>

namespace GleeBug {

#ifdef _WIN64
static constexpr ZydisOperatingMode ZYDIS_OPERATING_MODE = ZYDIS_OPERATING_MODE_64BIT;
#else
static constexpr ZydisOperatingMode ZYDIS_OPERATING_MODE = ZYDIS_OPERATING_MODE_32BIT;
#endif

X86Emulator::X86Emulator(Process* process)
    : _process(process)
{
}

bool X86Emulator::IsActive() const
{
    return _active;
}

bool X86Emulator::WaitForEvent(DEBUG_EVENT& debugEvent)
{
    if(_hasEvent == false)
        return false;

    debugEvent = _currentEvent;
    _hasEvent = false;

    return true;
}

bool GenerateDebugEvent(const ZydisInstructionInfo& instrInfo, Emulation::X86Context& ctx, DEBUG_EVENT& debugEvent)
{
    if (ctx.regs.TrapFlag)
    {
        debugEvent.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;

        auto& exData = debugEvent.u.Exception;
        exData.ExceptionRecord.ExceptionCode = STATUS_SINGLE_STEP;
        exData.ExceptionRecord.ExceptionAddress = (PVOID)ctx.regs.Gip.Get();
        exData.ExceptionRecord.NumberParameters = 0;
        exData.dwFirstChance = 1;

        return true;
    }
    // TODO: Deal with breakpoint emulation.

    return false;
}

bool X86Emulator::Emulate(Thread* thread)
{
    SetActiveThread(thread);

    uint8_t instrBytes[16];
    ptr bytesRead = 0;
    if (!_process->MemRead(_registers->Gip.Get(), instrBytes, sizeof(instrBytes), &bytesRead))
    {
        Flush();
        return false;
    }

    const ptr curIP = _registers->Gip.Get();

    ZydisInstructionInfo instrInfo;
    if (ZYDIS_SUCCESS(ZydisDecode(ZYDIS_OPERATING_MODE, instrBytes, 16, curIP, &instrInfo)))
    {
#ifdef _DEBUG
        ZydisInstructionFormatter formatter;
        ZydisFormatterInitInstructionFormatter(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

        char decodedInstr[128];
        ZydisFormatterFormatInstruction(&formatter, &instrInfo, decodedInstr, sizeof(decodedInstr));
#endif

        const Emulation::X86Instruction *instr = Emulation::LookupInstruction(instrInfo);
        if (instr == nullptr)
        {
            Flush();
            return false;
        }

        bool isSingleStepping = _activeThread->isInternalStepping || _activeThread->isSingleStepping;

        Emulation::X86Context ctx(*_registers, *_activeThread);
        if (instr->Execute(instrInfo, ctx))
        {
            _registers->Gip.Set(curIP + instrInfo.length);
        }

        //dprintf("Stepped with emulation at %p\n", curIP);

        _currentEvent.dwProcessId = _process->dwProcessId;
        _currentEvent.dwThreadId = _activeThread->dwThreadId;

        if (GenerateDebugEvent(instrInfo, ctx, _currentEvent))
        {
            if (isSingleStepping)
            {
                ctx.regs.TrapFlag = false;
            }
            _hasEvent = true;
            Flush();
        }
    }
    else
    {
        Flush();
        return false;
    }

    return true;
}

void X86Emulator::Flush()
{
    if (_registers != nullptr)
    {
        delete _registers;
        _registers = nullptr;
    }
    _activeThread = nullptr;
}

void X86Emulator::SetActiveThread(Thread *thread)
{
    if (_activeThread != thread)
    {
        // Commit current state if we change threads between.
        Flush();

        _registers = new Registers(thread->hThread, CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS | CONTEXT_INTEGER);
        _activeThread = thread;
    }
}

}
