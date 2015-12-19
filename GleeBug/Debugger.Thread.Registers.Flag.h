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
\brief Class that represents a flag. This class provides an abstraction for a flag to allow intuitive operations.
\tparam FlagIndex The enum index of the flag.
\tparam ThisPtr Pointer to the Registers class.
*/
template<F FlagIndex>
class Flag
{
public:
    /**
    \brief Constructor.
    \param registers Pointer to the registers object.
    */
    explicit Flag(Registers* registers) : _registers(registers) {}

    /**
    \brief Gets the flag.
    \return true if the flag was set, false otherwise.
    */
    bool Get() const
    {
        return _registers->GetFlag(FlagIndex);
    }

    /**
    \brief Sets the flag.
    \param value (Optional) True to set the flag, false to unset the flag.
    */
    void Set(bool value = true)
    {
        _registers->SetFlag(FlagIndex, value);
    }

    /**
    \brief Gets the flag.
    \return true if the flag was set, false otherwise.
    */
    ///
    bool operator()() const
    {
        return Get();
    }

    /**
    \brief Assignment operator (sets the flag).
    \param value True to set the flag, false to unset the flag.
    \return A shallow copy of this object.
    */
    Flag<FlagIndex> & operator=(const bool & value)
    {
        Set(value);
        return *this;
    }

    /**
    \brief bool casting operator. Uses the flag status.
    */
    operator bool() const
    {
        return Get();
    }

    /**
    \brief Bitwise 'or' assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Flag<FlagIndex> & operator|=(const bool & value)
    {
        Set(Get() | value);
        return *this;
    }

    /**
    \brief Bitwise 'exclusive or' assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Flag<FlagIndex> & operator^=(const bool & value)
    {
        Set(Get() ^ value);
        return *this;
    }

    /**
    \brief Bitwise 'and' assignment operator.
    \param value The value to perform the operation with.
    \return The result of the operation.
    */
    Flag<FlagIndex> & operator&=(const bool & value)
    {
        Set(Get() & value);
        return *this;
    }

private:
    Registers* _registers;
};

#endif //_DEBUGGER_THREAD_REGISTERS_FLAG_H