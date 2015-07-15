#ifndef _DEBUGGER_THREAD_REGISTERS_FLAG_H
#define _DEBUGGER_THREAD_REGISTERS_FLAG_H

#include "Debugger.Thread.Registers.h"

/**
\brief Flags enum.
*/
enum class F
{
    Trap = 0x100,
    Resume = 0x10000
}; //F

/**
\brief Class that represents a flag.
\tparam FlagIndex The enum index of the flag.
\tparam ThisPtr Pointer to the Registers class.
*/
template<F FlagIndex>
class Flag
{
public:
    explicit Flag(Registers* registers) : _registers(registers) {}

    bool Get() const
    {
        return _registers->GetFlag(FlagIndex);
    }

    void Set(bool value = true)
    {
        _registers->SetFlag(FlagIndex, value);
    }

    bool operator()() const
    {
        return Get();
    }

    Flag<FlagIndex> & operator=(const bool & other)
    {
        Set(other);
        return *this;
    }

    operator bool() const
    {
        return Get();
    }

private:
    Registers* _registers;
};

#endif //_DEBUGGER_THREAD_REGISTERS_FLAG_H