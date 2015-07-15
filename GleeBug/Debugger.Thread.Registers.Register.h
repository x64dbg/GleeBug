#ifndef _DEBUGGER_THREAD_REGISTERS_REGISTER_H
#define _DEBUGGER_THREAD_REGISTERS_REGISTER_H

#include "Debugger.Thread.Registers.h"

/**
\brief Registers enum.
*/
enum class R
{
    DR0,
    DR1,
    DR2,
    DR3,
    DR6,
    DR7,

    EFlags,

    EAX,
    AX,
    AH,
    AL,
    EBX,
    BX,
    BH,
    BL,
    ECX,
    CX,
    CH,
    CL,
    EDX,
    DX,
    DH,
    DL,
    EDI,
    DI,
    ESI,
    SI,
    EBP,
    BP,
    ESP,
    SP,
    EIP,

#ifdef _WIN64
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    SIL,
    RDI,
    DIL,
    RBP,
    BPL,
    RSP,
    SPL,
    RIP,
    R8,
    R8D,
    R8W,
    R8B,
    R9,
    R9D,
    R9W,
    R9B,
    R10,
    R10D,
    R10W,
    R10B,
    R11,
    R11D,
    R11W,
    R11B,
    R12,
    R12D,
    R12W,
    R12B,
    R13,
    R13D,
    R13W,
    R13B,
    R14,
    R14D,
    R14W,
    R14B,
    R15,
    R15D,
    R15W,
    R15B,
#endif //_WIN64

    GAX,
    GBX,
    GCX,
    GDX,
    GDI,
    GSI,
    GBP,
    GSP,
    GIP,
}; //R

/**
\brief Class that represents a register.
\tparam RegisterIndex The enum index of the register.
\tparam Type Type of the register value.
\tparam ThisPtr Pointer to the Registers class.
*/
template<R RegisterIndex, typename Type>
class Register
{
public:
    explicit Register(Registers* registers) : _registers(registers) {}

    Type Get() const
    {
        return Type(_registers->Get(RegisterIndex));
    }

    void Set(Type value)
    {
        _registers->Set(RegisterIndex, ptr(value));
    }

    Type operator()() const
    {
        return Get();
    }

    Register<RegisterIndex, Type> & operator=(const Type & other)
    {
        Set(other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator+=(const Type & other)
    {
        Set(Get() + other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator-=(const Type & other)
    {
        Set(Get() - other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator*=(const Type & other)
    {
        Set(Get() * other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator/=(const Type & other)
    {
        Set(Get() / other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator%=(const Type & other)
    {
        Set(Get() % other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator^=(const Type & other)
    {
        Set(Get() ^ other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator&=(const Type & other)
    {
        Set(Get() & other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator|=(const Type & other)
    {
        Set(Get() | other);
        return *this;
    }

    Register<RegisterIndex, Type> & operator++()
    {
        Set(Get() + 1);
        return *this;
    }

    Register<RegisterIndex, Type> & operator++(int)
    {
        return operator++();
    }

    bool  operator==(const Type & other) const
    {
        return Get() == other;
    }

    bool operator!=(const Type & other) const
    {
        return !operator==(other);
    }

private:
    Registers* _registers;
};

#endif //_DEBUGGER_THREAD_REGISTERS_REGISTER_H