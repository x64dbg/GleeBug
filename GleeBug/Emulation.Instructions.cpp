#include "Emulation.Instructions.h"
#include "Debugger.Thread.Registers.h"
#include "Emulation.Helpers.h"

#include <limits>
#include <stdint.h>
#include <unordered_map>

namespace GleeBug {
namespace Emulation  {

enum EFLAGS
{
    EFL_CF = (1 << 0),
    EFL_PF = (1 << 2),
    EFL_AF = (1 << 4),
    EFL_ZF = (1 << 6),
    EFL_SF = (1 << 7),
    EFL_DF = (1 << 10),
    EFL_OF = (1 << 11),
};

inline const size_t InstructionDataHash(size_t state, const void* data, const size_t len)
{
    const uint8_t *buf = reinterpret_cast<const uint8_t*>(data);

    for (size_t i = 0; i < len; ++buf, ++i)
    {
        state ^= ((i & 1) == 0) ? ((state << 7) ^ (*buf) * (state >> 3)) :
            (~((state << 11) + ((*buf) ^ (state >> 5))));
    }

    return state;
}

template<typename T>
inline const size_t InstructionDataHash(const T& data)
{
    return InstructionDataHash(0x811c9dc5, &data, sizeof(T));
}

inline const size_t InstructionOperandHash(size_t N, uint16_t TYPE)
{
    return InstructionDataHash(InstructionDataHash(N));
}

template<size_t N, uint16_t TYPE>
struct OpSig
{
    static constexpr size_t Hash() noexcept
    {
        return InstructionOperandHash(N, TYPE);
    }
    static bool Matches(const ZydisInstructionInfo& instr, const ZydisOperandInfo& op)
    {
        if(op.type != TYPE)
            return false;
        if(op.size != N)
            return false;
        return true;
    }
};

using OpNone = OpSig<0, ZYDIS_OPERAND_TYPE_UNUSED>;

template<uint16 MNEMONIC, 
    typename OP0 = OpNone,
    typename OP1 = OpNone,
    typename OP2 = OpNone,
    typename OP3 = OpNone,
    typename OP4 = OpNone>
struct InstrSig : X86Instruction
{
    static constexpr size_t Hash() noexcept
    {
        return InstructionDataHash(InstructionDataHash(N));
    }

    virtual uint16_t GetMnemonic() const
    {
        return MNEMONIC;
    }

