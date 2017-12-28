#ifndef DEBUGGER_THREAD_REGISTERS_REGISTER_H
#define DEBUGGER_THREAD_REGISTERS_REGISTER_H

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

    GS,
    FS,
    ES,
    DS,
    CS,
    SS
}; //R

/**
\brief Class that represents a register. This class provides an abstraction for a register to allow intuitive operations.
\tparam RegisterIndex The enum index of the register.
\tparam Type Type of the register value.
\tparam ThisPtr Pointer to the Registers class.
*/
template<R RegisterIndex, typename Type>
class Register
{
public:
    /**
    \brief Constructor.
    \param registers Pointer to the registers.
    */
    explicit Register(Registers* registers)
        : mRegisters(registers)
    {
    }

    /**
    \brief Gets the register value.
    \return The register value.
    */
    Type Get() const
    {
        auto ptr = (Type*)mRegisters->getPtr(RegisterIndex);
        if (ptr)
            return *ptr;
        return Type();
    }

    /**
    \brief Sets the register value.
    \param value The new register value.
    */
    void Set(Type value)
    {
        auto ptr = (Type*)mRegisters->getPtr(RegisterIndex);
        if (ptr)
            *ptr = value;
    }

    /**
    \brief Gets the register value.
    \return The register value.
    */
    Type operator()() const
    {
        return Get();
    }

    /**
    \brief Assignment operator.
    \param value The new register value.
    \return A shallow copy of this object.
    */
    Register<RegisterIndex, Type> & operator=(const Type & value)
    {
        Set(value);
        return *this;
    }

    /**
    \brief Addition assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator+=(const Type & value)
    {
        Set(Get() + value);
        return *this;
    }

    /**
    \brief Subtraction assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator-=(const Type & value)
    {
        Set(Get() - value);
        return *this;
    }

    /**
    \brief Multiplication assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator*=(const Type & value)
    {
        Set(Get() * value);
        return *this;
    }

    /**
    \brief Division assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator/=(const Type & value)
    {
        Set(Get() / value);
        return *this;
    }

    /**
    \brief Modulus assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator%=(const Type & value)
    {
        Set(Get() % value);
        return *this;
    }

    /**
    \brief Bitwise 'exclusive or' assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator^=(const Type & value)
    {
        Set(Get() ^ value);
        return *this;
    }

    /**
    \brief Bitwise 'and' assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator&=(const Type & value)
    {
        Set(Get() & value);
        return *this;
    }

    /**
    \brief Bitwise 'or' assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Register<RegisterIndex, Type> & operator|=(const Type & value)
    {
        Set(Get() | value);
        return *this;
    }

    /**
    \brief Increment operator.
    \return The register before the operation.
    */
    Register<RegisterIndex, Type> operator++()
    {
        auto ret = *this;
        Set(Get() + 1);
        return ret;
    }

    /**
    \brief Increment operator.
    \return The register before the operation.
    */
    Register<RegisterIndex, Type> operator++(int)
    {
        return operator++();
    }

    /**
    \brief Equality operator.
    \param value The value to compare with.
    \return true if the register is equal to the value.
    */
    bool  operator==(const Type & value) const
    {
        return Get() == value;
    }

    /**
    \brief Inequality operator.
    \param value The value to compare with.
    \return true if the register is not equal to the value.
    */
    bool operator!=(const Type & value) const
    {
        return !operator==(value);
    }

private:
    Registers* mRegisters;
};

#endif //DEBUGGER_THREAD_REGISTERS_REGISTER_H