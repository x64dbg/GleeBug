#ifndef EMULATIONINSTRUCTIONS_H
#define EMULATIONINSTRUCTIONS_H

#include "Debugger.Global.h"
#include "Debugger.Process.h"
#include "Debugger.Breakpoint.h"
#include "Debugger.Thread.h"
#include "Debugger.Thread.Registers.h"

#define ZYDIS_EXPORTS
#define ZYDIS_ENABLE_FEATURE_IMPLICITLY_USED_REGISTERS
#define ZYDIS_ENABLE_FEATURE_AFFECTED_FLAGS
#include <Zydis/Zydis.h>

namespace GleeBug {
namespace Emulation {

struct X86Context
{
    Registers& regs;
    Thread& thread;

    explicit X86Context(Registers& inRegs, Thread& inThread) : regs(inRegs), 
        thread(inThread)
    {
    }
};

struct X86Instruction
{
    virtual bool Matches(const ZydisInstructionInfo& info) = 0;
    virtual bool Execute(const ZydisInstructionInfo& info, X86Context& ctx) = 0;
};

const X86Instruction* LookupInstruction(const ZydisInstructionInfo& info);

} // Emulation
} // GleeBug

#endif // EMULATIONINSTRUCTIONS_H
