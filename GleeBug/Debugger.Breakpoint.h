#ifndef _DEBUGGER_BREAKPOINT_H
#define _DEBUGGER_BREAKPOINT_H

#include "Debugger.Global.h"

namespace GleeBug
{
    enum class BreakpointType
    {
        Software,
        Hardware,
        Memory
    };

    enum class SoftwareBreakpointType
    {
        ShortInt3
    };

    enum class HardwareBreakpointSlot
    {
        Dr0 = 0,
        Dr1 = 1,
        Dr2 = 2,
        Dr3 = 3
    };

    enum class HardwareBreakpointType
    {
        Access,
        Write,
        Execute
    };

    enum class HardwareBreakpointSize
    {
        SizeByte = 1,
        SizeWord = 2,
        SizeDword = 4,
#ifdef _WIN64
        SizeQword = 8
#endif //_WIN64
    };

    enum class MemoryBreakpointType
    {
        Acess,
        Write,
        Execute
    };

    /**
    \brief Structure describing internal breakpoint info.
    */
    struct BreakpointInternalInfo
    {
        union
        {
            struct
            {
                SoftwareBreakpointType type;
                ptr size;
                uint8 newbytes[2];
                uint8 oldbytes[2];
            } software;
            struct
            {
                HardwareBreakpointSlot slot;
                HardwareBreakpointType type;
                HardwareBreakpointSize size;
            } hardware;
            struct
            {
                MemoryBreakpointType type;
                ptr size;
            } memory;
        };
    };

    /**
    \brief Structure describing a breakpoint.
    */
    struct BreakpointInfo
    {
        ptr address;
        bool enabled;
        bool singleshoot;
        BreakpointType type;
        BreakpointInternalInfo internal;
    };
};

#endif //_DEBUGGER_BREAKPOINT_H