    virtual bool Matches(const ZydisInstructionInfo& info) const
    {
        if(info.mnemonic != MNEMONIC)
            return false;
        if (!OP0::Matches(info, info.operands[0]))
            return false;
        if (!OP1::Matches(info, info.operands[1]))
            return false;
        if (!OP2::Matches(info, info.operands[2]))
            return false;
        if (!OP3::Matches(info, info.operands[3]))
            return false;
        if (!OP4::Matches(info, info.operands[4]))
            return false;

        return true;
    }
};

static std::vector<std::vector<X86Instruction*>> _registeredInstructions;

template<typename T>
struct X86InstructionRegistrator
{
    T inst;
    X86InstructionRegistrator()
    {
        uint16_t groupId = inst.GetMnemonic();
        if (groupId >= _registeredInstructions.size())
        {
            _registeredInstructions.resize(groupId + 1);
        }
        _registeredInstructions[groupId].push_back(&inst);
    }
};

#define REGISTER_X86_INSTRUCTION(instrCls) static X86InstructionRegistrator<##instrCls##> __##instrCls##__
    
template<typename T>
struct SubImpl
{
    static T Execute(T val1, T val2, Registers& regs, uint32_t flagsOut)
    {
        constexpr int NumBits = sizeof(T) * 8;
        constexpr T SignMask = T(1) << (NumBits - 1);
        constexpr T InvMask = ~T(0);

        T res = val1 - val2;

        flagsOut = 0;
        setCondFlag(flagsOut, res & SignMask, EFL_SF);
        setCondFlag(flagsOut, (res & InvMask) == 0, EFL_ZF);
        setCondFlag(flagsOut, isParity(res & 0xFF), EFL_PF);

        T bc = (res & (~val1 | val2)) | (~val1 & val2);
        setCondFlag(flagsOut, bc & SignMask, EFL_CF);
        setCondFlag(flagsOut, xor2(bc >> (NumBits - 2)), EFL_OF);
        setCondFlag(flagsOut, bc & 0x08, EFL_AF);

        return res;
    }
};

struct Instr_MovR32R32 : InstrSig< ZYDIS_MNEMONIC_MOV, OpSig<32, ZYDIS_OPERAND_TYPE_REGISTER>, OpSig<32, ZYDIS_OPERAND_TYPE_REGISTER> >
{
    virtual bool Execute(const ZydisInstructionInfo& info, X86Context& ctx) const override
    {
        int32_t val1 = getRegisterValue<int32_t>(info.operands[1].reg, ctx.regs);

        setRegisterValue(info.operands[0].reg, ctx.regs, val1);
        return true;
    }
};

struct Instr_MovR32I32 : InstrSig< ZYDIS_MNEMONIC_MOV, OpSig<32, ZYDIS_OPERAND_TYPE_REGISTER>, OpSig<32, ZYDIS_OPERAND_TYPE_IMMEDIATE> >
{
    virtual bool Execute(const ZydisInstructionInfo& info, X86Context& ctx) const override
    {
        int32_t val1 = info.operands[1].imm.value.sdword;

        setRegisterValue(info.operands[0].reg, ctx.regs, val1);
        return true;
    }
};

struct Instr_MovR64R64 : InstrSig< ZYDIS_MNEMONIC_MOV, OpSig<64, ZYDIS_OPERAND_TYPE_REGISTER>, OpSig<64, ZYDIS_OPERAND_TYPE_REGISTER> >
{
    virtual bool Execute(const ZydisInstructionInfo& info, X86Context& ctx) const override
    {
        int64_t val2 = getRegisterValue<int64_t>(info.operands[1].reg, ctx.regs);

        setRegisterValue(info.operands[0].reg, ctx.regs, val2);
        return true;
    }
};

struct Instr_Sub32 : InstrSig< ZYDIS_MNEMONIC_SUB, OpSig<32, ZYDIS_OPERAND_TYPE_REGISTER>, OpSig<32, ZYDIS_OPERAND_TYPE_IMMEDIATE> >
{
    virtual bool Execute(const ZydisInstructionInfo& info, X86Context& ctx) const override
    {
        // Value is sign extended.
        int32_t val1 = getRegisterValue<int32_t>(info.operands[0].reg, ctx.regs);
        int32_t val2 = info.operands[1].imm.value.sdword;

        uint32_t flagsOut = 0;
        int32_t res = SubImpl<uint32_t>::Execute(val1, val2, ctx.regs, flagsOut);

        setRegisterValue(info.operands[0].reg, ctx.regs, res);
        ctx.regs.Eflags.Set(ctx.regs.Eflags.Get() | flagsOut);

        return true;
    }
};

struct Instr_SubR64I64 : InstrSig< ZYDIS_MNEMONIC_SUB, OpSig<64, ZYDIS_OPERAND_TYPE_REGISTER>, OpSig<64, ZYDIS_OPERAND_TYPE_IMMEDIATE> >
{
    virtual bool Execute(const ZydisInstructionInfo& info, X86Context& ctx) const override
    {
        // Value is sign extended.
        int64_t val1 = getRegisterValue<int64_t>(info.operands[0].reg, ctx.regs);
        int64_t val2 = info.operands[1].imm.value.sqword;

        uint32_t flagsOut = 0;
        int64_t res = SubImpl<uint64_t>::Execute(val1, val2, ctx.regs, flagsOut);

        setRegisterValue(info.operands[0].reg, ctx.regs, res);
        ctx.regs.Eflags.Set(ctx.regs.Eflags.Get() | flagsOut);

        return true;
    }
};

struct Instr_SubR64I32 : InstrSig< ZYDIS_MNEMONIC_SUB, OpSig<64, ZYDIS_OPERAND_TYPE_REGISTER>, OpSig<32, ZYDIS_OPERAND_TYPE_IMMEDIATE> >
{
    virtual bool Execute(const ZydisInstructionInfo& info, X86Context& ctx) const override
    {
        // Value is sign extended.
        int64_t val1 = getRegisterValue<int64_t>(info.operands[0].reg, ctx.regs);
        int64_t val2 = info.operands[1].imm.value.sdword;

        uint32_t flagsOut = 0;
        int64_t res = SubImpl<uint64_t>::Execute(val1, val2, ctx.regs, flagsOut);

        setRegisterValue(info.operands[0].reg, ctx.regs, res);
        ctx.regs.Eflags.Set(ctx.regs.Eflags.Get() | flagsOut);

        return true;
    }
};

static bool _initialzed = false;

const X86Instruction* LookupInstruction(const ZydisInstructionInfo& info)
{
    if (_initialzed == false)
    {
        REGISTER_X86_INSTRUCTION(Instr_MovR32R32);
        REGISTER_X86_INSTRUCTION(Instr_MovR32I32);
        REGISTER_X86_INSTRUCTION(Instr_MovR64R64);
        REGISTER_X86_INSTRUCTION(Instr_Sub32);
        REGISTER_X86_INSTRUCTION(Instr_SubR64I64);
        REGISTER_X86_INSTRUCTION(Instr_SubR64I32);
        _initialzed = true;
    }

    if(info.mnemonic >= _registeredInstructions.size())
        return nullptr;

    auto& group = _registeredInstructions[info.mnemonic];
    if(group.empty())
        return nullptr;

    for (X86Instruction *instr : group)
    {
        if (instr->Matches(info))
            return instr;
    }

    return nullptr;
}

}
}
