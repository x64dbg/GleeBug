#ifndef _DEBUGGER_THREAD_REGISTERS_REGISTER_H
#define _DEBUGGER_THREAD_REGISTERS_REGISTER_H

#include "Debugger.Thread.Registers.h"

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

    bool  operator==(const Type & other)
    {
        return Get() == other;
    }

    bool operator!=(const Type & other)
    {
        return !operator==(other);
    }

private:
    Registers* _registers;
};

#endif //_DEBUGGER_THREAD_REGISTERS_REGISTER_H