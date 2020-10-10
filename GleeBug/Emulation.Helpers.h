#ifndef EMULATIONHELPERS_H
#define EMULATIONHELPERS_H

#include <assert.h>

namespace GleeBug
{

template<typename T>
inline T getRegisterValue(const ZydisRegister reg, Registers& regs)
{
    switch (reg)
    {
    // 64 Bit.
    case ZYDIS_REGISTER_RAX:
        return (T)regs.Rax.Get();
    case ZYDIS_REGISTER_RBX:
        return (T)regs.Rbx.Get();
    case ZYDIS_REGISTER_RCX:
        return (T)regs.Rcx.Get();
    case ZYDIS_REGISTER_RDX:
        return (T)regs.Rdx.Get();
    case ZYDIS_REGISTER_RBP:
        return (T)regs.Rbp.Get();
    case ZYDIS_REGISTER_RSP:
        return (T)regs.Rsp.Get();
    case ZYDIS_REGISTER_RSI:
        return (T)regs.Rsi.Get();
    case ZYDIS_REGISTER_RDI:
        return (T)regs.Rdi.Get();
    case ZYDIS_REGISTER_R8:
        return (T)regs.R8.Get();
    case ZYDIS_REGISTER_R9:
        return (T)regs.R9.Get();
    case ZYDIS_REGISTER_R10:
        return (T)regs.R10.Get();
    case ZYDIS_REGISTER_R11:
        return (T)regs.R11.Get();
    case ZYDIS_REGISTER_R12:
        return (T)regs.R12.Get();
    case ZYDIS_REGISTER_R13:
        return (T)regs.R13.Get();
    case ZYDIS_REGISTER_R14:
        return (T)regs.R14.Get();
    // 32 bit
    case ZYDIS_REGISTER_EAX:
        return (T)regs.Eax.Get();
    case ZYDIS_REGISTER_EBX:
        return (T)regs.Ebx.Get();
    case ZYDIS_REGISTER_ECX:
        return (T)regs.Ecx.Get();
    case ZYDIS_REGISTER_EDX:
        return (T)regs.Edx.Get();
    case ZYDIS_REGISTER_EBP:
        return (T)regs.Ebp.Get();
    case ZYDIS_REGISTER_ESP:
        return (T)regs.Esp.Get();
    case ZYDIS_REGISTER_ESI:
        return (T)regs.Esi.Get();
    case ZYDIS_REGISTER_EDI:
        return (T)regs.Edi.Get();
    case ZYDIS_REGISTER_R8D:
        return (T)regs.R8d.Get();
    case ZYDIS_REGISTER_R9D:
        return (T)regs.R9d.Get();
    case ZYDIS_REGISTER_R10D:
        return (T)regs.R10d.Get();
    case ZYDIS_REGISTER_R11D:
        return (T)regs.R11d.Get();
    case ZYDIS_REGISTER_R12D:
        return (T)regs.R12d.Get();
    case ZYDIS_REGISTER_R13D:
        return (T)regs.R13d.Get();
    case ZYDIS_REGISTER_R14D:
        return (T)regs.R14d.Get();
    default:
        assert(false);
    }
}

template<typename T>
inline void setRegisterValue(const ZydisRegister reg, Registers& regs, T val)
{
    switch (reg)
    {
    // 64 bit.
    case ZYDIS_REGISTER_RAX:
        return regs.Rax.Set(val);
    case ZYDIS_REGISTER_RBX:
        return regs.Rbx.Set(val);
    case ZYDIS_REGISTER_RCX:
        return regs.Rcx.Set(val);
    case ZYDIS_REGISTER_RDX:
        return regs.Rdx.Set(val);
    case ZYDIS_REGISTER_RBP:
        return regs.Rbp.Set(val);
    case ZYDIS_REGISTER_RSP:
        return regs.Rsp.Set(val);
    case ZYDIS_REGISTER_RSI:
        return regs.Rsi.Set(val);
    case ZYDIS_REGISTER_RDI:
        return regs.Rdi.Set(val);
    case ZYDIS_REGISTER_R8:
        return regs.R8.Set(val);
    case ZYDIS_REGISTER_R9:
        return regs.R9.Set(val);
    case ZYDIS_REGISTER_R10:
        return regs.R10.Set(val);
    case ZYDIS_REGISTER_R11:
        return regs.R11.Set(val);
    case ZYDIS_REGISTER_R12:
        return regs.R12.Set(val);
    case ZYDIS_REGISTER_R13:
        return regs.R13.Set(val);
    case ZYDIS_REGISTER_R14:
        return regs.R14.Set(val);
    // 32 bit
    case ZYDIS_REGISTER_EAX:
        return regs.Eax.Set(val);
    case ZYDIS_REGISTER_EBX:
        return regs.Ebx.Set(val);
    case ZYDIS_REGISTER_ECX:
        return regs.Ecx.Set(val);
    case ZYDIS_REGISTER_EDX:
        return regs.Edx.Set(val);
    case ZYDIS_REGISTER_EBP:
        return regs.Ebp.Set(val);
    case ZYDIS_REGISTER_ESP:
        return regs.Esp.Set(val);
    case ZYDIS_REGISTER_ESI:
        return regs.Esi.Set(val);
    case ZYDIS_REGISTER_EDI:
        return regs.Edi.Set(val);
    case ZYDIS_REGISTER_R8D:
        return regs.R8d.Set(val);
    case ZYDIS_REGISTER_R9D:
        return regs.R9d.Set(val);
    case ZYDIS_REGISTER_R10D:
        return regs.R10d.Set(val);
    case ZYDIS_REGISTER_R11D:
        return regs.R11d.Set(val);
    case ZYDIS_REGISTER_R12D:
        return regs.R12d.Set(val);
    case ZYDIS_REGISTER_R13D:
        return regs.R13d.Set(val);
    case ZYDIS_REGISTER_R14D:
        return regs.R14d.Set(val);
    default:
        assert(false);
    }
}

inline void setCondFlag(uint32_t& flags, int64_t cond, uint32_t f)
{
    if(!!cond)
        flags |= f;        
}

inline bool isParity(uint32_t v)
{
    v ^= v >> 1;
    v ^= v >> 2;
    v = (v & 0x11111111U) * 0x11111111U;
    return (v >> 28) & 1;
}

inline bool isParity(uint64_t v)
{
    v ^= v >> 1;
    v ^= v >> 2;
    v = (v & 0x1111111111111111UL) * 0x1111111111111111UL;
    return (v >> 60) & 1;
}

template<typename T>
inline T xor2(T x)
{
    return (((x) ^ ((x) >> 1)) & 0x1);
}

} // GleeBug

#endif // EMULATIONHELPERS_H
